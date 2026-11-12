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
#include "wizardry/resources/files.h"
#include "wizardry/resources/scenario_file.h"

namespace Wizardry {

bool ScenarioFile::open(const char *name) {
	// The title is a Pascal string of up to 40 characters. Then come four
	// arrays with one word for each record type.
	const uint ARRAYS = 0x2A;

	_title.clear();
	for (Type &t : _types)
		t = Type();

	if (!name || !readFile(name, _data) || _data.size() < ARRAYS + 4 * 2 * kRecordTypeCount)
		return false;
	if (_data[0] < 5 || _data[0] > 40)
		return false;

	for (uint i = 0; i < kRecordTypeCount; ++i) {
		Type &t = _types[i];
		t.perPair = READ_LE_UINT16(&_data[ARRAYS + i * 2]);
		t.count = READ_LE_UINT16(&_data[ARRAYS + 16 + i * 2]);
		t.third = READ_LE_UINT16(&_data[ARRAYS + 32 + i * 2]);
		t.firstBlock = READ_LE_UINT16(&_data[ARRAYS + 48 + i * 2]);

		if (!t.perPair || t.perPair > 64 || t.firstBlock * BLOCK_SIZE > _data.size())
			return false;

		// The IBM-PC releases store the record size in the third word. On
		// the Japanese machines it is a number of blocks, and all that is
		// known is the room a record has.
		t.recordSize = 2 * BLOCK_SIZE / t.perPair;
		if (t.third && t.third <= t.recordSize && (uint)t.third * t.perPair > 2 * BLOCK_SIZE - t.third)
			t.recordSize = t.third;
	}

	_title = Common::String((const char *)&_data[1], _data[0]);
	return true;
}

const byte *ScenarioFile::record(RecordType t, uint index) const {
	const Type &type = _types[t];
	if (!type.perPair || index >= type.count)
		return nullptr;

	const uint offset = (type.firstBlock + 2 * (index / type.perPair)) * BLOCK_SIZE +
		(index % type.perPair) * type.recordSize;
	if (offset + type.recordSize > _data.size())
		return nullptr;

	return &_data[offset];
}

} // End of namespace Wizardry
