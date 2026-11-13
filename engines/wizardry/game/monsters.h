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

#ifndef WIZARDRY_GAME_MONSTERS_H
#define WIZARDRY_GAME_MONSTERS_H

#include "common/array.h"
#include "wizardry/game/character.h"
#include "wizardry/resources/scenario_file.h"

namespace Wizardry {

/**
 * A kind of monster: record type 2 of the scenario file of IBM-PC
 * Wizardry I (94 bytes)
 */
struct Monster {
	enum {
		kRecordSize = 94,
		kMaxAttacks = 7
	};

	enum Flags {
		kStones = 1 << 0,
		kPoisons = 1 << 1,
		kParalyses = 1 << 2,
		kKillsOutright = 1 << 3,
		kCanSleep = 1 << 4,
		kMayRun = 1 << 5,
		kMayCallHelp = 1 << 6
	};

	int16 pictureId = 0;
	Dice3 groupSize = { 0, 0, 0 };
	Dice3 hitDice = { 0, 0, 0 };    ///< the number of dice is also the monster's level
	int16 monsterClass = 0;
	int16 armorClass = 0;
	int16 attackCount = 0;
	Dice3 attackDamage[kMaxAttacks] = {};
	int16 levelDrain = 0;
	int16 regeneration = 0;
	int16 partnerId = 0;            ///< the monster that may come along as the next group
	int16 partnerChance = 0;
	int16 mageSpellLevel = 0;
	int16 priestSpellLevel = 0;
	int16 breathType = 0;
	int16 magicResistance = 0;
	uint16 elementResist = 0;
	uint16 flags = 0;

	/**
	 * What defeating one of them is worth
	 */
	int64 experience() const;
};

class MonsterList {
private:
	Common::Array<Monster> _monsters;

public:
	bool load(const ScenarioFile &scenario);

	uint size() const { return _monsters.size(); }
	const Monster &operator[](uint id) const { return _monsters[id]; }

	/**
	 * Message number of a monster's name: there are four for each, for
	 * one or several of them, recognised or not
	 */
	static uint nameMessage(uint id, bool identified, bool plural) {
		return 13000 + id * 4 + (identified ? 2 : 0) + (plural ? 1 : 0);
	}
};

} // End of namespace Wizardry

#endif
