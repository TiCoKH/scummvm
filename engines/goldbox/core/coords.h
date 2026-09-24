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

#ifndef GOLDBOX_CORE_COORDS_H
#define GOLDBOX_CORE_COORDS_H

#include "common/scummsys.h"

namespace Goldbox {

/**
 * Coordinate systems used by the engine:
 *
 *   MapPos    (16x16 dungeon grid)     — world/GEO navigation
 *   TilePos   (50x25 combat grid)      — battlefield placement and LOS
 *   ScreenPos (40x25 character grid)   — primary rendering grid; 8x8 px per cell
 *   PixelPos  (320x200 pixel surface)  — final blit coordinates
 *
 * MapPos and TilePos are independent domain types.
 * Both ultimately render through ScreenPos → PixelPos.
 *
 * Conversions:
 *   TilePos   → ScreenPos : col*3, row*3  (3 screen cells per combat tile)
 *   ScreenPos → PixelPos  : col*8, row*8  (8 pixels per screen cell)
 * Note: icon rendering adds a +1 pixel offset after the ScreenPos→PixelPos step.
 */

/**
 * A position on the 50x25 combat battlefield tile grid.
 * col = column (0-49), row = row (0-24).
 */
struct TilePos {
	uint8 col;
	uint8 row;

	TilePos() : col(0), row(0) {}
	TilePos(uint8 c, uint8 r) : col(c), row(r) {}

	bool operator==(const TilePos &o) const { return col == o.col && row == o.row; }
	bool operator!=(const TilePos &o) const { return !(*this == o); }
};

/** Returns true if pos is within the 50x25 combat battlefield bounds. */
inline bool isValidTilePos(TilePos pos) {
	return pos.col < 50 && pos.row < 25;
}

/**
 * A position on the 16x16 GEO dungeon map grid.
 * x = column (0-15), y = row (0-15). Signed to allow delta arithmetic.
 */
struct MapPos {
	int8 x;
	int8 y;

	MapPos() : x(0), y(0) {}
	MapPos(int8 x_, int8 y_) : x(x_), y(y_) {}

	bool operator==(const MapPos &o) const { return x == o.x && y == o.y; }
	bool operator!=(const MapPos &o) const { return !(*this == o); }
};

/**
 * A position on the 40x25 terminal character/tile grid.
 * col = column (0-39), row = row (0-24).
 * Each cell is an 8x8 pixel character or GFX tile.
 */
struct ScreenPos {
	int16 col;
	int16 row;

	ScreenPos() : col(0), row(0) {}
	ScreenPos(int16 c, int16 r) : col(c), row(r) {}

	bool operator==(const ScreenPos &o) const { return col == o.col && row == o.row; }
	bool operator!=(const ScreenPos &o) const { return !(*this == o); }
};

/**
 * Raw pixel coordinates on the 320x200 display surface.
 */
struct PixelPos {
	int16 x;
	int16 y;

	PixelPos() : x(0), y(0) {}
	PixelPos(int16 x_, int16 y_) : x(x_), y(y_) {}

	bool operator==(const PixelPos &o) const { return x == o.x && y == o.y; }
	bool operator!=(const PixelPos &o) const { return !(*this == o); }
};

/** Convert a TilePos (combat grid) to ScreenPos (character grid). */
inline ScreenPos tilePosToScreen(TilePos pos) {
	return ScreenPos((int16)(pos.col * 3), (int16)(pos.row * 3));
}

/** Convert a ScreenPos (character grid) to PixelPos. */
inline PixelPos screenPosToPixel(ScreenPos pos) {
	return PixelPos((int16)(pos.col * 8), (int16)(pos.row * 8));
}

} // namespace Goldbox

#endif // GOLDBOX_CORE_COORDS_H
