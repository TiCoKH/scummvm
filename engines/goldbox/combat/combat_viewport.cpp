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

void CombatViewport::centerOn(TilePos pos) {
    _topLeftCol = pos.col - CENTER_X;
    _topLeftRow = pos.row - CENTER_Y;
    clamp();
}

bool CombatViewport::adjustToInclude(TilePos target, uint8 radius) {
    int centerCol = _topLeftCol + CENTER_X;
    int centerRow = _topLeftRow + CENTER_Y;

    if (radius != 0xFF) {
        if (ABS((int)target.col - centerCol) <= (int)radius &&
            ABS((int)target.row - centerRow) <= (int)radius)
            return false;
    }

    int oldCol = _topLeftCol;
    int oldRow = _topLeftRow;

    while ((int)target.col < centerCol && centerCol > CENTER_X)
        centerCol--;
    while ((int)target.col > centerCol && centerCol < MAP_COLS - 1 - CENTER_X)
        centerCol++;
    while ((int)target.row < centerRow && centerRow > CENTER_Y)
        centerRow--;
    while ((int)target.row > centerRow && centerRow < MAP_ROWS - 1 - CENTER_Y)
        centerRow++;

    _topLeftCol = centerCol - CENTER_X;
    _topLeftRow = centerRow - CENTER_Y;
    clamp();

    return (_topLeftCol != oldCol || _topLeftRow != oldRow);
}

bool CombatViewport::isTileVisible(TilePos pos) const {
    return (int)pos.col >= _topLeftCol && (int)pos.col < _topLeftCol + VIEW_COLS &&
           (int)pos.row >= _topLeftRow && (int)pos.row < _topLeftRow + VIEW_ROWS;
}

bool CombatViewport::mapToLocal(TilePos pos, int &localCol, int &localRow) const {
    localCol = (int)pos.col - _topLeftCol;
    localRow = (int)pos.row - _topLeftRow;
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
