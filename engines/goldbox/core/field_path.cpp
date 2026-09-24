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

#include "goldbox/core/field_path.h"
#include "common/util.h"

namespace Goldbox {

// Maps (sy+1, sx+1) → compass Direction of the step taken.
// Mirrors the original kStepDirection[4][3] table in COMBAT_stepBresenham.
// Row index = stepY+1 (0=north, 1=none, 2=south; row 3 reserved for future use)
// Col index = stepX+1 (0=west,  1=none, 2=east)
static const Direction kStepDirection[4][3] = {
    { DIR_NW, DIR_N,    DIR_NE },
    { DIR_W,  DIR_NONE, DIR_E  },
    { DIR_SW, DIR_S,    DIR_SE },
    { DIR_NONE, DIR_S,  DIR_E  }
};

void initBresenham(FieldPath &p) {
    p.moveCost = 0;
    p.deltaX = (int16)ABS((int)p.endCol - (int)p.start.col);
    p.deltaY = (int16)ABS((int)p.endRow - (int)p.start.row);

    const int dc = (int)p.endCol - (int)p.start.col;
    const int dr = (int)p.endRow - (int)p.start.row;
    p.stepX = (int8)(dc > 0 ? 1 : dc < 0 ? -1 : 0);
    p.stepY = (int8)(dr > 0 ? 1 : dr < 0 ? -1 : 0);

    p.current = p.start;
    p.stepDirection = DIR_NONE;

    if (p.deltaY < p.deltaX) {
        p.error          = (int16)(p.deltaY * 2 - p.deltaX);
        p.errorStep      = (int16)((p.deltaY - p.deltaX) * 2);
        p.minorErrorStep = (int16)(p.deltaY * 2);
    } else {
        p.error          = (int16)(p.deltaX * 2 - p.deltaY);
        p.errorStep      = (int16)((p.deltaX - p.deltaY) * 2);
        p.minorErrorStep = (int16)(p.deltaX * 2);
    }
}

bool stepBresenham(FieldPath &p) {
    int8 sx = 0, sy = 0;

    if (p.deltaY < p.deltaX) {
        // X-major
        if ((int16)p.current.col == p.endCol) {
            p.stepDirection = DIR_NONE;
            return false;
        }
        sx = p.stepX;
        if (p.error < 0) {
            p.error += p.minorErrorStep;
            p.moveCost += 2;
        } else {
            sy = p.stepY;
            p.current.row = (uint8)((int)p.current.row + p.stepY);
            p.error += p.errorStep;
            p.moveCost += 3;
        }
        p.current.col = (uint8)((int)p.current.col + p.stepX);
    } else {
        // Y-major
        if ((int16)p.current.row == p.endRow) {
            p.stepDirection = DIR_NONE;
            return false;
        }
        sy = p.stepY;
        if (p.error < 0) {
            p.error += p.minorErrorStep;
            p.moveCost += 2;
        } else {
            sx = p.stepX;
            p.current.col = (uint8)((int)p.current.col + p.stepX);
            p.error += p.errorStep;
            p.moveCost += 3;
        }
        p.current.row = (uint8)((int)p.current.row + p.stepY);
    }

    p.stepDirection = kStepDirection[sy + 1][sx + 1];
    return true;
}

} // namespace Goldbox
