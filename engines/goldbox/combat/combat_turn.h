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

#ifndef GOLDBOX_COMBAT_COMBAT_TURN_H
#define GOLDBOX_COMBAT_COMBAT_TURN_H

#include "common/scummsys.h"
#include "common/array.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"

namespace Goldbox {
namespace ECL {
class AddressSpace;
}
namespace Data {
namespace Effects {
class EffectRuntime;
}
}

namespace Combat {

struct CombatGlobals;

/**
 * Returns true if the given combat side is being ambushed.
 *
 * Mirrors COMBAT_IsSideAmbushed: checks bit 0 for CS_PARTY, bit 1 for CS_ENEMY.
 *
 * @param side         The combat side to check
 * @param ambushFlags  D_CombatIsAmbush byte from VM party state
 */
bool isSideAmbushed(Data::CombatSide side, uint8 ambushFlags);

/**
 * Recalculate primary attack count for one character.
 *
 * Mirrors COMBAT_RecalcPrimaryAttacks: resets curPrimaryRoll.attacks from base,
 * uses fireRate for ranged weapons (min 2), runs ES_COMBAT_RATE_MODIFIER,
 * caps by ammo stack if ranged, then conditionally writes back based on unknownBool.
 */
void recalcPrimaryAttacks(Data::ADnDCharacter *adnd, CombatGlobals *globals,
                          Data::Effects::EffectRuntime *effectRuntime);

/**
 * Compute the per-round secondary attack count with even-turn bonus.
 *
 * Mirrors COMBAT_CalcAttackCountWithEvenTurnBonus:
 *   - Odd turnCounter (= even-numbered turn) grants +1 attack before halving.
 *   - Result = (baseAttackCount + bonus) / 2.
 *
 * @param attacks      Raw attack count (EFFECT_SET18_EXCHANGE_VALUE after effect processing)
 * @param turnCounter  Current round counter (COMBAT_TURN_COUNTER global in original)
 * @return             Effective attacks this round
 */
uint8 calcAttackCountWithEvenTurnBonus(uint8 attacks, uint8 turnCounter);

/**
 * Compute the move budget for a character this turn.
 *
 * Mirrors COMBAT_CalcMoveBudget:
 *   movement.current + effectState.mods.movement, clamped to [0, 255].
 *
 * @param ch  Character to compute move budget for
 * @return    Move points available this turn
 */
uint8 calcMoveBudget(const Data::PlayerCharacter *ch);

/**
 * Reset per-turn CombatAction fields for one character.
 *
 * Mirrors original COMBAT_InitCharacterTurnState.
 * Loads sec_attack into globals->effectSet18 (isMovement=false), runs
 * ES_COMBAT_RATE_MODIFIER (effect set 18: HASTE, SLOW, IMMOBILIZED)
 * whose handlers modify globals->effectSet18.value, then reads the result
 * back through calcAttackCountWithEvenTurnBonus into cs.attackCount.
 * Move budget is set directly after the effect set (not part of the exchange).
 *
 * @param ch             Character to initialise (must have valid combatState)
 * @param ambushFlags    D_CombatIsAmbush value: 0=none, 1=party, 2=enemy, 3=both (bit0=party, bit1=enemy)
 * @param effectRuntime  May be nullptr; used for ES_COMBAT_RATE_MODIFIER
 * @param globals        Combat globals (attackCount written here; turnCounter read for bonus)
 */
void initCharacterTurnState(Data::PlayerCharacter *ch,
                            Data::Effects::EffectRuntime *effectRuntime,
                            CombatGlobals *globals,
                            ECL::AddressSpace *eclMemory,
                            const VmGlobalLayout *vmLayout);

/**
 * Reset turn state for every character in the roster.
 */
void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster,
                       Data::Effects::EffectRuntime *effectRuntime,
                       CombatGlobals *globals,
                       ECL::AddressSpace *eclMemory,
                       const VmGlobalLayout *vmLayout);

/**
 * Select the next character to act this round.
 *
 * Mirrors COMBAT_SelectNextActor: returns the enabled character with the
 * highest initiative value that has not yet acted (initiative != 0xFF).
 * Returns nullptr when all characters have acted (round complete) or a
 * side has no members.
 *
 * @param roster   Full combat roster
 * @param globals  Combat globals (reads sideCount to detect round end)
 * @return         Next actor, or nullptr if the round is over
 */
Data::PlayerCharacter *selectNextActor(
    const Common::Array<Data::PlayerCharacter *> &roster,
    const CombatGlobals &globals);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_TURN_H
