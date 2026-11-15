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

#ifndef GOLDBOX_DATA_FORMAT_PC98_DAX_FORMAT_H
#define GOLDBOX_DATA_FORMAT_PC98_DAX_FORMAT_H

#include "goldbox/data/format/dax_format.h"

namespace Goldbox {
namespace Data {

/**
 * PC-98 DAX files: little-endian directory, one file per family, blocks
 * either stored or packed with a fill-byte run-length scheme.
 *
 * Pictures are redrawn at double resolution in the 8 digital colors. A block
 * starts with a 9-byte header (LE width and height in pixels, then zeros;
 * 8X8D blocks have an all-zero header and 16x16 cells), followed by items of
 * width x height pixels. Each item holds its bitplanes one after another in
 * the order red, green, blue, and for art drawn over a background a 4th
 * plane that is set where the art is transparent.
 */
class Pc98DaxFormat : public DaxFormat {
public:
	void parseHeaders(Common::ReadStream &stream,
		DaxHeaderContainer &headers) const override;
	bool unpack(const Common::Array<uint8> &packed,
		Common::Array<uint8> &out) const override;
	bool normalize(const Common::Path &file, ContentType type,
		uint8 blockId, Common::Array<uint8> &data) const override;
	void familyFiles(const char *pattern, uint16 dosIndices,
		Common::Array<Common::Path> &files) const override;

private:
	/**
	 * Convert planar art to the DOS layout at its own (double) size, with
	 * the colors as EGA indices, so the common decoders read it. Transparent
	 * pixels become color 0 and opaque black color 8.
	 */
	static bool planarToDos(uint8 blockId, bool cells16, uint planeCount,
		Common::Array<uint8> &data);

	/**
	 * Convert an animation (PIC, SPRIT) to the DOS frame layout. PC-98: a
	 * leading byte, then frames of a 9-byte header (LE width, height, x and
	 * y in 8-pixel units, 0) and the planes. PIC frames after the first are
	 * XOR deltas against the first, as on DOS.
	 */
	static bool animToDos(uint8 blockId, uint planeCount, bool xorFrames,
		Common::Array<uint8> &data);
};

} // namespace Data
} // namespace Goldbox

#endif
