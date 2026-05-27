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
#include "goldbox/core/menu_item.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/runtime/runtime_exchange.h"
#include "goldbox/runtime/runtime_geo.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"

namespace {

static const uint8 kMonsterSlotStart = Goldbox::Gfx::SLOT_DYNAMIC_START;
// Legacy x86/m68k naming parity:
// - PTR_GEO_BUFF receives static GEO map planes (NE/SW/events/doors).
// - VM bank0 (0x4900 base) stores GEO-related metadata/state fields.
// These are related concepts but not guaranteed to be byte-identical streams.
static const uint16 kStaticMapPayloadSize = 0x0400;
static const uint16 kVmBank0GeoMetadataOffset = 0x4900;

static const int kPicture3DAreaCharX = 3;
static const int kPicture3DAreaCharY = 3;
static const int kPicture3DAreaPixelX = kPicture3DAreaCharX * 8;
static const int kPicture3DAreaPixelY = kPicture3DAreaCharY * 8;
static uint32 kModalMenuSinkCounter = 0;

static bool isAsciiAlphaNum(char c) {
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9');
}

static char toUpperAscii(char c) {
    if (c >= 'a' && c <= 'z')
        return static_cast<char>(c - ('a' - 'A'));
    return c;
}

static char toLowerAscii(char c) {
    if (c >= 'A' && c <= 'Z')
        return static_cast<char>(c + ('a' - 'A'));
    return c;
}

// Build a menu model equivalent to MENU_processShortcuts behavior:
// - '~X' marks X as shortcut
// - letters are normalized to lowercase in body text
// - shortcut is stored as uppercase
static Goldbox::MenuItemList buildLegacyHorizontalMenuModel(
        const Common::Array<Common::String> &options) {
    Goldbox::MenuItemList model;

    for (uint i = 0; i < options.size(); ++i) {
        const Common::String &src = options[i];
        Goldbox::MenuItem item;
        item.active = true;
        item.shortcutFirst = true;
        item.shortcut = '\0';

        int markerPos = -1;
        for (uint p = 0; p + 1 < src.size(); ++p) {
            if (src[p] == '~') {
                markerPos = static_cast<int>(p);
                break;
            }
        }

        if (markerPos >= 0 && markerPos + 1 < static_cast<int>(src.size())) {
            item.shortcut = toUpperAscii(src[markerPos + 1]);
            item.shortcutFirst = (markerPos == 0);

            Common::String left = src.substr(0, markerPos);
            Common::String right = src.substr(markerPos + 2);
            item.text = left + right;
        } else {
            item.text = src;
            for (uint p = 0; p < item.text.size(); ++p) {
                if (isAsciiAlphaNum(item.text[p])) {
                    item.shortcut = toUpperAscii(item.text[p]);
                    item.shortcutFirst = (p == 0);
                    Common::String left = item.text.substr(0, p);
                    Common::String right = item.text.substr(p + 1);
                    item.text = left + right;
                    break;
                }
            }
        }

        for (uint p = 0; p < item.text.size(); ++p) {
            if (item.text[p] != (char)0xFF)
                item.text.setChar(toLowerAscii(item.text[p]), p);
        }

        if (item.shortcut == '\0')
            item.shortcut = ' ';

        model.items.push_back(item);
    }

    model.currentSelection = 0;
    return model;
}

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

class AsyncMenuResultSink : public Goldbox::UIElement {
public:
    bool done;
    bool success;
    int value;

    AsyncMenuResultSink(const Common::String &name, Goldbox::UIElement *parent)
        : Goldbox::UIElement(name, parent), done(false), success(false), value(-1) {
    }

    void handleMenuResult(const Goldbox::MenuResultMessage &result) override {
        done = true;
        success = result._success;
        value = result._hasIntValue ? result._intValue : -1;
    }
};

class AsyncDelaySink : public Goldbox::UIElement {
public:
    bool done;

    AsyncDelaySink(const Common::String &name, Goldbox::UIElement *parent,
            uint frames)
        : Goldbox::UIElement(name, parent), done(false) {
        delayFrames(frames);
    }

    void timeout() override {
        done = true;
    }
};

PoolradEngineHostImpl::PoolradEngineHostImpl(::Goldbox::Engine *engine,
        ECL::AddressSpace *memory)
    : EclSyscallImpl(engine, memory) {
    memset(_staticMapPayloadBuffer, 0, sizeof(_staticMapPayloadBuffer));
}

PoolradEngineHostImpl::~PoolradEngineHostImpl() {
    if (_asyncMenuSink)
        delete _asyncMenuSink;
    _asyncMenuSink = nullptr;
    _asyncHorizontalMenu = nullptr;
    _asyncMenuModel.reset();
    _asyncMenuPending = false;

    if (_asyncPrintSink)
        delete _asyncPrintSink;
    _asyncPrintSink = nullptr;
    _asyncPrintPending = false;
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
    const Goldbox::RuntimeExchange *exchange = _engine->getRuntimeExchange();
    Goldbox::RuntimeMapSnapshot snapshot;
    if (exchange && exchange->captureMapSnapshot(snapshot) && snapshot.valid
            && !snapshot.indoorMode) {
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

int16 PoolradEngineHostImpl::horizontalMenu(
        const Common::Array<Common::String> &options) {
    if (options.empty())
        return -1;
    if (!_engine || !g_events)
        return -1;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return -1;

    // Build legacy-compatible menu metadata so the UI layer can consume
    // shortcut/text splits identical to MENU_processShortcuts semantics.
    Goldbox::MenuItemList parsed =
        buildLegacyHorizontalMenuModel(options);

    Views::Dialogs::HorizontalMenuConfig cfg;
    cfg.promptTxt = "";
    cfg.menuItemList = &parsed;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 15;
    cfg.allowNumPad = true;
    cfg.backgroundColor = 0;

    Common::String sinkName = Common::String::format("EclMenuSink_%u",
        ++kModalMenuSinkCounter);
    AsyncMenuResultSink *sink = new AsyncMenuResultSink(sinkName, focused);

    Views::Dialogs::HorizontalMenu *menu =
        new Views::Dialogs::HorizontalMenu("EclHorizontalMenu", cfg);
    sink->subView(menu);
    menu->activate();
    menu->redraw();

    while (menu->isActive() && !sink->done) {
        if (!g_events->pumpModalInputFrame())
            break;
    }

    const int16 result = (sink->done && sink->success && sink->value >= 0)
        ? static_cast<int16>(sink->value)
        : static_cast<int16>(-1);

    delete sink;
    return result;
}

VmResult PoolradEngineHostImpl::handleCallOpcode(uint16 callId) {
    // 0x2D CALL target dispatcher (x86/m68k compatible IDs).
    switch (callId) {
    case 0x2C90:
        // CALL_MAP_3D_COLOR_UPDATE + DIALOG_StateArea in originals.
        // ScummVM route: refresh active InGameView, which applies
        // MAP_3DColorUpdate-style branching (indoor 3D vs wilderness area).
        _updateViewState();
        return VM_OK;

    case 0x8000:
    case 0x8001: {
        // SPECIAL_COMBAT_MODE on/off. Original behavior modifies a linked
        // encounter list and may synthesize an extra hostile NPC entry.
        // Current safe subset: toggle selected character quickfight/hostile.
        Poolrad::Data::PoolradCharacter *selected =
            dynamic_cast<Poolrad::Data::PoolradCharacter *>(
                VmInterface::getSelectedCharacter());
        if (selected) {
            if (callId == 0x8000) {
                selected->quickfight = true;
                selected->hostile = false;
            } else {
                selected->quickfight = false;
                selected->hostile = false;
            }
        }
        return VM_OK;
    }

    case 0xBA03:
        // Original: sound indirection through WORD_SOUND_FLAG -> sound 11/12.
        // TODO: wire mapped sound bank/state once sound flag field mapping is
        // available in VmGlobalLayout/RuntimeLayout.
        return VM_OK;

    case 0xC01E:
        // Original: Map_StepForwardWrap / MAP_moveOnMap.
        // TODO: route through in-game movement service with wrap semantics.
        return VM_OK;

    case 0xC018:
        // Original (only when map type == 1): update position map_nibble.
        // TODO: implement nibble sampling from active GEO map model.
        return VM_OK;

    default:
        return VM_OK;
    }
}

VmResult PoolradEngineHostImpl::spriteOff() {
    // Legacy SPRITE_OFF calls MAP_3DColorUpdate when a sprite overlay is active.
    // ScummVM route: refresh active InGameView to re-run map-state drawing.
    _updateViewState();
    return VM_OK;
}

VmResult PoolradEngineHostImpl::beginHorizontalMenuAsync(uint16 resultAddr,
        const Common::Array<Common::String> &options) {
    if (!_engine || !g_events || options.empty())
        return VM_ERROR;
    if (_asyncMenuPending)
        return VM_ERROR;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return VM_ERROR;

    _asyncMenuModel.reset(new Goldbox::MenuItemList(
        buildLegacyHorizontalMenuModel(options)));

    Views::Dialogs::HorizontalMenuConfig cfg;
    cfg.promptTxt = "";
    cfg.menuItemList = _asyncMenuModel.get();
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 15;
    cfg.allowNumPad = true;
    cfg.backgroundColor = 0;

    Common::String sinkName = Common::String::format("EclAsyncMenuSink_%u",
        ++kModalMenuSinkCounter);
    _asyncMenuSink = new AsyncMenuResultSink(sinkName, focused);
    _asyncHorizontalMenu = new Views::Dialogs::HorizontalMenu(
        "EclAsyncHorizontalMenu", cfg);
    _asyncMenuSink->subView(_asyncHorizontalMenu);
    _asyncHorizontalMenu->activate();
    _asyncHorizontalMenu->redraw();

    _asyncMenuResultAddr = resultAddr;
    _asyncMenuPending = true;
    return VM_YIELD;
}

VmResult PoolradEngineHostImpl::beginPrintAsync(const Common::String &text,
        bool clearBox) {
    if (!_engine || !g_events)
        return VM_OK;
    if (_asyncMenuPending || _asyncPrintPending)
        return VM_ERROR;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return VM_OK;

    // Start visual print immediately, then yield for pacing frames.
    printText(text, clearBox);

    const uint textDelay = VmInterface::getTextDelay();
    const uint frames = textDelay * FRAME_RATE / 2;
    if (frames == 0)
        return VM_OK;

    Common::String sinkName = Common::String::format("EclAsyncPrintSink_%u",
        ++kModalMenuSinkCounter);
    _asyncPrintSink = new AsyncDelaySink(sinkName, focused, frames);
    _asyncPrintPending = true;
    return VM_YIELD;
}

bool PoolradEngineHostImpl::hasPendingAsync() const {
    return _asyncMenuPending || _asyncPrintPending;
}

bool PoolradEngineHostImpl::isPendingAsyncReady() const {
    if (_asyncMenuPending) {
        if (!_asyncMenuSink)
            return false;
        const AsyncMenuResultSink *sink =
            dynamic_cast<const AsyncMenuResultSink *>(_asyncMenuSink);
        return sink && sink->done;
    }

    if (_asyncPrintPending) {
        if (!_asyncPrintSink)
            return false;
        const AsyncDelaySink *sink =
            dynamic_cast<const AsyncDelaySink *>(_asyncPrintSink);
        return sink && sink->done;
    }

    return false;
}

VmResult PoolradEngineHostImpl::finalizePendingAsync() {
    if (_asyncMenuPending) {
        AsyncMenuResultSink *sink =
            dynamic_cast<AsyncMenuResultSink *>(_asyncMenuSink);
        if (!sink || !sink->done)
            return VM_YIELD;

        if (_memory && sink->success && sink->value >= 0)
            _memory->write16LE(_asyncMenuResultAddr,
                static_cast<uint16>(sink->value));

        delete _asyncMenuSink;
        _asyncMenuSink = nullptr;
        _asyncHorizontalMenu = nullptr;
        _asyncMenuModel.reset();
        _asyncMenuResultAddr = 0;
        _asyncMenuPending = false;
        return VM_OK;
    }

    if (_asyncPrintPending) {
        AsyncDelaySink *sink =
            dynamic_cast<AsyncDelaySink *>(_asyncPrintSink);
        if (!sink || !sink->done)
            return VM_YIELD;

        delete _asyncPrintSink;
        _asyncPrintSink = nullptr;
        _asyncPrintPending = false;
        return VM_OK;
    }

    return VM_OK;
}

VmResult PoolradEngineHostImpl::loadGeoBlock(uint8 blockId) {
    if (!_engine)
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
    if (geoRaw.size() < kStaticMapPayloadSize) {
        warning("PoolradEngineHostImpl::loadGeoBlock: GEO block %u too small (%u bytes)",
            (unsigned)blockId, (unsigned)geoRaw.size());
        return VmResult::VM_ERROR;
    }

    // Populate legacy static buffer (backward compat).
    memcpy(_staticMapPayloadBuffer, geoRaw.data(), kStaticMapPayloadSize);
    _staticMapPayloadLoaded = true;
    _staticMapPayloadBlockId = blockId;

    // Populate the global mutable RuntimeGeoBlock (resets to disk state).
    RuntimeGeoBlock &rtGeo = _engine->getRuntimeGeo();
    rtGeo.loadFromGeoBlock(*geoBlock);
    rtGeo.setMapId(blockId);

    debug(2, "PoolradEngineHostImpl::loadGeoBlock: loaded GEO block %u into RuntimeGeoBlock + static payload",
        (unsigned)blockId);
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

    Goldbox::Gfx::WalldefSlotCache &walldefCache =
        _engine->getWalldefSlotCache();
    Goldbox::Gfx::Tile8x8Cache &tileCache = _engine->getTileCache();

    // Original DECO flow uses 0xFF to invalidate/clear a slot.
    if (blockId == 0xFF) {
        const int slotIdx = setSlot - 1;
        walldefCache.clearSlot(setSlot);
        _walldefTiles[slotIdx].reset();
        tileCache.setSlot(setSlot, nullptr);
        _wallSetStates[slotIdx].loaded = false;
        _wallSetStates[slotIdx].walldefBlockId = 0xFF;
        _wallSetStates[slotIdx].tileBlockId = 0xFF;
        _wallSetStates[slotIdx].chunkIndex = 0;
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

        const int slotIdx = curSlot - 1;

        // Reset any previous runtime tile/surface state for this slot before
        // rebuilding it from the newly loaded walldef chunk.
        walldefCache.clearSlot(curSlot);
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

        // Build the runtime wall-region surfaces only after the slot-specific
        // 8x8 tile atlas is attached to the tile cache.  The debugger commands
        // (`walldef`, `fpview`) inspect this runtime-built cache directly.
        walldefCache.loadSlot(curSlot, walldef, i, tileCache);

        _wallSetStates[slotIdx].loaded = true;
        _wallSetStates[slotIdx].walldefBlockId = blockId;
        _wallSetStates[slotIdx].tileBlockId = static_cast<uint8>(tileBlockId);
        _wallSetStates[slotIdx].chunkIndex = static_cast<uint8>(i);
    }

    return VmResult::VM_OK;
}

const PoolradEngineHostImpl::WallSetRuntimeState &
PoolradEngineHostImpl::wallSetState(int slot) const {
    static WallSetRuntimeState kEmptyState;
    if (slot < 1 || slot > 3)
        return kEmptyState;
    return _wallSetStates[slot - 1];
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

bool PoolradEngineHostImpl::tryOpenDoor() {
    if (!_engine)
        return false;

    RuntimeGeoBlock &rtGeo = _engine->getRuntimeGeo();
    if (!rtGeo.isLoaded())
        return false;

    const RuntimeExchange *exchange = _engine->getRuntimeExchange();
    if (!exchange)
        return false;

    RuntimeMapSnapshot snapshot;
    if (!exchange->captureMapSnapshot(snapshot) || !snapshot.valid)
        return false;

    const int x = static_cast<int>(snapshot.dungeonX);
    const int y = static_cast<int>(snapshot.dungeonY);
    // Wire direction: 0=N, 2=E, 4=S, 6=W
    const uint8 wireDir = static_cast<uint8>((snapshot.dungeonDir & 0x03) * 2);

    // Check if there's a door flag (non-zero) in the facing direction.
    const uint8 doorFlag = rtGeo.getWallFlag(x, y, wireDir);
    if (doorFlag == 0)
        return false;

    // Clear the door flag (open the door).
    rtGeo.clearFlag(x, y, wireDir);
    debug(3, "PoolradEngineHostImpl::tryOpenDoor: opened door at (%d,%d) dir=%u",
        x, y, (unsigned)wireDir);
    return true;
}

void PoolradEngineHostImpl::playSound(uint8 soundId) {
    // TODO: Wire to ScummVM audio mixer once sound system is implemented.
    // Original: PlaySound(ARRAY_SOUND_MAP[soundId]) / WORD_SOUND_ID_N.
    debug(3, "PoolradEngineHostImpl::playSound: sound %u requested",
        (unsigned)soundId);
}

} // namespace Poolrad
} // namespace Goldbox
