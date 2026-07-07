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

const BattlefieldTilemap::TileProp BattlefieldTilemap::kTilePropTable[] = {
/*  idx   gfxID  pass   pad  block    notes                          */
/* [00] */ { 0x00, -1, 0x00, 0x02 }, // impassable, hard block
/* [01] */ { 0x00, -1, 0x00, 0x02 }, // impassable, hard block
/* [02] */ { 0x01, -1, 0x00, 0x02 }, // impassable, hard block
/* [03] */ { 0x02, -1, 0x00, 0x02 }, // impassable, hard block
/* [04] */ { 0x03, 0x01, 0x00, 0x00 }, // walkable
/* [05] */ { 0x04, -1, 0x00, 0x02 }, // impassable, hard block
/* [06] */ { 0x05, -1, 0x00, 0x02 }, // impassable, hard block
/* [07] */ { 0x06, -1, 0x00, 0x02 }, // impassable, hard block
/* [08] */ { 0x07, 0x01, 0x00, 0x00 }, // walkable
/* [09] */ { 0x08, -1, 0x00, 0x02 }, // impassable, hard block
/* [10] */ { 0x09, 0x01, 0x00, 0x00 }, // walkable
/* [11] */ { 0x0A, -1, 0x00, 0x02 }, // impassable, hard block
/* [12] */ { 0x0B, 0x01, 0x00, 0x00 }, // walkable
/* [13] */ { 0x0C, -1, 0x00, 0x02 }, // impassable, hard block
/* [14] */ { 0x0D, 0x01, 0x00, 0x00 }, // walkable
/* [15] */ { 0x0E, -1, 0x00, 0x02 }, // impassable, hard block
/* [16] */ { 0x0F, 0x01, 0x00, 0x00 }, // walkable
/* [17] */ { 0x10, -1, 0x00, 0x02 }, // impassable, hard block
/* [18] */ { 0x11, -1, 0x00, 0x02 }, // impassable, hard block
/* [19] */ { 0x12, -1, 0x00, 0x02 }, // impassable, hard block
/* [20] */ { 0x13, -1, 0x00, 0x02 }, // impassable, hard block
/* [21] */ { 0x14, -1, 0x00, 0x02 }, // impassable, hard block
/* [22] */ { 0x15, 0x01, 0x00, 0x00 }, // walkable
/* [23] */ { 0x16, 0x01, 0x00, 0x00 }, // walkable  <-- TILE_OPEN_PLAIN (randomizable base)
/* [24] */ { 0x17, -1, 0x00, 0x02 }, // impassable, hard block
/* [25] */ { 0x18, 0x01, 0x00, 0x00 }, // walkable
/* [26] */ { 0x22, 0x01, 0x00, 0x00 }, // walkable  <-- TILE_LIGHT_VEGETATION
/* [27] */ { 0x23, 0x01, 0x00, 0x00 }, // walkable
/* [28] */ { 0x24, 0x01, 0x00, 0x00 }, // walkable
/* [29] */ { 0x25, 0x01, 0x00, 0x00 }, // walkable
/* [30] */ { 0x26, 0x01, 0x00, 0x00 }, // walkable
/* [31] */ { 0x27, -1, 0x00, 0x02 }, // impassable, hard block
/* --- second tileset bank (indices 32..64) --- */
/* [32] */ { 0x00, -1, 0x00, 0x02 }, // impassable, hard block
/* [33] */ { 0x01, -1, 0x00, 0x02 }, // impassable, hard block
/* [34] */ { 0x02, -1, 0x00, 0x02 }, // impassable, hard block
/* [35] */ { 0x03, 0x01, 0x00, 0x00 }, // walkable
/* [36] */ { 0x04, 0x01, 0x00, 0x00 }, // walkable
/* [37] */ { 0x05, 0x01, 0x00, 0x00 }, // walkable
/* [38] */ { 0x06, 0x01, 0x00, 0x00 }, // walkable
/* [39] */ { 0x07, -1, 0x00, 0x00 }, // impassable, no hard block
/* [40] */ { 0x08, -1, 0x00, 0x00 }, // impassable, no hard block
/* [41] */ { 0x09, 0x01, 0x00, 0x00 }, // walkable
/* [42] */ { 0x0A, 0x01, 0x00, 0x00 }, // walkable
/* [43] */ { 0x0B, 0x01, 0x00, 0x00 }, // walkable
/* [44] */ { 0x0C, 0x01, 0x00, 0x00 }, // walkable
/* [45] */ { 0x0D, 0x01, 0x00, 0x00 }, // walkable
/* [46] */ { 0x0E, 0x01, 0x00, 0x00 }, // walkable
/* [47] */ { 0x0F, 0x01, 0x00, 0x00 }, // walkable
/* [48] */ { 0x10, 0x01, 0x00, 0x00 }, // walkable
/* [49] */ { 0x11, -1, 0x00, 0x00 }, // impassable, no hard block
/* [50] */ { 0x12, -1, 0x00, 0x00 }, // impassable, no hard block
/* [51] */ { 0x13, 0x01, 0x00, 0x00 }, // walkable
/* [52] */ { 0x14, 0x01, 0x00, 0x00 }, // walkable
/* [53] */ { 0x15, 0x01, 0x00, 0x00 }, // walkable
/* [54] */ { 0x16, -1, 0x00, 0x00 }, // impassable, no hard block
/* [55] */ { 0x17, -1, 0x00, 0x00 }, // impassable, no hard block
/* [56] */ { 0x18, 0x01, 0x00, 0x00 }, // walkable
/* [57] */ { 0x19, 0x01, 0x00, 0x00 }, // walkable
/* [58] */ { 0x1A, 0x01, 0x00, 0x00 }, // walkable
/* [59] */ { 0x1B, -1, 0x00, 0x00 }, // impassable, no hard block
/* [60] */ { 0x1C, -1, 0x00, 0x02 }, // impassable, hard block <-- TILE_SMALL_ROCKS
/* [61] */ { 0x1D, -1, 0x00, 0x02 }, // impassable, hard block <-- TILE_LARGE_BOULDERS
/* [62] */ { 0x1E, -1, 0x00, 0x02 }, // impassable, hard block
/* [63] */ { 0x1F, 0x01, 0x00, 0x00 }, // walkable
/* [64] */ { 0x20, -1, 0x00, 0x02 }, // impassable, hard block
};

// Wilderness tilemap stub — TODO: populate from game data extraction
const uint8 BattlefieldTilemap::kWildernessTilemap[kWildTilemapRows][kWildTilemapCols] = {
    {0}
};

BattlefieldTilemap::BattlefieldTilemap()
    : _geo(nullptr), _playerY(0), _built(false), _isDungeon(true),
      _centerX(0), _centerY(0),
      _eclScriptId(0), _wildCell(0), _terrainOverrideFlags(0),
      _mapType(1), _wildX(0), _wildY(0),
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

uint8 BattlefieldTilemap::checkOpenPassage(int8 mapX, int8 mapY, uint8 wireDir) const {
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
    uint8 aboveWest = checkOpenPassage(_cellAbsX, _cellAbsY - 1, 6);
    // Original: COMBAT_GetBidirectionalPassability(C_BF_CELL_X - 1, C_BF_CELL_Y, 0) = North check on cell left
    uint8 leftNorth = checkOpenPassage(_cellAbsX - 1, _cellAbsY, 0);
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
    uint8 aboveEast = checkOpenPassage(_cellAbsX, _cellAbsY - 1, 2);
    // Original: local_c = COMBAT_GetBidirectionalPassability(C_BF_CELL_X + 1, C_BF_CELL_Y, 0)
    uint8 rightNorth = checkOpenPassage(_cellAbsX + 1, _cellAbsY, 0);
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
            _cellPassWest  = checkOpenPassage(_cellAbsX, _cellAbsY, 6);
            _cellPassNorth = checkOpenPassage(_cellAbsX, _cellAbsY, 0);
            _cellPassEast  = checkOpenPassage(_cellAbsX, _cellAbsY, 2);

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

    // Fill entire field with open plain (tile 23 in original = index 22+1)
    memset(_tileBuffer, kTileOpenPlain + 1, sizeof(_tileBuffer));

    // Look up wilderness cell from tilemap
    int wx = (int)_wildX;
    if (_mapType == 3)
        wx += 13;
    if (_mapType == 4)
        wx += 26;
    int wy = (int)_wildY * 44;
    if (wx + wy < kWildTilemapRows * kWildTilemapCols)
        _wildCell = kWildernessTilemap[0][wx + wy];
    else
        _wildCell = 0;

    setTilePatternRiver();
    setTilePatternTrees();
    setTilePatternCover();
}

// --- Wilderness terrain flags ---

uint8 BattlefieldTilemap::getTerrainFlags() const {
    uint8 flags;
    int wt = (int)_wildCell;

    if      (wt == 0 || wt == 1 || wt == 2)             flags = 0x01;
    else if (wt >= 0x28 && wt <= 0x29)                   flags = 0x01;
    else if (wt >= 0x4F && wt <= 0x56)                   flags = 0x01;
    else if (wt >= 0x5B && wt <= 0x5C)                   flags = 0x01;
    else if (wt == 0x2B || wt == 0xF1)                   flags = 0x01;
    else if (wt >= 0xA8 && wt <= 0xB1)                   flags = 0x21;
    else if (wt == 0xB9 || (wt >= 0xBC && wt <= 0xC0))   flags = 0x11;
    else if (wt == 3 || wt == 4)                         flags = 0x02;
    else if (wt == 6 || wt == 7)                         flags = 0x02;
    else if (wt == 8 || wt == 9)                         flags = 0x02;
    else if (wt >= 0x13 && wt <= 0x15)                   flags = 0x02;
    else if (wt >= 0x17 && wt <= 0x19)                   flags = 0x02;
    else if (wt == 0x16)                                 flags = 0x02;
    else if (wt >= 0x1A && wt <= 0x24)                   flags = 0x02;
    else if (wt >= 0xC9 && wt <= 0xD0)                   flags = 0x02;
    else if (wt >= 0xD1 && wt <= 0xD5)                   flags = 0x12;
    else if (wt == 0x25 || wt == 0x39 || wt == 0x40 ||
             (wt >= 0x9A && wt <= 0x9E) ||
             (wt >= 0xA4 && wt <= 0xA5) ||
             wt == 0xB4 || wt == 0xF3 ||
             (wt >= 0xEF && wt <= 0xF0) ||
             wt == 0xFA || wt == 0xFC)                   flags = 0x04;
    else if ((wt >= 0x27 && wt <= 0x2C) ||
             wt == 0x99 || wt == 0x9F)                   flags = 0x44;
    else if (wt >= 0x2D && wt <= 0x2E)                   flags = 0x44;
    else if (wt >= 0x33 && wt <= 0x38)                   flags = 0x44;
    else if (wt >= 0x43 && wt <= 0x4D)                   flags = 0x44;
    else if (wt >= 0x5D && wt <= 0x61)                   flags = 0x44;
    else if ((wt >= 0xB5 && wt <= 0xB8) ||
             wt == 0xBA ||
             (wt >= 0xC1 && wt <= 0xC8))                 flags = 0x14;
    else if (wt >= 0x2F && wt <= 0x32)                   flags = 0x40;
    else if (wt >= 0x62 && wt <= 0x65)                   flags = 0x40;
    else if (wt == 0xF2)                                 flags = 0x40;
    else if (wt == 0x26)                                 flags = 0x41;
    else if (wt == 0xB2 || wt == 0xB3)                   flags = 0x60;
    else if (wt >= 0x3A && wt <= 0x3C)                   flags = 0x48;
    else if (wt == 0x41 || wt == 0x42)                   flags = 0x48;
    else if (wt >= 0x3D && wt <= 0x3F)                   flags = 0x08;
    else if (wt >= 0x57 && wt <= 0x5A)                   flags = 0x88;
    else if (wt == 0xA3)                                 flags = 0x88;
    else if (wt == 0xE0)                                 flags = 0x88;
    else if (wt == 0x7C || wt == 0x7D ||
             wt == 0x8A || wt == 0x8C ||
             wt == 0x8E || wt == 0x8F ||
             wt == 0x94 || wt == 0x96)                   flags = 0x09;
    else if (wt >= 0x7E && wt <= 0x85)                   flags = 0x28;
    else if (wt >= 0xE5 && wt <= 0xE7)                   flags = 0x28;
    else if (wt >= 0xD9 && wt <= 0xDF)                   flags = 0x18;
    else if (wt == 0x4E || wt == 0xF5 ||
             (wt >= 0xE8 && wt <= 0xE9))                 flags = 0x20;
    else if (wt == 0x6D ||
             (wt >= 0xF4 && wt <= 0xF6))                 flags = 0x80;
    else if (wt >= 0x6E && wt <= 0x71)                   flags = 0x81;
    else if (wt >= 0x73 && wt <= 0x78)                   flags = 0x81;
    else if (wt >= 0x66 && wt <= 0x6C)                   flags = 0x90;
    else if (wt == 0x72)                                 flags = 0x90;
    else if ((wt >= 0xE1 && wt <= 0xE4) ||
             (wt >= 0xEA && wt <= 0xED))                 flags = 0xA0;
    else                                                 flags = 0x00;

    if ((flags & kTerrainUnderground) && (_terrainOverrideFlags == 0xFF))
        flags = (flags & ~kTerrainUnderground) | kTerrainWater;

    return flags;
}

// --- Wilderness river ---

void BattlefieldTilemap::markFordPair(int col, int row, int &pairCount) {
    if (col >= 0 && col < kPlayfieldCols - 1 &&
        row >= 0 && row < kPlayfieldRows) {
        _tileBuffer[row][col] = kTileFordA + 1;
        _tileBuffer[row][col + 1] = kTileFordB + 1;
        pairCount++;
    }
}

void BattlefieldTilemap::setTilePatternRiver() {
    uint8 terrainFlags = getTerrainFlags();
    uint8 chanceThreshold = 0;

    if (terrainFlags & kTerrainRuins)
        chanceThreshold = 35;
    if (terrainFlags & kTerrainHills)
        chanceThreshold = 75;

    int diceRoll = g_engine->rollDice(1, 100);
    if (diceRoll > (int)chanceThreshold)
        return;

    // Starting column: roll 5d4 (5..20), streamCol = 34 - result
    int startRoll = g_engine->rollDice(5, 4);
    int streamCol = 34 - startRoll;

    // Align to 7-column grid
    while (((streamCol + 2) % 7) != 0)
        streamCol--;

    int streamColStart = streamCol;
    int pairCount = 0;

    // Paint stream diagonally across field
    for (int row = 0; row < kPlayfieldRows; row++) {
        if (streamCol >= 0 && streamCol < kPlayfieldCols - 1) {
            _tileBuffer[row][streamCol] = kTileStreamA + 1;
            _tileBuffer[row][streamCol + 1] = kTileStreamB + 1;

            if (g_engine->rollDice(1, 20) == 1) {
                markFordPair(streamCol, row, pairCount);
            } else if (pairCount / 2 == 0) {
                markFordPair(streamCol, row, pairCount);
            }
        }
        streamCol++;
    }

    // Fallback: force two ford pairs if none placed
    if (pairCount == 0) {
        int fordsCol = 16 - g_engine->rollDice(1, 9);
        int fordsRow = fordsCol + streamColStart;
        if (fordsRow >= 0 && fordsRow < kPlayfieldRows - 1) {
            markFordPair(fordsCol, fordsRow, pairCount);
            markFordPair(fordsCol + 1, fordsRow + 1, pairCount);
        }
    }
}

// --- Wilderness trees ---

void BattlefieldTilemap::setTilePatternTrees() {
    uint8 terrainFlags = getTerrainFlags();
    uint8 maxDensity = 1;

    if (terrainFlags & kTerrainForest)
        maxDensity = 3;
    if (terrainFlags & kTerrainDesert)
        maxDensity = 4;
    if (terrainFlags & kTerrainMarsh)
        maxDensity = 7;
    if (terrainFlags & kTerrainUnderground)
        maxDensity = 0;

    for (int col = 0; col < kPlayfieldCols; col++) {
        for (int row = 1; row < kPlayfieldRows; row++) {
            uint8 rawCur = _tileBuffer[row][col];
            uint8 rawAbove = _tileBuffer[row - 1][col];
            if (rawCur == 0 || rawAbove == 0)
                continue;

            uint8 tidCur = rawCur - 1;
            uint8 tidAbove = rawAbove - 1;
            if (tidCur >= kTilePropTableCount || tidAbove >= kTilePropTableCount)
                continue;
            if (kTilePropTable[tidCur].gfxID != kTileOpenPlain)
                continue;
            if (kTilePropTable[tidAbove].gfxID != kTileOpenPlain)
                continue;

            int diceRoll = g_engine->rollDice(1, 100);
            if (diceRoll > (int)maxDensity)
                continue;

            // Select tree variant
            uint8 treeVariant;
            int varRoll = g_engine->rollDice(1, 10);
            if (varRoll < 9)
                treeVariant = (uint8)((varRoll + 1) >> 1);  // 1..4
            else
                treeVariant = g_engine->rollDice(1, 2) + 4; // 5 or 6

            // Desert override
            if (getTerrainFlags() & kTerrainDesert)
                treeVariant = g_engine->rollDice(1, 3) + 3; // 4..6

            // Place bottom tile
            _tileBuffer[row][col] = (treeVariant + kTileTreeBotBase) + 1;

            // Place top tile for variants 1..4
            if (treeVariant < 5)
                _tileBuffer[row - 1][col] = (treeVariant + kTileTreeTopBase) + 1;
        }
    }
}

// --- Wilderness ground cover ---

void BattlefieldTilemap::setTilePatternCover() {
    uint8 terrainFlags = getTerrainFlags();

    uint8 bandBase = 0;
    uint8 bandWidth1 = 15;
    uint8 bandWidth2 = 40;
    uint8 bandWidth3 = 0;

    if (terrainFlags & kTerrainRiver) {
        bandWidth1 = 45;
        bandWidth2 = 10;
    }
    if (terrainFlags & kTerrainForest)
        bandWidth1 = 25;
    if (terrainFlags & kTerrainDesert) {
        bandWidth2 = 5;
        bandWidth3 = 30;
    }
    if (terrainFlags & kTerrainUnderground) {
        bandBase = 15;
        bandWidth2 = 25;
        bandWidth1 = 10;
    }

    int threshA = (int)bandBase;
    int threshB = threshA + 6;
    int threshC = threshB + (int)bandWidth1;
    int threshD = threshC + (int)bandWidth2;
    int threshE = threshD + (int)bandWidth3;

    for (int col = 0; col < kPlayfieldCols; col++) {
        for (int row = 0; row < kPlayfieldRows; row++) {
            uint8 raw = _tileBuffer[row][col];
            if (raw == 0)
                continue;
            uint8 tileId = raw - 1;
            if (tileId >= kTilePropTableCount)
                continue;
            if (kTilePropTable[tileId].gfxID != kTileOpenPlain)
                continue;

            int roll = (int)(uint8)g_engine->rollDice(1, 255);

            if (roll <= threshA) {
                // Zone A: wall pairs / zone-A cover
                int featureRoll = g_engine->rollDice(1, 4);
                if (featureRoll == 4 && row > 0) {
                    uint8 aboveRaw = _tileBuffer[row - 1][col];
                    if (aboveRaw != 0) {
                        uint8 aboveTid = aboveRaw - 1;
                        if (aboveTid < kTilePropTableCount &&
                            kTilePropTable[aboveTid].gfxID == kTileOpenPlain) {
                            _tileBuffer[row - 1][col] = kTileWallTop + 1;
                            _tileBuffer[row][col] = kTileWallBottom + 1;
                        }
                    }
                } else if (featureRoll < 4) {
                    _tileBuffer[row][col] = (uint8)(featureRoll + kTileZoneABase) + 1;
                }
            } else if (roll <= threshB) {
                // Zone B: scrub
                int featureRoll = g_engine->rollDice(1, 3);
                _tileBuffer[row][col] = (uint8)(featureRoll + kTileScrubBase - 1) + 1;
            } else if (roll <= threshC) {
                // Zone C: ground cover
                int featureRoll = g_engine->rollDice(1, 4);
                _tileBuffer[row][col] = (uint8)(featureRoll + kTileCoverBase - 1) + 1;
            } else if (roll <= threshD) {
                // Zone D: crop rows
                int d10 = g_engine->rollDice(1, 10);
                int variant = ((d10 - 1) / 3) + 1;
                _tileBuffer[row][col] = (uint8)(variant + kTileCropBase - 1) + 1;
            } else if (roll <= threshE) {
                // Zone E: cliff/special
                int featureRoll = g_engine->rollDice(1, 4);
                if (featureRoll == 4) {
                    if (!(getTerrainFlags() & kTerrainDesert))
                        featureRoll = g_engine->rollDice(1, 3);
                }
                _tileBuffer[row][col] = (uint8)(featureRoll + kTileCliffBase - 1) + 1;
            }
        }
    }
}

void BattlefieldTilemap::setRandomFloorTiles() {
    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            uint8 raw = _tileBuffer[row][col];
            if (raw == 0)
                continue;
            uint8 tileId = raw - 1;
            if (tileId >= kTilePropTableCount)
                continue;
            if (kTilePropTable[tileId].gfxID != kTileFloor)
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
                                bool isDungeon, uint8 eclScriptId,
                                uint8 wildX, uint8 wildY,
                                uint8 mapType, uint8 terrainOverride) {
    _geo = &geo;
    _playerY = playerY;
    _isDungeon = isDungeon;
    _eclScriptId = eclScriptId;
    _wildX = wildX;
    _wildY = wildY;
    _mapType = mapType;
    _terrainOverrideFlags = terrainOverride;

    memset(_header, 0, sizeof(_header));
    memset(_tileBuffer, 0, sizeof(_tileBuffer));

    _header[4] = 0;
    _header[5] = 1;
    _header[6] = 0;

    _centerX = centerX;
    _centerY = centerY;

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
                                     uint8 eclScriptId,
                                     uint8 wildX, uint8 wildY,
                                     uint8 mapType, uint8 terrainOverride) {
    _geo = &geo;
    _playerY = playerY;
    _eclScriptId = eclScriptId;
    _wildX = wildX;
    _wildY = wildY;
    _mapType = mapType;
    _terrainOverrideFlags = terrainOverride;
    _centerX = centerX;
    _centerY = centerY;
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
