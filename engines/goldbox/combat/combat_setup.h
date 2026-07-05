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

#ifndef GOLDBOX_COMBAT_COMBAT_SETUP_H
#define GOLDBOX_COMBAT_COMBAT_SETUP_H

#include "common/scummsys.h"
#include "common/array.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

/**
 * Initialize combat state for all combatants.
 *
 * Mirrors original COMBAT_InitCombatStates:
 * - Allocates CombatState for each character
 * - Sets initial facing from approach direction (way_flag)
 * - Flips hostile characters 180 degrees
 * - Marks characters beyond partyCount as not-in-team
 * - Assigns morale-based NPC value for neutral non-team characters
 *
 * @param combatants  All characters participating in combat
 * @param partyCount  Number of player-controlled characters
 * @param wayFlag     Approach direction (0-7, from position struct)
 * @param moraleThreshold  Party morale threshold for neutral NPC assignment
 */
void initCombatStates(Common::Array<Data::PlayerCharacter *> &combatants,
                         int partyCount, uint8 wayFlag,
                         uint8 moraleThreshold);

/**
 * Free combat state for all combatants (end of combat cleanup).
 */
void freeCombatStates(Common::Array<Data::PlayerCharacter *> &combatants);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_SETUP_H
