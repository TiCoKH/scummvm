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
#include "wizardry/resources/picture_file.h"

namespace Wizardry {

bool PictureFile::open(const Profile &profile, const GfxSet &gfx) {
	_index.clear();
	_data.clear();

	if (!gfx.pictures || !readFile(gfx.pictures, _data))
		return false;

	_cols = profile.pictureCols;
	_rows = profile.pictureRows;
	_cellW = gfx.cellW;
	_cellH = gfx.cellH;
	_planes = (gfx.layout == kPixelsPlanarBRG) ? 3 : 1;
	_packed = profile.picturePacking != kPicturesPlain;

	switch (profile.picturePacking) {
	case kPicturesPackedIndexed: {
		Common::Array<byte> interp;
		if (!profile.interpreter || !readFile(profile.interpreter, interp) || !readTable(interp))
			return false;
		break;
	}

	case kPicturesPackedSequential:
		walkPictures();
		break;

	default: {
		// Records are a whole number of blocks
		const uint recordSize = (pictureDataSize() + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE;
		for (uint offset = 0; offset + recordSize <= _data.size(); offset += recordSize) {
			Entry e = { offset, recordSize };
			_index.push_back(e);
		}
		break;
	}
	}

	return !_index.empty();
}

bool PictureFile::readTable(const Common::Array<byte> &interp) {
	// The interpreter holds one entry of four bytes per picture: first block
	// (word), number of blocks (byte), zero. Its place differs between
	// builds, so look for the longest run in which every picture starts
	// where the one before it ends.
	const uint fileBlocks = _data.size() / BLOCK_SIZE;
	uint bestPos = 0, bestCount = 0;

	for (uint base = 0; base + 8 < interp.size(); ++base) {
		if (interp[base] || interp[base + 1] || !interp[base + 2] || interp[base + 2] >= 64)
			continue;

		uint count = 0, expect = 0;
		for (uint pos = base; pos + 4 <= interp.size(); pos += 4, ++count) {
			const uint start = READ_LE_UINT16(&interp[pos]);
			const uint blocks = interp[pos + 2];
			if (start != expect || interp[pos + 3] || !blocks || blocks >= 64 ||
					start + blocks > fileBlocks)
				break;
			expect = start + blocks;
		}

		if (count > bestCount) {
			bestCount = count;
			bestPos = base;
		}
	}

	if (bestCount < 8)
		return false;

	for (uint i = 0; i < bestCount; ++i) {
		const byte *p = &interp[bestPos + i * 4];
		Entry e = { READ_LE_UINT16(p) * BLOCK_SIZE, p[2] * BLOCK_SIZE };
		_index.push_back(e);
	}

	return true;
}

static bool anyByteSet(const Common::Array<byte> &data, uint pos, uint count) {
	for (uint i = pos; i < pos + count && i < data.size(); ++i) {
		if (data[i])
			return true;
	}
	return false;
}

void PictureFile::walkPictures() {
	// No table: every picture starts on a block boundary, so take them one
	// after another until only empty blocks follow
	const uint groups = pictureDataSize() / 64;
	Common::Array<byte> scratch;

	for (uint pos = 0; pos < _data.size() && anyByteSet(_data, pos, 4 * BLOCK_SIZE);) {
		const uint used = unpack(&_data[pos], _data.size() - pos, scratch, groups);
		const uint size = (used + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE;
		Entry e = { pos, size };
		_index.push_back(e);
		pos += size;
	}
}

uint PictureFile::unpack(const byte *src, uint srcSize, Common::Array<byte> &dst, uint groups) {
	// The picture is cut into groups of 64 bytes. A zero byte stands for a
	// group of zeros. Any other byte is a mask, highest bit first, over
	// eight rows of eight bytes: a clear bit is a row of zeros, a set bit is
	// followed by a second mask that says which bytes of the row are stored.
	uint pos = 0;
	dst.clear();
	dst.resize(groups * 64);
	byte *out = dst.empty() ? nullptr : &dst[0];

	for (uint g = 0; g < groups && pos < srcSize; ++g, out += 64) {
		const byte rowMask = src[pos++];
		if (!rowMask)
			continue;

		for (uint row = 0; row < 8; ++row) {
			if (!(rowMask & (0x80 >> row)))
				continue;

			const byte byteMask = (pos < srcSize) ? src[pos] : 0;
			++pos;
			for (uint col = 0; col < 8; ++col) {
				if (byteMask & (0x80 >> col)) {
					out[row * 8 + col] = (pos < srcSize) ? src[pos] : 0;
					++pos;
				}
			}
		}
	}

	return pos;
}

bool PictureFile::load(uint id, CellImage &image) const {
	if (id < 1 || id > _index.size())
		return false;

	const Entry &e = _index[id - 1];
	const byte *src = &_data[e.offset];
	uint srcSize = MIN<uint>(e.size, _data.size() - e.offset);
	Common::Array<byte> unpacked;

	if (_packed) {
		unpack(src, srcSize, unpacked, pictureDataSize() / 64);
		src = &unpacked[0];
		srcSize = unpacked.size();
	}

	image.create(_cols, _rows, _cellW, _cellH);
	const uint cellBytes = cellDataSize(_cellH, _planes);

	for (uint cell = 0; cell < (uint)_cols * _rows; ++cell) {
		const uint offset = cell * cellBytes;
		decodeCell(src + MIN(offset, srcSize), (offset < srcSize) ? srcSize - offset : 0,
			image.getBasePtr((cell % _cols) * _cellW, (cell / _cols) * _cellH),
			image.width(), _cellW, _cellH, _planes);
	}

	return true;
}

} // End of namespace Wizardry
