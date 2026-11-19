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

#include "wizardry/resources/tile_image.h"

namespace Wizardry {

void TileImage::create(uint16 cols_, uint16 rows_, uint8 tileW_, uint8 tileH_) {
	cols = cols_;
	rows = rows_;
	tileW = tileW_;
	tileH = tileH_;
	pixels.clear();
	pixels.resize(width() * height());
}

void decodeTile(const byte *src, uint srcSize, byte *dst, uint pitch,
		uint8 tileW, uint8 tileH, uint8 planes) {
	for (uint y = 0; y < tileH; ++y, dst += pitch) {
		for (uint x = 0; x < tileW; ++x)
			dst[x] = 0;

		for (uint p = 0; p < planes; ++p) {
			const uint offset = p * 2 * tileH + 2 * y;
			uint bits = 0;
			if (offset < srcSize)
				bits = src[offset] << 8;
			if (offset + 1 < srcSize)
				bits |= src[offset + 1];

			if (tileW == 8) {
				for (uint x = 0; x < 8; ++x)
					dst[x] = (bits >> (14 - 2 * x)) & 3;
			} else {
				for (uint x = 0; x < 16; ++x)
					dst[x] |= ((bits >> (15 - x)) & 1) << p;
			}
		}
	}
}

} // End of namespace Wizardry
