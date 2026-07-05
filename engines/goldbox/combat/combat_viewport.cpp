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

#include "goldbox/combat/combat_viewport.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

CombatViewport::CombatViewport() : _topLeftCol(0), _topLeftRow(0) {
}

void CombatViewport::centerOn(int tileCol, int tileRow) {
    _topLeftCol = tileCol - CENTER_X;
    _topLeftRow = tileRow - CENTER_Y;
    clamp();
}

bool CombatViewport::adjustToInclude(int targetCol, int targetRow, uint8 radius) {
    int centerCol = _topLeftCol + CENTER_X;
    int centerRow = _topLeftRow + CENTER_Y;

    // If within radius, no scroll needed
    if (radius != 0xFF) {
        if (ABS(targetCol - centerCol) <= (int)radius &&
            ABS(targetRow - centerRow) <= (int)radius)
            return false;
    }

    int oldCol = _topLeftCol;
    int oldRow = _topLeftRow;

    // Scroll toward target
    while (targetCol < centerCol && centerCol > CENTER_X) {
        centerCol--;
    }
    while (targetCol > centerCol && centerCol < MAP_COLS - 1 - CENTER_X) {
        centerCol++;
    }
    while (targetRow < centerRow && centerRow > CENTER_Y) {
        centerRow--;
    }
    while (targetRow > centerRow && centerRow < MAP_ROWS - 1 - CENTER_Y) {
        centerRow++;
    }

    _topLeftCol = centerCol - CENTER_X;
    _topLeftRow = centerRow - CENTER_Y;
    clamp();

    return (_topLeftCol != oldCol || _topLeftRow != oldRow);
}

bool CombatViewport::isTileVisible(int col, int row) const {
    return col >= _topLeftCol && col < _topLeftCol + VIEW_COLS &&
           row >= _topLeftRow && row < _topLeftRow + VIEW_ROWS;
}

bool CombatViewport::mapToLocal(int col, int row, int &localCol, int &localRow) const {
    localCol = col - _topLeftCol;
    localRow = row - _topLeftRow;
    return (localCol >= 0 && localCol < VIEW_COLS &&
            localRow >= 0 && localRow < VIEW_ROWS);
}

Common::Rect CombatViewport::getSourceRect(int tileSize) const {
    int x = _topLeftCol * tileSize;
    int y = _topLeftRow * tileSize;
    return Common::Rect(x, y, x + VIEW_COLS * tileSize, y + VIEW_ROWS * tileSize);
}

void CombatViewport::clamp() {
    if (_topLeftCol < 0)
        _topLeftCol = 0;
    if (_topLeftRow < 0)
        _topLeftRow = 0;
    if (_topLeftCol > MAP_COLS - VIEW_COLS)
        _topLeftCol = MAP_COLS - VIEW_COLS;
    if (_topLeftRow > MAP_ROWS - VIEW_ROWS)
        _topLeftRow = MAP_ROWS - VIEW_ROWS;
}

} // namespace Combat
} // namespace Goldbox
