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

#ifndef GOLDBOX_DATA_FORMAT_AMIGA_DAX_FORMAT_H
#define GOLDBOX_DATA_FORMAT_AMIGA_DAX_FORMAT_H

#include "goldbox/data/format/dax_format.h"

namespace Goldbox {
namespace Data {

/**
 * Amiga DAX files: big-endian directory, ByteKiller-style packing, one file
 * per family, pictures and tiles as 4 bitplanes.
 */
class AmigaDaxFormat : public DaxFormat {
public:
	void parseHeaders(Common::ReadStream &stream,
		DaxHeaderContainer &headers) const override;
	bool unpack(const Common::Array<uint8> &packed,
		Common::Array<uint8> &out) const override;
	bool normalize(ContentType type, uint8 blockId,
		Common::Array<uint8> &data) const override;
	void familyFiles(const char *pattern, uint16 dosIndices,
		Common::Array<Common::Path> &files) const override;

private:
	/**
	 * Rewrite a planar picture or tile block as a DOS block: 17-byte header
	 * and 4-bit packed pixels. Tile sets hold two tiles per Amiga item.
	 */
	static bool planarToDos(uint8 blockId, bool tilePairs,
		Common::Array<uint8> &data);
};

} // namespace Data
} // namespace Goldbox

#endif
