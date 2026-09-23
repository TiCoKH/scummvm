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

static const Direction kStepDirection[4][3] = {
    { DIR_NW, DIR_N,    DIR_NE },
    { DIR_W,  DIR_NONE, DIR_E  },
    { DIR_SW, DIR_S,    DIR_SE },
    { DIR_NONE, DIR_S,  DIR_E  }
};

void initBresenham(FieldPath &p) {
    p.moveCost = 0;
    p.deltaCol = (int16)ABS((int)p.endCol - (int)p.startCol);
    p.deltaRow = (int16)ABS((int)p.endRow - (int)p.startRow);

    const int dc = (int)p.endCol - (int)p.startCol;
    const int dr = (int)p.endRow - (int)p.startRow;
    p.stepCol = (int8)(dc > 0 ? 1 : dc < 0 ? -1 : 0);
    p.stepRow = (int8)(dr > 0 ? 1 : dr < 0 ? -1 : 0);

    p.col = p.startCol;
    p.row = p.startRow;
    p.stepDirection = DIR_NONE;

    if (p.deltaRow < p.deltaCol) {
        p.error          = (int16)(p.deltaRow * 2 - p.deltaCol);
        p.errorStep      = (int16)((p.deltaRow - p.deltaCol) * 2);
        p.minorErrorStep = (int16)(p.deltaRow * 2);
    } else {
        p.error          = (int16)(p.deltaCol * 2 - p.deltaRow);
        p.errorStep      = (int16)((p.deltaCol - p.deltaRow) * 2);
        p.minorErrorStep = (int16)(p.deltaCol * 2);
    }
}

bool stepBresenham(FieldPath &p) {
    int8 sx = 0, sy = 0;
    bool stepped = false;

    if (p.deltaRow < p.deltaCol) {
        // X-major
        if (p.col != p.endCol) {
            sx = p.stepCol;
            if (p.error < 0) {
                p.error += p.minorErrorStep;
                p.moveCost += 2;
            } else {
                sy = p.stepRow;
                p.row += p.stepRow;
                p.error += p.errorStep;
                p.moveCost += 3;
            }
            p.col += p.stepCol;
            stepped = true;
        }
    } else {
        // Y-major
        if (p.row != p.endRow) {
            sy = p.stepRow;
            if (p.error < 0) {
                p.error += p.minorErrorStep;
                p.moveCost += 2;
            } else {
                sx = p.stepCol;
                p.col += p.stepCol;
                p.error += p.errorStep;
                p.moveCost += 3;
            }
            p.row += p.stepRow;
            stepped = true;
        }
    }

    p.stepDirection = kStepDirection[sy + 1][sx + 1];
    return stepped;
}

} // namespace Goldbox
