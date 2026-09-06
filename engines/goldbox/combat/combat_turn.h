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
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
namespace Effects {
class EffectRuntime;
}
}

namespace Combat {

struct CombatGlobals;

/**
 * Returns true if the given combat side is ambushed this round.
 *
 * D_CombatIsAmbush encoding:
 *   0x00 = nobody ambushed
 *   0x01 = party ambushed
 *   0x02 = enemy ambushed
 *   0x03 = both sides ambushed
 *
 * Treated as a 2-bit field: bit0 = party, bit1 = enemy.
 */
bool isSideAmbushed(Data::CombatSide side, uint8 ambushFlags);

/**
 * Reset per-turn CombatAction fields for one character.
 *
 * Mirrors original COMBAT_InitCharacterTurnState:
 *   - Resets spellId=0, canCast=true, canUse=true, unknownBool=false, attackId=2
 *   - Recalculates attackCount from primary roll, then runs ES_TARGET_SELECTION_FILTER
 *     (effect set 18) which may modify it
 *   - Sets maxTargets from attackLevel
 *   - Rolls initiative: 1d6 + getDexSpeedBonus(), min 1, ambush side -6, clamp 0..20
 *     (initiative=0 means no turn; disabled characters always get initiative=0)
 *   - Calculates movePoints
 *
 * @param ch             Character to initialise (must have valid combatState)
 * @param ambushFlags    D_CombatIsAmbush value: 0=none, 1=party, 2=enemy, 3=both (bit0=party, bit1=enemy)
 * @param effectRuntime  May be nullptr; used for ES_TARGET_SELECTION_FILTER
 * @param globals        Combat globals passed to effect runtime
 */
void initCharacterTurnState(Data::PlayerCharacter *ch,
                            uint8 ambushFlags,
                            Data::Effects::EffectRuntime *effectRuntime,
                            CombatGlobals *globals);

/**
 * Reset turn state for every character in the roster.
 *
 * Mirrors the per-character loop at the top of each round in COMBAT_MainLoop.
 * Skips null entries and characters without combatState.
 */
void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster,
                       uint8 ambushFlags,
                       Data::Effects::EffectRuntime *effectRuntime,
                       CombatGlobals *globals);

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
