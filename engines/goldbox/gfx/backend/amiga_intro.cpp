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

#include "goldbox/gfx/backend/amiga_intro.h"
#include "common/endian.h"
#include "common/file.h"
#include "common/textconsole.h"

namespace Goldbox {
namespace Gfx {

static const uint32 kHunkHeader = 0x3F3;
static const uint32 kHunkCode = 0x3E9;
static const uint32 kHunkData = 0x3EA;
static const uint32 kHunkBss = 0x3EB;
static const uint32 kHunkReloc32 = 0x3EC;
static const uint32 kHunkEnd = 0x3F2;
static const uint32 kHunkSymbol = 0x3F0;
static const uint32 kHunkDebug = 0x3F1;

// Unpacked program: hunk 5 is the chip data (CHIP DATA 0x1F5B0 bytes).
static const uint kChipHunk = 5;
static const uint kChipHunkSize = 0x1F5B0;
static const uint kPaintingOffset = 0x0;
static const uint kPaintingPlaneStride = 8000;   // 320 x 200 / 8
static const uint kSheetOffset = 0x7D20;
static const uint kSheetPlaneStride = 24000;     // 320 x 600 / 8
static const uint kPaletteOffsets[2] = { 0x1F52E, 0x1F54E };

bool AmigaIntro::readHunks(const Common::Array<uint8> &file,
		Common::Array<Hunk> &hunks) {
	uint pos = 0;
	auto readLong = [&](uint32 &value) {
		if (pos + 4 > file.size())
			return false;
		value = READ_BE_UINT32(&file[pos]);
		pos += 4;
		return true;
	};

	uint32 value;
	if (!readLong(value) || value != kHunkHeader)
		return false;
	// Resident library names (none expected), then the hunk table.
	while (readLong(value) && value != 0)
		pos += value * 4;
	uint32 tableSize, first, last;
	if (!readLong(tableSize) || !readLong(first) || !readLong(last) || last < first)
		return false;
	const uint count = last - first + 1;
	hunks.resize(count);
	for (uint i = 0; i < count; ++i) {
		if (!readLong(value))
			return false;
		hunks[i].type = 0;
	}

	uint index = 0;
	while (pos < file.size() && index < count) {
		uint32 type;
		if (!readLong(type))
			return false;
		type &= 0x3FFFFFFF;
		// The crunched intro has its code and data blocks without a
		// HUNK_END between them: a second content block starts a new hunk.
		if ((type == kHunkCode || type == kHunkData || type == kHunkBss)
				&& hunks[index].type != 0 && ++index >= count)
			return false;
		if (type == kHunkCode || type == kHunkData) {
			uint32 longs;
			if (!readLong(longs) || pos + longs * 4 > file.size())
				return false;
			hunks[index].type = type;
			hunks[index].data.resize(longs * 4);
			memcpy(hunks[index].data.data(), &file[pos], longs * 4);
			pos += longs * 4;
		} else if (type == kHunkBss) {
			uint32 longs;
			if (!readLong(longs))
				return false;
			hunks[index].type = type;
			hunks[index].data.resize(longs * 4, 0);
		} else if (type == kHunkReloc32) {
			// The pictures need no relocation.
			uint32 offsets;
			while (readLong(offsets) && offsets != 0)
				pos += 4 + offsets * 4;
		} else if (type == kHunkEnd) {
			++index;
		} else if (type == kHunkSymbol) {
			uint32 longs;
			while (readLong(longs) && longs != 0)
				pos += longs * 4 + 4;
		} else if (type == kHunkDebug) {
			uint32 longs;
			if (!readLong(longs))
				return false;
			pos += longs * 4;
		} else {
			warning("AmigaIntro: unexpected hunk type 0x%x", type);
			return false;
		}
	}
	return index == count;
}

bool AmigaIntro::decrunch(const Common::Array<uint8> &packed,
		const uint8 efficiency[4], Common::Array<uint8> &out) {
	// PowerPacker: the bit stream is read backwards from the end, low bit
	// first; the last long holds the unpacked length (24 bits) and the bits
	// to skip in the first stream long. The output is written backwards.
	uint srcPos = packed.size();
	if (srcPos < 8 || srcPos % 4)
		return false;
	auto readLong = [&]() -> uint32 {
		srcPos -= 4;
		return READ_BE_UINT32(&packed[srcPos]);
	};

	uint32 last = readLong();
	const uint skip = last & 0xFF;
	const uint length = last >> 8;
	out.resize(length);
	uint dst = length;

	uint32 bits = readLong() >> skip;
	int bitsLeft = 32 - skip;
	bool overrun = false;
	auto getBits = [&](int count) -> uint {
		uint value = 0;
		while (count-- > 0) {
			value = (value << 1) | (bits & 1);
			bits >>= 1;
			if (--bitsLeft == 0) {
				if (srcPos < 4) {
					overrun = true;
					return value;
				}
				bits = readLong();
				bitsLeft = 32;
			}
		}
		return value;
	};

	while (dst > 0 && !overrun) {
		if (getBits(1) == 0) {
			// Literal run: 2-bit counts while all ones, plus one.
			uint count = 0, part;
			do {
				part = getBits(2);
				count += part;
			} while (part == 3 && !overrun);
			for (uint i = 0; i <= count && dst > 0; ++i)
				out[--dst] = getBits(8);
			if (dst == 0)
				break;
		}

		const uint index = getBits(2);
		uint offsetBits = efficiency[index];
		uint count = index + 1;
		uint offset;
		if (count == 4) {
			if (getBits(1) == 0)
				offsetBits = 7;
			offset = getBits(offsetBits);
			uint part;
			do {
				part = getBits(3);
				count += part;
			} while (part == 7 && !overrun);
		} else {
			offset = getBits(offsetBits);
		}
		for (uint i = 0; i <= count && dst > 0; ++i) {
			if (dst + offset >= length)
				return false;
			const uint8 value = out[dst + offset];
			out[--dst] = value;
		}
	}
	return !overrun && dst == 0;
}

void AmigaIntro::decodePlanar(const uint8 *src, int height, int planeStride,
		Graphics::ManagedSurface &dst) {
	dst.create(kWidth, height, Graphics::PixelFormat::createFormatCLUT8());
	const int rowBytes = kWidth / 8;
	for (int y = 0; y < height; ++y) {
		uint8 *row = (uint8 *)dst.getBasePtr(0, y);
		for (int x = 0; x < kWidth; ++x) {
			const int offset = y * rowBytes + x / 8;
			const int bit = 7 - (x & 7);
			uint8 color = 0;
			for (int plane = 0; plane < 4; ++plane)
				color |= ((src[plane * planeStride + offset] >> bit) & 1) << plane;
			row[x] = color;
		}
	}
}

bool AmigaIntro::load(const Common::Path &path) {
	Common::File file;
	if (!file.open(path)) {
		warning("AmigaIntro: %s not found", path.toString().c_str());
		return false;
	}
	Common::Array<uint8> outer(file.size());
	file.read(outer.data(), outer.size());

	Common::Array<Hunk> hunks;
	if (!readHunks(outer, hunks)) {
		warning("AmigaIntro: %s is not an Amiga program", path.toString().c_str());
		return false;
	}

	auto isProgram = [](const Common::Array<Hunk> &h) {
		return h.size() > kChipHunk && h[kChipHunk].type == kHunkData
			&& h[kChipHunk].data.size() == kChipHunkSize;
	};

	// The original file is crunched: hunk 0 is the decruncher, its last 8
	// bytes the efficiency table and a spare long, hunk 1 the program. An
	// install may hold the program already unpacked.
	if (!isProgram(hunks)) {
		if (hunks.size() != 2 || hunks[0].type != kHunkCode
				|| hunks[0].data.size() < 8 || hunks[1].type != kHunkData) {
			Common::String layout;
			for (uint i = 0; i < hunks.size(); ++i)
				layout += Common::String::format(" %x:%u", hunks[i].type,
					hunks[i].data.size());
			warning("AmigaIntro: %s has an unknown layout (%u hunks:%s)",
				path.toString().c_str(), hunks.size(), layout.c_str());
			return false;
		}
		// Copied out: the hunk list is cleared before unpacking.
		uint8 efficiency[4];
		memcpy(efficiency, &hunks[0].data[hunks[0].data.size() - 8], 4);
		Common::Array<uint8> program;
		const Common::Array<uint8> packed = hunks[1].data;
		hunks.clear();
		if (!decrunch(packed, efficiency, program) || !readHunks(program, hunks)
				|| !isProgram(hunks)) {
			warning("AmigaIntro: %s could not be unpacked", path.toString().c_str());
			return false;
		}
	}

	const uint8 *chip = hunks[kChipHunk].data.data();
	decodePlanar(chip + kPaintingOffset, kPaintingHeight, kPaintingPlaneStride,
		_painting);
	decodePlanar(chip + kSheetOffset, kSheetHeight, kSheetPlaneStride, _sheet);
	for (int p = 0; p < 2; ++p)
		for (int c = 0; c < kColors; ++c)
			_palettes[p][c] = READ_BE_UINT16(chip + kPaletteOffsets[p] + c * 2);
	return true;
}

} // namespace Gfx
} // namespace Goldbox
