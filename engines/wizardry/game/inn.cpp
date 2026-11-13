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

#include "common/endian.h"
#include "wizardry/game/inn.h"

namespace Wizardry {

enum {
	kMsgLearnedSpells = 428,
	kMsgDiedOfOldAge = 429,
	kMsgYouLost = 430,
	kMsgYouGained = 431,
	kMsgNextLevel = 432,
	kMsgYouNeed = 433,
	kMsgMore = 434,
	kMsgToMakeLevel = 435,
	kMsgGainedHitPoints = 442,
	kMsgBirthday = 444,
	kMsgAttributeNames = 10020
};

const Inn::Room Inn::ROOMS[kRoomCount] = {
	{ 0, 0 }, { 1, 10 }, { 3, 50 }, { 7, 200 }, { 10, 500 }
};

// First and last spell of each spell level
static const byte MAGE_SPELLS[7][2] = {
	{ 1, 4 }, { 5, 6 }, { 7, 8 }, { 9, 11 }, { 12, 14 }, { 15, 18 }, { 19, 21 }
};
static const byte PRIEST_SPELLS[7][2] = {
	{ 22, 26 }, { 27, 30 }, { 31, 34 }, { 35, 38 }, { 39, 44 }, { 45, 48 }, { 49, 50 }
};

Common::String substCaret(const Common::String &message, const Common::String &text) {
	const size_t pos = message.findFirstOf('^');
	if (pos == Common::String::npos)
		return message;

	return message.substr(0, pos) + text + message.substr(pos + 1);
}

bool Inn::canRest(const Character &c, const Room &room) {
	return c.gold >= room.cost && c.hp < c.hpMax;
}

bool Inn::restOneWeek(Character &c, const Room &room) {
	++c.age;
	c.hp = MIN<int>(c.hp + room.heal, c.hpMax);
	c.gold = MAX<int64>(c.gold - room.cost, 0);

	// Age is counted in weeks
	return c.age % 52 == 1;
}

int64 Inn::experienceForNextLevel(const Character &c) const {
	// Record type 7 has thirteen numbers for each class, each of three
	// words of four decimal digits. Numbers 1 to 12 are the experience
	// for levels 2 to 13; number 0 is what each further level costs.
	const byte *table = _scenario.record(kRecordExperience, 0);
	if (!table || c.classType < 0 || c.classType > kClassNinja)
		return 0;
	table += c.classType * 13 * 6;

	const int index = CLIP<int>(c.level, 0, 12);
	const byte *entry = table + index * 6;
	int64 needed = READ_LE_UINT16(entry) + READ_LE_UINT16(entry + 2) * (int64)10000 +
		READ_LE_UINT16(entry + 4) * (int64)100000000;

	if (c.level >= 13) {
		const int64 step = READ_LE_UINT16(table) + READ_LE_UINT16(table + 2) * (int64)10000 +
			READ_LE_UINT16(table + 4) * (int64)100000000;
		needed += step * (c.level - 12);
	}

	return needed;
}

void Inn::checkLevel(Character &c, bool birthday, Common::Array<Common::String> &lines) {
	if (birthday)
		lines.push_back(_messages.get(kMsgBirthday));

	const int64 needed = experienceForNextLevel(c);
	if (needed <= c.experience) {
		gainLevel(c, lines);
	} else {
		lines.push_back(_messages.get(kMsgYouNeed) +
			Common::String::format("%lld", (long long)(needed - c.experience)) +
			_messages.get(kMsgMore));
		lines.push_back(_messages.get(kMsgToMakeLevel));
	}

	// Resting brings all spells back, unless the stay ended badly
	if (c.status == kStatusOk)
		setSpellSlots(c);
}

void Inn::gainLevel(Character &c, Common::Array<Common::String> &lines) {
	lines.push_back(_messages.get(kMsgNextLevel));

	++c.level;
	c.maxLevel = MAX(c.maxLevel, c.level);

	setSpellSlots(c);
	learnSpells(c, lines);
	if (!changeAttributes(c, lines))
		return;   // died of old age

	// Hit points are rolled afresh for all levels; at least one is gained
	int hp = 0;
	for (int i = 0; i < c.level; ++i)
		hp += rollHitDie(c);
	if (c.classType == kClassSamurai)
		hp += rollHitDie(c);
	if (hp <= c.hpMax)
		hp = c.hpMax + 1;

	lines.push_back(substCaret(_messages.get(kMsgGainedHitPoints),
		Common::String::format("%d", hp - c.hpMax)));
	c.hp += hp - c.hpMax;
	c.hpMax = hp;
}

int Inn::rollHitDie(const Character &c) {
	int die;

	switch (c.classType) {
	case kClassFighter:
	case kClassLord:
		die = _dice.roll(10) + 1;
		break;
	case kClassMage:
		die = _dice.roll(4) + 1;
		break;
	case kClassPriest:
	case kClassSamurai:
		die = _dice.roll(8) + 1;
		break;
	default:
		die = _dice.roll(6) + 1;
		break;
	}

	switch (c.attributes[kVitality]) {
	case 3:
		die -= 2;
		break;
	case 4:
	case 5:
		die -= 1;
		break;
	case 16:
		die += 1;
		break;
	case 17:
		die += 2;
		break;
	case 18:
		die += 3;
		break;
	default:
		break;
	}

	return MAX(die, 1);
}

bool Inn::learnSpellLevel(Character &c, uint first, uint last, int attribute) {
	bool knowsOne = false, learned = false;
	for (uint spell = first; spell <= last; ++spell)
		knowsOne |= c.knowsSpell(spell);

	// Each spell not yet known is learned on a roll under the attribute;
	// the first spell of a new level is learned in any case
	for (uint spell = first; spell <= last; ++spell) {
		if (c.knowsSpell(spell))
			continue;

		// The die is rolled even when the result does not matter
		const bool rolled = (int)_dice.roll(30) < attribute;
		if (rolled || !knowsOne) {
			c.learnSpell(spell);
			knowsOne = learned = true;
		}
	}

	return learned;
}

void Inn::learnSpells(Character &c, Common::Array<Common::String> &lines) {
	bool learned = false;

	for (uint level = 0; level < 7; ++level) {
		if (c.mageSlots[level] > 0)
			learned |= learnSpellLevel(c, MAGE_SPELLS[level][0], MAGE_SPELLS[level][1], c.attributes[kIQ]);
	}
	for (uint level = 0; level < 7; ++level) {
		if (c.priestSlots[level] > 0)
			learned |= learnSpellLevel(c, PRIEST_SPELLS[level][0], PRIEST_SPELLS[level][1], c.attributes[kPiety]);
	}

	if (learned)
		lines.push_back(_messages.get(kMsgLearnedSpells));
	setSpellSlots(c);
}

bool Inn::changeAttributes(Character &c, Common::Array<Common::String> &lines) {
	for (int a = 0; a < kAttributeCount; ++a) {
		// Three times in four the attribute changes
		if (_dice.roll(4) == 0)
			continue;

		const Common::String &name = _messages.get(kMsgAttributeNames + a);
		int value = c.attributes[a];

		// The older, the more likely it is to fall (age in years out of 130)
		if ((int)_dice.roll(130) < c.age / 52) {
			// An 18 is kept five times in six
			const bool keep = (_dice.roll(6) != 4) && value == 18;
			if (!keep) {
				--value;
				lines.push_back(substCaret(_messages.get(kMsgYouLost), name));
				c.attributes[a] = value;

				if (a == kVitality && value == 2) {
					lines.push_back(_messages.get(kMsgDiedOfOldAge));
					c.status = kStatusLost;
					c.hp = 0;
					return false;
				}
			}
		} else if (value != 18) {
			c.attributes[a] = value + 1;
			lines.push_back(substCaret(_messages.get(kMsgYouGained), name));
		}
	}

	return true;
}

static void slotsByLevel(int16 *slots, int level, int start, int step) {
	int count = level - start;
	if (count < 1)
		return;

	for (int i = 0; i < 7 && count >= 1; ++i, count -= step)
		slots[i] = MAX<int>(slots[i], count);
	for (int i = 0; i < 7; ++i)
		slots[i] = MIN<int>(slots[i], 9);
}

void Inn::setSpellSlots(Character &c) {
	for (uint level = 0; level < 7; ++level) {
		c.mageSlots[level] = c.priestSlots[level] = 0;
		for (uint spell = MAGE_SPELLS[level][0]; spell <= MAGE_SPELLS[level][1]; ++spell)
			c.mageSlots[level] += c.knowsSpell(spell);
		for (uint spell = PRIEST_SPELLS[level][0]; spell <= PRIEST_SPELLS[level][1]; ++spell)
			c.priestSlots[level] += c.knowsSpell(spell);
	}

	switch (c.classType) {
	case kClassMage:
		slotsByLevel(c.mageSlots, c.level, 0, 2);
		break;
	case kClassPriest:
		slotsByLevel(c.priestSlots, c.level, 0, 2);
		break;
	case kClassBishop:
		slotsByLevel(c.priestSlots, c.level, 3, 4);
		slotsByLevel(c.mageSlots, c.level, 0, 4);
		break;
	case kClassSamurai:
		slotsByLevel(c.mageSlots, c.level, 3, 3);
		break;
	case kClassLord:
		slotsByLevel(c.priestSlots, c.level, 3, 3);
		break;
	default:
		break;
	}
}

} // End of namespace Wizardry
