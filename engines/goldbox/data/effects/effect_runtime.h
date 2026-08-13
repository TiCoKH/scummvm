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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_RUNTIME_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_RUNTIME_H

#include "common/array.h"
#include "common/scummsys.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/data/effects/effect.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;

namespace Effects {

class CharacterEffects;
class EffectHandlerBase;
class EffectHostBridge;

/**
 * Trigger-set ids mirroring Poolrad m68k EFFECT_applyEffectSet(0..19).
 *
 * Names are intentionally semantic aliases for integration points. The numeric
 * values remain stable to preserve decompile traceability.
 */
enum EffectTriggerSet : uint8 {
    ETS_PRE_ACTION = 0,
    ETS_START_OF_COMBAT_ROUND = 1,
    ETS_IMMUNITY_STRIP = 2,
    ETS_DEFENSIVE_PASSIVE = 3,
    ETS_ATTACKER_OFFENSE = 4,
    ETS_DEFENDER_REACTIVE = 5,
    ETS_ON_DAMAGE_TAKEN = 6,
    ETS_POST_MOVEMENT_TILE = 7,
    ETS_COMBAT_AURA = 8,
    ETS_APPLY_WITH_MESSAGE_GUARDS = 9,
    ETS_ATTACKER_TO_HIT = 10,
    ETS_ALIGN_PROTECTION = 11,
    ETS_SAVING_THROW_MODS = 12,
    ETS_ON_DEATH = 13,
    ETS_ON_SPECIAL_ATTACK = 14,
    ETS_POISON_CYCLE = 15,
    ETS_DEFENDER_TO_HIT = 16,
    ETS_SIMPLE_BLESS_CURSE = 17,
    ETS_TARGET_SELECTION_FILTER = 18,
    ETS_SPELL_POST_PROCESS = 19
};

/**
 * Runtime trigger-set evaluator for active character effects.
 */
class EffectRuntime {
public:
    EffectRuntime(EffectHandlerBase *handler,
            EffectHostBridge *bridge = nullptr);

    void setHandler(EffectHandlerBase *handler);
    void setHostBridge(EffectHostBridge *bridge);

    void applyTriggerSet(EffectTriggerSet triggerSet,
            CharacterEffects &effects,
            PlayerCharacter &character,
            Combat::CombatGlobals *combat = nullptr) const;

    bool hasAnyInTriggerSet(EffectTriggerSet triggerSet,
            const CharacterEffects &effects) const;

    /**
     * Simplified legacy apply-effect entry point.
     *
     * Uses globally available runtime context to resolve party/combat state,
     * then executes the same handler-driven logic as the full overload.
     */
    bool checkEffect(PlayerCharacter &target,
            uint8 effectType) const;

    /**
     * Return the raw effect ids that participate in a trigger set.
     *
     * This mirrors the original m68k-style table-driven effect dispatch
     * model and keeps the data available to engine code without exposing the
     * internal storage details.
     */
    static const Effects *getTriggerSetEffects(EffectTriggerSet triggerSet,
            uint &count);

    /**
     * Check whether a character is affected by a group-radiating effect
     * (e.g. Silence 15' Radius, Prot from Evil 10' Radius) either directly
     * on themselves or inherited from a nearby party member.
     *
     * Mirrors original EFFECT_applyEffect group-effect proximity logic.
     *
     * @param effectType  Raw effect id to query.
     * @param target      Character being tested.
     * @param party       All characters in the current party/combat list.
     * Combat mode is inferred from target.combatState != nullptr.
     * @return true if the target is under the effect's influence.
     */
    bool isAffectedByGroupEffect(uint8 effectType,
            PlayerCharacter &target,
            const Common::Array<PlayerCharacter *> &party,
            const Combat::CombatantTable *combatTable = nullptr) const;

    static bool isGroupRadiatingEffect(uint8 effectType);

private:
    EffectHandlerBase *_handler;
    EffectHostBridge *_bridge;
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_RUNTIME_H
