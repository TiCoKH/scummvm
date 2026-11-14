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

#ifndef WIZARDRY_GAME_CAMP_SPELLS_H
#define WIZARDRY_GAME_CAMP_SPELLS_H

#include "wizardry/game/maze.h"
#include "wizardry/game/spells.h"

namespace Wizardry {

/**
 * The spells that can be cast in camp, by the rules of Wizardry I.
 *
 * The caster must know the spell and have a casting of its level left;
 * the target is asked for; the casting is used up; and only then, in a
 * field where spells fizzle, the spell fails all the same.
 */
class CampSpells {
public:
	enum Result {
		kNotACampSpell,   ///< "what?"
		kNotYet,          ///< a camp spell this version cannot do yet
		kFizzled,
		kDone,
		kHealed,          ///< hit points were given: show them
		kNotDead,         ///< raising someone who is not dead: nothing is used up
		kIsLost,          ///< raising someone who is gone for good: the same
		kTryKadorto,      ///< DI on ashes
		kRaised,
		kRaiseFailed
	};

	/** True if the spell does something in camp */
	static bool isCampSpell(int spell);

	/** True if it is cast on a member of the party */
	static bool needsTarget(int spell);

	/**
	 * Casts a spell the caster is able to cast
	 * @param target  Place in the party of whom it is for, if it needs one
	 */
	static Result cast(Party &party, uint caster, int spell, uint target, MazeState &maze, Dice &dice);
};

} // End of namespace Wizardry

#endif
