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
#include "wizardry/game/monsters.h"

namespace Wizardry {

static Dice3 readDice(const byte *src) {
	const Dice3 d = { READ_LE_INT16(src), READ_LE_INT16(src + 2), READ_LE_INT16(src + 4) };
	return d;
}

bool MonsterList::load(const ScenarioFile &scenario) {
	_monsters.clear();

	const ScenarioFile::Type &type = scenario.type(kRecordMonster);
	if (type.recordSize != Monster::kRecordSize)
		return false;

	for (uint i = 0; i < type.count; ++i) {
		const byte *src = scenario.record(kRecordMonster, i);
		if (!src)
			break;

		Monster m;
		m.pictureId = READ_LE_INT16(src);
		m.groupSize = readDice(src + 2);
		m.hitDice = readDice(src + 8);
		m.monsterClass = READ_LE_INT16(src + 14);
		m.armorClass = READ_LE_INT16(src + 16);
		m.attackCount = READ_LE_INT16(src + 18);
		for (uint a = 0; a < Monster::kMaxAttacks; ++a)
			m.attackDamage[a] = readDice(src + 20 + a * 6);
		m.levelDrain = READ_LE_INT16(src + 68);
		m.regeneration = READ_LE_INT16(src + 70);
		m.partnerId = READ_LE_INT16(src + 76);
		m.partnerChance = READ_LE_INT16(src + 78);
		m.mageSpellLevel = READ_LE_INT16(src + 80);
		m.priestSpellLevel = READ_LE_INT16(src + 82);
		m.breathType = READ_LE_INT16(src + 86);
		m.magicResistance = READ_LE_INT16(src + 88);
		m.elementResist = READ_LE_UINT16(src + 90);
		m.flags = READ_LE_UINT16(src + 92);
		_monsters.push_back(m);
	}

	return true;
}

// k for one, doubled for each one more; nothing for none
static int64 doubled(int n, int64 k) {
	if (n <= 0)
		return 0;
	return k << MIN(n - 1, 30);
}

static int bitsSet(uint value, int first, int last) {
	int count = 0;
	for (int bit = first; bit <= last; ++bit)
		count += (value >> bit) & 1;
	return count;
}

int64 Monster::experience() const {
	int64 x = (int64)hitDice.count * hitDice.sides * (breathType > 0 ? 40 : 20);
	x += doubled(mageSpellLevel, 35);
	x += doubled(priestSpellLevel, 35);
	x += doubled(levelDrain, 200);
	x += doubled(regeneration, 90);
	x += (11 - armorClass) * 40;
	if (attackCount > 1)
		x += doubled(attackCount, 30);
	if (magicResistance > 0)
		x += doubled(magicResistance / 10 + 1, 40);
	x += doubled(bitsSet(elementResist, 1, 6), 35);
	x += doubled(bitsSet(flags, 0, 6), 40);
	return x;
}

} // End of namespace Wizardry
