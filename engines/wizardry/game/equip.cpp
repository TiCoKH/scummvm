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

#include "wizardry/game/equip.h"
#include "wizardry/game/training.h"

namespace Wizardry {

const int16 Equip::KINDS[kKindCount] = { 0, 1, 2, 3, 4, 6 };

// How much each class and race lowers the five saving throws
static const int16 CLASS_SAVES[kClassCount][5] = {
	{ 3, 0, 0, 0, 0 },    // fighter
	{ 0, 0, 0, 0, 3 },    // mage
	{ 0, 3, 0, 0, 0 },    // priest
	{ 0, 0, 0, 3, 0 },    // thief
	{ 0, 2, 2, 0, 2 },    // bishop
	{ 2, 0, 0, 0, 2 },    // samurai
	{ 2, 2, 0, 0, 0 },    // lord
	{ 3, 2, 4, 3, 2 }     // ninja
};
static const int16 RACE_SAVES[6][5] = {
	{ 0, 0, 0, 0, 0 },
	{ 1, 0, 0, 0, 0 },    // human
	{ 0, 0, 2, 0, 0 },    // elf
	{ 0, 0, 0, 4, 0 },    // dwarf
	{ 0, 2, 0, 0, 0 },    // gnome
	{ 0, 0, 0, 0, 3 }     // hobbit
};

static bool isFighterKind(int classType) {
	return classType == kClassFighter || classType == kClassSamurai ||
		classType == kClassLord || classType == kClassNinja;
}

void Equip::begin(Character &c, const ItemList &items, bool unequip) {
	const int classType = CLIP<int>(c.classType, 0, kClassCount - 1);
	const int race = (c.race >= kRaceHuman && c.race <= kRaceHobbit) ? c.race : 0;

	const int base = MAX(20 - c.level / 5 - c.attributes[kLuck] / 6, 1);
	for (int i = 0; i < 5; ++i) {
		// Class and race lower it one after the other, never below 1
		int save = MAX(base - CLASS_SAVES[classType][i], 1);
		c.savingThrows[i] = MAX(save - RACE_SAVES[race][i], 1);
	}

	if (unequip) {
		for (int i = 0; i < c.itemCount; ++i)
			c.items[i].equipped = false;
	}

	c.hitBonus = (isFighterKind(classType) || classType == kClassPriest) ? c.level / 3 + 2 : c.level / 5;

	// Bare hands
	const Dice3 hands = { 2, (int16)(classType == kClassNinja ? 4 : 2), 0 };
	c.damage = hands;

	const int strength = c.attributes[kStrength];
	if (strength >= 16) {
		c.hitBonus += strength - 15;
		c.damage.bonus = strength - 15;
	} else if (strength < 6) {
		c.hitBonus += strength - 6;
	}

	c.healPoints = 0;
	c.criticalHits = (classType == kClassNinja);
	c.armorClass = 10;
	c.swingCount = 1;
	if (isFighterKind(classType))
		c.swingCount = 1 + c.level / 5 + (classType == kClassNinja ? 1 : 0);
	c.swingCount = MIN<int>(c.swingCount, 10);

	// What is carried counts here, equipped or not
	c.protectVs = c.resist = c.doubleDamageVs = 0;
	for (int i = 0; i < c.itemCount; ++i) {
		const int id = c.items[i].id;
		if (id < 0 || id >= (int)items.size())
			continue;

		c.healPoints = MAX(c.healPoints, items[id].healPoints);
		c.protectVs |= items[id].protectVs & 0x3fff;
		c.resist |= items[id].resist & 0x7f;
	}
}

void Equip::applyItem(Character &c, uint slot, const ItemList &items) {
	const Item &item = items[c.items[slot].id];
	c.items[slot].cursed = item.cursed;

	if (item.alignment == 0 || item.alignment == c.alignment) {
		c.swingCount = MAX(c.swingCount, item.swings);
		c.armorClass -= item.armorMod;
		c.hitBonus += item.hitMod;

		if (item.kind == 0) {
			// The weapon's dice, with the bonus of strength on top
			const int16 bonus = c.damage.bonus;
			c.damage = item.damage;
			c.damage.bonus += bonus;
			c.criticalHits |= item.criticalHits;
			c.doubleDamageVs = item.doubleDamageVs;
		}
	} else {
		--c.hitBonus;
		++c.armorClass;
		c.criticalHits = false;
		c.items[slot].cursed = true;
	}
}

void Equip::end(Character &c, bool anyApplied) {
	if (c.classType == kClassNinja && !anyApplied)
		c.armorClass -= c.level / 3 + 2;
}

void Equip::recalc(Character &c, const ItemList &items) {
	bool any = false;
	begin(c, items, false);

	for (int i = 0; i < c.itemCount; ++i) {
		if (c.items[i].equipped && c.items[i].id >= 0 && c.items[i].id < (int)items.size()) {
			applyItem(c, i, items);
			any = true;
		}
	}

	end(c, any);
}

Common::Array<uint> Equip::candidates(const Character &c, int kind, const ItemList &items) {
	Common::Array<uint> result;

	for (int i = 0; i < c.itemCount; ++i) {
		const int id = c.items[i].id;
		if (id > 0 && id < (int)items.size() && items[id].kind == kind && items[id].usableBy(c))
			result.push_back(i);
	}

	return result;
}

bool Equip::hasPower(const Character &c, uint slot, const ItemList &items) {
	const int id = c.items[slot].id;
	return id > 0 && id < (int)items.size() && items[id].specialPower > 0 && items[id].usableBy(c);
}

void Equip::invokePower(Party &party, uint member, uint slot, const ItemList &items, Dice &dice) {
	Character &c = party.member(member);
	const Item &item = items[c.items[slot].id];

	// The item may be used up
	if ((int)dice.roll(100) < item.changeChance)
		c.items[slot].id = item.changeTo;

	const int power = item.specialPower;
	if (power >= 1 && power <= 12) {
		// An attribute up or down, as long as it stays within 3 to 18
		const int attribute = (power - 1) % 6;
		const int value = c.attributes[attribute] + (power <= 6 ? 1 : -1);
		if (value > 2 && value < 19)
			c.attributes[attribute] = value;
		return;
	}

	switch (power) {
	case 13:
		// A year younger, but not below twenty
		if (c.age > 1040)
			c.age -= 52;
		break;
	case 14:
		c.age += 52;
		break;
	case 15:
		c.classType = kClassSamurai;
		break;
	case 16:
		c.classType = kClassLord;
		break;
	case 17:
		c.classType = kClassNinja;
		break;
	case 18:
		c.gold = MIN<int64>(c.gold + 50000, 999999999999LL);
		break;
	case 19:
		c.experience = MIN<int64>(c.experience + 50000, 999999999999LL);
		break;
	case 20:
		c.status = kStatusLost;
		break;
	case 21:
		c.status = kStatusOk;
		c.hp = c.hpMax;
		c.poison = 0;
		break;
	case 22:
		++c.hpMax;
		break;
	case 23:
		for (uint i = 0; i < party.size(); ++i)
			party.member(i).hp = party.member(i).hpMax;
		break;
	case 24:
		// Samurai, lord or ninja
		c.classType = kClassSamurai + dice.roll(3);
		break;
	case 25:
		// One of the first five classes, at the price of one's life
		c.classType = kClassFighter + dice.roll(5);
		c.status = kStatusDead;
		c.hp = 0;
		break;
	case 26:
	case 27:
		for (int i = 0; i < 7; ++i)
			c.mageSlots[i] = c.priestSlots[i] = 9;
		// The second of these also wipes the spell books
		if (power == 27) {
			for (uint spell = 1; spell <= 50; ++spell)
				c.forgetSpell(spell);
		}
		break;
	default:
		break;
	}
}

} // End of namespace Wizardry
