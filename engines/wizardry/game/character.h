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

#ifndef WIZARDRY_GAME_CHARACTER_H
#define WIZARDRY_GAME_CHARACTER_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Wizardry {

enum CharacterClass {
	kClassFighter, kClassMage, kClassPriest, kClassThief,
	kClassBishop, kClassSamurai, kClassLord, kClassNinja
};

enum CharacterStatus {
	kStatusOk, kStatusAfraid, kStatusAsleep, kStatusParalysed,
	kStatusStoned, kStatusDead, kStatusAshes, kStatusLost
};

enum Attribute {
	kStrength, kIQ, kPiety, kVitality, kAgility, kLuck, kAttributeCount
};

enum Alignment {
	kAlignGood = 1,
	kAlignNeutral = 2,
	kAlignEvil = 3
};

/**
 * One place in a character's pack
 */
struct ItemSlot {
	bool equipped;
	bool cursed;
	bool identified;
	int16 id;   ///< number of the item record
};

/**
 * A character, as kept in record type 5 of the scenario file of IBM-PC
 * Wizardry I (208 bytes). A free place in the roster is a record with the
 * status "lost".
 *
 * Only the fields the town needs so far are taken out. The whole record is
 * kept, and the fields are written back into it, so nothing is lost.
 */
struct Character {
	enum {
		kRecordSize = 208,
		kMaxItems = 8
	};

	Common::String name;
	Common::String password;
	bool inMaze = false;       ///< in the party, or left in the maze
	int16 race = 0;
	int16 classType = 0;       ///< CharacterClass
	int16 age = 0;             ///< in weeks
	int16 status = kStatusLost;
	int16 alignment = kAlignNeutral;
	int64 gold = 0;
	int16 attributes[6] = {};  ///< strength, IQ, piety, vitality, agility, luck
	int64 experience = 0;
	int16 maxLevel = 0;        ///< highest level reached
	int16 level = 0;
	int16 mageSlots[7] = {};   ///< spells that can be cast, by spell level
	int16 priestSlots[7] = {};
	int16 hp = 0, hpMax = 0;
	int16 armorClass = 0;
	int16 poison = 0;
	int16 lostPosition = 0;    ///< where in the maze the body lies; 0 in town
	int16 lostLevel = 0;       ///< maze level where the body lies; 0 in town
	int16 itemCount = 0;
	ItemSlot items[kMaxItems] = {};

	byte record[kRecordSize] = {};

	void load(const byte *src);
	void save(byte *dst);

	bool isFree() const { return status == kStatusLost; }

	bool addItem(int16 id, bool identified);
	void removeItem(uint index);

	/**
	 * Spells are numbered 1 to 21 (mage) and 22 to 50 (priest)
	 */
	bool knowsSpell(uint spell) const;
	void learnSpell(uint spell);
};

} // End of namespace Wizardry

#endif
