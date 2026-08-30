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

#ifndef GOLDBOX_CORE_TILE_POS_H
#define GOLDBOX_CORE_TILE_POS_H

#include "common/scummsys.h"

namespace Goldbox {

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

} // namespace Goldbox

#endif // GOLDBOX_CORE_TILE_POS_H
