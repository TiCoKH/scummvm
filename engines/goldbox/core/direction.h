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

#ifndef GOLDBOX_CORE_DIRECTION_H
#define GOLDBOX_CORE_DIRECTION_H

#include "common/scummsys.h"

namespace Goldbox {

/**
 * 8-direction movement deltas and helpers.
 *
 * Wire format (legacy ECL/GEO convention):
 *   0=North, 1=NE, 2=East, 3=SE, 4=South, 5=SW, 6=West, 7=NW 8=NoMove
 *
 * Cardinal directions use even indices (0/2/4/6).
 * Diagonal directions use odd indices (1/3/5/7) — used by combat tactical map.
 *
 * Y axis: negative = north (up on screen), positive = south (down).
 */
static const int8 kDirDeltaX[9] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };
static const int8 kDirDeltaY[9] = { -1, -1, 0, 1, 1, 1, 0, -1, 0 };

/** Turn left (counterclockwise) by one 90-degree step (cardinal only). */
inline uint8 dirLeft90(uint8 dir) { return (dir + 6) % 8; }

/** Turn right (clockwise) by one 90-degree step (cardinal only). */
inline uint8 dirRight90(uint8 dir) { return (dir + 2) % 8; }

/** Reverse direction (180 degrees). */
inline uint8 dirReverse(uint8 dir) { return (dir + 4) % 8; }

/** Convert 8-direction wire format to 4-direction index (0=N,1=E,2=S,3=W). */
inline int dir8ToDir4(uint8 dir) { return (dir / 2) % 4; }

} // namespace Goldbox

#endif // GOLDBOX_CORE_DIRECTION_H
