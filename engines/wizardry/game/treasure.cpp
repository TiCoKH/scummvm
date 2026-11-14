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
#include "wizardry/game/treasure.h"
#include "wizardry/game/spells.h"

namespace Wizardry {

enum {
	kMsgFound = 802,          // "^ FOUND : #"
	kMsgTrapNames = 803,      // trapless chest, poison needle, gas bomb, then the three
	                          // mechanical ones, teleporter, the two blasters, alarm
	kMsgIsSlain = 813
};

bool RewardList::load(const ScenarioFile &scenario) {
	_rewards.clear();

	const ScenarioFile::Type &type = scenario.type(kRecordReward);
	if (type.recordSize != Reward::kRecordSize)
		return false;

	for (uint i = 0; i < type.count; ++i) {
		const byte *src = scenario.record(kRecordReward, i);
		if (!src)
			break;

		Reward r;
		r.hasChest = (READ_LE_UINT16(src) & 1) != 0;
		r.traps = READ_LE_UINT16(src + 2);
		r.count = CLIP<int>(READ_LE_INT16(src + 4), 0, Reward::kMaxEntries);
		for (uint e = 0; e < Reward::kMaxEntries; ++e) {
			const byte *p = src + 6 + e * 18;
			r.entries[e].chance = READ_LE_INT16(p);
			r.entries[e].isItem = READ_LE_INT16(p + 2);
			for (uint v = 0; v < 7; ++v)
				r.entries[e].values[v] = READ_LE_INT16(p + 4 + v * 2);
		}
		_rewards.push_back(r);
	}

	return true;
}

Chest::Chest() {
	for (uint i = 0; i < Party::kMaxMembers; ++i)
		_inspected[i] = false;
}

uint Chest::trapNameMessage(int trap, int variant) {
	if (trap < kMechanical)
		return kMsgTrapNames + trap;
	if (trap == kMechanical)
		return kMsgTrapNames + 3 + variant;
	return kMsgTrapNames + 2 + trap;
}

void Chest::pickTrap(const Reward &reward, int mazeLevel, Dice &dice) {
	_variant = dice.roll(3);
	_trap = kNoTrap;

	if (!(reward.traps & 0xfe))
		return;
	// On the upper levels some chests have no trap; from level 3 all have
	if ((int)dice.roll(15) > mazeLevel + 11)
		return;

	// Round the kinds allowed until the steps are used up. Leaving the
	// mechanical kind costs three, so the kind after it comes up more.
	int steps = dice.roll(100);
	while (steps > 0) {
		do {
			if (_trap < kAlarm) {
				steps -= (_trap == kMechanical) ? 3 : 1;
				++_trap;
			} else {
				_trap = kPoisonNeedle;
			}
		} while (!((reward.traps >> _trap) & 1));
	}
}

void Chest::damage(Character &c, int amount, const MessageFile &messages, Common::Array<Common::String> &lines) {
	if (c.status >= kStatusDead)
		return;

	c.hp -= amount;
	if (c.hp < 1) {
		c.hp = 0;
		c.status = kStatusDead;

		Common::String line = messages.get(kMsgIsSlain);
		const size_t pos = line.findFirstOf('^');
		if (pos != Common::String::npos)
			line = line.substr(0, pos) + c.name + line.substr(pos + 1);
		lines.push_back(line);
	}
}

void Chest::classCurse(Party &party, int classType, Dice &dice) {
	// The class the trap is made for is paralysed, or turned to stone
	// if the saving throw fails; a kindred class is paralysed if it
	// fails. A state is only ever made worse.
	const int kindred = (classType == kClassMage) ? kClassSamurai : kClassBishop;

	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);
		const bool saved = (int)dice.roll(20) < c.savingThrows[4];

		int status = kStatusOk;
		if (c.classType == classType)
			status = saved ? kStatusParalysed : kStatusStoned;
		else if (c.classType == kindred && !saved)
			status = kStatusParalysed;

		c.status = MAX<int>(c.status, status);
	}
}

void Chest::spring(Party &party, uint member, MazeState &maze, const MessageFile &messages, Dice &dice,
		Common::Array<Common::String> &lines) {
	Character &opener = party.member(member);

	switch (_trap) {
	case kPoisonNeedle:
		++opener.poison;
		break;

	case kGasBomb:
		// As the original has it: poisoned on a roll under the saving throw
		for (uint i = 0; i < party.size(); ++i) {
			if ((int)dice.roll(20) < party.member(i).savingThrows[0])
				party.member(i).poison = 1;
		}
		break;

	case kMechanical:
		if (_variant == kCrossbowBolt) {
			int amount = 0;
			for (int d = 0; d < maze.level; ++d)
				amount += dice.roll(8) + 1;
			damage(opener, amount, messages, lines);
		} else if (_variant == kExplodingBox) {
			for (uint i = 0; i < party.size(); ++i) {
				int sides = 0;
				if (dice.roll(100) < 50)
					sides = 8;
				else if (dice.roll(100) < 50)
					sides = 5;

				int amount = 0;
				for (int d = 0; sides && d < maze.level; ++d)
					amount += dice.roll(sides) + 1;
				if (amount)
					damage(party.member(i), amount, messages, lines);
			}
		} else {
			opener.status = MAX<int>(opener.status, kStatusParalysed);
		}
		break;

	case kTeleporter:
		// To a place at random on the level
		maze.x = dice.roll(MazeLevel::kSize);
		maze.y = dice.roll(MazeLevel::kSize);
		maze.facing = dice.roll(4);
		maze.randomPosition = true;
		break;

	case kMageBlaster:
		classCurse(party, kClassMage, dice);
		break;

	case kPriestBlaster:
		classCurse(party, kClassPriest, dice);
		break;

	case kAlarm:
		_alarm = true;
		break;

	default:
		break;
	}
}

bool Chest::open(Party &party, uint member, MazeState &maze, const MessageFile &messages, Dice &dice,
		Common::Array<Common::String> &lines) {
	if (_trap == kNoTrap || (int)dice.roll(1000) < party.member(member).level)
		return false;

	spring(party, member, maze, messages, dice, lines);
	return true;
}

uint Chest::inspect(Party &party, uint member, MazeState &maze, const MessageFile &messages, Dice &dice,
		bool &sprung, Common::Array<Common::String> &lines) {
	sprung = false;
	if (_inspected[member])
		return 0;
	_inspected[member] = true;

	const Character &c = party.member(member);
	int chance = c.attributes[kAgility];
	if (c.classType == kClassThief)
		chance *= 6;
	else if (c.classType == kClassNinja)
		chance *= 4;
	chance = MIN(chance, 95);

	if ((int)dice.roll(100) < chance)
		return trapNameMessage();

	if ((int)dice.roll(20) > c.attributes[kAgility]) {
		sprung = true;
		spring(party, member, maze, messages, dice, lines);
		return trapNameMessage();
	}

	// A wrong answer, which looks just like a right one
	return trapNameMessage(dice.roll(8), dice.roll(3));
}

uint Chest::calfo(Character &c, Dice &dice) {
	if (!Spells::canCast(c, CALFO))
		return 0;
	Spells::spend(c, CALFO);

	if (dice.roll(100) < 95)
		return trapNameMessage();
	return trapNameMessage(dice.roll(8), dice.roll(3));
}

Chest::DisarmResult Chest::disarm(Party &party, uint member, const Common::String &name, MazeState &maze,
		const MessageFile &messages, Dice &dice, Common::Array<Common::String> &lines) {
	const Character &c = party.member(member);

	if (name != messages.get(trapNameMessage())) {
		spring(party, member, maze, messages, dice, lines);
		return kSetOff;
	}

	int chance = c.level - maze.level - 7;
	if (c.classType == kClassThief || c.classType == kClassNinja)
		chance += 50;

	if ((int)dice.roll(70) < chance)
		return kDisarmed;
	if ((int)dice.roll(20) < c.attributes[kAgility])
		return kTryAgain;

	spring(party, member, maze, messages, dice, lines);
	return kSetOff;
}

static int rollDice(Dice &dice, int count, int sides, int bonus) {
	int result = bonus;
	for (int i = 0; i < count; ++i) {
		if (sides > 0)
			result += dice.roll(sides) + 1;
	}
	return result;
}

int64 Treasure::collect(Party &party, const Reward &reward, const ItemList &items, int goldFactor,
		const MessageFile &messages, Dice &dice, Common::Array<Common::String> &lines) {
	int64 gold = 0;

	for (int e = 0; e < reward.count; ++e) {
		const Reward::Entry &entry = reward.entries[e];
		if ((int)dice.roll(100) > entry.chance)
			continue;

		const int16 *v = entry.values;
		if (!entry.isItem) {
			gold += (int64)rollDice(dice, v[0], v[1], v[2]) * v[3] * rollDice(dice, v[4], v[5], v[6]) * goldFactor;
			continue;
		}

		// An item from a range, further along it now and then
		int steps = 0;
		while ((int)dice.roll(100) + 1 <= v[4] && steps < v[2])
			++steps;
		int id = v[0] + v[1] * steps;
		if (v[3] > 0)
			id += dice.roll(v[3]);
		id = MIN<int>(id, items.size() - 1);

		// To the first healthy member with room, starting with one at
		// random; lost if nobody has room
		const uint first = dice.roll(party.size());
		for (uint n = 0; n < party.size(); ++n) {
			Character &c = party.member((first + n) % party.size());
			if (c.status != kStatusOk || c.itemCount >= Character::kMaxItems)
				continue;

			c.addItem(id, false);

			Common::String line = messages.get(kMsgFound);
			size_t pos = line.findFirstOf('^');
			if (pos != Common::String::npos)
				line = line.substr(0, pos) + c.name + line.substr(pos + 1);
			pos = line.findFirstOf('#');
			if (pos != Common::String::npos)
				line = line.substr(0, pos) + messages.get(ItemList::nameMessage(id, false)) + line.substr(pos + 1);
			lines.push_back(line);
			break;
		}
	}

	uint survivors = 0;
	for (uint i = 0; i < party.size(); ++i)
		survivors += party.member(i).status == kStatusOk;
	if (!survivors)
		return 0;

	const int64 share = gold / survivors;
	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);
		if (c.status == kStatusOk)
			c.gold = MIN<int64>(c.gold + share, 999999999999LL);
	}

	return share;
}

} // End of namespace Wizardry
