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

#ifndef GOLDBOX_DATA_RECORD_IO_H
#define GOLDBOX_DATA_RECORD_IO_H

#include "common/str.h"
#include "common/stream.h"

namespace Goldbox {
namespace Data {

/**
 * How a port stores the game's records (characters, items, effects): the
 * fields are the same, but byte order, alignment and strings differ.
 */
struct RecordLayout {
	bool bigEndian;    ///< Words and longs are big-endian
	bool alignWords;   ///< Words and longs start at an even record offset
	bool cStringNames; ///< Names are zero-terminated, not length-prefixed

	/** The DOS layout, also used by the engine's own saves. */
	static const RecordLayout &dos();
};

/** Reads one record field by field in a platform's layout. */
class RecordReader {
public:
	RecordReader(Common::SeekableReadStream &stream, const RecordLayout &layout);

	uint8 readByte();
	int8 readSByte();
	uint16 readUint16();
	uint32 readUint32();
	void read(void *buffer, uint32 size);

	/** A name field of maxLength + 1 bytes holding up to maxLength characters. */
	Common::String readName(uint maxLength = 15);

private:
	void align();

	Common::SeekableReadStream &_stream;
	const RecordLayout &_layout;
	int64 _start;
};

/** Writes one record field by field in a platform's layout. */
class RecordWriter {
public:
	RecordWriter(Common::WriteStream &stream, const RecordLayout &layout);

	void writeByte(uint8 value);
	void writeSByte(int8 value);
	void writeUint16(uint16 value);
	void writeUint32(uint32 value);
	void write(const void *buffer, uint32 size);

	/** A name field of maxLength + 1 bytes holding up to maxLength characters. */
	void writeName(const Common::String &name, uint maxLength = 15);

	/**
	 * Pad the record to an even size when words are aligned, as a C struct
	 * written with sizeof(); not every record is stored that way.
	 */
	void finish();

private:
	void align();

	Common::WriteStream &_stream;
	const RecordLayout &_layout;
	uint32 _size;
};

} // namespace Data
} // namespace Goldbox

#endif
