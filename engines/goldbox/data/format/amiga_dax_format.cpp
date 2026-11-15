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

#include "goldbox/data/format/amiga_dax_format.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {
namespace Data {

static const uint kAmigaPicHeaderSize = 12;
static const uint kDosPicHeaderSize = 17;

// DOS portrait heads are 88 pixels wide; Amiga heads are 56 pixels and
// DisplayPortraitHead draws them two cells right of the DOS position.
static const uint kDosHeadWidth = 88;

void AmigaDaxFormat::parseHeaders(Common::ReadStream &stream,
		DaxHeaderContainer &headers) const {
	headers.parseHeadersBE(stream);
}

bool AmigaDaxFormat::unpack(const Common::Array<uint8> &packed,
		Common::Array<uint8> &out) const {
	// The bit stream is read backwards from the end and the output written
	// backwards. The last three BE longwords hold the first bit word, a
	// checksum and the unpacked size.
	const uint srcLength = packed.size();
	const uint dstLength = out.size();
	if (srcLength < 12 || READ_BE_UINT32(&packed[srcLength - 4]) != dstLength)
		return false;

	const uint8 *src = packed.data();
	uint8 *dst = out.data();
	uint srcPos = srcLength - 12;
	uint32 bits = READ_BE_UINT32(src + srcPos);
	uint32 checksum = READ_BE_UINT32(src + srcLength - 8) ^ bits;

	auto getBit = [&]() -> uint {
		uint bit = bits & 1;
		bits >>= 1;
		if (bits == 0 && srcPos >= 4) {
			srcPos -= 4;
			const uint32 word = READ_BE_UINT32(src + srcPos);
			checksum ^= word;
			bit = word & 1;
			bits = 0x80000000 | (word >> 1);
		}
		return bit;
	};
	auto getBits = [&](int count) -> uint {
		uint value = 0;
		while (count-- > 0)
			value = (value << 1) | getBit();
		return value;
	};

	uint dstPos = dstLength;
	auto literal = [&](uint count) {
		while (count-- > 0 && dstPos > 0)
			dst[--dstPos] = getBits(8);
	};
	auto copy = [&](uint count, uint offset) {
		while (count-- > 0 && dstPos > 0) {
			--dstPos;
			dst[dstPos] = (dstPos + offset < dstLength) ? dst[dstPos + offset] : 0;
		}
	};

	while (dstPos > 0) {
		if (getBit()) {
			const uint type = getBits(2);
			if (type == 3) {
				literal(getBits(8) + 9);
			} else if (type < 2) {
				const uint count = type + 3;
				copy(count, getBits(9 + type));
			} else {
				const uint count = getBits(8) + 1;
				copy(count, getBits(12));
			}
		} else if (getBit()) {
			copy(2, getBits(8));
		} else {
			literal(getBits(3) + 1);
		}
	}

	return checksum == 0;
}

bool AmigaDaxFormat::normalize(const Common::Path &file, ContentType type,
		uint8 blockId, Common::Array<uint8> &data) const {
	switch (type) {
	case ContentType::TILE:
		return planarToDos(blockId, true, 0, data);
	case ContentType::PIC: {
		Common::String name = file.baseName();
		name.toLowercase();
		const uint width = name.hasPrefix("head") ? kDosHeadWidth : 0;
		return planarToDos(blockId, false, width, data);
	}
	case ContentType::CTILE:
		return planarToDos(blockId, false, 0, data);
	default:
		// ECL, GEO and WALLDEF are identical to DOS. Animated pictures
		// (EGAPIC, SPRIT) still use the Amiga layout.
		return true;
	}
}

void AmigaDaxFormat::familyFiles(const char *pattern, uint16 dosIndices,
		Common::Array<Common::Path> &files) const {
	// The title screens are in the separate "intro" program.
	if (!strcmp(pattern, "title"))
		return;
	files.push_back(Common::Path(singleFileName(pattern)));
}

const RecordLayout &AmigaDaxFormat::recordLayout() const {
	// 68000 build: big-endian, words and longs at even offsets, C strings.
	static const RecordLayout kAmiga = { true, true, true };
	return kAmiga;
}

void AmigaDaxFormat::addAliases(const Common::Path &file,
		DaxBlockContainer &container) const {
	Common::String name = file.baseName();
	name.toLowercase();

	// CHEAD has no action pictures (DOS ids + 128): LoadIconAnim loads the
	// ready head for both frames.
	if (name.hasPrefix("chead")) {
		for (uint id = 0; id < 128; ++id)
			container.addAlias(id + 128, id);
	}

	// The combat cursor is COMSPR 12 (GameInit: LoadIconAnim COMSPR 12 into
	// slot 25), DOS COMSPR 25; its action frame is 140, DOS 153.
	if (name.hasPrefix("comspr")) {
		container.addAlias(25, 12);
		container.addAlias(153, 140);
	}
}

bool AmigaDaxFormat::planarToDos(uint8 blockId, bool tilePairs,
		uint padWidth, Common::Array<uint8> &data) {
	// Amiga header (BE): height, char width, 2 unknown words, item count,
	// plane size. Planes are stored plane-major: plane p of item i is at
	// (p * itemCount + i) * planeSize.
	if (data.size() < kAmigaPicHeaderSize) {
		warning("AmigaDaxFormat: block %d is too short", blockId);
		return false;
	}

	const uint height = READ_BE_UINT16(&data[0]);
	const uint charWidth = READ_BE_UINT16(&data[2]);
	const uint itemCount = READ_BE_UINT16(&data[8]);
	const uint planeSize = READ_BE_UINT16(&data[10]);
	const uint rowBytes = ((charWidth + 1) / 2) * 2;
	const uint tilesPerItem = tilePairs ? 2 : 1;
	const uint tileWidth = charWidth * 8 / tilesPerItem;
	const uint tileCount = itemCount * tilesPerItem;

	if (planeSize < rowBytes * height || tileWidth == 0 || tileWidth % 8
			|| tileCount > 255
			|| data.size() < kAmigaPicHeaderSize + 4 * itemCount * planeSize) {
		warning("AmigaDaxFormat: block %d has an unexpected layout", blockId);
		return false;
	}

	// A narrower picture is centered on whole cells of a padWidth picture.
	const uint outWidth = MAX(tileWidth, padWidth);
	const uint xPad = ((outWidth - tileWidth) / 2) & ~7u;

	Common::Array<uint8> out(kDosPicHeaderSize + tileCount * outWidth * height / 2, 0);
	WRITE_LE_UINT16(&out[0], height);
	WRITE_LE_UINT16(&out[2], outWidth / 8);
	out[8] = tileCount;

	const uint8 *planes = &data[kAmigaPicHeaderSize];
	uint8 *dst = &out[kDosPicHeaderSize];
	for (uint tile = 0; tile < tileCount; ++tile) {
		const uint item = tile / tilesPerItem;
		const uint xBase = (tile % tilesPerItem) * tileWidth;
		for (uint y = 0; y < height; ++y) {
			for (uint x = 0; x < outWidth; ++x) {
				uint8 color = 0;
				if (x >= xPad && x < xPad + tileWidth) {
					const uint px = xBase + x - xPad;
					const uint bit = 7 - (px & 7);
					for (uint plane = 0; plane < 4; ++plane) {
						const uint8 b = planes[(plane * itemCount + item) * planeSize
							+ y * rowBytes + px / 8];
						color |= ((b >> bit) & 1) << plane;
					}
				}
				if (x & 1)
					*dst++ |= color;
				else
					*dst = color << 4;
			}
		}
	}

	data = out;
	return true;
}

} // namespace Data
} // namespace Goldbox
