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

#include "wizardry/game/training.h"

namespace Wizardry {

// Strength, IQ, piety, vitality, agility, luck
static const int16 RACE_BASE[6][kAttributeCount] = {
	{ 0, 0, 0, 0, 0, 0 },
	{ 8, 8, 5, 8, 8, 9 },       // human
	{ 7, 10, 10, 6, 9, 6 },     // elf
	{ 10, 7, 10, 10, 5, 6 },    // dwarf
	{ 7, 7, 10, 8, 10, 7 },     // gnome
	{ 5, 7, 7, 6, 10, 15 }      // hobbit
};

// Least attributes, then whether good, neutral and evil are allowed
static const struct {
	int16 least[kAttributeCount];
	bool alignment[3];
} CLASS_NEEDS[kClassCount] = {
	{ { 11, 0, 0, 0, 0, 0 }, { true, true, true } },          // fighter
	{ { 0, 11, 0, 0, 0, 0 }, { true, true, true } },          // mage
	{ { 0, 0, 11, 0, 0, 0 }, { true, false, true } },         // priest
	{ { 0, 0, 0, 0, 11, 0 }, { false, true, true } },         // thief
	{ { 0, 12, 12, 0, 0, 0 }, { true, false, true } },        // bishop
	{ { 15, 11, 10, 14, 10, 0 }, { true, true, false } },     // samurai
	{ { 15, 12, 12, 15, 14, 15 }, { true, false, false } },   // lord
	{ { 17, 17, 17, 17, 17, 17 }, { false, false, true } }    // ninja
};

const int16 *Training::raceBase(int race) {
	return RACE_BASE[(race >= kRaceHuman && race <= kRaceHobbit) ? race : 0];
}

void Training::newCharacter(Character &c, Dice &dice) {
	c = Character();
	c.age = 500;
	c.gold = 100 + dice.roll(100);
	c.status = kStatusOk;
	c.hp = c.hpMax = 8;
	c.classType = kClassNone;
	c.race = kRaceNone;
	c.alignment = kAlignNone;
	c.level = 1;
	c.armorClass = 10;
}

int Training::rollBonus(Dice &dice) {
	int bonus = 5 + dice.roll(5);
	if (dice.roll(20) == 10)
		bonus += 10;
	if (dice.roll(20) == 10)
		bonus += 10;
	return bonus;
}

bool Training::isEligible(int classType, const int16 *attributes, int alignment) {
	if (classType < 0 || classType >= kClassCount || alignment < kAlignGood || alignment > kAlignEvil)
		return false;

	for (int a = 0; a < kAttributeCount; ++a) {
		if (attributes[a] < CLASS_NEEDS[classType].least[a])
			return false;
	}

	return CLASS_NEEDS[classType].alignment[alignment - kAlignGood];
}

void Training::setClass(Character &c, int classType, Dice &dice) {
	const int16 *base = raceBase(c.race);
	for (int a = 0; a < kAttributeCount; ++a)
		c.attributes[a] = base[a];

	c.classType = classType;
	c.level = 1;
	c.experience = 0;
	c.age += 52 * dice.roll(3) + 252;

	// The first spells: two of the first level
	if (classType == kClassMage || classType == kClassBishop) {
		c.learnSpell(3);
		c.learnSpell(1);
	} else if (classType == kClassPriest) {
		c.learnSpell(23);
		c.learnSpell(24);
	}

	for (int i = 0; i < 7; ++i)
		c.mageSlots[i] = c.priestSlots[i] = 0;

	for (int i = 0; i < c.itemCount; ++i) {
		if (!c.items[i].cursed)
			c.items[i].equipped = false;
	}
}

void Training::finish(Character &c, int classType, const int16 *attributes, Dice &dice) {
	setClass(c, classType, dice);
	c.maxLevel = 1;
	for (int a = 0; a < kAttributeCount; ++a)
		c.attributes[a] = attributes[a];
}

} // End of namespace Wizardry
