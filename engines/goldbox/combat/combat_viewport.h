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

#ifndef GOLDBOX_COMBAT_COMBAT_VIEWPORT_H
#define GOLDBOX_COMBAT_COMBAT_VIEWPORT_H

#include "common/scummsys.h"
#include "common/rect.h"

namespace Goldbox {
namespace Combat {

/**
 * 7x7 tile viewport over the 50x25 combat battlefield.
 *
 * Manages camera position and scrolling. The viewport center
 * is at offset (3,3) within the 7x7 window.
 *
 * Reference: BattleSetup_Analysis.md "Viewport and Rendering"
 */
class CombatViewport {
public:
    static const int VIEW_COLS = 7;
    static const int VIEW_ROWS = 7;
    static const int CENTER_X = 3;
    static const int CENTER_Y = 3;
    static const int MAP_COLS = 50;
    static const int MAP_ROWS = 25;

    CombatViewport();

    /** Center viewport on a tile position (clamped to map bounds). */
    void centerOn(int tileCol, int tileRow);

    /**
     * Scroll viewport to include a target tile.
     * @param targetCol Target tile column
     * @param targetRow Target tile row
     * @param radius    If target is within radius of center, no scroll.
     *                  Use 0xFF to always scroll.
     * @return true if viewport moved
     */
    bool adjustToInclude(int targetCol, int targetRow, uint8 radius = 0xFF);

    /** Top-left corner of viewport in map coordinates. */
    int getTopLeftCol() const { return _topLeftCol; }
    int getTopLeftRow() const { return _topLeftRow; }

    /** Center tile in map coordinates. */
    int getCenterCol() const { return _topLeftCol + CENTER_X; }
    int getCenterRow() const { return _topLeftRow + CENTER_Y; }

    /** Check if a map tile is visible in the viewport. */
    bool isTileVisible(int col, int row) const;

    /** Convert map tile to viewport-local coordinates. Returns false if off-screen. */
    bool mapToLocal(int col, int row, int &localCol, int &localRow) const;

    /** Get the source rect in pixel space for blitting from the full tilemap surface. */
    Common::Rect getSourceRect(int tileSize) const;

private:
    int _topLeftCol;
    int _topLeftRow;

    void clamp();
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_VIEWPORT_H
