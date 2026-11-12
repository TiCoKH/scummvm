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

#ifndef WIZARDRY_RESOURCES_SCENARIO_FILE_H
#define WIZARDRY_RESOURCES_SCENARIO_FILE_H

#include "common/array.h"
#include "common/str.h"

namespace Wizardry {

enum RecordType {
	kRecordHeader,
	kRecordMaze,
	kRecordMonster,
	kRecordReward,
	kRecordItem,
	kRecordCharacter,
	kRecordSpecial,
	kRecordExperience,
	kRecordTypeCount
};

/**
 * SCENARIO.DATA of Wizardry I to IV: a table of contents and records of
 * eight types. Records are packed into pairs of blocks (1024 bytes) and
 * never cross from one pair into the next.
 *
 * This class finds records. What is inside a record depends on the type
 * and on whether the release is an IBM-PC or a Japanese one.
 */
class ScenarioFile {
public:
	struct Type {
		uint16 perPair;      ///< records in two blocks
		uint16 count;        ///< number of records
		uint16 third;        ///< IBM-PC: record size. Japanese releases: blocks used.
		uint16 firstBlock;
		uint16 recordSize;   ///< bytes from one record to the next
	};

private:
	Common::Array<byte> _data;
	Common::String _title;
	Type _types[kRecordTypeCount];

public:
	bool open(const char *name);

	/**
	 * The game title on the IBM-PC, a build mark on the Japanese machines
	 */
	const Common::String &title() const { return _title; }

	const Type &type(RecordType t) const { return _types[t]; }

	/**
	 * Returns a record, or nullptr if there is none with this number
	 */
	const byte *record(RecordType t, uint index) const;
};

} // End of namespace Wizardry

#endif
