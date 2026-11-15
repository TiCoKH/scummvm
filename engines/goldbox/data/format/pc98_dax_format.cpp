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

#include "goldbox/data/format/pc98_dax_format.h"
#include "common/debug.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {
namespace Data {

static const uint8 kPc98PackedRle = 0x05;
static const uint8 kPc98Stored = 0xFF;

static const uint kPc98PicHeaderSize = 9;
static const uint kDosPicHeaderSize = 17;

// The 8X8D symbol font (DOS 8x8 1bpp glyphs) is 16x16 here; it is kept raw.
static const uint8 kFontBlockId = 201;

// Digital color (bit 0 blue, bit 1 red, bit 2 green) to the EGA index the
// engine uses for the same color; the backend palette shows them as PC-98.
static const uint8 kDigitalToEga[8] = { 0, 9, 12, 13, 10, 11, 14, 15 };
static const uint8 kOpaqueBlack = 8;

static const uint kMaxFrames = 8;
static const uint kDosFrameHeaderSize = 21;

/**
 * The EGA color of pixel x, y of a planar item (planes red, green, blue and
 * an optional mask, each width / 8 bytes per row).
 */
static uint8 planarPixel(const uint8 *item, uint width, uint height,
		uint planeCount, uint x, uint y) {
	const uint planeSize = width / 8 * height;
	const uint offset = y * (width / 8) + x / 8;
	const uint bit = 7 - (x & 7);
	if (planeCount > 3 && ((item[3 * planeSize + offset] >> bit) & 1))
		return 0;
	const uint8 color = kDigitalToEga[((item[2 * planeSize + offset] >> bit) & 1)
		| (((item[offset] >> bit) & 1) << 1)
		| (((item[planeSize + offset] >> bit) & 1) << 2)];
	return (planeCount > 3 && color == 0) ? kOpaqueBlack : color;
}

void Pc98DaxFormat::parseHeaders(Common::ReadStream &stream,
		DaxHeaderContainer &headers) const {
	headers.parseHeadersLE(stream);
}

bool Pc98DaxFormat::unpack(const Common::Array<uint8> &packed,
		Common::Array<uint8> &out) const {
	// First byte selects the method. Packed blocks continue with a fill byte
	// and commands: 0x00-0x7F copy that many literals, 10nnnnnn repeats the
	// next byte n + 3 times, 11nnnnnn writes the fill byte n + 2 times.
	const uint inSize = packed.size();
	const uint outSize = out.size();
	if (inSize < 1)
		return false;

	if (packed[0] == kPc98Stored) {
		const uint n = MIN(inSize - 1, outSize);
		memcpy(out.data(), &packed[1], n);
		return n == outSize;
	}
	if (packed[0] != kPc98PackedRle || inSize < 2)
		return false;

	const uint8 fill = packed[1];
	uint in = 2;
	uint pos = 0;
	while (in < inSize && pos < outSize) {
		const uint8 cmd = packed[in++];
		if (cmd < 0x80) {
			const uint n = MIN<uint>(MIN<uint>(cmd, inSize - in), outSize - pos);
			memcpy(&out[pos], &packed[in], n);
			in += cmd;
			pos += n;
		} else if (!(cmd & 0x40)) {
			if (in >= inSize)
				break;
			const uint n = MIN<uint>((cmd & 0x3F) + 3, outSize - pos);
			memset(&out[pos], packed[in++], n);
			pos += n;
		} else {
			const uint n = MIN<uint>((cmd & 0x3F) + 2, outSize - pos);
			memset(&out[pos], fill, n);
			pos += n;
		}
	}
	return pos == outSize;
}

bool Pc98DaxFormat::normalize(const Common::Path &file, ContentType type,
		uint8 blockId, Common::Array<uint8> &data) const {
	Common::String name = file.baseName();
	name.toLowercase();

	switch (type) {
	case ContentType::TILE:
		// 8X8D: 16x16 cells with a mask plane; combat and overland tile
		// sets: opaque 48x48 tiles.
		if (name.hasPrefix("8x8d")) {
			if (blockId == kFontBlockId)
				return true;
			return planarToDos(blockId, true, 4, data);
		}
		return planarToDos(blockId, false, 3, data);
	case ContentType::PIC:
		// Portraits (HEAD, BODY).
		return planarToDos(blockId, false, 3, data);
	case ContentType::CTILE:
		// Icons and combat pictures (CBODY, CHEAD, COMSPR, CPIC).
		return planarToDos(blockId, false, 4, data);
	case ContentType::EGAPIC:
		// PIC: opaque frames, later ones XOR deltas.
		return animToDos(blockId, 3, true, data);
	case ContentType::SPRIT:
		// SPRIT: one masked frame per distance.
		return animToDos(blockId, 4, false, data);
	default:
		// Records, ECL, GEO and WALLDEF match DOS.
		return true;
	}
}

uint8 Pc98DaxFormat::digitalToEga(uint8 color) {
	return kDigitalToEga[color & 7];
}

bool Pc98DaxFormat::planarToDos(uint8 blockId, bool cells16, uint planeCount,
		Common::Array<uint8> &data) {
	if (data.size() < kPc98PicHeaderSize) {
		warning("Pc98DaxFormat: block %d is too short", blockId);
		return false;
	}

	uint width = READ_LE_UINT16(&data[0]);
	uint height = READ_LE_UINT16(&data[2]);
	if (cells16 && width == 0 && height == 0)
		width = height = 16;

	const uint planeSize = width / 8 * height;
	const uint itemSize = planeSize * planeCount;
	const uint dataSize = data.size() - kPc98PicHeaderSize;
	if (width == 0 || width % 8 || height == 0 || dataSize % itemSize
			|| dataSize / itemSize == 0 || dataSize / itemSize > 255) {
		warning("Pc98DaxFormat: block %d has an unexpected layout (%ux%u, %u bytes)",
			blockId, width, height, dataSize);
		return false;
	}
	const uint itemCount = dataSize / itemSize;

	Common::Array<uint8> out(kDosPicHeaderSize + itemCount * width * height / 2, 0);
	WRITE_LE_UINT16(&out[0], height);
	WRITE_LE_UINT16(&out[2], width / 8);
	out[8] = itemCount;

	const uint8 *src = &data[kPc98PicHeaderSize];
	uint8 *dst = &out[kDosPicHeaderSize];
	for (uint item = 0; item < itemCount; ++item) {
		const uint8 *pixels = src + item * itemSize;
		for (uint y = 0; y < height; ++y) {
			for (uint x = 0; x < width; ++x) {
				const uint8 color = planarPixel(pixels, width, height,
					planeCount, x, y);
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

bool Pc98DaxFormat::animToDos(uint8 blockId, uint planeCount, bool xorFrames,
		Common::Array<uint8> &data) {
	struct Frame {
		uint width, height, x, y;
		Common::Array<uint8> pixels; // EGA colors, full picture
	};
	Common::Array<Frame> frames;

	// The leading byte is not the frame count (frames are counted by
	// walking them); it is kept as the frame delay.
	const uint size = data.size();
	const uint32 delay = size ? data[0] : 0;
	const uint8 *firstItem = nullptr;
	uint offset = 1;
	while (offset + kPc98PicHeaderSize <= size && frames.size() < kMaxFrames) {
		Frame frame;
		frame.width = READ_LE_UINT16(&data[offset]);
		frame.height = READ_LE_UINT16(&data[offset + 2]);
		frame.x = READ_LE_UINT16(&data[offset + 4]);
		frame.y = READ_LE_UINT16(&data[offset + 6]);
		const uint itemSize = frame.width / 8 * frame.height * planeCount;
		offset += kPc98PicHeaderSize;
		if (frame.width == 0 || frame.width % 8 || frame.height == 0
				|| offset + itemSize > size) {
			warning("Pc98DaxFormat: animation block %d has an unexpected frame", blockId);
			return false;
		}

		const uint8 *item = &data[offset];
		Common::Array<uint8> planar;
		if (xorFrames && firstItem) {
			// Rebuild the full frame before converting its colors.
			planar.resize(itemSize);
			for (uint i = 0; i < itemSize; ++i)
				planar[i] = item[i] ^ firstItem[i];
			item = planar.data();
		}
		if (!firstItem)
			firstItem = &data[offset];
		if (xorFrames && !frames.empty() && (frame.width != frames[0].width
				|| frame.height != frames[0].height)) {
			warning("Pc98DaxFormat: animation block %d has frames of different sizes", blockId);
			return false;
		}

		frame.pixels.resize(frame.width * frame.height);
		for (uint y = 0; y < frame.height; ++y)
			for (uint x = 0; x < frame.width; ++x)
				frame.pixels[y * frame.width + x] = planarPixel(item,
					frame.width, frame.height, planeCount, x, y);
		frames.push_back(frame);
		offset += itemSize;
	}
	if (frames.empty() || offset != size) {
		warning("Pc98DaxFormat: animation block %d has an unexpected layout", blockId);
		return false;
	}

	// The engine places frames on 16-pixel cells; an odd 8-pixel position
	// becomes a transparent margin.
	if (!xorFrames) {
		for (Frame &frame : frames) {
			const uint padX = (frame.x & 1) * 8;
			const uint padY = (frame.y & 1) * 8;
			if (!padX && !padY)
				continue;
			const uint width = frame.width + padX;
			const uint height = frame.height + padY;
			Common::Array<uint8> pixels(width * height, 0);
			for (uint y = 0; y < frame.height; ++y)
				memcpy(&pixels[(y + padY) * width + padX],
					&frame.pixels[y * frame.width], frame.width);
			frame.pixels = pixels;
			frame.width = width;
			frame.height = height;
			frame.x -= padX / 8;
			frame.y -= padY / 8;
		}
	}

	uint outSize = 1;
	for (const Frame &frame : frames)
		outSize += kDosFrameHeaderSize + frame.width * frame.height / 2;
	Common::Array<uint8> out(outSize, 0);
	out[0] = frames.size();
	uint pos = 1;
	for (uint f = 0; f < frames.size(); ++f) {
		const Frame &frame = frames[f];
		WRITE_LE_UINT32(&out[pos], delay);
		WRITE_LE_UINT16(&out[pos + 4], frame.height);
		WRITE_LE_UINT16(&out[pos + 6], frame.width / 8);
		WRITE_LE_UINT16(&out[pos + 8], frame.x / 2);
		WRITE_LE_UINT16(&out[pos + 10], frame.y / 2);
		pos += kDosFrameHeaderSize;
		for (uint i = 0; i < frame.width * frame.height; ++i) {
			uint8 color = frame.pixels[i];
			if (xorFrames && f > 0)
				color ^= frames[0].pixels[i];
			if (i & 1)
				out[pos++] |= color;
			else
				out[pos] = color << 4;
		}
	}

	data = out;
	return true;
}

void Pc98DaxFormat::familyFiles(const char *pattern, uint16 dosIndices,
		Common::Array<Common::Path> &files) const {
	// The title screens are separate B/R/G plane files (TITLE1B.DAX etc.).
	if (!strcmp(pattern, "title"))
		return;
	files.push_back(Common::Path(singleFileName(pattern)));
}

} // namespace Data
} // namespace Goldbox
