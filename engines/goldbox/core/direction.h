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
 * 8-way movement direction (wire format, used by combat and movement systems).
 * Y axis: negative = north (up on screen), positive = south (down).
 */
enum Direction {
    DIR_N    = 0,
    DIR_NE   = 1,
    DIR_E    = 2,
    DIR_SE   = 3,
    DIR_S    = 4,
    DIR_SW   = 5,
    DIR_W    = 6,
    DIR_NW   = 7,
    DIR_NONE = 8   // stationary / no movement
};

/** Movement delta tables indexed by Direction (0-8). */
static const int8 kDirDeltaX[9] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };
static const int8 kDirDeltaY[9] = { -1, -1, 0, 1, 1, 1, 0, -1, 0 };

/** Turn left (counterclockwise) 90 degrees. */
inline Direction dirLeft90(Direction dir)  { return static_cast<Direction>((dir + 6) % 8); }

/** Turn right (clockwise) 90 degrees. */
inline Direction dirRight90(Direction dir) { return static_cast<Direction>((dir + 2) % 8); }

/** Reverse direction (180 degrees). */
inline Direction dirReverse(Direction dir) { return static_cast<Direction>((dir + 4) % 8); }

/**
 * Convert 8-way Direction to 4-direction index (0=N, 1=E, 2=S, 3=W).
 * DIR_NONE (8) maps to 0.
 */
inline uint8 dir8ToDir4(Direction dir) { return (static_cast<uint8>(dir) >> 1) & 3; }

/**
 * Convert half-direction index (0-3) to isometric 8-way Direction.
 * Table: { DIR_NW, DIR_E, DIR_SE, DIR_W } = { 7, 2, 3, 6 }
 */
inline Direction halfDirToIso(uint8 halfDir) {
    static const Direction kTable[4] = { DIR_NW, DIR_E, DIR_SE, DIR_W };
    return kTable[halfDir & 3];
}

} // namespace Goldbox

#endif // GOLDBOX_CORE_DIRECTION_H
