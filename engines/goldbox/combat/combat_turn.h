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

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

struct CombatGlobals;

/**
 * Reset per-turn CombatAction fields for one character.
 *
 * Mirrors original COMBAT_InitCharacterTurnState:
 *   - Clears fleeing, guarding, moralFailure, directionChange
 *   - Resets attackCount from character's base attack rate
 *   - Resets movePoints from character's movement stat
 *   - Preserves direction, notInTeam, bleeding, aiState across turns
 *
 * @param ch  Character whose combatState is reset (must be non-null with
 *            a valid combatState pointer — i.e. called during combat only).
 */
void initCharacterTurnState(Data::PlayerCharacter *ch);

/**
 * Reset turn state for every character in the roster.
 *
 * Mirrors the per-character loop at the top of each round in
 * COMBAT_MainLoop. Skips null entries and characters without combatState.
 */
void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster);

/**
 * Select the next character to act this round.
 *
 * Mirrors COMBAT_SelectNextActor:
 *   - Iterates roster in order
 *   - Skips disabled, dead, or already-acted characters (delay == 0xFF)
 *   - Returns the first character whose delay counter is lowest
 *   - Returns nullptr when all characters have acted (round complete)
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
