/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"
#include "common/debug.h"
#include "common/memstream.h"
#include "common/path.h"
#include "goldbox/engine.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/gfx/surface.h"
#include "goldbox/gfx/icon.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"

namespace {

static const uint8 kMonsterSlotStart = Goldbox::Gfx::SLOT_DYNAMIC_START;
static const uint16 kGeoVmBaseAddr = 0x4900;
static const uint16 kGeoVmSize = 0x0400;
static const uint16 kPoolradIndoorModeFlagVmAddr = 0x49E6;

static const int kPicture3DAreaCharX = 3;
static const int kPicture3DAreaCharY = 3;
static const int kPicture3DAreaPixelX = kPicture3DAreaCharX * 8;
static const int kPicture3DAreaPixelY = kPicture3DAreaCharY * 8;

static uint8 rollD6() {
    return static_cast<uint8>(Goldbox::VmInterface::rollDice(1, 6));
}

static void applyX86MonsterIconDefaults(Goldbox::Poolrad::Data::PoolradCharacter &monster) {
    using namespace Goldbox::Data;
    const uint8 bodyRoll = rollD6();
    const uint8 armRoll = rollD6();
    const uint8 legRoll = rollD6();
    const uint8 shieldRoll = rollD6();
    const uint8 weaponRoll = rollD6();

    switch (monster.race) {
    case R_DWARF:
    case R_GNOME:
    case R_HALFLING:
        monster.iconData.iconSize = 1;
        break;
    case R_ELF:
    case R_HALF_ELF:
    case R_HALF_ORC:
    case R_HUMAN:
    default:
        monster.iconData.iconSize = 2;
        break;
    }

    monster.iconData.setBodyColor(static_cast<uint8>(0x80 + bodyRoll * 0x11));
    monster.iconData.setArmColor(static_cast<uint8>(0x80 + armRoll * 0x11));
    monster.iconData.setLegColor(static_cast<uint8>(0x80 + legRoll * 0x11));
    monster.iconData.setHairFaceColor(0xC4);
    monster.iconData.setShieldColor(
        static_cast<uint8>(shieldRoll + ((shieldRoll + 8) << 4)));
    monster.iconData.setWeaponColor(
        static_cast<uint8>(weaponRoll + ((weaponRoll + 8) << 4)));
}

static bool loadMonsterTemplate(Goldbox::Engine *engine, uint8 monsterId,
        Goldbox::Poolrad::Data::PoolradCharacter &monster) {
    Goldbox::Data::DaxBlock *rawBlock =
        engine->getDaxManager().getMonCha().getBlockById(monsterId);
    if (!rawBlock) {
        warning("PoolradEngineHostImpl::loadMonster: MONCHA block %u not found",
            (unsigned)monsterId);
        return false;
    }

    Common::MemoryReadStream stream(rawBlock->_data.data(), rawBlock->_data.size());
    monster.load(stream);
    monster.inventory.clear();
    monster.effects.clear();
    monster.itemsAddress = 0;
    monster.numOfItems = 0;
    monster.clearEquippedItems();
    return true;
}

static void loadMonsterItems(Goldbox::Engine *engine, uint8 monsterId,
        Goldbox::Poolrad::Data::PoolradCharacter &monster) {
    Goldbox::Data::DaxBlock *rawBlock =
        engine->getDaxManager().getMonItm().getBlockById(monsterId);
    if (!rawBlock)
        return;

    Common::MemoryReadStream stream(rawBlock->_data.data(), rawBlock->_data.size());
    monster.inventory.loadFromStream(stream);
    monster.numOfItems = static_cast<int8>(monster.inventory.count());
    monster.resolveEquippedItems();
}

static void loadMonsterEffects(Goldbox::Engine *engine, uint8 monsterId,
        Goldbox::Poolrad::Data::PoolradCharacter &monster) {
    Goldbox::Data::DaxBlock *rawBlock =
        engine->getDaxManager().getMonSpc().getBlockById(monsterId);
    if (!rawBlock)
        return;

    Common::MemoryReadStream stream(rawBlock->_data.data(), rawBlock->_data.size());
    const uint32 recSize = 9;
    const uint32 total = rawBlock->_data.size();
    while (stream.pos() + recSize <= total) {
        Goldbox::Data::Effects::Effect effect;
        effect.load(stream);
        effect.nextAddress = 0;
        monster.effects.effects().push_back(effect);
    }
}

} // namespace

namespace Goldbox {
namespace Poolrad {

PoolradEngineHostImpl::PoolradEngineHostImpl(::Goldbox::Engine *engine,
        ECL::AddressSpace *memory)
    : EclSyscallImpl(engine, memory) {
}

PoolradEngineHostImpl::~PoolradEngineHostImpl() {
    clearMonsters();
}

uint8 PoolradEngineHostImpl::allocateMonsterIconSlot() const {
    return (_nextMonsterIconSlot >= Goldbox::Gfx::SLOT_DYNAMIC_START
        && _nextMonsterIconSlot <= Goldbox::Gfx::SLOT_DYNAMIC_END)
        ? _nextMonsterIconSlot : kMonsterSlotStart;
}

VmResult PoolradEngineHostImpl::loadMonster(uint8 monsterId, uint8 count,
        uint8 graphicId) {
    if (!_engine)
        return VmResult::VM_ERROR;
    if (_engine->getPlatform() == Common::kPlatformAmiga) {
        warning("PoolradEngineHostImpl::loadMonster: Amiga MONCHA/MONITM/MONSPC layout not implemented yet");
        return VmResult::VM_ERROR;
    }

    Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (!iconMgr) {
        warning("PoolradEngineHostImpl::loadMonster: icon manager unavailable");
        return VmResult::VM_ERROR;
    }

    Data::PoolradCharacter *templateMonster = new Data::PoolradCharacter();
    if (!loadMonsterTemplate(_engine, monsterId, *templateMonster)) {
        delete templateMonster;
        return VmResult::VM_ERROR;
    }
    loadMonsterItems(_engine, monsterId, *templateMonster);
    loadMonsterEffects(_engine, monsterId, *templateMonster);

    const uint8 slotId = allocateMonsterIconSlot();
    bool loadedIcon = false;

    if (graphicId != 0)
        loadedIcon = iconMgr->loadIcon(slotId,
            Goldbox::Gfx::ICON_KIND_CPIC, graphicId);

    if (!loadedIcon) {
        if (graphicId != 0) {
            warning("PoolradEngineHostImpl::loadMonster: failed to load CPIC icon %u into slot %u, falling back to MONCHA icon data",
                (unsigned)graphicId, (unsigned)slotId);
        }

        loadedIcon = iconMgr->loadIcon(slotId, templateMonster->iconData, false);
        if (!loadedIcon) {
            warning("PoolradEngineHostImpl::loadMonster: failed to build composite fallback icon for monster %u",
                (unsigned)monsterId);
            delete templateMonster;
            iconMgr->releaseIcon(slotId);
            return VmResult::VM_ERROR;
        }
    }

    templateMonster->iconData.iconSlotId = slotId;

    const uint8 spawnCount = (count == 0) ? 1 : count;
    for (uint8 i = 0; i < spawnCount; ++i) {
        Data::PoolradCharacter *monster = (i == 0)
            ? templateMonster
            : new Data::PoolradCharacter(*templateMonster);
        monster->iconData.iconSlotId = slotId;
        monster->clearEquippedItems();
        monster->resolveEquippedItems();
        _loadedMonsters.push_back(monster);
    }

    _monsterIconSlots.push_back(slotId);
    _nextMonsterIconSlot = (slotId < Goldbox::Gfx::SLOT_DYNAMIC_END)
        ? static_cast<uint8>(slotId + 1)
        : kMonsterSlotStart;

    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::clearMonsters() {
    Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (iconMgr) {
        for (uint i = 0; i < _monsterIconSlots.size(); ++i)
            iconMgr->releaseIcon(_monsterIconSlots[i]);
    }

    for (uint i = 0; i < _loadedMonsters.size(); ++i)
        delete _loadedMonsters[i];

    _loadedMonsters.clear();
    _monsterIconSlots.clear();
    _nextMonsterIconSlot = kMonsterSlotStart;
    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::displayPicture(uint8 picID) {
    if (!_engine)
        return VmResult::VM_ERROR;

    if (picID == 0xFF) {
        // Original 0x0E clear path triggers redraw state changes.
        // For now, request a view refresh and return.
        _updateViewState();
        return VmResult::VM_OK;
    }

    Graphics::Screen *screen = _engine->getScreen();
    if (!screen)
        return VmResult::VM_ERROR;

    // Outdoor mode draws the inner 3D picture window frame before blitting.
    if (_memory && _memory->read8(kPoolradIndoorModeFlagVmAddr) == 0) {
        Goldbox::Poolrad::Gfx::Surface screenSurface(*screen,
            Common::Rect(0, 0, screen->w, screen->h));
        screenSurface.drawWindow(3, 3, 13, 13);
    }

    Goldbox::Data::DaxBlock *rawBlock =
        _engine->getDaxManager().getPic().getBlockById(picID);
    if (!rawBlock) {
        warning("PoolradEngineHostImpl::displayPicture: PIC block %u not found",
            (unsigned)picID);
        return VmResult::VM_ERROR;
    }

    Goldbox::Data::DaxBlockPic *picBlock =
        dynamic_cast<Goldbox::Data::DaxBlockPic *>(rawBlock);
    if (!picBlock)
        return VmResult::VM_ERROR;

    Common::ScopedPtr<Goldbox::Gfx::Pic> pic(Goldbox::Gfx::Pic::read(picBlock));
    if (!pic)
        return VmResult::VM_ERROR;

    // Pic::read() allocates a right-sized ManagedSurface (block width/height).
    pic->draw(screen, kPicture3DAreaPixelX, kPicture3DAreaPixelY);
    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::loadGeoBlock(uint8 blockId) {
    if (!_engine || !_memory)
        return VmResult::VM_ERROR;

    Goldbox::Data::DaxBlock *rawBlock =
        _engine->getDaxManager().getGeo().getBlockById(blockId);
    if (!rawBlock) {
        warning("PoolradEngineHostImpl::loadGeoBlock: GEO block %u not found",
            (unsigned)blockId);
        return VmResult::VM_ERROR;
    }

    Goldbox::Data::DaxBlockGeo *geoBlock =
        dynamic_cast<Goldbox::Data::DaxBlockGeo *>(rawBlock);
    if (!geoBlock) {
        warning("PoolradEngineHostImpl::loadGeoBlock: block %u is not GEO",
            (unsigned)blockId);
        return VmResult::VM_ERROR;
    }

    const Common::Span<const uint8> geoRaw = geoBlock->raw();
    if (geoRaw.size() < kGeoVmSize) {
        warning("PoolradEngineHostImpl::loadGeoBlock: GEO block %u too small (%u bytes)",
            (unsigned)blockId, (unsigned)geoRaw.size());
        return VmResult::VM_ERROR;
    }

    _memory->loadBytes(kGeoVmBaseAddr,
        Common::Span<const uint8>(geoRaw.data(), kGeoVmSize));
    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::loadIconBlock() {
    if (!_engine)
        return VmResult::VM_ERROR;

    // Outdoor branch of LOAD_AREA_GEO requests BACPAC icon resources.
    // BACPAC is loaded at engine startup; nothing to do here.
    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::loadWallSet(uint8 blockId, uint8 setSlot) {
    if (!_engine) return VmResult::VM_ERROR;
    if (setSlot < 1 || setSlot > 3) return VmResult::VM_ERROR;

    PoolradEngine *poolrad = dynamic_cast<PoolradEngine *>(_engine);
    if (!poolrad) return VmResult::VM_ERROR;

    Goldbox::Gfx::WalldefSlotCache &walldefCache = poolrad->getWalldefSlotCache();
    Goldbox::Gfx::Tile8x8Cache &tileCache = poolrad->getTileCache();

    // Original DECO flow uses 0xFF to invalidate/clear a slot.
    if (blockId == 0xFF) {
        const int slotIdx = setSlot - 1;
        walldefCache.clearSlot(setSlot);
        _walldefTiles[slotIdx].reset();
        tileCache.setSlot(setSlot, nullptr);
        return VmResult::VM_OK;
    }

    Goldbox::Data::DaxBlock *rawBlock =
            _engine->getDaxManager().getWalldef().getBlockById(blockId);
    if (!rawBlock) {
        warning("PoolradEngineHostImpl::loadWallSet: walldef block %d not found",
                blockId);
        return VmResult::VM_ERROR;
    }
    Goldbox::Data::DaxBlockWalldef *walldef =
            dynamic_cast<Goldbox::Data::DaxBlockWalldef *>(rawBlock);
    if (!walldef) return VmResult::VM_ERROR;

    const int numChunks = walldef->chunkCount();
    for (int i = 0; i < numChunks; ++i) {
        const int curSlot = setSlot + i;
        if (curSlot < 1 || curSlot > 3)
            break;

        walldefCache.loadSlot(curSlot, walldef, i, tileCache);

        const int slotIdx = curSlot - 1;
        _walldefTiles[slotIdx].reset();
        tileCache.setSlot(curSlot, nullptr);

        // x86/m68k LoadWallSet behavior:
        // - one-chunk walldef: use blockId directly
        // - multi-chunk walldef: use blockId*10 + chunkIndex (1-based),
        //   with blockId 0 remapped to 10 before multiplication.
        uint16 tileBlockId = blockId;
        if (numChunks >= 2) {
            const uint16 tileBase = (blockId == 0)
                ? static_cast<uint16>(10)
                : static_cast<uint16>(blockId);
            tileBlockId = static_cast<uint16>(tileBase * 10 + (i + 1));
        }

        Goldbox::Data::DaxBlock *tileRaw =
                _engine->getDaxManager().get8x8d().getBlockById(
                        static_cast<uint8>(tileBlockId));
        if (tileRaw) {
            Goldbox::Data::DaxBlock8x8D *tile8x8 =
                dynamic_cast<Goldbox::Data::DaxBlock8x8D *>(tileRaw);
            if (tile8x8) {
                _walldefTiles[slotIdx].reset(new Goldbox::Gfx::DaxTile(tile8x8));
                tileCache.setSlot(curSlot, _walldefTiles[slotIdx].get());
            }
        } else {
            warning("PoolradEngineHostImpl::loadWallSet: 8x8d block %u not found for wall slot %d",
                (unsigned)tileBlockId, curSlot);
        }
    }

    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::onMapDataReady() {
    // Both geo and wallset data are ready for the current area.
    // Mirrors the final block of INSTR_LoadAreaDeco in the original:
    //   if (BOOL_SCREEN_REFRESH && BOOL_WALLSET_READY && BOOL_GEO_READY) {
    //       GAME_ScreenByState();
    //       DIALOG_ShowParty(PTR_SEL_CHARACTER);
    //       BOOL_SCREEN_REFRESH = false;
    //   }
    // TODO: trigger full screen redraw (ScreenByState + ShowParty) once
    // the view system wires BOOL_SCREEN_REFRESH.
    debug(2, "PoolradEngineHostImpl::onMapDataReady: area map data fully loaded");
    return VmResult::VM_OK;
}

} // namespace Poolrad
} // namespace Goldbox
