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
#include "wizardry/game/items.h"

namespace Wizardry {

enum {
	kOfsKind = 0,
	kOfsAlignment = 2,
	kOfsCursed = 4,
	kOfsSpecialPower = 6,
	kOfsChangeTo = 8,
	kOfsChangeChance = 10,
	kOfsHealPoints = 24,
	kOfsProtectVs = 26,
	kOfsResist = 28,
	kOfsArmorMod = 30,
	kOfsHitMod = 32,
	kOfsDamage = 34,
	kOfsSwings = 40,
	kOfsCriticalHits = 42,
	kOfsDoubleDamageVs = 44,
	kOfsPrice = 12,   // three words of four decimal digits, lowest first
	kOfsStock = 18,
	kOfsClassUse = 22
};

bool ItemList::load(const ScenarioFile &scenario) {
	_items.clear();

	const ScenarioFile::Type &type = scenario.type(kRecordItem);
	if (type.recordSize != Item::kRecordSize)
		return false;

	for (uint i = 0; i < type.count; ++i) {
		const byte *src = scenario.record(kRecordItem, i);
		if (!src)
			break;

		Item item;
		item.kind = READ_LE_INT16(src + kOfsKind);
		item.cursed = (READ_LE_UINT16(src + kOfsCursed) & 1) != 0;
		item.price = READ_LE_UINT16(src + kOfsPrice) +
			READ_LE_UINT16(src + kOfsPrice + 2) * (int64)10000 +
			READ_LE_UINT16(src + kOfsPrice + 4) * (int64)100000000;
		item.stock = READ_LE_INT16(src + kOfsStock);
		item.classUse = READ_LE_UINT16(src + kOfsClassUse);
		item.alignment = READ_LE_INT16(src + kOfsAlignment);
		item.specialPower = READ_LE_INT16(src + kOfsSpecialPower);
		item.changeTo = READ_LE_INT16(src + kOfsChangeTo);
		item.changeChance = READ_LE_INT16(src + kOfsChangeChance);
		item.healPoints = READ_LE_INT16(src + kOfsHealPoints);
		item.protectVs = READ_LE_UINT16(src + kOfsProtectVs);
		item.resist = READ_LE_UINT16(src + kOfsResist);
		item.armorMod = READ_LE_INT16(src + kOfsArmorMod);
		item.hitMod = READ_LE_INT16(src + kOfsHitMod);
		item.damage.count = READ_LE_INT16(src + kOfsDamage);
		item.damage.sides = READ_LE_INT16(src + kOfsDamage + 2);
		item.damage.bonus = READ_LE_INT16(src + kOfsDamage + 4);
		item.swings = READ_LE_INT16(src + kOfsSwings);
		item.criticalHits = (READ_LE_UINT16(src + kOfsCriticalHits) & 1) != 0;
		item.doubleDamageVs = READ_LE_UINT16(src + kOfsDoubleDamageVs);
		_items.push_back(item);
	}

	return true;
}

bool ItemList::synchronize(Common::Serializer &s) {
	uint16 count = _items.size();
	s.syncAsUint16LE(count);
	if (count != _items.size())
		return false;

	for (uint i = 0; i < count; ++i)
		s.syncAsSint16LE(_items[i].stock);

	return true;
}

} // End of namespace Wizardry
