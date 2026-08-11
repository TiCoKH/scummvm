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

#include "goldbox/data/effects/effect_runtime.h"

#include "common/array.h"
#include "goldbox/combat/combat_ground_info.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/data/effects/effect_handler_base.h"
#include "goldbox/data/effects/effect_host_bridge.h"
#include "goldbox/data/effects/effect_notify.h"
#include "goldbox/data/player_character.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Data {
namespace Effects {

namespace {

struct TriggerSetTable {
    const Effects *ids;
    uint size;
};

static bool isPoisonCycleEffect(uint8 effectType) {
    switch (effectType) {
    case E_POISON_DAMAGE:
    case E_POISON_PLUS_0:
    case E_POISON_PLUS_2:
    case E_POISON_PLUS_4:
    case E_POISON_NEG_2:
    case E_CAUSE_DISEASE_1:
    case E_DISEASE_CONFUSED:
        return true;
    default:
        return false;
    }
}

#define TS_DEF(name, ...) \
    static const Effects name[] = { __VA_ARGS__ }

TS_DEF(kSet0, E_UNIMPLEMENTED_126);
TS_DEF(kSet1, E_MINOR_GLOBE_OF_INVULNERABILITY, E_BLINK, E_INVISIBILITY,
    E_INVISIBLE);
TS_DEF(kSet2, E_REDUCE_DAMAGE_TO_ONE_IF_ITEM_FIELD7_AFFECT_55,
    E_GIANT_SLUG_SPIT_ACID, E_BEHOLDER_RAYS_AFFECT_57, E_FEEBLEMIND,
    E_FIRE_ATTACK_2D10, E_ANKHEG_ACID_ATTACK, E_WEAP_FROST_BRAND,
    E_PROT_DRAG_BREATH, E_BERSERK);
TS_DEF(kSet3, E_POISON_PLUS_0, E_POISON_PLUS_4, E_POISON_PLUS_2,
    E_THRI_KREEN_PARALYZE, E_FEEBLEMIND, E_INVISIBLE_TO_ANIMALS,
    E_POISON_NEG_2, E_FIRE_ATTACK_2D10,
    E_REDUCE_DAMAGE_TO_ONE_IF_ITEM_FIELD7_AFFECT_55,
    E_GIANT_SLUG_SPIT_ACID, E_BEHOLDER_RAYS_AFFECT_57);
TS_DEF(kSet4, E_RAY_OF_ENFEEBLEMENT, E_SWORD_VS_UNDEAD,
    E_IMMUNE_TO_ELECTRICITY);
TS_DEF(kSet5, E_MIRROR_IMAGE, E_PROT_FROM_NORMAL_MISSILES,
    E_RESIST_MAGIC_50, E_ANKHEG_ACID_SQUIRT_ATTACK, E_UNKNOWN_101,
    E_HALF_DAMAGE_MAGICAL_WEAPONS, E_VULNERABILITY_HOLY_WATER,
    E_BOULDER_EVASION, E_RESIST_SLEEP_CHARM_30, E_OWLBEAR_HUG_CHECK,
    E_DAMAGE_REDUCTION, E_RAKSHASA_RESIST_NORMAL_WEAPONS,
    E_IMMUNITY_NONMAGICAL_HALF_SILVER, E_HALF_DAMAGE_COLD);
TS_DEF(kSet6, E_HALF_DAMAGE_ELECTRICITY, E_FIRE_RESIST,
    E_IMMUNITY_NONMAGICAL_HALF_SILVER,
    E_RAKSHASA_RESIST_NORMAL_WEAPONS, E_CLOUD_KILL, E_RESIST_COLD,
    E_RESIST_FIRE, E_RESIST_MAGIC_15, E_RESIST_SLEEP_CHARM_90,
    E_EFREETI_FIRE_RESISTANCE, E_HALF_DAMAGE_PIERCING_SLASHING,
    E_IMMUNITY_NONMAGICAL_WEAPONS, E_SHIELD, E_HALF_FIRE_DAMAGE,
    E_UNKNOWN_101, E_MIRROR_IMAGE);
TS_DEF(kSet7, E_SNAKE_CHARM, E_PARALYZE, E_SLEEP, E_HELPLESS);
TS_DEF(kSet8, E_WILD_BOAR_AND_BULLETTE_AFFECT_63, E_HALF_DAMAGE,
    E_RESIST_FIRE_AND_COLD, E_DISPLACE, E_CAMOUFLAGE,
    E_ITEM_INVISIBILITY);
TS_DEF(kSet9, E_RESIST_MAGIC_15, E_RESIST_SLEEP_CHARM_90,
    E_IMMUNITY_SLEEP_CHARM, E_IMMUNITY_PARALYSIS, E_IMMUNITY_COLD,
    E_IMMUNITY_PARALYSIS_POISON, E_IMMUNITY_FIRE,
    E_EFREETI_FIRE_RESISTANCE, E_IMMUNITY_SLEEP_CHARM_PARALYSIS_POISON,
    E_IMMUNITY_GAZE_ATTACKS);
TS_DEF(kSet10, E_BLESSED, E_CURSED, E_BLINDED, E_BESTOW_CURSE, E_PRAYER,
    E_SWORD_VS_UNDEAD, E_IMMUNE_TO_ELECTRICITY,
    E_GNOME_VS_MAN_SIZED_GIANT, E_DWARF_VS_ORC);
TS_DEF(kSet11, E_BLINDED, E_SHIELD, E_PROTECTION_FROM_EVIL,
    E_PROTECTION_FROM_GOOD, E_PROT_FROM_EVIL_10_RADIUS,
    E_PROT_FROM_GOOD_10_RADIUS, E_STINKING_CLOUD);
TS_DEF(kSet12, E_PROTECTION_FROM_EVIL, E_PROTECTION_FROM_GOOD,
    E_RESIST_COLD, E_SHIELD, E_RESIST_FIRE, E_BLINDED, E_BESTOW_CURSE,
    E_PROT_FROM_EVIL_10_RADIUS, E_PROT_FROM_GOOD_10_RADIUS, E_PRAYER,
    E_FIRE_RESIST, E_IMMUNITY_FIRE, E_IMMUNITY_GAZE_ATTACKS,
    E_BREATH_ACID, E_CON_SAVING_BONUS);
TS_DEF(kSet13, E_WILD_BOAR_AND_BULLETTE_AFFECT_63, E_TROLL_FIRE_OR_ACID,
    E_THRI_KREEN_MISSILE_EVASION, E_WEAP_DRAGON_SLAYER, E_AFFECT_4A);
TS_DEF(kSet14, E_PETRIFYING_GAZE, E_SHAMBLING_ABSORB_LIGHTNING,
    E_BREATH_ELEC, E_VULNERABILITY_FIRE);
TS_DEF(kSet15, E_SILENCE_15_RADIUS, E_STINKING_CLOUD, E_AFFECT_4A,
    E_WEAP_DRAGON_SLAYER, E_CHARM_PERSON);
TS_DEF(kSet16, E_INVISIBILITY, E_INVISIBLE, E_BLINK,
    E_DWARF_AND_GNOME_VS_GIANTS, E_GNOME_LARGE_MONSTER, E_DISPLACE);
TS_DEF(kSet17, E_BLESSED, E_CURSED, E_CHARM_PERSON);
TS_DEF(kSet18, E_HASTE, E_SLOW, E_CLEAR_MOVEMENT);
TS_DEF(kSet19, E_REGEN_3_HP, E_SPIRITUAL_HAMMER, E_CAMOUFLAGE,
    E_ITEM_INVISIBILITY, E_CHARM_PERSON);

#undef TS_DEF

static const TriggerSetTable kTriggerSets[] = {
    { kSet0, ARRAYSIZE(kSet0) },
    { kSet1, ARRAYSIZE(kSet1) },
    { kSet2, ARRAYSIZE(kSet2) },
    { kSet3, ARRAYSIZE(kSet3) },
    { kSet4, ARRAYSIZE(kSet4) },
    { kSet5, ARRAYSIZE(kSet5) },
    { kSet6, ARRAYSIZE(kSet6) },
    { kSet7, ARRAYSIZE(kSet7) },
    { kSet8, ARRAYSIZE(kSet8) },
    { kSet9, ARRAYSIZE(kSet9) },
    { kSet10, ARRAYSIZE(kSet10) },
    { kSet11, ARRAYSIZE(kSet11) },
    { kSet12, ARRAYSIZE(kSet12) },
    { kSet13, ARRAYSIZE(kSet13) },
    { kSet14, ARRAYSIZE(kSet14) },
    { kSet15, ARRAYSIZE(kSet15) },
    { kSet16, ARRAYSIZE(kSet16) },
    { kSet17, ARRAYSIZE(kSet17) },
    { kSet18, ARRAYSIZE(kSet18) },
    { kSet19, ARRAYSIZE(kSet19) }
};

static_assert(ARRAYSIZE(kTriggerSets) == ETS_SPELL_POST_PROCESS + 1,
    "Trigger-set table must match EffectTriggerSet enum layout");

static const TriggerSetTable *getTriggerSetTable(EffectTriggerSet setId) {
    const uint idx = static_cast<uint>(setId);
    if (idx >= ARRAYSIZE(kTriggerSets))
        return nullptr;
    return &kTriggerSets[idx];
}

static bool containsEffectType(const CharacterEffects &effects,
        Effects type) {
    const Common::List<Effect> &list = effects.effects();
    const uint8 effectId = static_cast<uint8>(type);
    for (const Effect &effect : list) {
    if (effect.id == effectId)
            return true;
    }
    return false;
}

static bool findEffectInCharacter(PlayerCharacter &character, uint8 effectType,
        Effect *&foundEffect) {
    foundEffect = nullptr;

    CharacterEffects *effects = character.getEffects();
    if (!effects)
        return false;

    return effects->findEffectById(effectType, &foundEffect);
}

static bool isWithinPropagationRange(const Combat::CombatantTable &table,
        const PlayerCharacter &source, const PlayerCharacter &target,
        uint8 range) {
    const int sourceIdx = table.findIndex(&source);
    const int targetIdx = table.findIndex(&target);
    if (sourceIdx < 0 || targetIdx < 0)
        return false;

    const uint8 sourceCol = table.getTileCol(sourceIdx);
    const uint8 sourceRow = table.getTileRow(sourceIdx);
    const uint8 sourceSize = table.getSize(sourceIdx) & 7;

    const uint8 targetCol = table.getTileCol(targetIdx);
    const uint8 targetRow = table.getTileRow(targetIdx);
    const uint8 targetSize = table.getSize(targetIdx) & 7;

    for (uint8 sourceSlot = 0; sourceSlot < 4; ++sourceSlot) {
        int8 sourceColDelta = 0;
        int8 sourceRowDelta = 0;
        if (!Combat::getIconOffsetBySize(sourceSize, sourceSlot,
                sourceColDelta, sourceRowDelta))
            continue;

        const int sourceTileCol = static_cast<int>(sourceCol) +
            static_cast<int>(sourceColDelta);
        const int sourceTileRow = static_cast<int>(sourceRow) +
            static_cast<int>(sourceRowDelta);

        for (uint8 targetSlot = 0; targetSlot < 4; ++targetSlot) {
            int8 targetColDelta = 0;
            int8 targetRowDelta = 0;
            if (!Combat::getIconOffsetBySize(targetSize, targetSlot,
                    targetColDelta, targetRowDelta))
                continue;

            const int targetTileCol = static_cast<int>(targetCol) +
                static_cast<int>(targetColDelta);
            const int targetTileRow = static_cast<int>(targetRow) +
                static_cast<int>(targetRowDelta);

            const int deltaCol = ABS(targetTileCol - sourceTileCol);
            const int deltaRow = ABS(targetTileRow - sourceTileRow);
            if (MAX(deltaCol, deltaRow) <= static_cast<int>(range))
                return true;
        }
    }

    return false;
}

static bool findApplicableEffect(uint8 effectType, PlayerCharacter &target,
        const Common::Array<PlayerCharacter *> &party,
        const Combat::CombatantTable *combatTable, Effect *&foundEffect) {
    foundEffect = nullptr;
    const bool inCombat = (target.combatState != nullptr) ||
        (Goldbox::g_engine && Goldbox::VmInterface::getGameStatus() ==
            GS_COMBAT);

    // 1. If target has this effect directly, use it.
    if (findEffectInCharacter(target, effectType, foundEffect))
        return true;

    // 2. Otherwise, only radiating group effects may propagate.
    if (!EffectRuntime::isGroupRadiatingEffect(effectType))
        return false;

    for (uint i = 0; i < party.size(); ++i) {
        PlayerCharacter *member = party[i];
        if (!member || member == &target)
            continue;

        Effect *memberEffect = nullptr;
        if (!findEffectInCharacter(*member, effectType, memberEffect))
            continue;

        if (!inCombat) {
            foundEffect = memberEffect;
            return true;
        }

        if (!combatTable)
            continue;

        const uint8 range =
            (effectType == static_cast<uint8>(E_PRAYER)) ? 6 : 1;
        if (isWithinPropagationRange(*combatTable, *member, target, range)) {
            foundEffect = memberEffect;
            return true;
        }
    }

    return false;
}

} // namespace

EffectRuntime::EffectRuntime(EffectHandlerBase *handler,
        EffectHostBridge *bridge) : _handler(handler), _bridge(bridge) {
}

void EffectRuntime::setHandler(EffectHandlerBase *handler) {
    _handler = handler;
}

void EffectRuntime::setHostBridge(EffectHostBridge *bridge) {
    _bridge = bridge;
}

void EffectRuntime::applyTriggerSet(EffectTriggerSet triggerSet,
        CharacterEffects &effects, PlayerCharacter &character,
        Combat::CombatGlobals *combat) const {
    if (!_handler)
        return;

    if (triggerSet == ETS_POISON_CYCLE) {
        Common::List<Effect> &list = effects.effects();
        for (Effect &effect : list) {
            if (!isPoisonCycleEffect(effect.id))
                continue;
            const uint8 oldStatus = character.healthStatus;
            const uint32 oldFlags = character.effectState.flags;
            _handler->apply(EFF_TICK, effect, character, combat, _bridge);
            character.onEffectsChanged();
            notifyBridge(_bridge, EFF_TICK, character,
                oldStatus, oldFlags, true, true);
        }
        return;
    }

    const TriggerSetTable *table = getTriggerSetTable(triggerSet);
    if (!table)
        return;

    for (uint i = 0; i < table->size; ++i) {
        const Effects type = table->ids[i];
        const uint8 effectId = static_cast<uint8>(type);
        Common::List<Effect> &list = effects.effects();
        for (Effect &effect : list) {
            if (effect.id != effectId)
                continue;
            const uint8 oldStatus = character.healthStatus;
            const uint32 oldFlags = character.effectState.flags;
            _handler->apply(EFF_EVAL, effect, character, combat, _bridge);
            character.onEffectsChanged();
            notifyBridge(_bridge, EFF_EVAL, character,
                oldStatus, oldFlags, true, isStatusPanelEffect(type));
            break;
        }
    }
}

bool EffectRuntime::hasAnyInTriggerSet(EffectTriggerSet triggerSet,
        const CharacterEffects &effects) const {
    if (triggerSet == ETS_POISON_CYCLE) {
        const Common::List<Effect> &list = effects.effects();
        for (const Effect &effect : list) {
            if (isPoisonCycleEffect(effect.id))
                return true;
        }
        return false;
    }

    uint count = 0;
    const Effects *ids = getTriggerSetEffects(triggerSet, count);
    if (!ids || count == 0)
        return false;

    for (uint i = 0; i < count; ++i) {
        if (containsEffectType(effects, ids[i]))
            return true;
    }

    return false;
}

bool EffectRuntime::applyEffect(PlayerCharacter &target,
        uint8 effectType) const {
    Common::Array<PlayerCharacter *> party;
    if (Goldbox::g_engine) {
        Common::List<PlayerCharacter *> *partyList =
            Goldbox::VmInterface::getParty();
        if (partyList) {
            for (PlayerCharacter *member : *partyList) {
                if (member)
                    party.push_back(member);
            }
        }
    }

    Combat::CombatGlobals *combatGlobals = nullptr;
    const Combat::CombatantTable *combatTable = nullptr;
    if (Goldbox::g_engine) {
        Combat::CombatContext *combatContext = Goldbox::g_engine->
            getCombatContext();
        if (combatContext) {
            combatGlobals = &combatContext->globals;
            combatTable = &combatContext->table;
        }
    }

    return applyEffect(target, effectType, party,
        combatGlobals, combatTable);
}

const Effects *EffectRuntime::getTriggerSetEffects(
        EffectTriggerSet triggerSet, uint &count) {
    const TriggerSetTable *table = getTriggerSetTable(triggerSet);
    if (!table)
        return nullptr;

    count = table->size;
    return table->ids;
}

bool EffectRuntime::applyEffect(PlayerCharacter &target, uint8 effectType,
    const Common::Array<PlayerCharacter *> &party,
        Combat::CombatGlobals *combat,
        const Combat::CombatantTable *combatTable) const {
    if (!_handler)
        return false;

    Effect *foundEffect = nullptr;
    if (!findApplicableEffect(effectType, target, party,
            combatTable, foundEffect) || !foundEffect)
        return false;

    const uint8 oldStatus = target.healthStatus;
    const uint32 oldFlags = target.effectState.flags;
    _handler->apply(EFF_ADD, *foundEffect, target, combat, _bridge);
    target.onEffectsChanged();
    notifyBridge(_bridge, EFF_ADD, target,
        oldStatus, oldFlags, true, true);
    return true;
}

// Raw effect IDs (Poolrad) that radiate to nearby characters.
static const uint8 kGroupRadiatingEffects[] = {
    static_cast<uint8>(E_SILENCE_15_RADIUS),
    static_cast<uint8>(E_PROT_FROM_EVIL_10_RADIUS),
    static_cast<uint8>(E_PROT_FROM_GOOD_10_RADIUS),
    static_cast<uint8>(E_PRAYER)
};

bool EffectRuntime::isGroupRadiatingEffect(uint8 effectType) {
    for (uint i = 0; i < ARRAYSIZE(kGroupRadiatingEffects); ++i) {
        if (kGroupRadiatingEffects[i] == effectType)
            return true;
    }
    return false;
}

bool EffectRuntime::isAffectedByGroupEffect(uint8 effectType,
        PlayerCharacter &target,
        const Common::Array<PlayerCharacter *> &party,
        const Combat::CombatantTable *combatTable) const {
    Effect *foundEffect = nullptr;
    return findApplicableEffect(effectType, target, party,
        combatTable, foundEffect);
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
