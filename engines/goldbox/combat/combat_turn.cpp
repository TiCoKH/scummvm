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

#include "goldbox/combat/combat_turn.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_state.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "common/random.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

// ---------------------------------------------------------------------------
// isSideAmbushed
// ---------------------------------------------------------------------------

bool isSideAmbushed(Data::CombatSide side, uint8 ambushFlags) {
    switch (side) {
    case Data::CS_PARTY: return (ambushFlags & 1) != 0;
    case Data::CS_ENEMY: return (ambushFlags & 2) != 0;
    default:             return false;
    }
}

// ---------------------------------------------------------------------------
// initCharacterTurnState
// ---------------------------------------------------------------------------

void initCharacterTurnState(Data::PlayerCharacter *ch,
                            uint8 ambushFlags,
                            Data::Effects::EffectRuntime *effectRuntime,
                            CombatGlobals *globals) {
    if (!ch || !ch->combatState)
        return;

    Data::CombatAction &cs = *ch->combatState;

    // Fixed resets — mirrors original exactly.
    cs.spellId      = 0;
    cs.canCast      = true;
    cs.canUse       = true;
    cs.unknownBool  = false;
    cs.attackId     = 2;
    cs.target       = nullptr;
    cs.fleeing      = false;
    cs.guarding     = false;
    cs.moralFailure = false;
    cs.directionChange = 0;

    // --- Attack count ---
    // Base from primary roll (COMBAT_RecalcPrimaryAttacks equivalent).
    uint8 baseAttacks = 1;
    if (Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(ch))
        baseAttacks = (uint8)adnd->curPrimaryRoll.attacks;
    globals->attackCount = baseAttacks;
    globals->attackCountAdjusting = false;

    // ES_TARGET_SELECTION_FILTER (set 18) may modify globals->attackCount.
    if (effectRuntime && ch->getEffects())
        effectRuntime->checkEffectSet(Data::Effects::ES_TARGET_SELECTION_FILTER,
                                      *ch->getEffects(), *ch, globals);

    // Store the (possibly effect-modified) attack count.
    // COMBAT_adjustFireRate maps the raw count to a per-round rate;
    // for now store directly — the rate adjustment is a separate concern.
    cs.attackCount = globals->attackCount;

    // --- Max targets ---
    // Original: action->max_target = character->attack_level
    // attack_level maps to attackLevel on ADnDCharacter.
    if (Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(ch))
        cs.maxTargets = adnd->attackLevel;
    else
        cs.maxTargets = 1;

    // --- Initiative ---
    if (!ch->enabled) {
        // Disabled/dead characters get no turn.
        cs.initiative = 0;
    } else {
        // Roll 1d6 + dexterity speed bonus.
        static Common::RandomSource rng("combat_initiative");
        int roll = (int)(rng.getRandomNumber(5) + 1); // 1d6
        int init = roll + (int)ch->getDexSpeedBonus();

        // Minimum before ambush penalty.
        if (init < 1)
            init = 1;

        // Ambush: the ambushed side loses 6 initiative.
        if (isSideAmbushed(ch->combatSide, ambushFlags))
            init -= 6;

        // Out-of-range initiative means no turn this round.
        if (init < 0 || init > 20)
            init = 0;

        cs.initiative = (uint8)init;
    }

    // --- Move budget ---
    cs.movePoints = ch->movement.current;
}

// ---------------------------------------------------------------------------
// initAllTurnStates
// ---------------------------------------------------------------------------

void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster,
                       uint8 ambushFlags,
                       Data::Effects::EffectRuntime *effectRuntime,
                       CombatGlobals *globals) {
    for (uint i = 0; i < roster.size(); i++)
        initCharacterTurnState(roster[i], ambushFlags, effectRuntime, globals);
}

// ---------------------------------------------------------------------------
// selectNextActor
// ---------------------------------------------------------------------------

Data::PlayerCharacter *selectNextActor(
        const Common::Array<Data::PlayerCharacter *> &roster,
        const CombatGlobals &globals) {
    // Both sides must still have members for combat to continue.
    if (globals.sideCount[0] == 0 || globals.sideCount[1] == 0)
        return nullptr;

    // Return the enabled character with the highest initiative that has not
    // yet acted. initiative == 0 means no turn; 0xFF means already acted.
    Data::PlayerCharacter *best = nullptr;
    uint8 bestInitiative = 0;

    for (uint i = 0; i < roster.size(); i++) {
        Data::PlayerCharacter *ch = roster[i];
        if (!ch || !ch->enabled || !ch->combatState)
            continue;
        uint8 init = ch->combatState->initiative;
        if (init == 0 || init == 0xFF)
            continue;
        if (init > bestInitiative) {
            bestInitiative = init;
            best = ch;
        }
    }

    return best;
}

} // namespace Combat
} // namespace Goldbox
