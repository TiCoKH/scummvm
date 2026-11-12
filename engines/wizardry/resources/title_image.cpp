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

#include "wizardry/resources/files.h"
#include "wizardry/resources/title_image.h"

namespace Wizardry {

bool loadTitleImage(const Profile &profile, const GfxSet &gfx, CellImage &image) {
	const uint STRIDE = 80;
	Common::Array<byte> data;

	if (!gfx.title || !readFile(gfx.title, data))
		return false;

	// IBM-PC: 70 of the 80 bytes of a row are shown, starting at byte 4 of
	// the file, for 56 rows. What follows in the file is left-over disk data.
	const bool isIBM = profile.platform == Common::kPlatformDOS;
	const uint start = isIBM ? 4 : 0;
	const uint rowBytes = isIBM ? 70 : STRIDE;
	const uint bitsPerPixel = (gfx.layout == kPixelsCGA2bpp) ? 2 : 1;

	// The 640x400 title of the PC-98 has 112 rows; all others have 56
	uint height = (data.size() > 5120) ? 112 : 56;
	height = MIN<uint>(height, data.size() / STRIDE);

	const uint width = rowBytes * 8 / bitsPerPixel;
	image.create(width / gfx.cellW, (height + gfx.cellH - 1) / gfx.cellH, gfx.cellW, gfx.cellH);

	for (uint y = 0; y < height; ++y) {
		byte *dst = image.getBasePtr(0, y);

		for (uint i = 0; i < rowBytes; ++i) {
			const uint offset = start + y * STRIDE + i;
			const byte b = (offset < data.size()) ? data[offset] : 0;

			if (bitsPerPixel == 2) {
				for (uint k = 0; k < 4; ++k)
					*dst++ = (b >> (6 - 2 * k)) & 3;
			} else {
				for (uint k = 0; k < 8; ++k)
					*dst++ = (b >> (7 - k)) & 1;
			}
		}
	}

	return true;
}

} // End of namespace Wizardry
