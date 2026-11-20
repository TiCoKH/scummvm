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

#ifndef WIZARDRY_RESOURCES_CELL_IMAGE_H
#define WIZARDRY_RESOURCES_CELL_IMAGE_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Wizardry {

/**
 * An image measured in screen tiles. Every release uses a screen of 40 x 25
 * tiles; what differs is the size of a tile in pixels (8x8 on CGA 320x200,
 * 16x8 on the 640x200 screens, 16x16 on the PC-98 at 640x400). Glyphs,
 * pictures and the title are all whole tiles, so code that places them works
 * in tiles and only the final copy to the screen works in pixels.
 *
 * Pixels are colour numbers, one byte each, row after row.
 */
struct TileImage {
	uint16 cols = 0, rows = 0;
	uint8 tileW = 0, tileH = 0;
	Common::Array<byte> pixels;

	void create(uint16 cols_, uint16 rows_, uint8 tileW_, uint8 tileH_);

	uint width() const { return cols * tileW; }
	uint height() const { return rows * tileH; }
	bool empty() const { return pixels.empty(); }

	/** True if every pixel of a tile is 0 */
	bool isTileBlank(uint col, uint row) const {
		for (uint y = 0; y < tileH; ++y) {
			const byte *p = getBasePtr(col * tileW, row * tileH + y);
			for (uint x = 0; x < tileW; ++x) {
				if (p[x])
					return false;
			}
		}
		return true;
	}

	byte *getBasePtr(uint x, uint y) { return &pixels[y * width() + x]; }
	const byte *getBasePtr(uint x, uint y) const { return &pixels[y * width() + x]; }
};

/**
 * Bytes one tile takes in a file: two bytes per row and plane
 */
inline uint tileDataSize(uint8 tileH, uint8 planes) {
	return 2 * tileH * planes;
}

/**
 * Decodes one tile.
 * @param src      Tile data. Each row is a big-endian word.
 * @param srcSize  Bytes available at src; what is missing reads as zero
 * @param dst      Top left pixel of the tile in the destination
 * @param pitch    Bytes per row in the destination
 * @param tileW    8: the word holds eight pixels of two bits (CGA four colours).
 *                 16: the word holds sixteen pixels of one bit per plane.
 * @param planes   1, or 3 for planes in the order blue, red, green; the
 *                 colour number is then blue + 2 * red + 4 * green
 */
extern void decodeTile(const byte *src, uint srcSize, byte *dst, uint pitch,
	uint8 tileW, uint8 tileH, uint8 planes);

} // End of namespace Wizardry

#endif
