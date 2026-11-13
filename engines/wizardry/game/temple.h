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

#ifndef WIZARDRY_GAME_TEMPLE_H
#define WIZARDRY_GAME_TEMPLE_H

#include "wizardry/game/party.h"
#include "wizardry/game/random.h"

namespace Wizardry {

/**
 * Rules of the Temple of Cant of Wizardry I
 */
class Temple {
public:
	enum Outcome {
		kHealed,
		kNowAshes,   ///< the dead could not be raised
		kNowLost     ///< ashes could not be raised: gone for good
	};

	/**
	 * True if the temple takes the character in: afflicted, not lost and
	 * not lying in the maze
	 */
	static bool isPatient(const Character &c);

	/**
	 * The roster numbers the temple lists, at most 20
	 */
	static Common::Array<uint> patients(const Party &party);

	/**
	 * The donation asked: by affliction, times the character's level
	 */
	static int64 donation(const Character &c);

	/**
	 * Takes the donation from a member; if that member has too little,
	 * the whole party's gold is pooled first.
	 * @returns false if even that is not enough; the gold stays pooled
	 */
	static bool collect(Party &party, uint payer, int64 amount);

	/**
	 * The ritual. The dead rise if a roll of 0 to 99 is not above
	 * 50 + 3 x vitality (from ashes: 40 + 3 x vitality). Whoever is
	 * healed is up to a year older.
	 */
	static Outcome ritual(Character &c, Dice &dice);
};

} // End of namespace Wizardry

#endif
