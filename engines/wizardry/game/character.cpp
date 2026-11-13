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
	kOfsAttributes = 44,   // two words with three attributes of five bits each
	kOfsGold = 52,         // three words of four decimal digits, lowest first
	kOfsItemCount = 58,
	kOfsItems = 60,        // eight times four words: equipped, cursed, identified, item
	kOfsExperience = 124,  // the same
	kOfsMaxLevel = 130,
	kOfsSpellsKnown = 138, // one bit per spell
	kOfsMageSlots = 146,   // seven words
	kOfsPriestSlots = 160, // seven words
	kOfsLevel = 132,
	kOfsHp = 134,
	kOfsHpMax = 136,
	kOfsArmorClass = 176,
	kOfsPoison = 200,
	kOfsLostPosition = 202,
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

static int64 readLong(const byte *src) {
	return READ_LE_UINT16(src) + READ_LE_UINT16(src + 2) * (int64)10000 +
		READ_LE_UINT16(src + 4) * (int64)100000000;
}

static void writeLong(byte *dst, int64 value) {
	// The original stops at twelve digits
	value = CLIP<int64>(value, 0, 999999999999LL);
	WRITE_LE_UINT16(dst, value % 10000);
	WRITE_LE_UINT16(dst + 2, value / 10000 % 10000);
	WRITE_LE_UINT16(dst + 4, value / 100000000);
}

bool Character::knowsSpell(uint spell) const {
	return (record[kOfsSpellsKnown + spell / 8] >> (spell % 8)) & 1;
}

void Character::learnSpell(uint spell) {
	record[kOfsSpellsKnown + spell / 8] |= 1 << (spell % 8);
}

bool Character::addItem(int16 id, bool identified) {
	if (itemCount >= kMaxItems)
		return false;

	const ItemSlot slot = { false, false, identified, id };
	items[itemCount++] = slot;
	return true;
}

void Character::removeItem(uint index) {
	if ((int)index >= itemCount)
		return;

	for (int i = index; i + 1 < itemCount; ++i)
		items[i] = items[i + 1];
	--itemCount;
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
	gold = readLong(src + kOfsGold);
	experience = readLong(src + kOfsExperience);
	maxLevel = READ_LE_INT16(src + kOfsMaxLevel);
	for (uint i = 0; i < kAttributeCount; ++i)
		attributes[i] = (READ_LE_UINT16(src + kOfsAttributes + (i / 3) * 2) >> ((i % 3) * 5)) & 0x1f;
	for (uint i = 0; i < 7; ++i) {
		mageSlots[i] = READ_LE_INT16(src + kOfsMageSlots + i * 2);
		priestSlots[i] = READ_LE_INT16(src + kOfsPriestSlots + i * 2);
	}
	level = READ_LE_INT16(src + kOfsLevel);
	hp = READ_LE_INT16(src + kOfsHp);
	hpMax = READ_LE_INT16(src + kOfsHpMax);
	armorClass = READ_LE_INT16(src + kOfsArmorClass);
	poison = READ_LE_INT16(src + kOfsPoison);
	lostPosition = READ_LE_INT16(src + kOfsLostPosition);
	lostLevel = READ_LE_INT16(src + kOfsLostLevel);

	itemCount = CLIP<int>(READ_LE_INT16(src + kOfsItemCount), 0, kMaxItems);
	for (uint i = 0; i < kMaxItems; ++i) {
		const byte *p = src + kOfsItems + i * 8;
		items[i].equipped = (READ_LE_UINT16(p) & 1) != 0;
		items[i].cursed = (READ_LE_UINT16(p + 2) & 1) != 0;
		items[i].identified = (READ_LE_UINT16(p + 4) & 1) != 0;
		items[i].id = READ_LE_INT16(p + 6);
	}
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

	writeLong(record + kOfsGold, gold);
	writeLong(record + kOfsExperience, experience);
	WRITE_LE_INT16(record + kOfsMaxLevel, maxLevel);
	for (uint w = 0; w < 2; ++w) {
		// Bit 15 of these words is not an attribute; keep it
		uint word = READ_LE_UINT16(record + kOfsAttributes + w * 2) & 0x8000;
		for (uint i = 0; i < 3; ++i)
			word |= (attributes[w * 3 + i] & 0x1f) << (i * 5);
		WRITE_LE_UINT16(record + kOfsAttributes + w * 2, word);
	}
	for (uint i = 0; i < 7; ++i) {
		WRITE_LE_INT16(record + kOfsMageSlots + i * 2, mageSlots[i]);
		WRITE_LE_INT16(record + kOfsPriestSlots + i * 2, priestSlots[i]);
	}

	WRITE_LE_INT16(record + kOfsLevel, level);
	WRITE_LE_INT16(record + kOfsHp, hp);
	WRITE_LE_INT16(record + kOfsHpMax, hpMax);
	WRITE_LE_INT16(record + kOfsArmorClass, armorClass);
	WRITE_LE_INT16(record + kOfsPoison, poison);
	WRITE_LE_INT16(record + kOfsLostPosition, lostPosition);
	WRITE_LE_INT16(record + kOfsLostLevel, lostLevel);

	// Places past the last item keep whatever the record had
	WRITE_LE_INT16(record + kOfsItemCount, itemCount);
	for (int i = 0; i < itemCount; ++i) {
		byte *p = record + kOfsItems + i * 8;
		WRITE_LE_UINT16(p, items[i].equipped ? 1 : 0);
		WRITE_LE_UINT16(p + 2, items[i].cursed ? 1 : 0);
		WRITE_LE_UINT16(p + 4, items[i].identified ? 1 : 0);
		WRITE_LE_INT16(p + 6, items[i].id);
	}

	memcpy(dst, record, kRecordSize);
}

} // End of namespace Wizardry
