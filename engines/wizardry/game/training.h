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

#ifndef WIZARDRY_GAME_TRAINING_H
#define WIZARDRY_GAME_TRAINING_H

#include "wizardry/game/character.h"
#include "wizardry/game/random.h"

namespace Wizardry {

enum Race {
	kRaceNone, kRaceHuman, kRaceElf, kRaceDwarf, kRaceGnome, kRaceHobbit
};

enum {
	kClassCount = 8,
	kClassNone = 8,       ///< class of a character still being made
	kAlignNone = 0
};

/**
 * Rules of the training grounds of Wizardry I: making a character
 */
class Training {
public:
	/**
	 * Attributes a race starts with
	 */
	static const int16 *raceBase(int race);

	/**
	 * A new character before anything is chosen: 500 weeks old, 100 to
	 * 199 gold, 8 hit points, armour class 10
	 */
	static void newCharacter(Character &c, Dice &dice);

	/**
	 * Points to share out over the attributes: 5 to 9, and twice a
	 * chance of 1 in 20 of ten more
	 */
	static int rollBonus(Dice &dice);

	/**
	 * True if a class is open to someone of these attributes and alignment
	 */
	static bool isEligible(int classType, const int16 *attributes, int alignment);

	/**
	 * Makes the character a beginner of a class: the attributes of the
	 * race, level 1, no experience, some five to six years older, the
	 * first spells for a mage, bishop or priest, nothing equipped but
	 * what is cursed. The original uses this for a change of class too.
	 */
	static void setClass(Character &c, int classType, Dice &dice);

	/**
	 * Ends the making of a character: the class, then the attributes
	 * as they were shared out
	 */
	static void finish(Character &c, int classType, const int16 *attributes, Dice &dice);
};

} // End of namespace Wizardry

#endif
