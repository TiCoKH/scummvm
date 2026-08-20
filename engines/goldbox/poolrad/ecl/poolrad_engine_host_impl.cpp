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
#include "common/list.h"
#include "common/memstream.h"
#include "common/path.h"
#include "goldbox/engine.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/gfx/icon.h"
#include "goldbox/gfx/dax_tile.h"
#include "goldbox/gfx/encounter_sprite_cache.h"
#include "goldbox/gfx/picture_display_cache.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_host_bridge.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/runtime/runtime_exchange.h"
#include "goldbox/runtime/runtime_geo.h"
#include "goldbox/runtime/runtime_time.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"
#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/poolrad/views/combat_view.h"
#include "goldbox/core/direction.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/runtime_layout.h"

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

static const char *kEffectStatusChangedEventName = "EffectStatusChanged";

class PoolradEffectHostBridge : public Goldbox::Data::Effects::EffectHostBridge {
public:
    void postEffectMessage(Goldbox::Data::PlayerCharacter *character,
            const Common::String &text, bool withDelay) override {
        if (!Goldbox::g_events)
            return;

        Common::String msg;
        if (character && !character->name.empty())
            msg = character->name + " " + text;
        else
            msg = text;

        const int16 result = withDelay ? 1 : 0;
        Goldbox::g_events->postEclVmMessage(
            Goldbox::EclVmMessage::makeSyscallWithText(0, 0,
            Goldbox::EclVmMessage::SC_PRINT_ASYNC, msg, result));
    }

    void requestRefresh(uint32 refreshFlags) override {
        if (!Goldbox::g_events)
            return;

        if (refreshFlags & RF_STATUS_PANEL) {
            Goldbox::g_events->postEclStateMessage(
                Goldbox::EclVmMessage::ST_STATUS_DIRTY, 1,
                Goldbox::EclVmMessage::VT_UINT8);
        }

        if (refreshFlags & RF_CHARACTER_PANEL) {
            Goldbox::g_events->postEclStateMessage(
                Goldbox::EclVmMessage::ST_CHARACTER_DIRTY, 1,
                Goldbox::EclVmMessage::VT_UINT8);
        }

        if (refreshFlags & RF_VIEWPORT) {
            Goldbox::g_events->postEclStateMessage(
                Goldbox::EclVmMessage::ST_SKYBOX_DIRTY, 1,
                Goldbox::EclVmMessage::VT_UINT8);
        }

        if (refreshFlags & RF_AREA_MAP) {
            Goldbox::g_events->postEclStateMessage(
                Goldbox::EclVmMessage::ST_SCREEN_REFRESH, 1,
                Goldbox::EclVmMessage::VT_UINT8);
        }

        Goldbox::UIElement *focused = Goldbox::g_events->focusedView();
        if (focused)
            focused->redraw();
    }

    void applyDamage(Goldbox::Data::PlayerCharacter *character,
            uint8 baseDamage, Goldbox::Data::DamageModifier modifier,
            bool applyModifier) override {
        if (!character)
            return;

        Goldbox::Poolrad::PoolradEngine *poolEngine =
            dynamic_cast<Goldbox::Poolrad::PoolradEngine *>(Goldbox::g_engine);
        if (!poolEngine)
            return;

        Goldbox::Poolrad::Views::CombatView *combatView =
            dynamic_cast<Goldbox::Poolrad::Views::CombatView *>(
                poolEngine->findView("Combat"));
        if (combatView) {
            combatView->applyDamageMessage(character, baseDamage,
                    modifier, applyModifier);
            return;
        }

        Goldbox::Poolrad::Views::InGameView *inGameView =
            dynamic_cast<Goldbox::Poolrad::Views::InGameView *>(
                poolEngine->findView("InGame"));
        if (!inGameView)
            return;

        inGameView->applyDamageMessage(character, baseDamage,
                modifier, applyModifier);
    }

    void notifyStatusChanged(Goldbox::Data::PlayerCharacter *character,
            uint8 oldStatus, uint8 newStatus) override {
        (void)character;
        if (!Goldbox::g_events)
            return;

        const uint16 packed = static_cast<uint16>(oldStatus) << 8 |
            static_cast<uint16>(newStatus);
        const Common::String target = Goldbox::g_events->isPresent("InGame")
            ? Common::String("InGame") : Common::String();
        Goldbox::g_events->postMenuResult(target, true,
            Common::KEYCODE_INVALID,
            packed, Common::String(kEffectStatusChangedEventName), true,
            true);

        Goldbox::g_events->postEclStateMessage(
            Goldbox::EclVmMessage::ST_STATUS_DIRTY, 1,
            Goldbox::EclVmMessage::VT_UINT8);
        Goldbox::g_events->postEclStateMessage(
            Goldbox::EclVmMessage::ST_CHARACTER_DIRTY, 1,
            Goldbox::EclVmMessage::VT_UINT8);
    }

    void postVmState(uint16 tag, uint16 value,
            uint8 valueType) override {
        if (!Goldbox::g_events)
            return;

        Goldbox::g_events->postEclStateMessage(tag, value,
            static_cast<Goldbox::EclVmMessage::ValueType>(valueType));
    }

    void onCharacterDied(Goldbox::Data::PlayerCharacter *character) override {
        if (!character || !Goldbox::g_events)
            return;

        // Remove from combat map if in combat.
        Goldbox::Poolrad::PoolradEngine *poolEngine =
            dynamic_cast<Goldbox::Poolrad::PoolradEngine *>(Goldbox::g_engine);
        if (poolEngine) {
            Goldbox::Poolrad::Views::CombatView *combatView =
                dynamic_cast<Goldbox::Poolrad::Views::CombatView *>(
                    poolEngine->findView("Combat"));
            if (combatView)
                combatView->handleDeathOnMap(character);
        }

        // Timing delay: post dirty flags so the UI refreshes.
        Goldbox::g_events->postEclStateMessage(
            Goldbox::EclVmMessage::ST_STATUS_DIRTY, 1,
            Goldbox::EclVmMessage::VT_UINT8);
        Goldbox::g_events->postEclStateMessage(
            Goldbox::EclVmMessage::ST_CHARACTER_DIRTY, 1,
            Goldbox::EclVmMessage::VT_UINT8);
    }
};

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
        monster.iconData.bodyType = 1;
        break;
    case R_ELF:
    case R_HALF_ELF:
    case R_HALF_ORC:
    case R_HUMAN:
    default:
        monster.iconData.bodyType = 2;
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
        monster.effects.appendEffect(effect);
    }
}

} // namespace

namespace Goldbox {
namespace Poolrad {

Goldbox::Data::Effects::EffectHostBridge *getEffectHostBridge() {
    static PoolradEffectHostBridge s_bridge;
    return &s_bridge;
}

} // namespace Poolrad
} // namespace Goldbox

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
    _asyncPrintPending = false;
    _asyncDelayPending = false;
    _asyncShopPending = false;
    _asyncShopWasActivated = false;
    clearMonsters();
}

uint8 PoolradEngineHostImpl::allocateMonsterIconSlot() const {
    return (_nextMonsterIconSlot >= Goldbox::Gfx::SLOT_DYNAMIC_START
        && _nextMonsterIconSlot <= Goldbox::Gfx::SLOT_DYNAMIC_END)
        ? _nextMonsterIconSlot : kMonsterSlotStart;
}

VmResult PoolradEngineHostImpl::startCombat() {
    if (_asyncCombatPending)
        return VM_ERROR;

    // Setup safety guard: if no host enemies are loaded, do not enter
    // tactical combat setup even if legacy VM flags drift.
    if (_loadedMonsters.empty())
        return VmResult::VM_OK;

    Common::Array<Goldbox::Data::PlayerCharacter *> combatRoster;
    Common::List<Goldbox::Data::PlayerCharacter *> &party =
        _engine->getParty();
    const uint partySize = party.size();
    const uint enemySize = _loadedMonsters.size();

    if (_monstersAppendedToParty) {
        combatRoster.reserve(partySize);
    } else {
        combatRoster.reserve(partySize + enemySize);
    }

    for (Goldbox::Data::PlayerCharacter *member : party)
        combatRoster.push_back(member);

    int partyCount = static_cast<int>(partySize);
    if (_monstersAppendedToParty) {
        partyCount = (partySize >= enemySize)
            ? static_cast<int>(partySize - enemySize)
            : 0;
    } else {
        for (Common::List<Data::PoolradCharacter *>::const_iterator it =
                _loadedMonsters.begin(); it != _loadedMonsters.end(); ++it) {
            combatRoster.push_back(*it);
        }
    }

    // Keep a stable runtime "next character" anchor for legacy traversal
    // semantics (party head when available).
    if (_engine)
        _engine->_nextCharacter = party.empty() ? nullptr : party.front();

    debug(2, "PoolradEngineHostImpl::startCombat unified roster size=%u partyCount=%d enemyCount=%d",
        (unsigned)combatRoster.size(), partyCount,
        (int)combatRoster.size() - partyCount);

    Poolrad::PoolradEngine *poolEngine =
        dynamic_cast<Poolrad::PoolradEngine *>(_engine);
    if (!poolEngine || !_memory || !g_events)
        return VM_ERROR;

    Views::CombatView *combatView = dynamic_cast<Views::CombatView *>(
        poolEngine->findView("Combat"));
    if (!combatView) {
        warning("PoolradEngineHostImpl::startCombat: Combat view is not registered");
        return VM_ERROR;
    }

    Combat::CombatParams params;
    params.roster = combatRoster;
    params.partyCount = partyCount;
    params.geo = &_engine->getRuntimeGeo();
    params.nextChar = _engine->_nextCharacter;
    params.eclMemory = _memory;

    const RuntimeExchange *exchange = _engine->getRuntimeExchange();
    RuntimeMapSnapshot snapshot;
    if (exchange && exchange->captureMapSnapshot(snapshot) && snapshot.valid) {
        params.mapDirection = static_cast<uint8>((snapshot.dungeonDir & 0x03) * 2);
        params.isDungeon = snapshot.indoorMode;
        params.mapCenterX = static_cast<int8>(snapshot.dungeonX & 0xFF);
        params.mapCenterY = static_cast<int8>(snapshot.dungeonY & 0xFF);
        params.playerY = static_cast<int8>(snapshot.dungeonY & 0xFF);
        params.eclScriptId = snapshot.mapId;
        params.wildX = snapshot.wildernessX;
        params.wildY = snapshot.wildernessY;
        params.mapType = snapshot.mapType;
    }

    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    params.encounterDistance = static_cast<int>(_memory->read8(
        layout.vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr));
    params.moraleThreshold = _memory->read8(
        layout.vmGlobalField(kVmGlobalFieldMoraleThreshold).vmAddr);
    params.isAmbush = (_memory->read8(
        layout.vmGlobalField(kVmGlobalFieldCombatIsAmbush).vmAddr) != 0);

    const uint16 monsterLoadReadyAddr = layout.runtimeField(
        static_cast<Goldbox::ECL::EclRuntimeFieldId>(16));
    const uint16 menuCombatStateAddr = layout.runtimeField(
        static_cast<Goldbox::ECL::EclRuntimeFieldId>(4));

    params.monsterLoadReady = (_memory->read8(
        monsterLoadReadyAddr) != 0);
    params.combatTrigger = (_memory->read8(
        menuCombatStateAddr) != 0);

    combatView->setup(params);

    if (!g_events->isPresent("Combat"))
        g_events->addView(combatView);

    _asyncCombatPending = true;
    _asyncCombatWasActivated = false;
    return VM_YIELD;
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
    applyX86MonsterIconDefaults(*templateMonster);

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

    Common::List<Goldbox::Data::PlayerCharacter *> &party =
        _engine->getParty();

    const uint8 spawnCount = (count == 0) ? 1 : count;
    for (uint8 i = 0; i < spawnCount; ++i) {
        Data::PoolradCharacter *monster = (i == 0)
            ? templateMonster
            : new Data::PoolradCharacter(*templateMonster);

        // Preserve legacy combat assumptions: loaded monsters are hostile
        // and enabled (active in combat) by default.
        monster->hostile = true;
        monster->enabled = true;

        monster->iconData.iconSlotId = slotId;
        monster->clearEquippedItems();
        monster->resolveEquippedItems();
        _loadedMonsters.emplace_back(monster);
        party.push_back(monster);
    }

    _monstersAppendedToParty = true;

    _monsterIconSlots.push_back(slotId);
    _nextMonsterIconSlot = (slotId < Goldbox::Gfx::SLOT_DYNAMIC_END)
        ? static_cast<uint8>(slotId + 1)
        : kMonsterSlotStart;

    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::clearMonsters() {
    if (_engine && _monstersAppendedToParty) {
        Common::List<Goldbox::Data::PlayerCharacter *> &party =
            _engine->getParty();
        for (Data::PoolradCharacter *monster : _loadedMonsters) {
            party.remove(monster);
        }
    }

    _monstersAppendedToParty = false;

    Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (iconMgr) {
        for (uint i = 0; i < _monsterIconSlots.size(); ++i)
            iconMgr->releaseIcon(_monsterIconSlots[i]);
    }

    for (Data::PoolradCharacter *monster : _loadedMonsters) {
        delete monster;
    }

    _loadedMonsters.clear();
    _monsterIconSlots.clear();
    _nextMonsterIconSlot = kMonsterSlotStart;

    // Restore traversal anchor to party context.
    if (_engine) {
        Common::List<Goldbox::Data::PlayerCharacter *> &party =
            _engine->getParty();
        _engine->_nextCharacter = party.empty() ? nullptr : party.front();
    }

    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::displayPicture(uint8 picID) {
    if (!_engine)
        return VmResult::VM_ERROR;

    if (picID == 0xFF) {
        // Clear both picture displays — mirrors original clear behavior.
        _engine->getPictureDisplayCache().clear();
        _engine->getEncounterSpriteCache().clear();
        if (g_events) {
            g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
                EclVmMessage::VT_UINT8);
        }
        return VmResult::VM_OK;
    }

    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    const uint8 headPicId = _memory->read8(
        layout.vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr);

    if (_engine->getEncounterSpriteCache().isSpriteLoaded()) {
        // Sprite encounter active: load head into EncounterSpriteCache
        // so the existing sprite+head rendering path handles it.
        _engine->getEncounterSpriteCache().loadHead(headPicId, picID);
    } else {
        // No sprite: standalone portrait/scene via PictureDisplayCache.
        _engine->getPictureDisplayCache().load(headPicId, picID);
    }

    if (g_events) {
        g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
            EclVmMessage::VT_UINT8);
    }
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

    const bool singleItemMode = (options.size() == 1);

    // Build legacy-compatible menu metadata so the UI layer can consume
    // shortcut/text splits identical to MENU_processShortcuts semantics.
    Goldbox::MenuItemList parsed =
        buildLegacyHorizontalMenuModel(options);

    // In singleItemMode, clear menu items — only the prompt is shown.
    if (singleItemMode)
        parsed.items.clear();

    Views::Dialogs::HorizontalMenuConfig cfg;
    cfg.menuItemList = &parsed;
    cfg.allowNumPad = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = singleItemMode;

    if (singleItemMode) {
        Common::String promptText = options[0];
        if (promptText.hasPrefix("PRESS <RETURN>") ||
                promptText.hasPrefix("PRESS <ENTER>")) {
            promptText = "PRESS <ENTER>/<RETURN> TO CONTINUE";
        }
        cfg.promptTxt = promptText;
        cfg.textColor = 15;
        cfg.selectColor = 15;
        cfg.promptColor = 15;
    } else {
        cfg.promptTxt = "";
        cfg.textColor = 10;
        cfg.selectColor = 15;
        cfg.promptColor = 15;
    }

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

    // Clear prompt row 24 after menu closes.
    Graphics::Screen *screen = _engine->getScreen();
    if (screen) {
        Goldbox::Surface screenSurface(*screen,
            Common::Rect(0, 0, screen->w, screen->h));
        screenSurface.clearBox(0, 24, 39, 24, 0);
    }

    _updateViewState();
    return result;
}

VmResult PoolradEngineHostImpl::readGeoAtPosition() {
    // Mirrors: STRUCT_POSITION.geo_id = MAP_getGEOData(y, x)
    // Also samples the wall nibble in facing direction into MapWallType.
    if (!_engine)
        return VM_OK;

    RuntimeGeoBlock &rtGeo = _engine->getRuntimeGeo();
    if (!rtGeo.isLoaded())
        return VM_OK;

    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    const uint16 xAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
    const uint16 yAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
    const uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;
    const int x = static_cast<int>(_memory->read8(xAddr));
    const int y = static_cast<int>(_memory->read8(yAddr));

    const uint8 geoId = rtGeo.getGeoData(x, y);
    debug(1, "readGeoAtPosition: pos=(%d,%d) geoId=0x%02X eventId=%u indoor=%s skyColor=%u ceilColor=%u",
        x, y, (unsigned)geoId, (unsigned)(geoId & 0x7F),
        (geoId & 0x80) ? "yes" : "no",
        (unsigned)_memory->read8(layout.vmField(kVmFieldSkyColor).vmAddr),
        (unsigned)_memory->read8(layout.vmField(kVmFieldCeilingColor).vmAddr));
    const uint16 geoFieldAddr = layout.vmGlobalField(kVmGlobalFieldMapSquareInfo).vmAddr;
    _memory->write8(geoFieldAddr, geoId);

    // Sample wall nibble in facing direction into MapWallType.
    // Original CALL 0x2C90 does this as part of MAP_getGEOData flow.
    const uint8 wireDir = static_cast<uint8>((_memory->read8(dirAddr) & 0x03) * 2);
    const uint8 wallNibble = rtGeo.getMapNibble(x, y, wireDir);
    const uint16 wallTypeAddr = layout.vmGlobalField(kVmGlobalFieldMapWallType).vmAddr;
    _memory->write8(wallTypeAddr, wallNibble);

    // Immediately derive viewport colors from the freshly-written MapSquareInfo.
    // Original GFX_ViewPortUpdate reads event_id right after MAP_getGEOData.
    setDefaultSkyboxColors();

    return VM_OK;
}

VmResult PoolradEngineHostImpl::refreshViewport() {
    // Event-driven viewport update. The authoritative render path is the
    // view layer reacting to ST_POSITION_DIRTY/ST_SKYBOX_DIRTY.
    if (!_engine)
        return VM_OK;

    // Viewport refresh restores the 3D world view. Clear both picture and
    // sprite caches so the first-person renderer takes over.
    _engine->getPictureDisplayCache().clear();
    _engine->getEncounterSpriteCache().clear();

    // Derive viewport colors (sky/ceiling switch + color registers).
    // Original GFX_ViewPortUpdate does this on every refresh.
    setDefaultSkyboxColors();

    if (g_events) {
        g_events->postEclStateMessage(EclVmMessage::ST_POSITION_DIRTY, 1,
            EclVmMessage::VT_UINT8);
    }
    return VM_OK;
}

VmResult PoolradEngineHostImpl::handleCallOpcode(uint16 callId) {
    // 0x2D CALL target dispatcher (x86/m68k compatible IDs).
    // Note: 0x2C90 is handled by the opcode handler directly via
    // readGeoAtPosition() + refreshViewport().
    switch (callId) {
    case 0x2C90:
        // Fallback if called directly (shouldn't happen with new handler).
        _updateViewState();
        return VM_OK;

    case 0x8000:
    case 0x8001: {
        // SPECIAL_COMBAT_MODE on/off. Original behavior modifies a linked
        // encounter list and may synthesize an extra hostile NPC entry.
        // Current safe subset: toggle selected character ai_control/hostile.
        Poolrad::Data::PoolradCharacter *selected =
            dynamic_cast<Poolrad::Data::PoolradCharacter *>(
                VmInterface::getSelectedCharacter());
        if (selected) {
            if (callId == 0x8000) {
                selected->ai_control = true;
                selected->hostile = false;
            } else {
                selected->ai_control = false;
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

    case 0xC01E: {
        // MAP_StepForward: check wall flag, advance party one cell in
        // facing direction, clamp to map borders (0-15), and set
        // TriedToLeaveMap if clamped.
        if (!_engine)
            return VM_OK;

        RuntimeGeoBlock &rtGeo = _engine->getRuntimeGeo();
        const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
        const uint16 xAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
        const uint16 yAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
        const uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;
        const uint16 leaveAddr = layout.vmGlobalField(kVmGlobalFieldTriedToLeaveMap).vmAddr;

        const int x = static_cast<int>(_memory->read8(xAddr));
        const int y = static_cast<int>(_memory->read8(yAddr));
        // Direction stored as cardinal index (0=N, 1=E, 2=S, 3=W).
        const uint8 cardinalDir = static_cast<uint8>(_memory->read8(dirAddr) & 0x03);
        // Wire direction for geo lookups: 0=N, 2=E, 4=S, 6=W.
        const uint8 wireDir = static_cast<uint8>(cardinalDir * 2);
        // 8-direction index for delta tables: 0=N, 2=E, 4=S, 6=W.
        const uint8 dir8 = wireDir;

        // Clear TriedToLeaveMap.
        _memory->write16LE(leaveAddr, 0);

        // Check wall passability. getWallFlag returns 0 if blocked.
        const uint8 wallFlag = rtGeo.isLoaded()
            ? rtGeo.getWallFlag(x, y, wireDir) : 1;
        if (wallFlag == 0)
            return VM_OK; // Wall blocks movement.

        // Compute new position.
        int newX = x + kDirDeltaX[dir8];
        int newY = y + kDirDeltaY[dir8];

        // Clamp to map borders and flag if clamped.
        bool clamped = false;
        if (newX > 15) { newX = 15; clamped = true; }
        if (newX < 0)  { newX = 0;  clamped = true; }
        if (newY > 15) { newY = 15; clamped = true; }
        if (newY < 0)  { newY = 0;  clamped = true; }

        if (clamped)
            _memory->write16LE(leaveAddr, 1);

        _memory->write8(xAddr, static_cast<uint8>(newX));
        _memory->write8(yAddr, static_cast<uint8>(newY));
        if (g_events) {
            g_events->postEclVmMessage(xAddr, static_cast<uint8>(newX));
            g_events->postEclVmMessage(yAddr, static_cast<uint8>(newY));
            g_events->postEclStateMessage(EclVmMessage::ST_POSITION_DIRTY, 1,
                EclVmMessage::VT_UINT8);
        }
        return VM_OK;
    }

    case 0xC018: {
        // Nibble sampling: read wall type in facing direction at current
        // position from RuntimeGeoBlock and store in MapWallType.
        if (!_engine)
            return VM_OK;

        RuntimeGeoBlock &rtGeo = _engine->getRuntimeGeo();
        if (!rtGeo.isLoaded())
            return VM_OK;

        const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
        const uint16 xAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
        const uint16 yAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
        const uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;

        const int x = static_cast<int>(_memory->read8(xAddr));
        const int y = static_cast<int>(_memory->read8(yAddr));
        // Direction is stored as cardinal (0=N,1=E,2=S,3=W); convert to
        // wire format (0=N, 2=E, 4=S, 6=W) for nibble lookup.
        const uint8 wireDir = static_cast<uint8>((_memory->read8(dirAddr) & 0x03) * 2);

        const uint8 nibble = rtGeo.getMapNibble(x, y, wireDir);

        const uint16 wallTypeAddr =
            layout.vmGlobalField(kVmGlobalFieldMapWallType).vmAddr;
        _memory->write8(wallTypeAddr, nibble);
        return VM_OK;
    }

    default:
        return VM_OK;
    }
}

/**
 * MAP_CountStepsUntilWall equivalent.
 * Walks forward from (x,y) in wireDir up to 2 steps, stopping at walls.
 * Returns step count (0-2). For outdoor maps always returns 2.
 */
static uint8 countStepsUntilWall(const RuntimeGeoBlock &rtGeo, uint8 wireDir,
        int x, int y, bool indoorMode) {
    if (!indoorMode)
        return 2;

    static const int kDx[] = {0, 0, 1, 0, 0, 0, -1, 0};
    static const int kDy[] = {-1, 0, 0, 0, 1, 0, 0, 0};

    uint8 steps = 0;
    for (uint8 i = 0; i < 2; ++i) {
        uint8 nibble = rtGeo.getMapNibble(x, y, wireDir);
        if (nibble != 0)
            break;
        steps++;
        x += kDx[wireDir & 7];
        y += kDy[wireDir & 7];
    }
    return steps;
}

VmResult PoolradEngineHostImpl::drawEncounterStage(uint8 resourceId,
        uint8 distanceCap, uint8 variantId) {
    if (!_engine)
        return VM_ERROR;

    // Calculate monster distance (MAP_CountStepsUntilWall equivalent).
    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    const uint16 xAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
    const uint16 yAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
    const uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;
    const int x = static_cast<int>(_memory->read8(xAddr));
    const int y = static_cast<int>(_memory->read8(yAddr));
    const uint8 wireDir = static_cast<uint8>((_memory->read8(dirAddr) & 0x03) * 2);

    const bool indoorMode = (_memory->read8(
        layout.vmField(kVmFieldIndoorModeFlag).vmAddr) != 0);

    RuntimeGeoBlock &rtGeo = _engine->getRuntimeGeo();
    uint8 distance = rtGeo.isLoaded()
        ? countStepsUntilWall(rtGeo, wireDir, x, y, indoorMode)
        : 2;

    // Clamp to distance cap (D_DistanceCap < D_MonsterDistance).
    if (distanceCap < distance)
        distance = distanceCap;

    // Write D_MonsterDistance to VM memory.
    _memory->write16LE(
        layout.vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr, distance);

    // Populate the engine-global encounter sprite cache.
    Goldbox::Gfx::EncounterSpriteCache &cache = _engine->getEncounterSpriteCache();
    _pendingEncounterHeadReveal = false;
    _pendingEncounterHeadRevealTime = 0;
    cache.loadSprite(resourceId, variantId, distance);

    // Load head if distance == 0 (adjacent encounter).
    // Show the last sprite frame briefly, then reveal head/body.
    if (distance == 0) {
        uint8 speed = _memory->read8(layout.vmField(kVmFieldGameSpeed).vmAddr);
        if (speed == 0)
            speed = 1;

        // Flush sprite frame to screen first.
        _updateViewState();
        if (g_events) {
            g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
                EclVmMessage::VT_UINT8);
            g_events->pumpModalInputFrame();
        }
        g_system->delayMillis(static_cast<uint32>(speed) * 150);

        const uint8 headPicId = _memory->read8(
            layout.vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr);
        cache.loadHead(headPicId, variantId);
    }

    _updateViewState();
    if (g_events) {
        g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
            EclVmMessage::VT_UINT8);
    }

    return VM_OK;
}

VmResult PoolradEngineHostImpl::redrawEncounterStage(uint8 newDistance) {
    if (!_engine)
        return VM_ERROR;

    Goldbox::Gfx::EncounterSpriteCache &cache = _engine->getEncounterSpriteCache();
    cache.setDistance(newDistance);

    if (newDistance == 0) {
        // Show last sprite frame briefly, then reveal head/body immediately.
        // Original engine draws the closest-frame sprite for one animation
        // tick before compositing the portrait on top.
        const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
        uint8 speed = _memory->read8(layout.vmField(kVmFieldGameSpeed).vmAddr);
        if (speed == 0)
            speed = 1;

        // Flush the last sprite frame to screen before loading head.
        _updateViewState();
        if (g_events) {
            g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
                EclVmMessage::VT_UINT8);
            g_events->pumpModalInputFrame();
        }
        g_system->delayMillis(static_cast<uint32>(speed) * 150);

        // Now load the head portrait so it's visible before the next opcode.
        const uint8 headPicId = _memory->read8(
            layout.vmGlobalField(kVmGlobalFieldPictureHeadId).vmAddr);
        cache.loadHead(headPicId, cache.bodyPicId());
    }

    _updateViewState();
    if (g_events) {
        g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
            EclVmMessage::VT_UINT8);
    }

    return VM_OK;
}

VmResult PoolradEngineHostImpl::spriteOff() {
    if (!_engine)
        return VM_OK;
    _engine->getEncounterSpriteCache().clear();
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

    const bool singleItemMode = (options.size() == 1);

    _asyncMenuModel.reset(new Goldbox::MenuItemList(
        buildLegacyHorizontalMenuModel(options)));

    // In singleItemMode, clear menu items — only the prompt is shown.
    if (singleItemMode)
        _asyncMenuModel->items.clear();

    Views::Dialogs::HorizontalMenuConfig cfg;
    cfg.menuItemList = _asyncMenuModel.get();
    cfg.allowNumPad = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = singleItemMode;

    if (singleItemMode) {
        // Single-item mode: show option text as prompt, accept any key.
        // Original replaces "PRESS <RETURN> OR BUTTON TO CONTINUE" with
        // platform-appropriate text.
        Common::String promptText = options[0];
        if (promptText.hasPrefix("PRESS <RETURN>") ||
                promptText.hasPrefix("PRESS <ENTER>")) {
            promptText = "PRESS <ENTER>/<RETURN> TO CONTINUE";
        }
        cfg.promptTxt = promptText;
        cfg.textColor = 15;
        cfg.selectColor = 15;
        cfg.promptColor = 15;
    } else {
        cfg.promptTxt = "";
        cfg.textColor = 10;
        cfg.selectColor = 15;
        cfg.promptColor = 15;
    }

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

VmResult PoolradEngineHostImpl::beginDelay() {
    if (_asyncMenuPending || _asyncPrintPending || _asyncDelayPending)
        return VM_ERROR;

    // Read CFG_GAME_SPEED from VM memory (1-9 scale, 0 treated as 1).
    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    uint8 speed = _memory->read8(
        layout.vmField(kVmFieldGameSpeed).vmAddr);
    if (speed == 0)
        speed = 1;

    // Convert game speed to milliseconds: speed * 500ms per unit.
    _asyncDelayEndTime = g_system->getMillis() + static_cast<uint32>(speed) * 500;
    _asyncDelayPending = true;
    return VM_YIELD;
}

VmResult PoolradEngineHostImpl::advanceClock(uint8 amount) {
    if (!_memory)
        return VmResult::VM_ERROR;

    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    if (!party)
        return VmResult::VM_ERROR;

    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    TimeFieldAddresses clockAddrs = {{
        layout.vmField(kVmFieldClockUnits).vmAddr,
        layout.vmField(kVmFieldClockMinuteOnes).vmAddr,
        layout.vmField(kVmFieldClockMinuteTens).vmAddr,
        layout.vmField(kVmFieldClockHour).vmAddr,
        layout.vmField(kVmFieldClockDay).vmAddr,
        layout.vmField(kVmFieldClockMonth).vmAddr,
        layout.vmField(kVmFieldClockYearLo).vmAddr
    }};

    EffectHandler effectHandler;
    timeAddUnits(*_memory, clockAddrs, *party, &effectHandler,
        VmInterface::getGameStatus(), 1, amount);

    // First runtime trigger-set integration: periodic poison/disease cycle.
    Goldbox::Data::Effects::EffectRuntime runtime(&effectHandler,
        Goldbox::Poolrad::getEffectHostBridge());
    for (Goldbox::Data::PlayerCharacter *character : *party) {
        if (!character)
            continue;

        Goldbox::Data::Effects::CharacterEffects *effects =
            character->getEffects();
        if (!effects)
            continue;

        runtime.checkEffectSet(Goldbox::Data::Effects::ES_POISON_CYCLE,
            *effects, *character);
    }

    if (g_events) {
        g_events->postEclStateMessage(EclVmMessage::ST_STATUS_DIRTY, 1,
            EclVmMessage::VT_UINT8);
        g_events->postEclStateMessage(EclVmMessage::ST_CHARACTER_DIRTY, 1,
            EclVmMessage::VT_UINT8);
    }

    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::checkParty(uint16 attributeAddr,
        uint16 effectId, uint16 highAddr, uint16 lowAddr) {
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    if (!party || !_memory)
        return VmResult::VM_ERROR;

    // Decompile-aligned first pass:
    // - attributeAddr == 0 and effectId != 0 -> count members having effect.
    // - write count to lowAddr; highAddr is reserved for attribute mode.
    if (attributeAddr == 0 && effectId != 0) {
        uint16 count = 0;
        for (Goldbox::Data::PlayerCharacter *character : *party) {
            if (!character)
                continue;

            Goldbox::Data::Effects::CharacterEffects *effects =
                character->getEffects();
            if (!effects)
                continue;

            const Common::List<Goldbox::Data::Effects::Effect> &list =
                effects->effects();
            bool hasEffect = false;
            for (const Goldbox::Data::Effects::Effect &effect : list) {
                if (effect.id == static_cast<uint8>(effectId)) {
                    hasEffect = true;
                    break;
                }
            }

            if (hasEffect)
                ++count;
        }

        _memory->write16LE(lowAddr, count);
        if (highAddr != 0)
            _memory->write16LE(highAddr, 0);
        return VmResult::VM_OK;
    }

    // Attribute mode not fully mapped yet from decompile struct offsets.
    // Keep deterministic behavior while preserving script flow.
    if (attributeAddr != 0 && effectId == 0) {
        _memory->write16LE(highAddr, 0);
        _memory->write16LE(lowAddr, 0);
        return VmResult::VM_OK;
    }

    // Unsupported mixed-operand mode.
    _memory->write16LE(highAddr, 0);
    _memory->write16LE(lowAddr, 0);
    return VmResult::VM_OK;
}

bool PoolradEngineHostImpl::hasEffectActive(uint8 effectId) const {
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    if (!party)
        return false;

    for (Goldbox::Data::PlayerCharacter *character : *party) {
        if (!character)
            continue;

        Goldbox::Data::Effects::CharacterEffects *effects =
            character->getEffects();
        if (!effects)
            continue;

        const Common::List<Goldbox::Data::Effects::Effect> &list =
            effects->effects();
        for (const Goldbox::Data::Effects::Effect &effect : list) {
            if (effect.id == effectId)
                return true;
        }
    }

    return false;
}

VmResult PoolradEngineHostImpl::hasEffect(uint8 effectId, uint16 resultAddr) {
    const bool present = hasEffectActive(effectId);
    if (_memory && resultAddr != 0)
        _memory->write16LE(resultAddr, present ? 0 : 1);
    return VmResult::VM_OK;
}

void PoolradEngineHostImpl::clearTextBox() {
    if (g_events) {
        g_events->postEclSyscallMessage(0, 0,
            EclVmMessage::SC_CLEAR_TEXTBOX, 0);
    }
}

VmResult PoolradEngineHostImpl::beginPrintAsync(const Common::String &text,
        bool clearBox) {
    if (_asyncMenuPending || _asyncPrintPending || _asyncDelayPending
            || _asyncCombatPending)
        return VM_ERROR;

    if (!g_events) {
        printText(text, clearBox);
        return VM_OK;
    }

    // Post text to InGameView via EclVmMessage; View handles rendering.
    g_events->postEclVmMessage(
        EclVmMessage::makeSyscallWithText(0, 0,
            EclVmMessage::SC_PRINT_ASYNC, text,
            clearBox ? 1 : 0));

    // VM yields until View signals completion via RuntimeExchange.
    _asyncPrintPending = true;
    return VM_YIELD;
}

bool PoolradEngineHostImpl::hasPendingAsync() const {
    return _asyncMenuPending || _asyncPrintPending || _asyncDelayPending
    || _asyncShopPending || _asyncCombatPending;
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
        // VM resumes when View signals textbox completion via RuntimeExchange.
        const RuntimeExchange *exchange = _engine
            ? _engine->getRuntimeExchange() : nullptr;
        return !exchange || exchange->hasAsync(
            RuntimeExchange::kAsyncTextBoxDone);
    }

    if (_asyncDelayPending) {
        return g_system->getMillis() >= _asyncDelayEndTime;
    }

    if (_asyncShopPending) {
        // InGameView signals kAsyncShopDone via RuntimeExchange when the
        // shop dialog exits. We must wait at least one frame for the
        // ST_ENTER_SHOP event to dispatch before checking completion.
        if (!_asyncShopWasActivated) {
            // First frame: the event hasn't dispatched yet. Mark as
            // activated unconditionally after one pump cycle.
            _asyncShopWasActivated = true;
            return false;
        }
        // Once activated, wait for completion signal.
        const RuntimeExchange *exchange = _engine
            ? _engine->getRuntimeExchange() : nullptr;
        return !exchange || exchange->hasAsync(
            RuntimeExchange::kAsyncShopDone);
    }

    if (_asyncCombatPending) {
        if (!_asyncCombatWasActivated) {
            _asyncCombatWasActivated = true;
            return false;
        }

        return !g_events || !g_events->isPresent("Combat");
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

        // Gap #5: Clear prompt row 24 after menu closes.
        Graphics::Screen *screen = _engine ? _engine->getScreen() : nullptr;
        if (screen) {
            Goldbox::Surface screenSurface(*screen,
                Common::Rect(0, 0, screen->w, screen->h));
            screenSurface.clearBox(0, 24, 39, 24, 0);
        }

        // Gap #4: Refresh viewport if a picture/sprite was active.
        _updateViewState();
        return VM_OK;
    }

    if (_asyncPrintPending) {
        // Consume the async completion signal from RuntimeExchange.
        RuntimeExchange *exchange = _engine
            ? _engine->getRuntimeExchange() : nullptr;
        if (exchange) {
            RuntimeExchange::AsyncCompletion completion;
            exchange->pollAsync(completion);
        }
        _asyncPrintPending = false;
        return VM_OK;
    }

    if (_asyncDelayPending) {
        _asyncDelayPending = false;
        _pendingEncounterHeadReveal = false;
        _pendingEncounterHeadRevealTime = 0;
        return VM_OK;
    }

    if (_asyncShopPending) {
        // Consume the async completion signal from RuntimeExchange.
        RuntimeExchange *exchange = _engine
            ? _engine->getRuntimeExchange() : nullptr;
        if (exchange) {
            RuntimeExchange::AsyncCompletion completion;
            exchange->pollAsync(completion);
        }
        _asyncShopPending = false;
        _asyncShopWasActivated = false;
        return VM_OK;
    }

    if (_asyncCombatPending) {
        _asyncCombatPending = false;
        _asyncCombatWasActivated = false;
        clearMonsters();
        return VM_OK;
    }

    return VM_OK;
}

VmResult PoolradEngineHostImpl::loadScript(uint8 scriptID) {
    if (!_engine || !_memory)
        return VmResult::VM_ERROR;

    Goldbox::Data::DaxBlock *eclBlock =
        _engine->getDaxEcl().getBlockById(scriptID);
    if (!eclBlock || eclBlock->_data.size() <= 2) {
        warning("PoolradEngineHostImpl::loadScript: ECL block %u not found or too small",
            (unsigned)scriptID);
        return VmResult::VM_ERROR;
    }

    // Zero VMBANK3 (0x1E00 bytes at 0x9900) before loading new script.
    for (uint16 i = 0; i < 0x1E00; ++i)
        _memory->write8(static_cast<uint16>(0x9900 + i), 0);

    // ECL_LoadBlock skips first 2 bytes (size header) then copies into VMBANK3.
    const uint32 dataSize = eclBlock->_data.size() - 2;
    const uint32 copySize = (dataSize > 0x1E00) ? 0x1E00 : dataSize;
    _memory->loadBytes(0x9900,
        Common::Span<const uint8>(eclBlock->_data.data() + 2, copySize));

    debug(2, "PoolradEngineHostImpl::loadScript: loaded ECL block %u (%u bytes)",
        (unsigned)scriptID, (unsigned)copySize);
    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::loadGeoBlock(uint8 blockId) {
    debug(3, "PoolradEngineHostImpl::loadGeoBlock: requested blockId=%u", (unsigned)blockId);
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

    // Rebuild the 2D area map cache from the freshly loaded GEO data.
    _engine->getAreaMapCache().rebuild(rtGeo, _engine->getTileCache());

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
    debug(3, "PoolradEngineHostImpl::loadWallSet: blockId=%u setSlot=%u",
        (unsigned)blockId, (unsigned)setSlot);
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

    // First pass: load all tile atlases into the cache so that cross-slot
    // tile references (e.g. a surface in slot 1 referencing a tile from
    // slot 3) are resolved correctly during surface building.
    for (int i = 0; i < numChunks; ++i) {
        const int curSlot = setSlot + i;
        if (curSlot < 1 || curSlot > 3)
            break;

        const int slotIdx = curSlot - 1;

        walldefCache.clearSlot(curSlot);
        _walldefTiles[slotIdx].reset();
        tileCache.setSlot(curSlot, nullptr);

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

        _wallSetStates[slotIdx].loaded = true;
        _wallSetStates[slotIdx].walldefBlockId = blockId;
        _wallSetStates[slotIdx].tileBlockId = static_cast<uint8>(tileBlockId);
        _wallSetStates[slotIdx].chunkIndex = static_cast<uint8>(i);
    }

    // Second pass: build wall-region surfaces now that all tile atlases
    // for this walldef block are present in the cache.
    for (int i = 0; i < numChunks; ++i) {
        const int curSlot = setSlot + i;
        if (curSlot < 1 || curSlot > 3)
            break;
        walldefCache.loadSlot(curSlot, walldef, i, tileCache);
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
    debug(2, "PoolradEngineHostImpl::onMapDataReady: area map data fully loaded");

    if (!_engine)
        return VmResult::VM_OK;

    // GAME_ScreenByState + DIALOG_ShowParty: refresh the InGameView layout
    // and party panel for the current game state.
    if (g_events) {
        g_events->postEclStateMessage(EclVmMessage::ST_GAME_STATE,
            static_cast<uint16>(_engine->getGameState()),
            EclVmMessage::VT_UINT8);
    }

    return VmResult::VM_OK;
}

VmResult PoolradEngineHostImpl::enterShop() {
    if (_asyncShopPending)
        return VM_ERROR;
    if (!g_events)
        return VM_OK;

    _asyncShopPending = true;
    _asyncShopWasActivated = false;
    g_events->postEclStateMessage(EclVmMessage::ST_ENTER_SHOP,
        static_cast<uint16>(Views::Dialogs::SHOP_STORE),
        EclVmMessage::VT_UINT8);
    return VM_YIELD;
}

VmResult PoolradEngineHostImpl::enterTemple() {
    if (_asyncShopPending)
        return VM_ERROR;
    if (!g_events)
        return VM_OK;

    _asyncShopPending = true;
    _asyncShopWasActivated = false;
    g_events->postEclStateMessage(EclVmMessage::ST_ENTER_SHOP,
        static_cast<uint16>(Views::Dialogs::SHOP_TEMPLE),
        EclVmMessage::VT_UINT8);
    return VM_YIELD;
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
    const uint8 wireDir = static_cast<uint8>((snapshot.dungeonDir & 0x03) * 2);

    const uint8 doorFlag = rtGeo.getWallFlag(x, y, wireDir);

    // doorFlag 0 = solid wall (no door), 1 = open door (already passable).
    if (doorFlag < 2)
        return false;

    // Closed/locked door directly ahead — activate DoorDialog.
    Views::InGameView *igv = dynamic_cast<Views::InGameView *>(
        _engine->findView("InGame"));
    if (igv)
        igv->openDoor();

    return false; // Don't advance position here; DoorDialog handles it async.
}

void PoolradEngineHostImpl::playSound(uint8 soundId) {
    // TODO: Wire to ScummVM audio mixer once sound system is implemented.
    // Original: PlaySound(ARRAY_SOUND_MAP[soundId]) / WORD_SOUND_ID_N.
    debug(3, "PoolradEngineHostImpl::playSound: sound %u requested",
        (unsigned)soundId);
}

/**
 * ARRAY_COLOR_LOOKUP: maps G_SkyColor / G_CeilingColor world bank
 * values to EGA palette indices. First 16 entries from x86 original.
 */
static const uint8 kColorLookup[16] = {
    0x00, 0x0F, 0x0C, 0x0B, 0x0D, 0x0A, 0x09, 0x0E,
    0x00, 0x0F, 0x0C, 0x0B, 0x0D, 0x0A, 0x09, 0x0E
};

static uint8 lookupSkyColor(uint8 index) {
    if (index < 16)
        return kColorLookup[index];
    return index & 0x0F;
}

/**
 * GFX_Get3DDarkColor: COLOR_REG_FAR → skyline palette index.
 * Platform-identical on x86 and m68k.
 */
static uint8 get3DDarkColor(uint8 regFar) {
    switch (regFar) {
    case 0x08: return 0x00;
    case 0x41: return 0x00;
    case 0xE8: return 0x0F;
    default:   return 0x0F;
    }
}

/**
 * GFX_Get3DWallColor: COLOR_REG_FAR → horizon palette index.
 * Platform-identical on x86 and m68k.
 */
static uint8 get3DWallColor(uint8 regFar) {
    switch (regFar) {
    case 0x08: return 0x08;
    case 0x41: return 0x07;
    case 0xE8: return 0x07;
    default:   return 0x07;
    }
}

/**
 * GFX_Get3DFloorColor: COLOR_REG_FLOOR → floor palette index.
 * Uses x86 mapping (EGA palette).
 */
static uint8 get3DFloorColor(uint8 regFloor) {
    switch (regFloor) {
    case 0x0B: return 0x08;
    case 0x09: return 0x06;
    case 0xDB: return 0x06;
    case 0x06: return 0x01;
    default:   return 0x06;
    }
}

void PoolradEngineHostImpl::setDefaultSkyboxColors() {
    // GFX_ViewPortUpdate color derivation:
    // 1. Sky color depends on event_id at current position:
    //    - event_id < 0x80: sky = G_SkyColor (outdoor)
    //    - event_id >= 0x80: sky = G_CeilingColor (indoor)
    // 2. Skyline/horizon/floor derived from COLOR_REG_FAR / COLOR_REG_FLOOR.
    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();

    // Determine sky color based on event_id bit 7.
    const uint8 eventId = _memory->read8(
        layout.vmGlobalField(kVmGlobalFieldMapSquareInfo).vmAddr);
    if (eventId < 0x80) {
        _colorSky = lookupSkyColor(_memory->read8(
            layout.vmField(kVmFieldSkyColor).vmAddr));
    } else {
        _colorSky = lookupSkyColor(_memory->read8(
            layout.vmField(kVmFieldCeilingColor).vmAddr));
    }

    // Derive skyline/horizon/floor from color registers.
    const uint8 regFloor = _memory->read8(
        layout.vmGlobalField(kVmGlobalFieldColorFlagFloor).vmAddr);
    const uint8 regFar = _memory->read8(
        layout.vmGlobalField(kVmGlobalFieldColorFlagHorizon).vmAddr);

    _colorSkyline = get3DDarkColor(regFar);
    _colorHorizon = get3DWallColor(regFar);
    _colorFloor   = get3DFloorColor(regFloor);

    if (_engine)
        _engine->setColors(_colorSky, _colorSkyline, _colorHorizon, _colorFloor);
}

// ----------------------------------------------------------------------------
// 0x27 TREASURE — Populate global treasure pool with coins and items.
// ----------------------------------------------------------------------------

static uint8 rollRandomMagicItemType() {
    int roll = VmInterface::rollDice(1, 100);

    if (roll >= 1 && roll <= 60) {
        int roll2 = VmInterface::rollDice(1, 100);
        if (roll2 >= 1 && roll2 <= 47)
            return static_cast<uint8>(roll2);
        if (roll2 == 48 || roll2 == 49)
            return 59;
        if (roll2 >= 50 && roll2 <= 59)
            return static_cast<uint8>(roll2);
        if (roll2 >= 60 && roll2 <= 90) {
            int roll3 = VmInterface::rollDice(1, 10);
            if (roll3 >= 1 && roll3 <= 4) return 36;
            if (roll3 >= 5 && roll3 <= 7) return 35;
            if (roll3 == 8) return 34;
            if (roll3 == 9) return 37;
            return 38;
        }
        if (roll2 >= 91 && roll2 <= 94) return 73;
        if (roll2 >= 95 && roll2 <= 97) return 93;
        if (roll2 >= 98 && roll2 <= 100) return 77;
    } else if (roll >= 61 && roll <= 85) {
        return 61;
    } else if (roll >= 86 && roll <= 92) {
        return 62;
    } else if (roll >= 93 && roll <= 98) {
        int roll2 = VmInterface::rollDice(1, 16);
        if (roll2 >= 1 && roll2 <= 7)  return 71;
        if (roll2 == 8 || roll2 == 9)  return 70;
        if (roll2 == 10) return 84;
        if (roll2 == 11 || roll2 == 12) return 78;
        if (roll2 == 13 || roll2 == 14) return 79;
        if (roll2 == 15) return 92;
        return 67;
    } else {
        return 69;
    }

    return 36;
}

VmResult PoolradEngineHostImpl::setupTreasure(uint8 copper, uint8 silver,
        uint8 electrum, uint8 gold, uint8 platinum, uint8 gems,
        uint8 jewelry, uint8 itemSetId) {
    if (!_engine)
        return VM_ERROR;

    TreasurePool &pool = _engine->getTreasurePool();
    pool.clear();

    pool.setCoin(Goldbox::Data::VAL_COPPER,   copper);
    pool.setCoin(Goldbox::Data::VAL_SILVER,   silver);
    pool.setCoin(Goldbox::Data::VAL_ELECTRUM, electrum);
    pool.setCoin(Goldbox::Data::VAL_GOLD,     gold);
    pool.setCoin(Goldbox::Data::VAL_PLATINUM, platinum);
    pool.setCoin(Goldbox::Data::VAL_GEMS,     gems);
    pool.setCoin(Goldbox::Data::VAL_JEWELRY,  jewelry);

    if (itemSetId < 0x80) {
        Goldbox::Data::DaxBlockContainer &itemDax = _engine->getDaxItem();
        Goldbox::Data::DaxBlock *block = itemDax.getBlockById(itemSetId);
        if (!block || block->_data.empty()) {
            debug(1, "setupTreasure: unable to find item block %u",
                (unsigned)itemSetId);
            return VM_OK;
        }

        const uint32 blockSize = block->_data.size();
        const uint32 recSize = 63;
        const uint32 itemCount = blockSize / recSize;
        debug(3, "setupTreasure: ITEM block %u size=%u items=%u",
            (unsigned)itemSetId, blockSize, itemCount);

        Common::MemoryReadStream stream(block->_data.data(), blockSize);
        for (uint32 idx = 0; idx < itemCount; ++idx) {
            Goldbox::Data::Items::CharacterItem it;
            it.name = Goldbox::Data::PascalStringBuffer<41>::read(stream);
            it.nextAddress = stream.readUint32LE();
            it.typeIndex   = stream.readByte();
            it.nameCode1   = stream.readByte();
            it.nameCode2   = stream.readByte();
            it.nameCode3   = stream.readByte();
            it.bonus       = stream.readByte();
            it.saveBonus   = stream.readByte();
            it.readied     = stream.readByte();
            it.hidden      = stream.readByte();
            it.cursed      = stream.readByte();
            it.weight      = stream.readUint16LE();
            it.stackSize   = stream.readByte();
            it.value       = stream.readUint16LE();
            it.effect1     = stream.readByte();
            it.effect2     = stream.readByte();
            it.effect3     = stream.readByte();
            pool.addItem(it);
            debug(3, "  item[%u]: typeIndex=%u name='%s'",
                idx, (unsigned)it.typeIndex,
                it.getDisplayName().c_str());
        }

        debug(3, "setupTreasure: loaded %u items from ITEM block %u",
            itemCount, (unsigned)itemSetId);

    } else if (itemSetId != 0xFF) {
        const uint8 count = itemSetId - 0x80;
        for (uint8 i = 0; i < count; ++i) {
            const uint8 itemTypeId = rollRandomMagicItemType();

            // TODO: Full ITEM_createRandomMagicItem(itemTypeId) implementation.
            Goldbox::Data::Items::CharacterItem item;
            item.typeIndex = itemTypeId;
            item.bonus = 0;
            item.nameCode1 = 0;
            item.nameCode2 = 0;
            item.nameCode3 = 0;
            item.readied = 0;
            item.hidden = 0;
            item.cursed = 0;
            item.weight = 0;
            item.stackSize = 1;
            item.value = 0;
            item.effect1 = 0;
            item.effect2 = 0;
            item.effect3 = 0;
            item.nextAddress = 0;
            pool.addItem(item);
            debug(3, "  random item[%u]: typeIndex=%u",
                (unsigned)i, (unsigned)itemTypeId);
        }

        debug(3, "setupTreasure: generated %u random magic items",
            (unsigned)count);
    }

    return VM_OK;
}

} // namespace Poolrad
} // namespace Goldbox
