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
#include "goldbox/core/coords.h"
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
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/items/random_item.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/poolrad_spell_mapping.h"
#include "goldbox/runtime/runtime_exchange.h"
#include "goldbox/runtime/runtime_geo.h"
#include "goldbox/runtime/runtime_time.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/poolrad/post_combat.h"
#include "goldbox/poolrad/program_services.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_input.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/party_selector.h"
#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"
#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/ecl/ecl_string_writer.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/poolrad/views/combat_view.h"
#include "goldbox/core/direction.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/ecl/npc_recruitment.h"
#include "goldbox/ecl/encounter_rules.h"
#include "goldbox/poolrad/encounter_operation.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/effects/effect.h"

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

    void showHealResult(Goldbox::Data::PlayerCharacter *character) override {
        if (!character)
            return;

        const char *msg = (character->hitPoints.current >= character->hitPoints.max)
            ? "is fully healed"
            : "is partially healed";
        postEffectMessage(character, msg, true);

        if (!character->combatState)
            requestRefresh(RF_CHARACTER_PANEL | RF_STATUS_PANEL);
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
    int keyCode;

    AsyncMenuResultSink(const Common::String &name, Goldbox::UIElement *parent)
        : Goldbox::UIElement(name, parent), done(false), success(false), value(-1),
          keyCode(0) {
    }

    void handleMenuResult(const Goldbox::MenuResultMessage &result) override {
        done = true;
        success = result._success;
        value = result._hasIntValue ? result._intValue : -1;
        keyCode = result._keyCode;
    }
};

// GB13-003: sink for the HorizontalInput dialog (INPUT NUMBER / INPUT STRING).
// HorizontalInput posts a MenuResult carrying the typed string and the closing
// keycode (RETURN or ESCAPE). We capture the raw text and keycode; numeric
// validation and the bank-aware write happen on finalize so an invalid or
// cancelled entry never mutates VM memory.
class AsyncInputResultSink : public Goldbox::UIElement {
public:
    bool done;
    bool success;
    bool cancelled;
    Common::String text;

    AsyncInputResultSink(const Common::String &name, Goldbox::UIElement *parent)
        : Goldbox::UIElement(name, parent), done(false), success(false),
          cancelled(false) {
    }

    void handleMenuResult(const Goldbox::MenuResultMessage &result) override {
        done = true;
        success = result._success;
        // ESCAPE closes the input as a cancel; RETURN confirms.
        cancelled = (result._keyCode == Common::KEYCODE_ESCAPE);
        text = result._hasStringValue ? result._stringValue : Common::String();
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
    teardownInputAsync();
    teardownVertMenuAsync();
    teardownWhoAsync();
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

    // GB13-010: arm the exactly-once settlement for THIS battle, so two
    // consecutive fights each settle once.
    _combatSettled = false;

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
    params.advanceClockCtx = this;
    params.advanceClockFn = [](void *ctx, uint8 minutes) {
        static_cast<PoolradEngineHostImpl *>(ctx)->advanceClock(minutes);
    };

    const RuntimeExchange *exchange = _engine->getRuntimeExchange();
    RuntimeMapSnapshot snapshot;
    if (exchange && exchange->captureMapSnapshot(snapshot) && snapshot.valid) {
        params.mapDirection = static_cast<uint8>((snapshot.dungeonDir & 0x03) * 2);
        params.isDungeon = snapshot.indoorMode;
        params.mapCenter = MapPos(static_cast<int8>(snapshot.dungeonPos.x & 0xFF),
                                  static_cast<int8>(snapshot.dungeonPos.y & 0xFF));
        params.playerY = static_cast<int8>(snapshot.dungeonPos.y & 0xFF);
        params.eclScriptId = snapshot.mapId;
        params.wild = snapshot.wildernessPos;
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

    // GB13-009 (8.2): new battle setup starts a fresh context. Bump the
    // request generation so a late completion from a prior battle (or the
    // exploration context being suspended) cannot mutate this battle's owner.
    _pendingOps.bumpGeneration();

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

        // Preserve legacy combat assumptions: loaded monsters are enemies
        // and enabled (active in combat) by default.
        monster->combatSide = Goldbox::Data::CS_ENEMY;
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

// GB13-010 — adapter: an engine dice source for the pure settlement core.
// Forwards to the real VM RNG so the live loot-item inclusion uses the same
// sequence as the rest of combat. Tests drive the core with their own RNG.
namespace {
struct HostPostCombatRng : public Goldbox::Poolrad::PostCombatRng {
    int rollDice(int count, int sides) override {
        return Goldbox::VmInterface::rollDice(count, sides);
    }
};
} // namespace

void PoolradEngineHostImpl::settleCombatLoot(bool defeat) {
    // Exactly-once: a duplicated finalize completion must not re-award.
    if (_combatSettled || !_engine)
        return;
    _combatSettled = true;

    Common::List<Goldbox::Data::PlayerCharacter *> &party = _engine->getParty();

    // Build the immutable settlement input in roster order: party/NPC entries
    // first, host monster records last, matching the original creatureList
    // layout the settlement loops depend on. Ownership of the live roster is
    // retained by the engine until clearMonsters() runs AFTER this returns, so
    // every value/transfer is captured while the roster is still live.
    Goldbox::Poolrad::PostCombatInput input;
    input.partySize = 0;

    Common::Array<Goldbox::Data::PlayerCharacter *> order;
    // Pass 1: non-monster party/NPC members.
    for (Common::List<Goldbox::Data::PlayerCharacter *>::iterator it =
            party.begin(); it != party.end(); ++it) {
        Goldbox::Data::PlayerCharacter *ch = *it;
        if (!ch)
            continue;
        bool isMonster = false;
        for (Goldbox::Poolrad::Data::PoolradCharacter *m : _loadedMonsters) {
            if (m == ch) { isMonster = true; break; }
        }
        if (!isMonster)
            order.push_back(ch);
    }
    const uint partyBlock = order.size();
    // Pass 2: host monster records (in load order).
    for (Goldbox::Poolrad::Data::PoolradCharacter *m : _loadedMonsters)
        order.push_back(m);

    for (uint i = 0; i < order.size(); ++i) {
        Goldbox::Data::PlayerCharacter *ch = order[i];
        Goldbox::Poolrad::PostCombatCombatant c;
        c.isMonster = (i >= partyBlock);
        c.enabled = ch->enabled;
        c.status = ch->healthStatus;
        c.hostile = (ch->combatSide == Goldbox::Data::CS_ENEMY) ? 1 : 0;
        c.npc = ch->npc;
        c.classType = ch->classType;
        c.wisCur = ch->abilities.wisdom.current;
        c.strCur = ch->abilities.strength.current;
        c.intCur = ch->abilities.intelligence.current;
        c.dexCur = ch->abilities.dexterity.current;
        c.hpCur = static_cast<int16>(ch->hitPoints.current);

        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(ch);
        if (pc) {
            c.xpAward = pc->xpForDefeating;
            c.xpBonusPerHp = pc->bonusXpPerHp;
            c.hpRolled = pc->hitPointsRolled;
            // npcTreasureShare is not yet modeled on PoolradCharacter; the
            // morale byte (npc & 0x7f) is the closest source, and only the low
            // 3 bits are consumed. A dedicated field is added when the monster
            // record carries it (see integration note).
            c.npcTreasureShare = static_cast<uint8>(ch->npc & 0x7f);
            for (int d = 0; d < Goldbox::Data::VALUABLE_COUNT; ++d)
                c.coins[d] = pc->valuableItems[static_cast<Goldbox::Data::ValuableType>(d)];
        }
        input.combatants.push_back(c);
        if (!c.isMonster)
            ++input.partySize;
    }

    HostPostCombatRng rng;
    Goldbox::Poolrad::PostCombatResult result =
        Goldbox::Poolrad::settlePostCombat(input, rng);

    // Apply the pooled battle currency into the shared TreasurePool so the
    // loot UI (TreasureDialog / GS_AFTER_COMBAT) finds it. Widened pool, so
    // add in 32-bit without narrowing.
    Runtime::TreasurePool &pool = _engine->getTreasurePool();
    for (int d = 0; d < Goldbox::Data::VALUABLE_COUNT; ++d) {
        if (result.treasureMoney[d] > 0)
            pool.addCoinWide(static_cast<Goldbox::Data::ValuableType>(d),
                result.treasureMoney[d]);
    }

    // Apply XP to eligible recipients (experiencePoints is uint32).
    for (uint k = 0; k < result.xpAwards.size(); ++k) {
        const Goldbox::Poolrad::PostCombatXpAward &a = result.xpAwards[k];
        if (a.index < order.size())
            order[a.index]->experiencePoints += a.share;
    }

    // Apply the status transitions (running recovery, Dying->Unconscious,
    // positive-HP recovery). Record removals are left to clearMonsters /
    // party removal; here we only restore the persistent party status.
    for (uint k = 0; k < result.statusChanges.size(); ++k) {
        const Goldbox::Poolrad::PostCombatStatusChange &sc = result.statusChanges[k];
        if (sc.index >= order.size() || sc.remove)
            continue;
        order[sc.index]->healthStatus = sc.newStatus;
        if (sc.setEnabled)
            order[sc.index]->enabled = true;
    }

    // NOTE (integration-only): the VM COMBAT_RESULT global write (0 / $81 /
    // $80) and the loot/summary presentation (TreasureDialog, GS_AFTER_COMBAT,
    // the NPC-share "takes his share" window) are driven by the view layer and
    // are not performed here; this host hook only commits the deterministic
    // rewards (pooled currency, XP, status recovery) while the roster is live.
    (void)defeat;
}

VmResult PoolradEngineHostImpl::clearMonsters() {
    if (_engine && _monstersAppendedToParty) {
        Common::List<Goldbox::Data::PlayerCharacter *> &party =
            _engine->getParty();

        // GB13-011 (Requirement 4.4 / 18.3): the departed combatants ACTUALLY
        // leave here, so invalidate any pending target/selection reference for
        // each one BEFORE its pointer dies, so a stale targeting/selection
        // callback cannot act on it. The combatant stable-id source is the
        // same one GB13-006 uses for party removal: the member's 0-based index
        // in the live party list (PartyRemovalResult::departedIndex). Capture
        // each monster's index while it is still linked, then remove.
        for (Data::PoolradCharacter *monster : _loadedMonsters) {
            uint16 combatantId = 0;
            uint16 i = 0;
            bool found = false;
            for (Common::List<Goldbox::Data::PlayerCharacter *>::iterator it =
                    party.begin(); it != party.end(); ++it, ++i) {
                if (*it == monster) {
                    combatantId = i;
                    found = true;
                    break;
                }
            }
            if (found)
                invalidatePendingForCharacter(combatantId);
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

// GB13-006: synchronize the VM character window after party membership or
// selection changed. Keeps kVmGlobalFieldPartyCount and the selected-pc index
// coherent with the engine party list + engine selection in ONE place, so the
// bookkeeping is not scattered across dialogs (Requirement 5.3).
void PoolradEngineHostImpl::_syncPartyWindow(
        Goldbox::Data::PlayerCharacter *selection) {
    if (!_engine || !_memory)
        return;

    Common::List<Goldbox::Data::PlayerCharacter *> &party = _engine->getParty();
    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();

    // Party count mirrors the live list size (PARTY_SIZE).
    _memory->write8(layout.vmGlobalField(kVmGlobalFieldPartyCount).vmAddr,
        static_cast<uint8>(party.size() & 0xFF));

    // Selected-pc index is the 0-based position of the selection in list order.
    // When the selection is not (or no longer) a member, fall back to the head.
    uint8 selectedIndex = 0;
    Goldbox::Data::PlayerCharacter *resolved =
        party.empty() ? nullptr : party.front();
    uint8 i = 0;
    for (Common::List<Goldbox::Data::PlayerCharacter *>::iterator it =
            party.begin(); it != party.end(); ++it, ++i) {
        if (*it == selection) {
            selectedIndex = i;
            resolved = selection;
            break;
        }
    }

    _memory->write8(layout.vmGlobalField(kVmGlobalFieldSelectedPcIndex).vmAddr,
        selectedIndex);
    _engine->setSelectedCharacter(resolved);
    _engine->_nextCharacter = party.empty() ? nullptr : party.front();
}

VmResult PoolradEngineHostImpl::addNpc(uint8 monsterId, uint8 morale) {
    if (!_engine)
        return VmResult::VM_ERROR;
    if (_engine->getPlatform() == Common::kPlatformAmiga) {
        warning("PoolradEngineHostImpl::addNpc: Amiga MONCHA/MONITM/MONSPC layout not implemented yet");
        return VmResult::VM_ERROR;
    }

    Common::List<Goldbox::Data::PlayerCharacter *> &party = _engine->getParty();
    // AddCreatureToParty / NPC_LoadMonsterAndAddToParty refuse to recruit once
    // the party is full (PARTY_SIZE > 7). A full party is a no-op, not an error.
    if (party.size() >= 8)
        return VmResult::VM_OK;

    Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (!iconMgr) {
        warning("PoolradEngineHostImpl::addNpc: icon manager unavailable");
        return VmResult::VM_ERROR;
    }

    // Construct the recruit through the SAME decoded monster loader used by
    // loadMonster: owned inventory (MONITM) and effects (MONSPC) plus the
    // composite icon defaults (MONCHA). resolveEquippedItems() reconstructs the
    // already-equipped item links/derived state WITHOUT replaying equip events,
    // so GB13-017 ready bonuses are NOT applied twice (same load vs replay
    // distinction GB13-017 established).
    Data::PoolradCharacter *recruit = new Data::PoolradCharacter();
    if (!loadMonsterTemplate(_engine, monsterId, *recruit)) {
        delete recruit;
        return VmResult::VM_ERROR;
    }
    loadMonsterItems(_engine, monsterId, *recruit);
    loadMonsterEffects(_engine, monsterId, *recruit);
    applyX86MonsterIconDefaults(*recruit);

    // A recruit is a PERSISTENT party member, so its icon lives in a dynamic
    // slot tracked in _recruitIconSlots (never released by clearMonsters). Use
    // the MONCHA composite icon (the original LoadCharIconAnim path builds the
    // character icon rather than a CPIC encounter sprite).
    const uint8 slotId = allocateMonsterIconSlot();
    if (!iconMgr->loadIcon(slotId, recruit->iconData, false)) {
        warning("PoolradEngineHostImpl::addNpc: failed to build icon for monster %u",
            (unsigned)monsterId);
        delete recruit;
        iconMgr->releaseIcon(slotId);
        return VmResult::VM_ERROR;
    }
    recruit->iconData.iconSlotId = slotId;
    recruit->enabled = true;
    recruit->clearEquippedItems();
    recruit->resolveEquippedItems();

    // Advance the shared dynamic-slot cursor so a subsequent loadMonster does
    // not reuse this recruit's persistent icon slot.
    _nextMonsterIconSlot = (slotId < Goldbox::Gfx::SLOT_DYNAMIC_END)
        ? static_cast<uint8>(slotId + 1)
        : kMonsterSlotStart;

    // Append through the ONE party ownership API: the engine owns _party and
    // frees every entry at teardown. PTR_SELECTED_CHAR becomes the new recruit
    // (Inst_NPC_ADD sets PTR_SELECTED_CHAR via NPC_LoadMonsterAndAddToParty).
    party.push_back(recruit);
    _recruitIconSlots.push_back(slotId);

    // Set the two original NPC fields (npc byte + hostility by id) and recalc
    // derived stats, exactly as Inst_NPC_ADD does after the add.
    ECL::applyNpcRecruitFields(*recruit, monsterId, morale);
    recruit->recalcCombatStats();

    // DIALOG_ShowParty(PTR_SELECTED_CHAR): the recruit is selected and the VM
    // character window (party count + selected index + engine selection) is
    // updated together in this single owner.
    _syncPartyWindow(recruit);
    _updateViewState();
    return VmResult::VM_OK;
}

bool PoolradEngineHostImpl::removeLoadedCharacter(
        Goldbox::Data::PlayerCharacter *member) {
    if (!_engine || !member)
        return false;

    Common::List<Goldbox::Data::PlayerCharacter *> &party = _engine->getParty();
    const ECL::PartyRemovalResult removal =
        ECL::removeSelectedStable(party, member);
    if (!removal.removed)
        return false;

    // Free the departed member's icon slot (PARTY_removeMember free_iconSlot).
    // Only release through the IconManager when we own the dynamic slot as a
    // recruit; a slot we did not allocate is left to its owner.
    Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    const uint8 slotId = member->iconData.iconSlotId;
    for (uint i = 0; i < _recruitIconSlots.size(); ++i) {
        if (_recruitIconSlots[i] == slotId) {
            if (iconMgr)
                iconMgr->releaseIcon(slotId);
            _recruitIconSlots.remove_at(i);
            break;
        }
    }

    // Requirement 4.4: the member ACTUALLY LEFT, so invalidate any pending
    // target/selection reference for its stable id before the pointer dies.
    invalidatePendingForCharacter(removal.departedIndex);

    // Keep party size / order / selection / VM window coherent in one owner.
    _syncPartyWindow(removal.newSelection);

    // The engine owns party-character lifetimes (Engine::~Engine deletes each
    // _party entry), so freeing the unlinked member here matches the original
    // FreeCreature and avoids a leak now that it is off the list.
    delete removal.departed;

    _updateViewState();
    return true;
}

void PoolradEngineHostImpl::invalidatePendingForCharacter(uint16 charId) {
    const uint32 count = _pendingOps.invalidateForCharacter(charId);
    if (count != 0) {
        debug(2, "PoolradEngineHostImpl::invalidatePendingForCharacter: neutralized %u frame(s) for departed char=%u",
            (unsigned)count, (unsigned)charId);
    }
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
    cfg.suppressUnhandledKeys = true;
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

// ---------------------------------------------------------------------------
// GB13-003: INPUT NUMBER / INPUT STRING / VERTICAL MENU / WHO
// ---------------------------------------------------------------------------

namespace {

// Resolve a stable party index (0-based, list order) to its owning character.
// The party list is engine-owned; the index is stable while membership does
// not change, and the returned pointer is the stable identity WHO selects.
Goldbox::Data::PlayerCharacter *characterAtPartyIndex(
        ::Goldbox::Engine *engine, int index) {
    if (!engine || index < 0)
        return nullptr;
    Common::List<Goldbox::Data::PlayerCharacter *> &party = engine->getParty();
    int i = 0;
    for (Common::List<Goldbox::Data::PlayerCharacter *>::iterator it =
            party.begin(); it != party.end(); ++it, ++i) {
        if (i == index)
            return *it;
    }
    return nullptr;
}

} // anonymous namespace

void PoolradEngineHostImpl::teardownInputAsync() {
    if (_asyncInputSink)
        delete _asyncInputSink;
    _asyncInputSink = nullptr;
    _asyncInputDialog = nullptr;
    _asyncInputPending = false;
    _asyncInputKind = kInputNone;
    _asyncInputResultAddr = 0;
    _asyncInputMaxLen = 0;
}

void PoolradEngineHostImpl::teardownVertMenuAsync() {
    if (_asyncVertMenuSink)
        delete _asyncVertMenuSink;
    _asyncVertMenuSink = nullptr;
    _asyncVertMenu = nullptr;
    _asyncVertMenuPending = false;
    _asyncVertMenuResultAddr = 0;
    _asyncVertMenuOptions.clear();
    _asyncVertMenuModel.reset();
}

void PoolradEngineHostImpl::teardownWhoAsync() {
    if (_asyncWhoSink)
        delete _asyncWhoSink;
    _asyncWhoSink = nullptr;
    _asyncWhoDialog = nullptr;
    _asyncWhoPending = false;
}

VmResult PoolradEngineHostImpl::beginInputNumberAsync(uint16 resultAddr,
        uint8 maxDigits) {
    if (!_engine || !g_events)
        return VM_ERROR;
    if (hasPendingAsync())
        return VM_ERROR;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return VM_ERROR;

    Views::Dialogs::HorizontalInputConfig cfg;
    cfg.promptTxt = "";
    cfg.promptColor = 15;
    cfg.maxInputLength = maxDigits;

    Common::String sinkName = Common::String::format("EclInputNumSink_%u",
        ++kModalMenuSinkCounter);
    _asyncInputSink = new AsyncInputResultSink(sinkName, focused);
    _asyncInputDialog = new Views::Dialogs::HorizontalInput(
        "EclAsyncInputNumber", cfg);
    _asyncInputSink->subView(_asyncInputDialog);
    _asyncInputDialog->activate();
    _asyncInputDialog->redraw();

    _asyncInputPending = true;
    _asyncInputKind = kInputNumber;
    _asyncInputResultAddr = resultAddr;
    _asyncInputMaxLen = maxDigits;
    _asyncInputKey = _pendingOps.push(ECL::OperationKind::Input, 0, 0,
        ECL::ReturnContextRef());
    return VM_YIELD;
}

VmResult PoolradEngineHostImpl::beginInputStringAsync(uint16 resultAddr,
        uint8 maxLength) {
    if (!_engine || !g_events)
        return VM_ERROR;
    if (hasPendingAsync())
        return VM_ERROR;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return VM_ERROR;

    // The $6B00 character-name cell caps at 15 characters; a general INPUT
    // STRING caps at 40. The dialog accepts up to the same limit.
    uint8 cap = (resultAddr == ECL::kCharacterNameVmAddr)
        ? ECL::kCharacterNameMaxLength : ECL::kInputStringMaxLength;
    if (maxLength != 0 && maxLength < cap)
        cap = maxLength;

    Views::Dialogs::HorizontalInputConfig cfg;
    cfg.promptTxt = "";
    cfg.promptColor = 15;
    cfg.maxInputLength = cap;

    Common::String sinkName = Common::String::format("EclInputStrSink_%u",
        ++kModalMenuSinkCounter);
    _asyncInputSink = new AsyncInputResultSink(sinkName, focused);
    _asyncInputDialog = new Views::Dialogs::HorizontalInput(
        "EclAsyncInputString", cfg);
    _asyncInputSink->subView(_asyncInputDialog);
    _asyncInputDialog->activate();
    _asyncInputDialog->redraw();

    _asyncInputPending = true;
    _asyncInputKind = kInputString;
    _asyncInputResultAddr = resultAddr;
    _asyncInputMaxLen = cap;
    _asyncInputKey = _pendingOps.push(ECL::OperationKind::Input, 0, 0,
        ECL::ReturnContextRef());
    return VM_YIELD;
}

VmResult PoolradEngineHostImpl::beginVerticalMenuAsync(uint16 resultAddr,
        const Common::String &message,
        const Common::Array<Common::String> &options) {
    if (!_engine || !g_events || options.empty())
        return VM_ERROR;
    if (hasPendingAsync())
        return VM_ERROR;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return VM_ERROR;

    _asyncVertMenuOptions = options;

    // The vertical list items come from an owned MenuItemList (the dialog
    // dereferences menuItemList directly). promptOptions drives the horizontal
    // navigation hint only.
    _asyncVertMenuModel.reset(new Goldbox::MenuItemList());
    _asyncVertMenuModel->generateMenuItems(_asyncVertMenuOptions, true);

    Views::Dialogs::VerticalMenuConfig cfg;
    cfg.promptTxt = message;
    cfg.promptOptions = &_asyncVertMenuOptions;
    cfg.menuItemList = _asyncVertMenuModel.get();
    cfg.headColor = 15;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.xStart = 0;
    cfg.yStart = 0;
    cfg.xEnd = 39;
    cfg.yEnd = 24;
    cfg.title = "";
    cfg.addExit = false;

    Common::String sinkName = Common::String::format("EclVertMenuSink_%u",
        ++kModalMenuSinkCounter);
    _asyncVertMenuSink = new AsyncMenuResultSink(sinkName, focused);
    _asyncVertMenu = new Views::Dialogs::VerticalMenu(
        "EclAsyncVerticalMenu", cfg);
    _asyncVertMenuSink->subView(_asyncVertMenu);
    _asyncVertMenu->activate();
    _asyncVertMenu->redraw();

    _asyncVertMenuPending = true;
    _asyncVertMenuResultAddr = resultAddr;
    _asyncVertMenuKey = _pendingOps.push(ECL::OperationKind::Input, 0, 0,
        ECL::ReturnContextRef());
    return VM_YIELD;
}

VmResult PoolradEngineHostImpl::beginSelectPartyMemberAsync(
        const Common::String &message) {
    if (!_engine || !g_events)
        return VM_ERROR;
    if (hasPendingAsync())
        return VM_ERROR;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return VM_ERROR;

    Views::Dialogs::PartySelectorConfig cfg;
    cfg.promptText = message.empty() ? Common::String("WHO") : message;
    cfg.allowExit = true;
    cfg.excludedCharacter = nullptr;

    Common::String sinkName = Common::String::format("EclWhoSink_%u",
        ++kModalMenuSinkCounter);
    _asyncWhoSink = new AsyncMenuResultSink(sinkName, focused);
    _asyncWhoDialog = new Views::Dialogs::PartySelector(
        "EclAsyncWho", cfg);
    _asyncWhoSink->subView(_asyncWhoDialog);
    _asyncWhoDialog->activate();
    _asyncWhoDialog->redraw();

    _asyncWhoPending = true;
    _asyncWhoKey = _pendingOps.push(ECL::OperationKind::Input, 0, 0,
        ECL::ReturnContextRef());
    return VM_YIELD;
}

int16 PoolradEngineHostImpl::inputNumber(uint8 maxDigits) {
    // Synchronous fallback for callers that do not use the async path. The
    // int16 return cannot represent the full 0..65535 domain, so the live
    // game always rides the async typed path (beginInputNumberAsync). -1
    // means no value entered.
    (void)maxDigits;
    return -1;
}

Common::String PoolradEngineHostImpl::inputString(uint8 maxLength) {
    (void)maxLength;
    return Common::String();
}

int16 PoolradEngineHostImpl::verticalMenu(const Common::String &message,
        const Common::Array<Common::String> &options) {
    (void)message;
    (void)options;
    return -1;
}

int16 PoolradEngineHostImpl::selectPartyMember(const Common::String &message) {
    (void)message;
    return -1;
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
    const MapPos geoPos(static_cast<int8>(x), static_cast<int8>(y));

    const uint8 geoId = rtGeo.getGeoData(geoPos);
    debug(1, "readGeoAtPosition: pos=(%d,%d) geoId=0x%02X eventId=%u indoor=%s skyColor=%u ceilColor=%u",
        x, y, (unsigned)geoId, (unsigned)(geoId & 0x7F),
        (geoId & 0x80) ? "yes" : "no",
        (unsigned)_memory->read8(layout.vmField(kVmFieldSkyColor).vmAddr),
        (unsigned)_memory->read8(layout.vmField(kVmFieldCeilingColor).vmAddr));
    const uint16 geoFieldAddr = layout.vmGlobalField(kVmGlobalFieldMapSquareInfo).vmAddr;
    _memory->write8(geoFieldAddr, geoId);

    const uint8 wireDir = static_cast<uint8>((_memory->read8(dirAddr) & 0x03) * 2);
    const uint8 wallNibble = rtGeo.getMapNibble(geoPos, static_cast<Direction>(wireDir));
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

VmResult PoolradEngineHostImpl::executeProgram(uint8 programID) {
    // GB13-007 (Requirement 31.1): host-owned PROGRAM dispatch. The ecl/ layer
    // (handle_0x38_PROGRAM) already evaluated the program id and routes it
    // here through the EclEngineHost contract, so ecl/ stays poolrad-free.
    Poolrad::PoolradEngine *poolEngine =
        dynamic_cast<Poolrad::PoolradEngine *>(_engine);
    if (!poolEngine)
        return VM_ERROR;

    // Restore any temporary LOAD CHARACTER selection BEFORE dispatch so the
    // program runs against the REAL selected character (and selection is not
    // silently reset to the party front). planProgramDispatch is the pure
    // decision; the real selection is the engine's authoritative selection.
    //
    // OPEN (trace-gated): the SOURCE that holds the pre-program "real"
    // selection separate from a transient LOAD CHARACTER selection
    // (GB13-006/GB13-003) is not confirmed from the implementation package.
    // Here the engine selection IS the real selection, so the plan is a no-op
    // restore in the common case; once the temporary-selection source is
    // wired, pass it as currentSelection and the real pointer as realSelection.
    Goldbox::Data::PlayerCharacter *realSelection =
        VmInterface::getSelectedCharacter();
    Goldbox::Data::PlayerCharacter *currentSelection = realSelection;

    const ProgramDispatchPlan plan = planProgramDispatch(
        programID, realSelection, currentSelection);
    if (plan.service == kServiceUnknown) {
        warning("PoolradEngineHostImpl::executeProgram: unknown PROGRAM id %u",
            (unsigned)programID);
        return VM_ERROR;
    }
    if (plan.restoreSelectionBeforeDispatch)
        VmInterface::setSelectedCharacter(realSelection);

    return poolEngine->runProgramService(programID);
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
        // GB13-007 (Requirement 31.2/31.3): CharFightsAlone duel.
        // 0x8000 = mirror (the fighting character vs. a copy/mirror of
        // itself); 0x8001 = nonmirror (vs. the designated opponent).
        //
        // The duel runs on a TEMPORARY, isolated roster. The real party and
        // the real selected character are snapshotted and restored; the duel
        // outcome is routed through DUEL-SPECIFIC settlement (settleDuelOutcome)
        // so a lost mirror duel is NOT ordinary total-party-defeat: the
        // temporary combat data is discarded and the REAL PARTY SURVIVES.
        //
        // OPEN (trace-gated): CharFightsAlone's exact temporary-roster
        // construction and the mirror stat-copy (which PoolradCharacter fields
        // the mirror duplicates, and the opponent source for the nonmirror
        // case) are not confirmed from the implementation package. The
        // confirmable structure is implemented here: snapshot/restore the real
        // party + selection, isolate the duel at the pending-generation
        // boundary, and settle through the real-party-preserving decision.
        if (!_engine)
            return VM_ERROR;

        const bool mirror = (callId == 0x8000);

        // Snapshot the real party (members + order) and the real selection so
        // nothing the duel does can mutate them. We copy the live pointers;
        // the duel roster is a SEPARATE temporary allocation that never aliases
        // a real-party member, so cleanup frees only temporary members.
        Common::List<Goldbox::Data::PlayerCharacter *> &party =
            _engine->getParty();
        Common::Array<Goldbox::Data::PlayerCharacter *> savedParty;
        for (Common::List<Goldbox::Data::PlayerCharacter *>::iterator it =
                party.begin(); it != party.end(); ++it)
            savedParty.push_back(*it);
        Goldbox::Data::PlayerCharacter *savedSelection =
            VmInterface::getSelectedCharacter();

        // Isolate the duel context: bump the pending-operation request
        // generation (as startCombat does) so any stale completion minted in
        // the duel context cannot later touch the real party.
        _pendingOps.bumpGeneration();

        // Build the temporary duel roster. Ownership is local: every temporary
        // member is freed on EVERY return path below and a real-party member is
        // never freed here.
        Common::Array<Poolrad::Data::PoolradCharacter *> duelRoster;
        bool duelWon = false;
        bool duelResourceOk = true;

        Goldbox::Data::PlayerCharacter *fighter =
            VmInterface::getSelectedCharacter();
        Poolrad::Data::PoolradCharacter *fighterPc =
            dynamic_cast<Poolrad::Data::PoolradCharacter *>(fighter);
        if (!fighterPc) {
            // No eligible fighting character: a missing duel resource. Fail
            // truthfully; the real party is untouched (nothing was built).
            duelResourceOk = false;
        } else {
            // Mirror case constructs the opponent as a duplicate of the
            // fighting character; nonmirror uses the designated opponent. The
            // exact stat-copy/opponent source is OPEN, so no real-party member
            // is mutated and the temporary roster stays empty until the
            // CharFightsAlone construction is confirmed. The outcome is decided
            // by the confirmable settlement, not by toggling control flags.
            (void)mirror;
            // OPEN: populate duelRoster with the temporary fighter/opponent
            // copies here once CharFightsAlone construction is confirmed.
            // duelWon is left false (conservative) until the live duel runs;
            // settlement below preserves the real party regardless of outcome.
        }

        // Settle the duel through the DUEL-SPECIFIC decision. A lost mirror
        // duel is NOT total-party-defeat; the real party survives either way.
        const Poolrad::DuelSettlementDecision settle =
            Poolrad::settleDuelOutcome(duelWon, mirror);

        // Discard the temporary roster (free every temporary member). This
        // never frees a real-party member because duelRoster holds only the
        // local temporary copies.
        if (settle.discardTemporaryRoster) {
            for (uint i = 0; i < duelRoster.size(); ++i)
                delete duelRoster[i];
            duelRoster.clear();
        }

        // Restore the real party order and selection. settleDuelOutcome keeps
        // realPartySurvives/restoreRealSelection true for every duel outcome.
        if (settle.realPartySurvives) {
            party.clear();
            for (uint i = 0; i < savedParty.size(); ++i)
                party.push_back(savedParty[i]);
        }
        if (settle.restoreRealSelection)
            VmInterface::setSelectedCharacter(savedSelection);

        if (!duelResourceOk) {
            // Missing duel data: truthful failure, real party intact.
            warning("PoolradEngineHostImpl::handleCallOpcode: duel 0x%04X has "
                "no eligible fighting character (CharFightsAlone construction "
                "is OPEN); real party preserved", (unsigned)callId);
        }
        return VM_OK;
    }

    case 0xBA03:
        // Original: sound indirection through WORD_SOUND_FLAG.
        if (_engine) {
            const uint16 flag = _engine->getSoundFlag();
            if (flag == 8)
                _engine->soundPlay(0x0B);
            else if (flag == 10)
                _engine->soundPlay(0x0C);
        }
        return VM_OK;

    case 0xC01E: {
        // EclMoveForward: step one cell in the facing direction with 0..15
        // wraparound (no wall check, leave flag untouched), then refresh the
        // event and wall bytes. Moving off the west edge lands on x=15, which
        // is the entry cell of the neighbouring map.
        if (!_engine)
            return VM_OK;

        const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
        const uint16 xAddr = layout.vmGlobalField(kVmGlobalFieldDungeonX).vmAddr;
        const uint16 yAddr = layout.vmGlobalField(kVmGlobalFieldDungeonY).vmAddr;
        const uint16 dirAddr = layout.vmGlobalField(kVmGlobalFieldDungeonDir).vmAddr;

        const uint8 dir8 = static_cast<uint8>((_memory->read8(dirAddr) & 0x03) * 2);
        const uint8 newX = static_cast<uint8>((_memory->read8(xAddr) + kDirDeltaX[dir8]) & 0x0F);
        const uint8 newY = static_cast<uint8>((_memory->read8(yAddr) + kDirDeltaY[dir8]) & 0x0F);

        _memory->write8(xAddr, newX);
        _memory->write8(yAddr, newY);
        readGeoAtPosition();
        if (g_events) {
            g_events->postEclVmMessage(xAddr, newX);
            g_events->postEclVmMessage(yAddr, newY);
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
        const uint8 wireDir = static_cast<uint8>((_memory->read8(dirAddr) & 0x03) * 2);

        const MapPos curPos(static_cast<int8>(x), static_cast<int8>(y));
        const uint8 nibble = rtGeo.getMapNibble(curPos, static_cast<Direction>(wireDir));

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
        MapPos pos, bool indoorMode) {
    if (!indoorMode)
        return 2;

    static const int kDx[] = {0, 0, 1, 0, 0, 0, -1, 0};
    static const int kDy[] = {-1, 0, 0, 0, 1, 0, 0, 0};

    uint8 steps = 0;
    for (uint8 i = 0; i < 2; ++i) {
        uint8 nibble = rtGeo.getMapNibble(pos, static_cast<Direction>(wireDir));
        if (nibble != 0)
            break;
        steps++;
        pos.x = static_cast<int8>(pos.x + kDx[wireDir & 7]);
        pos.y = static_cast<int8>(pos.y + kDy[wireDir & 7]);
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
    const MapPos geoPos(static_cast<int8>(x), static_cast<int8>(y));

    const bool indoorMode = (_memory->read8(
        layout.vmField(kVmFieldIndoorModeFlag).vmAddr) != 0);

    RuntimeGeoBlock &rtGeo = _engine->getRuntimeGeo();
    uint8 distance = rtGeo.isLoaded()
        ? countStepsUntilWall(rtGeo, wireDir, geoPos, indoorMode)
        : 2;

    // Clamp to distance cap (D_DistanceCap < D_MonsterDistance).
    if (distanceCap < distance)
        distance = distanceCap;

    // Write D_MonsterDistance to VM memory.
    _memory->write8(
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

// GB13-002: present one horizontal hotkey menu synchronously and return the
// selected index (or -1 on break). Shared by the PARLAY menu and each
// encounter decision-loop iteration. Mirrors the synchronous horizontalMenu()
// pump; it never writes VM memory itself so a reopened prompt costs no write.
int16 PoolradEngineHostImpl::_runEncounterHotkeyMenu(
        const Common::Array<Common::String> &options) {
    if (options.empty() || !_engine || !g_events)
        return -1;

    UIElement *focused = g_events->focusedView();
    if (!focused)
        return -1;

    Goldbox::MenuItemList parsed = buildLegacyHorizontalMenuModel(options);

    Views::Dialogs::HorizontalMenuConfig cfg;
    cfg.menuItemList = &parsed;
    // Keypad/arrow keys would post selection 0 (COMBAT); the original only
    // switches the shown character on them.
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;
    cfg.promptTxt = "";
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 15;

    // MenuHorizontalCS with enterOk = False: only a hotkey picks an option;
    // Enter, Space and Escape leave the menu waiting.
    for (;;) {
        Common::String sinkName = Common::String::format("EclEncMenuSink_%u",
            ++kModalMenuSinkCounter);
        AsyncMenuResultSink *sink = new AsyncMenuResultSink(sinkName, focused);
        Views::Dialogs::HorizontalMenu *menu =
            new Views::Dialogs::HorizontalMenu("EclEncHorizontalMenu", cfg);
        sink->subView(menu);
        menu->activate();
        menu->redraw();

        bool quit = false;
        while (menu->isActive() && !sink->done) {
            _tickMenuPicture();
            if (!g_events->pumpModalInputFrame()) {
                quit = true;
                break;
            }
        }

        const bool done = sink->done;
        const bool success = sink->success;
        const int value = sink->value;
        const int keyCode = sink->keyCode;
        delete sink;

        if (quit || !done)
            return -1;
        if (keyCode == Common::KEYCODE_RETURN || keyCode == Common::KEYCODE_KP_ENTER
                || keyCode == Common::KEYCODE_SPACE
                || keyCode == Common::KEYCODE_ESCAPE)
            continue;
        if (success && value >= 0)
            return static_cast<int16>(value);
    }
}

int16 PoolradEngineHostImpl::parlayMenu(
        const Common::Array<Common::String> &attitudes) {
    // The original parley menu has no cancel: it always resolves to one of the
    // five attitudes. Reuse the synchronous hotkey menu; the ECL handler writes
    // the chosen attitude VALUE (not the index) to the destination.
    const bool menuWasVisible = _hideInGameMenu();
    const int16 sel = _runEncounterHotkeyMenu(attitudes);
    _restoreInGameMenu(menuWasVisible);
    return (sel < 0) ? 0 : sel;
}

bool PoolradEngineHostImpl::_tickMenuPicture() {
    // Original: anim := eclGraphics and picDrawn. Only a PIC close-up or
    // scene animates; portraits (head/body) stay still.
    if (!_engine)
        return false;

    bool changed = false;
    Goldbox::Gfx::EncounterSpriteCache &sprite =
        _engine->getEncounterSpriteCache();
    if (sprite.isSpriteLoaded()) {
        if (sprite.distance() == 0 && sprite.lastHeadPicId() == 0xFF)
            changed = sprite.tickAnimation();
    } else {
        Goldbox::Gfx::PictureDisplayCache &pic =
            _engine->getPictureDisplayCache();
        if (pic.isActive() && pic.headPicId() == 0xFF)
            changed = pic.tickAnimation();
    }

    if (changed && g_events) {
        g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
            EclVmMessage::VT_UINT8);
    }
    return changed;
}

void PoolradEngineHostImpl::tickAsyncMenuPicture() {
    if (_asyncMenuPending)
        _tickMenuPicture();
}

void PoolradEngineHostImpl::_writeVmValue(uint16 vmAddr, uint16 value) {
    Poolrad::PoolradEngine *poolEngine =
        dynamic_cast<Poolrad::PoolradEngine *>(_engine);
    ECL::EclVM *vm = poolEngine ? poolEngine->getEclVm() : nullptr;
    if (vm)
        vm->writeVmMemory(vmAddr, value, this);
    else if (_memory)
        _memory->write8(vmAddr, static_cast<uint8>(value & 0xFF));
}

bool PoolradEngineHostImpl::_hideInGameMenu() {
    // The command menu belongs to MapMainMenu only; while a script runs it
    // must neither draw over the script's prompt nor take its keys.
    Poolrad::PoolradEngine *poolEngine =
        dynamic_cast<Poolrad::PoolradEngine *>(_engine);
    Views::InGameView *inGameView = poolEngine
        ? dynamic_cast<Views::InGameView *>(poolEngine->findView("InGame"))
        : nullptr;
    if (!inGameView || !inGameView->isInGameMenuVisible())
        return false;
    inGameView->setInGameMenuVisible(false);
    return true;
}

void PoolradEngineHostImpl::_restoreInGameMenu(bool wasVisible) {
    if (!wasVisible)
        return;
    Poolrad::PoolradEngine *poolEngine =
        dynamic_cast<Poolrad::PoolradEngine *>(_engine);
    Views::InGameView *inGameView = poolEngine
        ? dynamic_cast<Views::InGameView *>(poolEngine->findView("InGame"))
        : nullptr;
    if (inGameView)
        inGameView->setInGameMenuVisible(true);
}

void PoolradEngineHostImpl::_showEncounterMessage(const Common::String &text,
        bool clearBox) {
    Poolrad::PoolradEngine *poolEngine =
        dynamic_cast<Poolrad::PoolradEngine *>(_engine);
    Views::InGameView *inGameView = poolEngine
        ? dynamic_cast<Views::InGameView *>(poolEngine->findView("InGame"))
        : nullptr;
    if (!inGameView || !g_events)
        return;

    // Go through the view's message queue like PRINT/PRINTCLEAR do, so a clear
    // the script queued earlier in this run cannot wipe this text afterwards.
    if (clearBox && text.empty())
        clearTextBox();
    else
        g_events->postEclVmMessage(EclVmMessage::makeSyscallWithText(0, 0,
            EclVmMessage::SC_PRINT_ASYNC, text, clearBox ? 1 : 0));
    if (!g_events->pumpModalInputFrame())
        return;

    while (inGameView->isTextBoxBusy()) {
        inGameView->tick();
        if (!g_events->pumpModalInputFrame())
            break;
    }

    // Let the view see busy->idle now and drop the completion it raises, so a
    // later PRINT does not consume it as its own.
    inGameView->tick();
    RuntimeExchange *exchange = _engine->getRuntimeExchange();
    if (exchange && exchange->hasAsync(RuntimeExchange::kAsyncTextBoxDone)) {
        RuntimeExchange::AsyncCompletion completion;
        exchange->pollAsync(completion);
    }
}

VmResult PoolradEngineHostImpl::encounterMenu(
        const Common::Array<uint8> &operands) {
    // EclEncounter (eclvm.pas): the decision loop runs synchronously; WAIT and
    // ADVANCE re-open the menu, the outcome is written once at the end.
    if (!_engine || !_memory)
        return VM_ERROR;
    if (operands.size() < 13)
        return VM_ERROR;

    // Unpack the fixed layout produced by handle_0x29_ENCOUNTER_MENU.
    const uint8 monsterId     = operands[0];
    const uint8 requestedDist = operands[1];
    const uint8 monsterPicId  = operands[2];
    const uint16 destAddr = static_cast<uint16>(operands[3]
        | (static_cast<uint16>(operands[4]) << 8));
    Common::Array<uint8> resp;
    for (uint8 i = 0; i < 5; ++i)
        resp.push_back(operands[static_cast<uint>(5 + i)]);
    const uint8 fleePar = operands[10];
    const uint8 fleeMon = operands[11];
    const uint8 msgCount = operands[12];

    Common::Array<Common::String> messages;
    uint pos = 13;
    for (uint8 m = 0; m < msgCount && pos <= operands.size(); ++m) {
        Common::String s;
        while (pos < operands.size() && operands[pos] != 0) {
            s += static_cast<char>(operands[pos]);
            ++pos;
        }
        ++pos; // skip NUL terminator
        messages.push_back(s);
    }
    while (messages.size() < 3)
        messages.push_back(Common::String());

    EncounterOperation op;
    op.build(monsterId, requestedDist, monsterPicId, destAddr, resp, messages,
        fleePar, fleeMon);

    const bool menuWasVisible = _hideInGameMenu();

    // MoveExtremes over the live party (no status filter on movement).
    Common::Array<ECL::MoveExtremeMember> members;
    if (Common::List<Goldbox::Data::PlayerCharacter *> *party =
            VmInterface::getParty()) {
        for (Common::List<Goldbox::Data::PlayerCharacter *>::const_iterator it =
                party->begin(); it != party->end(); ++it) {
            Goldbox::Data::PlayerCharacter *ch = *it;
            if (!ch)
                continue;
            ECL::MoveExtremeMember mem;
            mem.movementCur = ch->movement.current;
            if (const Goldbox::Data::Effects::CharacterEffects *fx =
                    ch->getEffects()) {
                mem.hasted = fx->hasEffect(Goldbox::Data::Effects::E_HASTE);
                mem.slowed = fx->hasEffect(Goldbox::Data::Effects::E_SLOW);
            }
            members.push_back(mem);
        }
    }
    const ECL::MoveExtremes extremes = ECL::computeMoveExtremes(members);

    // MONSTER_DIST_REQ := var 2; MONSTER_DIST := MaxMonsterDist capped by the
    // request; DisplayMonster at that distance.
    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    const uint16 distAddr =
        layout.vmGlobalField(kVmGlobalFieldMonsterDistance).vmAddr;
    _memory->write8(
        layout.vmGlobalField(kVmGlobalFieldMonsterDistanceReq).vmAddr,
        requestedDist);
    drawEncounterStage(monsterId, requestedDist, monsterPicId);
    const uint8 stageDist = _memory->read8(distAddr);
    const uint8 view3dFlag = _memory->read8(
        layout.vmField(kVmFieldIndoorModeFlag).vmAddr);

    op.begin(stageDist, view3dFlag, extremes);

    // Decision loop. Each reopen re-presents the hotkey menu with no VM write.
    const int kMaxIterations = 64; // guard against a malformed response table
    for (int iter = 0; iter < kMaxIterations && !op.isComplete(); ++iter) {
        // Message for the current distance; an empty one keeps the window.
        const Common::String msg = op.message(op.currentMessageSlot());
        _showEncounterMessage(msg, !msg.empty());

        Common::Array<Common::String> menuOptions;
        menuOptions.push_back("COMBAT");
        menuOptions.push_back("WAIT");
        menuOptions.push_back("FLEE");
        menuOptions.push_back(op.usesParleyMenu() ? "PARLAY" : "ADVANCE");

        const int16 rawSel = _runEncounterHotkeyMenu(menuOptions);
        if (rawSel < 0)
            break; // dialog broke; do not write a faked outcome

        const uint8 prevDist = op.monsterDist();
        const ECL::EncounterStep step =
            op.applySelection(static_cast<uint8>(rawSel));

        if (step.terminal) {
            if (step.outcome == ECL::kEncounterMonstersFlee)
                _showEncounterMessage("The monsters flee.", true);
        } else if (!step.redisplay) {
            _showEncounterMessage("Both sides wait.", true);
        } else if (step.monsterDist != prevDist) {
            _memory->write8(distAddr, step.monsterDist);
            redrawEncounterStage(step.monsterDist);
        }
    }

    if (op.isComplete())
        _writeVmValue(op.destAddr(), op.outcome());

    _restoreInGameMenu(menuWasVisible);

    // Clear the bottom message row regardless of outcome (ClearRect parity).
    Graphics::Screen *screen = _engine ? _engine->getScreen() : nullptr;
    if (screen) {
        Goldbox::Surface screenSurface(*screen,
            Common::Rect(0, 0, screen->w, screen->h));
        screenSurface.clearBox(0, 24, 39, 24, 0);
    }
    _updateViewState();
    return VM_OK;
}

VmResult PoolradEngineHostImpl::beginDamageReport(
        const Common::String &messages, bool partyKilled) {
    if (!messages.empty())
        _showEncounterMessage(messages, false);

    if (partyKilled) {
        _showEncounterMessage("The entire party is killed!", true);
        const uint32 endTime = g_system->getMillis() + 3000;
        while (g_events && g_system->getMillis() < endTime) {
            if (!g_events->pumpModalInputFrame())
                break;
        }
    }

    // DisplayAndPause: wait for any key; there is no result to store.
    Common::Array<Common::String> prompt;
    prompt.push_back("PRESS <ENTER>/<RETURN> TO CONTINUE");
    const VmResult r = beginHorizontalMenuAsync(0, prompt);
    if (r == VM_YIELD)
        _asyncMenuAnyKey = true;
    return r;
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
    // With several options keypad/arrow keys would post selection 0; the
    // original only switches the shown character on them.
    cfg.allowNumPad = singleItemMode;
    cfg.suppressUnhandledKeys = true;
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
    _asyncHorizontalMenu->setPostUnmatchedKeys(!singleItemMode);
    _asyncMenuSink->subView(_asyncHorizontalMenu);
    _asyncHorizontalMenu->activate();
    _asyncHorizontalMenu->redraw();

    _asyncMenuHidCommandMenu = _hideInGameMenu();
    _asyncMenuResultAddr = resultAddr;
    _asyncMenuEnterOk = singleItemMode;
    _asyncMenuAnyKey = false;
    _asyncMenuHotkey = (singleItemMode && !options[0].empty())
        ? toUpperAscii(options[0][0]) : 0;
    _asyncMenuPending = true;
    return VM_YIELD;
}

VmResult PoolradEngineHostImpl::beginDelay() {
    if (_asyncMenuPending || _asyncPrintPending || _asyncDelayPending)
        return VM_ERROR;

    // PauseOnly: gameDelay (0 = fastest .. 9 = slowest) * 225 ms.
    const ECL::EclLayoutAccess layout = ECL::getOpcodeLayout();
    const uint8 speed = _memory->read8(
        layout.vmField(kVmFieldGameSpeed).vmAddr);
    if (speed == 0)
        return VM_OK;

    _asyncDelayEndTime = g_system->getMillis() + static_cast<uint32>(speed) * 225;
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

    timeAddUnits(*_memory, clockAddrs, *party,
        Goldbox::Data::Effects::g_effectHandler,
        VmInterface::getGameStatus(), 1, amount);

    // First runtime trigger-set integration: periodic poison/disease cycle.
    Goldbox::Data::Effects::EffectRuntime runtime(
        Goldbox::Data::Effects::g_effectHandler,
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

// Scale one character's currency by the retained fraction (100 - percent)%,
// in the original denomination order (copper..jewelry). Mirrors RobMoney.
//
// OPEN (trace-gated, GB13-001): the original uses Turbo Pascal Trunc() on a
// Real. We compute the retained amount in a wide integer intermediate and
// truncate toward zero, which matches Trunc for the non-negative Word coin
// fields, but exact binary real-number rounding is unresolved and not claimed.
static void robMoney(Goldbox::Data::ADnDCharacter &character,
        uint8 percentMoney) {
    using namespace Goldbox::Data;
    const uint32 retainedPct = (percentMoney >= 100)
        ? 0 : static_cast<uint32>(100 - percentMoney);
    for (int i = 0; i < VALUABLE_COUNT; ++i) {
        const ValuableType type = static_cast<ValuableType>(i);
        const uint32 scaled =
            (static_cast<uint32>(character.valuableItems[type]) * retainedPct)
            / 100u;
        character.valuableItems[type] = static_cast<uint16>(scaled);
    }
}

// Roll each inventory item against the theft chance in source order. Mirrors
// RobItems: the chance is reduced for heavy items before the roll, a 1..100
// roll is drawn for EVERY item (preserving RNG draw order), and the item is
// removed when roll <= chance. Pointers are captured up front because the
// original captures the next link before removing the current item.
static void robItems(Goldbox::Data::ADnDCharacter &character,
        uint8 itemChance) {
    Common::Array<Goldbox::Data::Items::CharacterItem *> order;
    for (Goldbox::Data::Items::CharacterItem &item :
            character.getInventory().items())
        order.push_back(&item);

    uint8 chance = itemChance;
    for (uint i = 0; i < order.size(); ++i) {
        Goldbox::Data::Items::CharacterItem *item = order[i];
        if (item->weight > 255)
            chance = (chance > 90) ? static_cast<uint8>(chance - 90) : 0;
        else if (item->weight > 24)
            chance = (chance > 50) ? static_cast<uint8>(chance - 50) : 0;

        const uint8 roll =
            static_cast<uint8>(Goldbox::VmInterface::rollDice(1, 100));
        if (roll <= chance)
            character.removeItem(item);
    }
}

static void robCharacter(Goldbox::Data::ADnDCharacter &character,
        uint8 percentMoney, uint8 itemChance) {
    // Source order mirrors EclRob: scale currency, then roll item theft.
    robMoney(character, percentMoney);
    robItems(character, itemChance);
    character.recalcCombatStats();
}

VmResult PoolradEngineHostImpl::robParty(uint8 wholeParty, uint8 percentMoney,
        uint8 itemChance) {
    if (wholeParty == 0) {
        Goldbox::Data::ADnDCharacter *current = dynamic_cast<
            Goldbox::Data::ADnDCharacter *>(
            _engine ? _engine->getSelectedCharacter() : nullptr);
        if (!current)
            return VmResult::VM_OK;
        robCharacter(*current, percentMoney, itemChance);
    } else {
        Common::List<Goldbox::Data::PlayerCharacter *> *party =
            VmInterface::getParty();
        if (!party)
            return VmResult::VM_ERROR;
        for (Goldbox::Data::PlayerCharacter *member : *party) {
            Goldbox::Data::ADnDCharacter *character =
                dynamic_cast<Goldbox::Data::ADnDCharacter *>(member);
            if (!character)
                continue;
            robCharacter(*character, percentMoney, itemChance);
        }
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

        _writeVmValue(lowAddr, count);
        if (highAddr != 0)
            _writeVmValue(highAddr, 0);
        return VmResult::VM_OK;
    }

    // Attribute mode not fully mapped yet from decompile struct offsets.
    // Keep deterministic behavior while preserving script flow.
    if (attributeAddr != 0 && effectId == 0) {
        _writeVmValue(highAddr, 0);
        _writeVmValue(lowAddr, 0);
        return VmResult::VM_OK;
    }

    // Unsupported mixed-operand mode.
    _writeVmValue(highAddr, 0);
    _writeVmValue(lowAddr, 0);
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
        _writeVmValue(resultAddr, present ? 0 : 1);
    return VmResult::VM_OK;
}

bool PoolradEngineHostImpl::findItemInParty(uint8 itemId) const {
    // Pascal EclFindItem: walk every party inventory in list order and match
    // by item type (CharacterItem::typeIndex), with no status filters.
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    if (!party)
        return false;

    for (Goldbox::Data::PlayerCharacter *character : *party) {
        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(character);
        if (!adnd)
            continue;

        const Goldbox::Data::Items::CharacterInventory &inventory =
            adnd->getInventory();
        const Common::List<Goldbox::Data::Items::CharacterItem> &items =
            inventory.items();
        for (const Goldbox::Data::Items::CharacterItem &item : items) {
            if (item.typeIndex == itemId)
                return true;
        }
    }

    return false;
}

VmResult PoolradEngineHostImpl::findSpell(uint8 spellId, uint16 memberAddr,
        uint16 slotAddr) {
    // Pascal EclSpell outputs: slot/index value to address1 (slotAddr) and the
    // member index to address2 (memberAddr).
    //
    // TRACE-GATED (GB13-005): the original begins with an uninitialized found
    // flag, scans past the 21 memorized slots (slot up to 81) and truncates
    // both destinations to a byte. Those anomalies are OPEN until an executable
    // /caller trace exists; they are NOT reproduced. The implementation below is
    // the SAFE, bounded form: a 21-slot memorized-spell scan over the real party
    // in list order. Both outputs are always written and no out-of-range C++
    // access occurs. Any future out-of-slot record view must go through the
    // legacy codec, never adjacent host memory.
    if (!_memory)
        return VmResult::VM_ERROR;

    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();

    uint8 foundSlot = 255;
    uint8 memberIndex = 0;

    if (party) {
        uint8 idx = 0;
        for (Goldbox::Data::PlayerCharacter *character : *party) {
            Poolrad::Data::PoolradCharacter *poolChar =
                dynamic_cast<Poolrad::Data::PoolradCharacter *>(character);
            if (poolChar) {
                const uint8 *memorized = poolChar->getMemorizedSpells();
                for (int slot = 0; slot < 21; ++slot) {
                    if (memorized[slot] == spellId) {
                        foundSlot = static_cast<uint8>(slot);
                        memberIndex = idx;
                        break;
                    }
                }
            }
            if (foundSlot != 255)
                break;
            ++idx;
        }
    }

    // Write both outputs. foundSlot 255 signals absence (mirrors the original
    // sentinel), member index 0 when absent.
    _writeVmValue(slotAddr, foundSlot);
    _writeVmValue(memberAddr, memberIndex);
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
    || _asyncShopPending || _asyncCombatPending
    || _asyncInputPending || _asyncVertMenuPending || _asyncWhoPending;
}

bool PoolradEngineHostImpl::isPendingAsyncReady() const {
    if (_asyncInputPending) {
        const AsyncInputResultSink *sink =
            dynamic_cast<const AsyncInputResultSink *>(_asyncInputSink);
        return sink && sink->done;
    }

    if (_asyncVertMenuPending) {
        const AsyncMenuResultSink *sink =
            dynamic_cast<const AsyncMenuResultSink *>(_asyncVertMenuSink);
        return sink && sink->done;
    }

    if (_asyncWhoPending) {
        const AsyncMenuResultSink *sink =
            dynamic_cast<const AsyncMenuResultSink *>(_asyncWhoSink);
        return sink && sink->done;
    }

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
    // GB13-003: INPUT NUMBER / INPUT STRING.
    if (_asyncInputPending) {
        AsyncInputResultSink *sink =
            dynamic_cast<AsyncInputResultSink *>(_asyncInputSink);
        if (!sink || !sink->done)
            return VM_YIELD;

        // Single-owner / generation check: only the matching active owner may
        // apply the completion. A stale or out-of-generation result (NEW ECL,
        // new game, new battle in between) is discarded and mutates nothing.
        const InputKind kind = _asyncInputKind;
        const uint16 resultAddr = _asyncInputResultAddr;
        const uint8 maxLen = _asyncInputMaxLen;
        const bool cancelled = sink->cancelled;
        const Common::String typed = sink->text;

        // Carry the typed unsigned value on the completion SEPARATELY from
        // cancellation. The single int16 index cannot represent 0..65535, so a
        // number result uses the dedicated uint16 number payload; cancellation
        // is a distinct CompletionKind, not a sentinel value.
        ECL::Completion completion;
        completion.key = _asyncInputKey;
        completion.kind = cancelled
            ? ECL::CompletionKind::Cancelled : ECL::CompletionKind::Returned;
        if (!cancelled && kind == kInputNumber) {
            const ECL::NumberInputResult probe =
                ECL::validateNumberInput(typed, maxLen);
            if (probe.valid) {
                completion.tag = ECL::Completion::kPayloadNumber;
                completion.number = probe.value;
            }
        } else if (!cancelled && kind == kInputString) {
            completion.tag = ECL::Completion::kPayloadText;
            completion.text = typed;
        }
        ECL::Completion delivered;
        const bool owns = _pendingOps.deliver(_asyncInputKey, completion,
            delivered);

        if (owns && !cancelled && _memory) {
            if (kind == kInputNumber) {
                // Write the typed unsigned value carried on the completion.
                // InputUint16 validation (digits only, no overflow past 65535)
                // already ran; an invalid/empty entry left no number payload,
                // so nothing is written (no mutation).
                if (delivered.tag == ECL::Completion::kPayloadNumber)
                    _writeVmValue(resultAddr, delivered.number);
            } else if (kind == kInputString) {
                // Bank-aware VM string writer: cell width from the destination
                // bank. The Poolrad ECL script bank (0x9900-0xB6FF) is byte-
                // addressed; world/dat/heap word banks use 2-byte cells.
                // Empty input -> single space; $6B00 name capped at 15 chars.
                const bool eclBank = (resultAddr >= 0x9900
                    && resultAddr <= 0xB6FF);
                const uint8 width = eclBank ? 1 : 2;
                const uint8 cap = (resultAddr == ECL::kCharacterNameVmAddr)
                    ? ECL::kCharacterNameMaxLength : ECL::kInputStringMaxLength;
                ECL::writeVmString(*_memory, resultAddr, typed, width, cap);
            }
        }

        if (g_events) {
            g_events->postEclSyscallMessage(0,
                (kind == kInputNumber) ? 0x0F : 0x10,
                (kind == kInputNumber)
                    ? EclVmMessage::SC_INPUT_NUMBER
                    : EclVmMessage::SC_INPUT_STRING,
                static_cast<int16>(owns ? VM_OK : VM_ERROR));
        }

        if (!_pendingOps.empty())
            _pendingOps.pop();
        teardownInputAsync();

        Graphics::Screen *screen = _engine ? _engine->getScreen() : nullptr;
        if (screen) {
            Goldbox::Surface screenSurface(*screen,
                Common::Rect(0, 0, screen->w, screen->h));
            screenSurface.clearBox(0, 24, 39, 24, 0);
        }
        _updateViewState();
        return VM_OK;
    }

    // GB13-003: VERTICAL MENU.
    if (_asyncVertMenuPending) {
        AsyncMenuResultSink *sink =
            dynamic_cast<AsyncMenuResultSink *>(_asyncVertMenuSink);
        if (!sink || !sink->done)
            return VM_YIELD;

        ECL::Completion completion;
        completion.key = _asyncVertMenuKey;
        // Explicit success/cancel/quit outcomes: a valid selection writes the
        // index; cancel/quit write nothing (not a blanket -1 write).
        const bool selected = sink->success && sink->value >= 0;
        completion.kind = selected
            ? ECL::CompletionKind::Selected : ECL::CompletionKind::Cancelled;
        ECL::Completion delivered;
        const bool owns = _pendingOps.deliver(_asyncVertMenuKey, completion,
            delivered);

        const uint16 resultAddr = _asyncVertMenuResultAddr;
        const int value = sink->value;

        if (owns && selected && _memory)
            _writeVmValue(resultAddr, static_cast<uint16>(value));

        if (g_events) {
            g_events->postEclSyscallMessage(0, 0x15,
                EclVmMessage::SC_VERTICAL_MENU,
                static_cast<int16>(selected ? value : -1));
        }

        if (!_pendingOps.empty())
            _pendingOps.pop();
        teardownVertMenuAsync();

        Graphics::Screen *screen = _engine ? _engine->getScreen() : nullptr;
        if (screen) {
            Goldbox::Surface screenSurface(*screen,
                Common::Rect(0, 0, screen->w, screen->h));
            screenSurface.clearBox(0, 24, 39, 24, 0);
        }
        _updateViewState();
        return VM_OK;
    }

    // GB13-003: WHO (party-member selection by stable identity).
    if (_asyncWhoPending) {
        AsyncMenuResultSink *sink =
            dynamic_cast<AsyncMenuResultSink *>(_asyncWhoSink);
        if (!sink || !sink->done)
            return VM_YIELD;

        ECL::Completion completion;
        completion.key = _asyncWhoKey;
        const bool selected = sink->success && sink->value >= 0;
        completion.kind = selected
            ? ECL::CompletionKind::Selected : ECL::CompletionKind::Cancelled;
        ECL::Completion delivered;
        const bool owns = _pendingOps.deliver(_asyncWhoKey, completion,
            delivered);

        const int index = sink->value;

        // WHO updates the current selection by stable identity (the owning
        // PlayerCharacter*), NOT a numeric VM destination. Cancel leaves the
        // selection unchanged.
        if (owns && selected) {
            Goldbox::Data::PlayerCharacter *chosen =
                characterAtPartyIndex(_engine, index);
            if (chosen)
                _engine->setSelectedCharacter(chosen);
        }

        if (!_pendingOps.empty())
            _pendingOps.pop();
        teardownWhoAsync();
        _updateViewState();
        return VM_OK;
    }

    if (_asyncMenuPending) {
        AsyncMenuResultSink *sink =
            dynamic_cast<AsyncMenuResultSink *>(_asyncMenuSink);
        if (!sink || !sink->done)
            return VM_YIELD;

        // MenuHorizontalCS: Enter ends the menu only with enterOk (a single
        // option, selection 0); a letter or digit ends it with its hotkey
        // index, or 255 when it is no hotkey; every other key keeps waiting.
        const int key = sink->keyCode;
        const bool isEnter = key == Common::KEYCODE_RETURN
            || key == Common::KEYCODE_KP_ENTER;
        bool reopen = false;
        if (_asyncMenuEnterOk && !_asyncMenuAnyKey) {
            const char ch = (key >= Common::KEYCODE_a && key <= Common::KEYCODE_z)
                ? static_cast<char>(key - Common::KEYCODE_a + 'A')
                : (key >= Common::KEYCODE_0 && key <= Common::KEYCODE_9)
                    ? static_cast<char>(key) : 0;
            if (isEnter || (ch != 0 && ch == _asyncMenuHotkey))
                sink->value = 0;
            else if (ch != 0)
                sink->value = 255;
            else
                reopen = true;
            sink->success = !reopen;
        } else if (!_asyncMenuEnterOk) {
            reopen = isEnter || key == Common::KEYCODE_SPACE
                || key == Common::KEYCODE_ESCAPE;
        }
        if (reopen && _asyncHorizontalMenu) {
            sink->done = false;
            sink->success = false;
            sink->value = -1;
            _asyncHorizontalMenu->activate();
            _asyncHorizontalMenu->redraw();
            return VM_YIELD;
        }

        if (_memory && _asyncMenuResultAddr != 0 && sink->success
                && sink->value >= 0)
            _writeVmValue(_asyncMenuResultAddr,
                static_cast<uint16>(sink->value));

        delete _asyncMenuSink;
        _asyncMenuSink = nullptr;
        _asyncHorizontalMenu = nullptr;
        _asyncMenuModel.reset();
        _asyncMenuResultAddr = 0;
        _asyncMenuPending = false;
        _restoreInGameMenu(_asyncMenuHidCommandMenu);
        _asyncMenuHidCommandMenu = false;

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

        // GB13-009 (8.3): the shop/temple branch of opcode 0x24 returned
        // VM_YIELD, so the handler skipped its post-encounter cleanup tail.
        // Run it now, exactly once, before the VM resumes at the recorded PC.
        // The diagnostic uses postEclSyscallMessage (a trace), NOT
        // ST_ENTER_SHOP, so it cannot re-open the shop dialog.
        if (_memory) {
            ECL::runEncounterCleanupTail(*_memory);
            if (g_events)
                g_events->postEclSyscallMessage(0, 0x24,
                    EclVmMessage::SC_START_COMBAT,
                    static_cast<int16>(VM_OK));
        }
        return VM_OK;
    }

    if (_asyncCombatPending) {
        _asyncCombatPending = false;
        _asyncCombatWasActivated = false;

        // GB13-011: run the explicit battle-teardown boundary BEFORE host
        // monster ownership is released. The session captures the settlement
        // input (GB13-010 input) and detaches the spell presenter/targeter and
        // delayed callbacks first; clearMonsters() then invalidates each
        // departed combatant's pending references and frees the monsters. This
        // capture -> detach -> invalidate -> free order is the no-use-after-
        // free guarantee.
        Poolrad::PoolradEngine *poolEngine =
            dynamic_cast<Poolrad::PoolradEngine *>(_engine);
        Views::CombatView *combatView = poolEngine
            ? dynamic_cast<Views::CombatView *>(poolEngine->findView("Combat"))
            : nullptr;
        if (combatView) {
            // GS_END_GAME at this seam means the party was defeated (set by
            // CombatView::finishCombat before close); it is NOT an application
            // quit. Defeat still captures a (defeat) settlement snapshot. A
            // true application-quit teardown (TD_QUIT, which skips settlement)
            // is driven at engine shutdown / view destruction, where the
            // CombatSession destructor already unbinds the spell targeter.
            const Combat::CombatSession::TeardownReason reason =
                (_engine->getGameState() == GS_END_GAME)
                    ? Combat::CombatSession::TD_DEFEAT
                    : Combat::CombatSession::TD_NORMAL;
            combatView->teardownBattle(reason);

            // GB13-010: run post-combat settlement HERE, after the GB13-011
            // outcome capture / presentation detach and BEFORE clearMonsters
            // frees the host monster roster, so the settlement reads coins/XP
            // from the still-live monster records. Exactly-once is enforced by
            // _combatSettled, so a duplicated finalize completion cannot award
            // rewards twice. TD_QUIT never reaches this seam (quit teardown
            // happens at shutdown and awards nothing).
            settleCombatLoot(reason == Combat::CombatSession::TD_DEFEAT);
        }

        clearMonsters();

        // GB13-009 (8.3): the combat branch of opcode 0x24 returned VM_YIELD,
        // so the handler skipped its post-encounter cleanup tail. Run it now,
        // exactly once, before the VM resumes at the recorded PC. Even on an
        // END_GAME halt the state is reset consistently first.
        if (_memory) {
            ECL::runEncounterCleanupTail(*_memory);
            if (g_events)
                g_events->postEclSyscallMessage(0, 0x24,
                    EclVmMessage::SC_START_COMBAT,
                    static_cast<int16>(VM_OK));
        }

        if (_engine && _engine->getGameState() == GS_END_GAME)
            return VM_HALTED;
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

    // GB13-009 (8.2): NEW ECL replaces the script context. Bump the request
    // generation so any completion still in flight for the OLD script is
    // discarded as out-of-generation rather than applied to the new script.
    _pendingOps.bumpGeneration();

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

    // LOADPIECES 255 only invalidates the slot's reload cache entry; the wall
    // graphics and symbols already in the slot (e.g. the second chunk of a
    // two-chunk set loaded into the slot below) stay in use.
    if (blockId == 0xFF)
        return VmResult::VM_OK;

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
    // for this walldef block are present in the cache. Every chunk takes the
    // symbol shift of the requested slot (LoadWallSet symOfs).
    const int symbolOffset =
        Goldbox::Gfx::WalldefSlotCache::symbolOffsetForSlot(setSlot);
    for (int i = 0; i < numChunks; ++i) {
        const int curSlot = setSlot + i;
        if (curSlot < 1 || curSlot > 3)
            break;
        walldefCache.loadSlot(curSlot, walldef, i, symbolOffset, tileCache);
    }

    // Slots outside this block may draw symbols whose tiles just changed.
    for (int slot = 1; slot <= 3; ++slot) {
        if (slot < setSlot || slot >= setSlot + numChunks)
            walldefCache.rebuildSlot(slot, tileCache);
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

void PoolradEngineHostImpl::bumpPendingGeneration() {
    _pendingOps.bumpGeneration();
    debug(2, "PoolradEngineHostImpl::bumpPendingGeneration: generation now=%u",
        (unsigned)_pendingOps.generation());
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

    const int x = static_cast<int>(snapshot.dungeonPos.x);
    const int y = static_cast<int>(snapshot.dungeonPos.y);
    const uint8 wireDir = static_cast<uint8>((snapshot.dungeonDir & 0x03) * 2);

    const uint8 doorFlag = rtGeo.getWallFlag(MapPos(static_cast<int8>(x), static_cast<int8>(y)), static_cast<Direction>(wireDir));

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
    // Original: SOUND_Play(ARRAY_SOUND_MAP[soundId])
    if (_engine)
        _engine->soundPlay(soundId);
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

// GB13-032: the category roll and bonus/payload fill now live in the pure
// item/rules TU (data/items/random_item). The host only provides the RNG by
// forwarding the generator's injected source to VmInterface::rollDice.
namespace {
class HostItemRollSource : public Goldbox::Data::Items::IItemRollSource {
public:
    int rollDice(int number, int sides) override {
        return VmInterface::rollDice(number, sides);
    }
};
} // namespace

VmResult PoolradEngineHostImpl::setupTreasure(uint8 copper, uint8 silver,
        uint8 electrum, uint8 gold, uint8 platinum, uint8 gems,
        uint8 jewelry, uint8 itemSetId) {
    if (!_engine)
        return VM_ERROR;

    Runtime::TreasurePool &pool = _engine->getTreasurePool();
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
        HostItemRollSource rollSource;
        for (uint8 i = 0; i < count; ++i) {
            // Owned canonical item from the item/rules generator (fully
            // populated: category roll -> base template -> bonus/charges ->
            // identification/payload). Distinct from immutable shop stock.
            Goldbox::Data::Items::CharacterItem item =
                Goldbox::Data::Items::generateRandomItem(
                    rollSource, Goldbox::Engine::gItemProps);
            pool.addItem(item);
            debug(3, "  random item[%u]: typeIndex=%u bonus=%u charges=%u",
                (unsigned)i, (unsigned)item.typeIndex,
                (unsigned)item.bonus, (unsigned)item.getCharges());
        }

        debug(3, "setupTreasure: generated %u random magic items",
            (unsigned)count);
    }

    return VM_OK;
}

} // namespace Poolrad
} // namespace Goldbox
