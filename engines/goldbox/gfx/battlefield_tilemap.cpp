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

#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/runtime/runtime_geo.h"
#include "goldbox/engine.h"
#include <string.h>

namespace Goldbox {
namespace Gfx {

// Wire-format direction constants from direction.h:
//   0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
// GEO API wire directions (cardinal only): 0=N, 2=E, 4=S, 6=W

BattlefieldTilemap::BattlefieldTilemap()
    : _geo(nullptr), _playerY(0), _built(false), _isDungeon(true),
      _eclScriptId(0),
      _cellAbsX(0), _cellAbsY(0), _cellOffsetX(0), _cellOffsetY(0),
      _cellPassWest(0), _cellPassNorth(0), _cellPassEast(0) {
    memset(_header, 0, sizeof(_header));
    memset(_tileBuffer, 0, sizeof(_tileBuffer));
    _surface.create(kSurfaceWidth, kSurfaceHeight);
    _surface.clear(0);
}

BattlefieldTilemap::~BattlefieldTilemap() {
    _surface.free();
}

void BattlefieldTilemap::clear() {
    memset(_header, 0, sizeof(_header));
    memset(_tileBuffer, 0, sizeof(_tileBuffer));
    _surface.clear(0);
    _built = false;
}

uint8 BattlefieldTilemap::getRawTile(int col, int row) const {
    if (col < 0 || col >= kPlayfieldCols ||
        row < 0 || row >= kPlayfieldRows)
        return 0;
    return _tileBuffer[row][col];
}

uint8 BattlefieldTilemap::getTileId(int col, int row) const {
    uint8 raw = getRawTile(col, row);
    return (raw == 0) ? 0xFF : static_cast<uint8>(raw - 1);
}

Common::Rect BattlefieldTilemap::tileToPixelRect(int col, int row, int w, int h) const {
    return Common::Rect(col * kIconSize, row * kIconSize,
                        (col + w) * kIconSize, (row + h) * kIconSize);
}

Common::Rect BattlefieldTilemap::getActiveAreaRect() const {
    return tileToPixelRect(kColMargin, kRowMargin,
                           kPlayfieldCols - kColMargin,
                           kPlayfieldRows - kRowMargin);
}

void BattlefieldTilemap::writeTile(int localCol, int localRow, uint8 tileId) {
    // Original: cVar3 = map_x + C_BF_OFFSET_Y * 5 + C_BF_OFFSET_X * 6 + 21
    //           cVar1 = map_y + C_BF_OFFSET_Y * 5 + 10
    int absCol = localCol + (_cellOffsetY * kCellTileRows) + (_cellOffsetX * kCellTileCols) + kColMargin;
    int absRow = localRow + (_cellOffsetY * kCellTileRows) + kRowMargin;

    if (absCol < 0 || absCol >= kPlayfieldCols)
        return;
    if (absRow < 0 || absRow >= kPlayfieldRows)
        return;

    _tileBuffer[absRow][absCol] = tileId + 1;
}

// --- Geometry queries (wireDir: 0=N, 2=E, 4=S, 6=W) ---

uint8 BattlefieldTilemap::checkCell(int8 mapX, int8 mapY, uint8 wireDir) const {
    if (mapX < kMapBoundMin || mapX > kMapBoundMax ||
        mapY < kMapBoundMin || mapY > kMapBoundMax) {
        // OOB special: cells on player's Y row checked N/S are open
        if (mapY == _playerY && (wireDir == 0 || wireDir == 4))
            return kCellOpen;
        return kCellBlocked;
    }

    if (_geo->getWallFlag(mapX, mapY, wireDir) == 0)
        return kCellBlocked;

    if (_geo->getMapNibble(mapX, mapY, wireDir) == 0)
        return kCellOpen;

    return kCellWall;
}

uint8 BattlefieldTilemap::checkCellAndOpposite(int8 mapX, int8 mapY, uint8 wireDir) const {
    uint8 oppositeWire = (wireDir + 4) % 8;

    uint8 thisCell = checkCell(mapX, mapY, wireDir);

    int8 nextX = mapX + kDirDeltaX[wireDir];
    int8 nextY = mapY + kDirDeltaY[wireDir];
    uint8 nextCell = checkCell(nextX, nextY, oppositeWire);

    return thisCell | nextCell;
}

// --- Pattern functions ---

void BattlefieldTilemap::setTilePatternWestSide() {
    for (int row = 2; row <= 4; row++)
        for (int col = 0; col <= 5; col++)
            writeTile(col, row, kTileFloor);

    if (_cellPassWest == kCellBlocked) {
        for (int row = 2; row <= 4; row++) {
            writeTile(row - 1, row, 4);
            writeTile(row, row, 3);
            writeTile(row + 1, row, 13);
        }
    } else if (_cellPassWest == kCellWall) {
        writeTile(1, 2, 8);
        writeTile(5, 4, 0);
    }
}

void BattlefieldTilemap::setTilePatternNorthSide() {
    if (_cellPassNorth == kCellBlocked) {
        writeTile(3, 0, 5);
        writeTile(4, 0, 5);
        writeTile(3, 1, 10);
        writeTile(4, 1, 10);
    } else {
        writeTile(3, 0, kTileFloor);
        writeTile(4, 0, kTileFloor);
        writeTile(3, 1, kTileFloor);
        writeTile(4, 1, kTileFloor);
    }
}

void BattlefieldTilemap::setTilePatternNWCorner() {
    // Original: COMBAT_GetBidirectionalPassability(C_BF_CELL_X, C_BF_CELL_Y - 1, 6) = West check on cell above
    uint8 aboveWest = checkCellAndOpposite(_cellAbsX, _cellAbsY - 1, 6);
    // Original: COMBAT_GetBidirectionalPassability(C_BF_CELL_X - 1, C_BF_CELL_Y, 0) = North check on cell left
    uint8 leftNorth = checkCellAndOpposite(_cellAbsX - 1, _cellAbsY, 0);
    bool isCornerOpen = (aboveWest == kCellOpen) && (leftNorth == kCellOpen);

    uint8 tileNW, tileNE, tileSW, tileSE;

    // Tile 1: position (1,0) - unaff_D6b
    if (_cellPassNorth == kCellOpen) {
        if (_cellPassWest == kCellOpen)
            tileNW = kTileFloor;
        else if (_cellPassWest == kCellWall)
            tileNW = 13;
        else
            tileNW = isCornerOpen ? 0 : 13;
    } else {
        if (_cellPassWest == kCellOpen)
            tileNW = isCornerOpen ? 15 : 5;
        else
            tileNW = isCornerOpen ? 18 : 2;
    }

    // Tile 2: position (2,0) - unaff_D4b
    if (_cellPassNorth == kCellOpen)
        tileNE = kTileFloor;
    else if (_cellPassNorth == kCellWall)
        tileNE = 17;
    else
        tileNE = 5;

    // Tile 3: position (1,1) - unaff_D5b
    if (_cellPassWest == kCellOpen) {
        if (_cellPassNorth == kCellOpen)
            tileSW = kTileFloor;
        else
            tileSW = isCornerOpen ? 16 : 10;
    } else if (_cellPassWest == kCellWall) {
        tileSW = isCornerOpen ? 20 : 7;
    } else {
        tileSW = isCornerOpen ? 1 : 3;
    }

    // Tile 4: position (2,1) - local_9
    if (_cellPassWest == kCellOpen || _cellPassWest == kCellWall) {
        if (_cellPassNorth == kCellOpen)
            tileSE = kTileFloor;
        else if (_cellPassNorth == kCellWall)
            tileSE = 23;
        else
            tileSE = 10;
    } else {
        if (_cellPassNorth == kCellOpen)
            tileSE = 13;
        else if (_cellPassNorth == kCellWall)
            tileSE = 21;
        else
            tileSE = 6;
    }

    writeTile(1, 0, tileNW);
    writeTile(2, 0, tileNE);
    writeTile(1, 1, tileSW);
    writeTile(2, 1, tileSE);
}

void BattlefieldTilemap::setTilePatternNECorner() {
    // Original: local_b = COMBAT_GetBidirectionalPassability(C_BF_CELL_X, C_BF_CELL_Y - 1, 2)
    uint8 aboveEast = checkCellAndOpposite(_cellAbsX, _cellAbsY - 1, 2);
    // Original: local_c = COMBAT_GetBidirectionalPassability(C_BF_CELL_X + 1, C_BF_CELL_Y, 0)
    uint8 rightNorth = checkCellAndOpposite(_cellAbsX + 1, _cellAbsY, 0);
    bool isCornerOpen = (aboveEast == kCellOpen) && (rightNorth == kCellOpen);

    uint8 tileNW, tileNE, tileSW, tileSE;

    // Tile 1: position (5,0) - unaff_D6b
    if (_cellPassNorth == kCellOpen) {
        if (aboveEast == kCellBlocked)
            tileNW = 4;
        else
            tileNW = kTileFloor;
    } else if (_cellPassNorth == kCellWall) {
        tileNW = 15;
    } else {
        tileNW = 5;
    }

    // Tile 2: position (6,0) - unaff_D4b
    if (_cellPassNorth == kCellOpen) {
        if (aboveEast == kCellOpen) {
            tileNE = kTileFloor;
        } else if (aboveEast == kCellWall) {
            tileNE = (_cellPassEast == kCellOpen && rightNorth != kCellOpen) ? 24 : 1;
        } else {
            tileNE = (_cellPassEast == kCellOpen && rightNorth != kCellOpen) ? 11 : 3;
        }
    } else {
        if (_cellPassEast == kCellOpen) {
            if (rightNorth == kCellOpen)
                tileNE = isCornerOpen ? 17 : 19;
            else
                tileNE = 5;
        } else {
            tileNE = 9;
        }
    }

    // Tile 3: position (5,1) - unaff_D5b
    if (_cellPassNorth == kCellOpen)
        tileSW = kTileFloor;
    else if (_cellPassNorth == kCellWall)
        tileSW = 16;
    else
        tileSW = 10;

    // Tile 4: position (6,1) - local_9
    if (_cellPassNorth == kCellOpen) {
        if (aboveEast == kCellOpen)
            tileSE = kTileFloor;
        else if (_cellPassEast == kCellOpen)
            tileSE = (rightNorth == kCellOpen) ? 8 : 12;
        else
            tileSE = 4;
    } else {
        if (_cellPassEast == kCellOpen)
            tileSE = (rightNorth == kCellOpen) ? 23 : 10;
        else
            tileSE = 14;
    }

    writeTile(5, 0, tileNW);
    writeTile(6, 0, tileNE);
    writeTile(5, 1, tileSW);
    writeTile(6, 1, tileSE);
}

// --- Generation ---

void BattlefieldTilemap::generateDungeon(int8 centerX, int8 centerY) {
    // Original loops: C_BF_OFFSET_Y = -2..+2, C_BF_OFFSET_X = -6..+6
    for (_cellOffsetY = -2; _cellOffsetY < 3; ++_cellOffsetY) {
        for (_cellOffsetX = -6; _cellOffsetX < 7; ++_cellOffsetX) {
            _cellAbsX = _cellOffsetX + centerX;
            _cellAbsY = _cellOffsetY + centerY;

            // Wire directions: 6=W, 0=N, 2=E
            _cellPassWest  = checkCellAndOpposite(_cellAbsX, _cellAbsY, 6);
            _cellPassNorth = checkCellAndOpposite(_cellAbsX, _cellAbsY, 0);
            _cellPassEast  = checkCellAndOpposite(_cellAbsX, _cellAbsY, 2);

            setTilePatternWestSide();
            setTilePatternNorthSide();
            setTilePatternNWCorner();
            setTilePatternNECorner();
        }
    }
}

void BattlefieldTilemap::generateWilderness(int8 centerX, int8 centerY) {
    (void)centerX;
    (void)centerY;
    // TODO: Implement COMBAT_GenerateWildernessBattlefield
    for (int row = kRowMargin; row < kPlayfieldRows; row++)
        for (int col = kColMargin; col < kPlayfieldCols; col++)
            _tileBuffer[row][col] = kTileFloor + 1;
}

void BattlefieldTilemap::setRandomFloorTiles() {
    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            if (_tileBuffer[row][col] != kTileFloor + 1)
                continue;

            int roll = g_engine->rollDice(1, 100);

            if (roll == 98) {
                _tileBuffer[row][col] = kTileLightVegetation + 1;
            } else if (roll == 99) {
                _tileBuffer[row][col] = kTileDenseShrub + 1;
            } else if (roll == 100) {
                if (_eclScriptId == 10)
                    continue;
                if (g_engine->rollDice(1, 100) != 1)
                    continue;
                int d10 = g_engine->rollDice(1, 10);
                if (d10 <= 6)
                    _tileBuffer[row][col] = kTileSmallRocks + 1;
                else
                    _tileBuffer[row][col] = kTileLargeBoulders + 1;
            }
        }
    }
}

// --- Build ---

void BattlefieldTilemap::build(const RuntimeGeoBlock &geo,
                                int8 centerX, int8 centerY, int8 playerY,
                                bool isDungeon, uint8 eclScriptId) {
    _geo = &geo;
    _playerY = playerY;
    _isDungeon = isDungeon;
    _eclScriptId = eclScriptId;

    memset(_header, 0, sizeof(_header));
    memset(_tileBuffer, 0, sizeof(_tileBuffer));

    _header[4] = 0;
    _header[5] = 1;
    _header[6] = 0;

    if (_isDungeon) {
        generateDungeon(centerX, centerY);
    } else {
        generateWilderness(centerX, centerY);
    }

    setRandomFloorTiles();
    _built = true;
}

void BattlefieldTilemap::regenerate(const RuntimeGeoBlock &geo,
                                     int8 centerX, int8 centerY, int8 playerY,
                                     uint8 eclScriptId) {
    _geo = &geo;
    _playerY = playerY;
    _eclScriptId = eclScriptId;
    memset(_tileBuffer, 0, sizeof(_tileBuffer));

    if (_isDungeon) {
        generateDungeon(centerX, centerY);
    } else {
        generateWilderness(centerX, centerY);
    }

    setRandomFloorTiles();
}

// --- Rendering ---

void BattlefieldTilemap::render(const IconManager &iconMgr) {
    _surface.clear(0);

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            uint8 raw = _tileBuffer[row][col];
            if (raw == 0)
                continue;

            uint8 slotId = raw - 1;
            const Pic *pic = iconMgr.getPic(slotId);
            if (!pic)
                continue;

            int pixX = col * kIconSize;
            int pixY = row * kIconSize;
            pic->draw(&_surface, pixX, pixY);
        }
    }
}

// --- Blit ---

void BattlefieldTilemap::blitTo(Graphics::ManagedSurface *dst,
                                 const Common::Point &dstPos,
                                 const Common::Rect &srcRect) const {
    if (!dst)
        return;
    dst->blitFrom(_surface, srcRect, dstPos);
}

void BattlefieldTilemap::blitTo(Graphics::ManagedSurface *dst,
                                 const Common::Point &dstPos) const {
    if (!dst)
        return;
    dst->blitFrom(_surface, dstPos);
}

} // namespace Gfx
} // namespace Goldbox
