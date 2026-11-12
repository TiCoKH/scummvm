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
#include "common/util.h"
#include "wizardry/game/character.h"

namespace Wizardry {

// Offsets in the record
enum {
	kOfsName = 0,          // length byte and 15 characters
	kOfsPassword = 16,     // the same
	kOfsInMaze = 32,
	kOfsRace = 34,
	kOfsClass = 36,
	kOfsAge = 38,
	kOfsStatus = 40,
	kOfsAlignment = 42,
	kOfsGold = 52,         // three words of four decimal digits, lowest first
	kOfsLevel = 132,
	kOfsHp = 134,
	kOfsHpMax = 136,
	kOfsArmorClass = 176,
	kOfsPoison = 200,
	kOfsLostLevel = 204
};

static Common::String readPascalString(const byte *src) {
	return Common::String((const char *)src + 1, MIN<uint>(src[0], 15));
}

static void writePascalString(byte *dst, const Common::String &str) {
	const uint length = MIN<uint>(str.size(), 15);
	dst[0] = length;
	for (uint i = 0; i < 15; ++i)
		dst[1 + i] = (i < length) ? str[i] : 0;
}

void Character::load(const byte *src) {
	memcpy(record, src, kRecordSize);

	name = readPascalString(src + kOfsName);
	password = readPascalString(src + kOfsPassword);
	inMaze = (READ_LE_UINT16(src + kOfsInMaze) & 1) != 0;
	race = READ_LE_INT16(src + kOfsRace);
	classType = READ_LE_INT16(src + kOfsClass);
	age = READ_LE_INT16(src + kOfsAge);
	status = READ_LE_INT16(src + kOfsStatus);
	alignment = READ_LE_INT16(src + kOfsAlignment);
	gold = READ_LE_UINT16(src + kOfsGold) +
		READ_LE_UINT16(src + kOfsGold + 2) * (int64)10000 +
		READ_LE_UINT16(src + kOfsGold + 4) * (int64)100000000;
	level = READ_LE_INT16(src + kOfsLevel);
	hp = READ_LE_INT16(src + kOfsHp);
	hpMax = READ_LE_INT16(src + kOfsHpMax);
	armorClass = READ_LE_INT16(src + kOfsArmorClass);
	poison = READ_LE_INT16(src + kOfsPoison);
	lostLevel = READ_LE_INT16(src + kOfsLostLevel);
}

void Character::save(byte *dst) {
	writePascalString(record + kOfsName, name);
	writePascalString(record + kOfsPassword, password);
	WRITE_LE_UINT16(record + kOfsInMaze, inMaze ? 1 : 0);
	WRITE_LE_INT16(record + kOfsRace, race);
	WRITE_LE_INT16(record + kOfsClass, classType);
	WRITE_LE_INT16(record + kOfsAge, age);
	WRITE_LE_INT16(record + kOfsStatus, status);
	WRITE_LE_INT16(record + kOfsAlignment, alignment);

	// The original stops at twelve digits
	const int64 g = CLIP<int64>(gold, 0, 999999999999LL);
	WRITE_LE_UINT16(record + kOfsGold, g % 10000);
	WRITE_LE_UINT16(record + kOfsGold + 2, g / 10000 % 10000);
	WRITE_LE_UINT16(record + kOfsGold + 4, g / 100000000);

	WRITE_LE_INT16(record + kOfsLevel, level);
	WRITE_LE_INT16(record + kOfsHp, hp);
	WRITE_LE_INT16(record + kOfsHpMax, hpMax);
	WRITE_LE_INT16(record + kOfsArmorClass, armorClass);
	WRITE_LE_INT16(record + kOfsPoison, poison);
	WRITE_LE_INT16(record + kOfsLostLevel, lostLevel);

	memcpy(dst, record, kRecordSize);
}

} // End of namespace Wizardry
