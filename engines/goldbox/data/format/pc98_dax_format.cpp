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
#include "common/util.h"
#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {
namespace Data {

static const uint8 kPc98PackedRle = 0x05;
static const uint8 kPc98Stored = 0xFF;

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

bool Pc98DaxFormat::normalize(ContentType type, uint8 blockId,
		Common::Array<uint8> &data) const {
	switch (type) {
	case ContentType::TILE:
	case ContentType::PIC:
	case ContentType::CTILE:
	case ContentType::EGAPIC:
	case ContentType::SPRIT:
		debug(1, "Pc98DaxFormat: picture block %d is not supported yet", blockId);
		return false;
	default:
		return true;
	}
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
