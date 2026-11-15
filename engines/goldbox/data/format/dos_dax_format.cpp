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

#include "goldbox/data/format/dos_dax_format.h"
#include "common/util.h"
#include "goldbox/data/daxheadercontainer.h"

namespace Goldbox {
namespace Data {

void DosDaxFormat::parseHeaders(Common::ReadStream &stream,
		DaxHeaderContainer &headers) const {
	headers.parseHeadersLE(stream);
}

bool DosDaxFormat::unpack(const Common::Array<uint8> &packed,
		Common::Array<uint8> &out) const {
	// A signed count byte: n >= 0 copies n + 1 literal bytes,
	// n < 0 repeats the next byte -n times.
	const int inSize = packed.size();
	const int outSize = out.size();
	int in = 0;
	int pos = 0;

	while (in + 1 < inSize && pos < outSize) {
		const int count = static_cast<int8>(packed[in]);
		if (count >= 0) {
			int n = MIN(count + 1, outSize - pos);
			n = MIN(n, inSize - in - 1);
			memcpy(&out[pos], &packed[in + 1], n);
			in += count + 2;
			pos += n;
		} else {
			const int n = MIN(-count, outSize - pos);
			memset(&out[pos], packed[in + 1], n);
			in += 2;
			pos += n;
		}
	}
	return pos == outSize;
}

bool DosDaxFormat::normalize(const Common::Path &file, ContentType type,
		uint8 blockId, Common::Array<uint8> &data) const {
	return true;
}

void DosDaxFormat::familyFiles(const char *pattern, uint16 dosIndices,
		Common::Array<Common::Path> &files) const {
	if (!dosIndices) {
		files.push_back(Common::Path(singleFileName(pattern)));
		return;
	}
	for (int i = 1; i < 16; ++i) {
		if (dosIndices & (1 << i))
			files.push_back(Common::Path(Common::String::format(pattern, i) + ".dax"));
	}
}

} // namespace Data
} // namespace Goldbox
