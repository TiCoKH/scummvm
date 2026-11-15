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

#include "goldbox/data/record_io.h"
#include "common/util.h"

namespace Goldbox {
namespace Data {

const RecordLayout &RecordLayout::dos() {
	static const RecordLayout kDos = { false, false, false };
	return kDos;
}

RecordReader::RecordReader(Common::SeekableReadStream &stream,
		const RecordLayout &layout) :
		_stream(stream), _layout(layout), _start(stream.pos()) {
}

uint8 RecordReader::readByte() {
	return _stream.readByte();
}

int8 RecordReader::readSByte() {
	return _stream.readSByte();
}

uint16 RecordReader::readUint16() {
	align();
	return _layout.bigEndian ? _stream.readUint16BE() : _stream.readUint16LE();
}

uint32 RecordReader::readUint32() {
	align();
	return _layout.bigEndian ? _stream.readUint32BE() : _stream.readUint32LE();
}

void RecordReader::read(void *buffer, uint32 size) {
	_stream.read(buffer, size);
}

Common::String RecordReader::readName(uint maxLength) {
	char buffer[256] = {};
	maxLength = MIN<uint>(maxLength, sizeof(buffer) - 1);
	uint length;
	if (_layout.cStringNames) {
		// maxLength + 1 bytes ending in a zero.
		_stream.read(buffer, maxLength + 1);
		length = 0;
		while (length < maxLength && buffer[length])
			++length;
	} else {
		// Length byte, then maxLength characters.
		length = MIN<uint>(_stream.readByte(), maxLength);
		_stream.read(buffer, maxLength);
	}
	return Common::String(buffer, length);
}

void RecordReader::align() {
	if (_layout.alignWords && ((_stream.pos() - _start) & 1))
		_stream.skip(1);
}

RecordWriter::RecordWriter(Common::WriteStream &stream,
		const RecordLayout &layout) :
		_stream(stream), _layout(layout), _size(0) {
}

void RecordWriter::writeByte(uint8 value) {
	_stream.writeByte(value);
	++_size;
}

void RecordWriter::writeSByte(int8 value) {
	_stream.writeSByte(value);
	++_size;
}

void RecordWriter::writeUint16(uint16 value) {
	align();
	if (_layout.bigEndian)
		_stream.writeUint16BE(value);
	else
		_stream.writeUint16LE(value);
	_size += 2;
}

void RecordWriter::writeUint32(uint32 value) {
	align();
	if (_layout.bigEndian)
		_stream.writeUint32BE(value);
	else
		_stream.writeUint32LE(value);
	_size += 4;
}

void RecordWriter::write(const void *buffer, uint32 size) {
	_stream.write(buffer, size);
	_size += size;
}

void RecordWriter::writeName(const Common::String &name, uint maxLength) {
	const uint length = MIN<uint>(name.size(), maxLength);
	if (!_layout.cStringNames)
		writeByte(length);
	for (uint i = 0; i < maxLength; ++i)
		writeByte(i < length ? name[i] : 0);
	if (_layout.cStringNames)
		writeByte(0);
}

void RecordWriter::finish() {
	align();
}

void RecordWriter::align() {
	if (_layout.alignWords && (_size & 1))
		writeByte(0);
}

} // namespace Data
} // namespace Goldbox
