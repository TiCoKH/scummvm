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

#include "common/algorithm.h"
#include "common/endian.h"
#include "wizardry/resources/files.h"
#include "wizardry/resources/messages.h"

namespace Wizardry {

bool MessageFile::open(const char *name, const char *huffmanTable) {
	Common::Array<byte> data, table;
	_entries.clear();

	if (!name || !readFile(name, data) || data.size() < 16)
		return false;

	if (huffmanTable) {
		if (!readFile(huffmanTable, table) || table.size() < 768)
			return false;
		loadHuffman(data, table);
	} else {
		loadPlain(data);
	}

	sortEntries();
	return !_entries.empty();
}

/**
 * Wizardry I to III. Header: block and byte count of the index, block and
 * byte count of the range table. A range is five words: first id, last id,
 * index of the first id, and two links the game uses for searching. An index
 * entry is the position of a string in words. A string is a length byte and
 * then its bytes, which are scrambled with the message number and the
 * position in the string.
 */
void MessageFile::loadPlain(const Common::Array<byte> &data) {
	const uint indexPos = READ_LE_UINT16(&data[0]) * BLOCK_SIZE;
	const uint indexCount = READ_LE_UINT16(&data[2]) / 2;
	const uint rangePos = READ_LE_UINT16(&data[4]) * BLOCK_SIZE;
	const uint rangeBytes = READ_LE_UINT16(&data[6]);

	if (!indexPos || indexPos >= rangePos || rangePos + rangeBytes > data.size() ||
			indexPos + indexCount * 2 > rangePos || !rangeBytes || (rangeBytes % 10))
		return;

	for (uint r = 0; r < rangeBytes / 10; ++r) {
		const byte *range = &data[rangePos + r * 10];
		const uint first = READ_LE_UINT16(range);
		const uint last = READ_LE_UINT16(range + 2);
		const uint base = READ_LE_UINT16(range + 4);
		if (first > last || last - first > 4000)
			continue;

		for (uint id = first; id <= last; ++id) {
			const uint k = base + id - first;
			if (k >= indexCount)
				continue;

			const uint offset = READ_LE_UINT16(&data[indexPos + k * 2]) * 2;
			if (offset >= data.size())
				continue;

			const uint length = MIN<uint>(data[offset], data.size() - offset - 1);
			const uint key = 67 * (id % 51);

			Entry e;
			e.id = id;
			for (uint j = 0; j < length; ++j)
				e.text += (char)(data[offset + 1 + j] - key - 23 * (j + 1));
			_entries.push_back(e);
		}
	}
}

namespace {

/**
 * Decoder for the text of Wizardry IV and V, which the original does in the
 * native driver. The table is three arrays of 256 bytes: flags, the branch
 * for a 1 bit and the branch for a 0 bit. Bits are taken from the low end
 * of each byte. Flag bit 0 (for a 1 bit) or bit 1 (for a 0 bit) says that
 * the value reached is a decoded byte and not the next node.
 */
class HuffmanReader {
private:
	const byte *_table;
	const byte *_data;
	uint _size, _pos;
	uint _bits = 0, _bitCount = 0;

public:
	bool _failed = false;

	HuffmanReader(const byte *table, const byte *data, uint size, uint pos) :
		_table(table), _data(data), _size(size), _pos(pos) {}

	byte symbol() {
		uint node = 0;

		for (int depth = 0; depth < 256; ++depth) {
			if (!_bitCount) {
				if (_pos >= _size)
					break;
				_bits = _data[_pos++];
				_bitCount = 8;
			}

			const bool one = (_bits & 1) != 0;
			_bits >>= 1;
			--_bitCount;

			const bool leaf = (_table[node] & (one ? 1 : 2)) != 0;
			node = _table[(one ? 256 : 512) + node];
			if (leaf)
				return node;
		}

		_failed = true;
		return 0;
	}
};

} // End of anonymous namespace

/**
 * Wizardry IV and V. Header: block and byte count of the index, which has
 * one word per data block: the number of its first message. Messages are
 * packed from the start of each block, each starting on a byte boundary;
 * the first decoded byte is the length. A directory is read backwards from
 * the last byte of the block: the number of runs, then for each run the
 * number of messages and one byte length for each. Between two runs is the
 * gap in message numbers, in one byte, or in two when bit 7 of the first is
 * set. The next run starts at: first id of this run + count - 1 + gap.
 */
void MessageFile::loadHuffman(const Common::Array<byte> &data, const Common::Array<byte> &table) {
	const uint indexBlock = READ_LE_UINT16(&data[0]);
	const uint indexCount = READ_LE_UINT16(&data[2]) / 2;
	const uint indexPos = indexBlock * BLOCK_SIZE;

	if (indexPos + indexCount * 2 > data.size())
		return;

	for (uint block = 0; block < MIN(indexBlock, indexCount); ++block) {
		uint dir = (block + 1) * BLOCK_SIZE - 1;
		uint pos = block * BLOCK_SIZE + (block == 0 ? 4 : 0);
		uint id = READ_LE_UINT16(&data[indexPos + block * 2]);

		const uint runs = data[dir--];
		for (uint r = 0; r < runs; ++r) {
			const uint count = data[dir--];

			for (uint i = 0; i < count; ++i) {
				const uint length = data[dir--];

				HuffmanReader reader(&table[0], &data[0], data.size(), pos);
				Entry e;
				e.id = id + i;
				for (uint n = reader.symbol(); n > 0 && !reader._failed; --n)
					e.text += (char)reader.symbol();
				if (reader._failed)
					e.text = "?";
				_entries.push_back(e);

				pos += length;
			}

			if (r + 1 < runs) {
				uint gap = data[dir--];
				if (gap >= 0x80)
					gap = ((gap - 0x80) << 8) | data[dir--];
				id += count - 1 + gap;
			}
		}
	}
}

void MessageFile::sortEntries() {
	// Keep the order of loading for equal ids, then let the last one win
	Common::Array<uint> order;
	for (uint i = 0; i < _entries.size(); ++i)
		order.push_back(i);

	const Common::Array<Entry> &entries = _entries;
	struct Less {
		const Common::Array<Entry> &_e;
		Less(const Common::Array<Entry> &e) : _e(e) {}
		bool operator()(uint a, uint b) const {
			return _e[a].id != _e[b].id ? _e[a].id < _e[b].id : a < b;
		}
	};
	Common::sort(order.begin(), order.end(), Less(entries));

	Common::Array<Entry> sorted;
	for (uint i = 0; i < order.size(); ++i) {
		const Entry &e = _entries[order[i]];
		if (!sorted.empty() && sorted.back().id == e.id)
			sorted.back() = e;
		else
			sorted.push_back(e);
	}

	_entries = sorted;
}

bool MessageFile::contains(uint id) const {
	return &get(id) != &_empty;
}

const Common::String &MessageFile::get(uint id) const {
	uint low = 0, high = _entries.size();

	while (low < high) {
		const uint mid = (low + high) / 2;
		if (_entries[mid].id < id)
			low = mid + 1;
		else
			high = mid;
	}

	return (low < _entries.size() && _entries[low].id == id) ? _entries[low].text : _empty;
}

} // End of namespace Wizardry
