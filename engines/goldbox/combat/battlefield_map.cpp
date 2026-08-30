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

#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/core/direction.h"
#include "goldbox/engine.h"
#include "goldbox/runtime/runtime_geo.h"

#include <string.h>

namespace Goldbox {
namespace Combat {

const uint8 BattlefieldMap::kWildernessTilemap[kWildTilemapRows][kWildTilemapCols] = {
    {0}
};

BattlefieldMap::BattlefieldMap()
        : _geo(nullptr), _isDungeon(true), _center(),
            _playerY(0), _eclScriptId(0), _wildCell(0), _terrainOverrideFlags(0), _mapType(1), _wild(),
      _tileProps(nullptr),
      _cellAbs(), _cellOffsetX(0), _cellOffsetY(0),
      _cellPassWest(0), _cellPassNorth(0), _cellPassEast(0) {
    resetPlayfieldState();
}

void BattlefieldMap::setTilePropertyProvider(const TilePropertyProvider *provider) {
    _tileProps = provider;
}

const TilePropertyProvider *BattlefieldMap::getTilePropertyProvider() const {
    return _tileProps;
}

void BattlefieldMap::resetPlayfieldState() {
    memset(&_playfield, 0, sizeof(_playfield));
    _playfield.viewportStart = TilePos(21, 10);
    _playfield.size = 1;
    memset(_dirtyBitmap, 0, sizeof(_dirtyBitmap));
    _dirtyCount = 0;
}

void BattlefieldMap::clear() {
    resetPlayfieldState();
    _geo = nullptr;
    _isDungeon = true;
    _center = MapPos();
    _playerY = 0;
    _eclScriptId = 0;
    _wildCell = 0;
    _terrainOverrideFlags = 0;
    _mapType = 1;
    _wild = TilePos();
    _cellAbs = MapPos();
    _cellOffsetX = 0;
    _cellOffsetY = 0;
    _cellPassWest = 0;
    _cellPassNorth = 0;
    _cellPassEast = 0;
}

void BattlefieldMap::build(const RuntimeGeoBlock &geo,
                                    MapPos center, int8 playerY,
                                    bool isDungeon, uint8 eclScriptId,
                                    TilePos wild,
                                    uint8 mapType, uint8 terrainOverride) {
    clear();
    _geo = &geo;
    _isDungeon = isDungeon;
    _center = center;
    _playerY = playerY;
    _eclScriptId = eclScriptId;
    _wild = wild;
    _mapType = mapType;
    _terrainOverrideFlags = terrainOverride;

    if (isDungeon)
        generateDungeon(center);
    else
        generateWilderness();

    setRandomFloorTiles();
}

void BattlefieldMap::regenerate(const RuntimeGeoBlock &geo,
                                         MapPos center, int8 playerY,
                                         bool isDungeon, uint8 eclScriptId,
                                         TilePos wild,
                                         uint8 mapType, uint8 terrainOverride) {
    clear();
    _geo = &geo;
    _isDungeon = isDungeon;
    _center = center;
    _playerY = playerY;
    _eclScriptId = eclScriptId;
    _wild = wild;
    _mapType = mapType;
    _terrainOverrideFlags = terrainOverride;

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            setRawTile(TilePos((uint8)col, (uint8)row), 0);
        }
    }

    if (isDungeon)
        generateDungeon(center);
    else
        generateWilderness();

    setRandomFloorTiles();
}

void BattlefieldMap::writeTile(int localCol, int localRow, uint8 tileId) {
    int absCol = localCol + (_cellOffsetY * kCellTileRows) +
        (_cellOffsetX * kCellTileCols) + _playfield.viewportStart.col;
    int absRow = localRow + (_cellOffsetY * kCellTileRows) +
        _playfield.viewportStart.row;

    if (absCol < 0 || absCol >= kPlayfieldCols)
        return;
    if (absRow < 0 || absRow >= kPlayfieldRows)
        return;

    _playfield.fieldTileMap[absRow][absCol] = tileId + 1;
}

uint8 BattlefieldMap::getRawTile(TilePos pos) const {
    if (pos.col >= kPlayfieldCols || pos.row >= kPlayfieldRows)
        return 0;
    return _playfield.fieldTileMap[pos.row][pos.col];
}

void BattlefieldMap::setRawTile(TilePos pos, uint8 rawTile) {
    if (pos.col >= kPlayfieldCols || pos.row >= kPlayfieldRows)
        return;
    if (_playfield.fieldTileMap[pos.row][pos.col] != rawTile) {
        _playfield.fieldTileMap[pos.row][pos.col] = rawTile;
        markTileDirty(pos);
    }
}

void BattlefieldMap::markTileDirty(TilePos pos) const {
    int bitIdx = pos.row * kPlayfieldCols + pos.col;
    int byteIdx = bitIdx / 8;
    uint8 mask = 1 << (bitIdx & 7);
    if (!(_dirtyBitmap[byteIdx] & mask)) {
        _dirtyBitmap[byteIdx] |= mask;
        _dirtyCount++;
    }
}

bool BattlefieldMap::isTileDirty(TilePos pos) const {
    if (pos.col >= kPlayfieldCols || pos.row >= kPlayfieldRows)
        return false;
    int bitIdx = pos.row * kPlayfieldCols + pos.col;
    return (_dirtyBitmap[bitIdx / 8] & (1 << (bitIdx & 7))) != 0;
}

void BattlefieldMap::acknowledgeDirtyTiles() {
    memset(_dirtyBitmap, 0, sizeof(_dirtyBitmap));
    _dirtyCount = 0;
}

uint8 BattlefieldMap::getTileId(TilePos pos) const {
    uint8 raw = getRawTile(pos);
    return (raw == 0) ? 0xFF : static_cast<uint8>(raw - 1);
}

TilePos BattlefieldMap::getViewportStart() const {
    return _playfield.viewportStart;
}

uint8 BattlefieldMap::getSize() const {
    return _playfield.size;
}

bool BattlefieldMap::getTargetCursor() const {
    return _playfield.targetCursor;
}

bool BattlefieldMap::getIgnoreWalls() const {
    return _playfield.ignoreWalls;
}

bool BattlefieldMap::isDungeon() const {
    return _isDungeon;
}

MapPos BattlefieldMap::getCenter() const {
    return _center;
}

uint8 BattlefieldMap::checkCell(MapPos mapPos, uint8 wireDir) const {
    if (mapPos.x < kMapBoundMin || mapPos.x > kMapBoundMax ||
        mapPos.y < kMapBoundMin || mapPos.y > kMapBoundMax) {
        if (mapPos.y == _playerY && (wireDir == 0 || wireDir == 4))
            return kCellOpen;
        return kCellBlocked;
    }

    if (_geo->getWallFlag(mapPos, wireDir) == 0)
        return kCellBlocked;

    if (_geo->getMapNibble(mapPos, wireDir) == 0)
        return kCellOpen;

    return kCellWall;
}

uint8 BattlefieldMap::checkOpenPassage(MapPos mapPos, uint8 wireDir) const {
    uint8 oppositeWire = (wireDir + 4) % 8;

    uint8 thisCell = checkCell(mapPos, wireDir);
    MapPos next(static_cast<int8>(mapPos.x + kDirDeltaX[wireDir]),
                static_cast<int8>(mapPos.y + kDirDeltaY[wireDir]));
    uint8 nextCell = checkCell(next, oppositeWire);

    return thisCell | nextCell;
}

void BattlefieldMap::setTilePatternWestSide() {
    for (int row = 2; row <= 4; row++) {
        for (int col = 0; col <= 5; col++) {
            writeTile(col, row, kTileFloor);
        }
    }

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

void BattlefieldMap::setTilePatternNorthSide() {
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

void BattlefieldMap::setTilePatternNWCorner() {
    uint8 aboveWest = checkOpenPassage(MapPos(_cellAbs.x, static_cast<int8>(_cellAbs.y - 1)), 6);
    uint8 leftNorth = checkOpenPassage(MapPos(static_cast<int8>(_cellAbs.x - 1), _cellAbs.y), 0);
    bool isCornerOpen = (aboveWest == kCellOpen) && (leftNorth == kCellOpen);

    uint8 tileNW;
    uint8 tileNE;
    uint8 tileSW;
    uint8 tileSE;

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

    if (_cellPassNorth == kCellOpen)
        tileNE = kTileFloor;
    else if (_cellPassNorth == kCellWall)
        tileNE = 17;
    else
        tileNE = 5;

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

void BattlefieldMap::setTilePatternNECorner() {
    uint8 aboveEast = checkOpenPassage(MapPos(_cellAbs.x, static_cast<int8>(_cellAbs.y - 1)), 2);
    uint8 rightNorth = checkOpenPassage(MapPos(static_cast<int8>(_cellAbs.x + 1), _cellAbs.y), 0);
    bool isCornerOpen = (aboveEast == kCellOpen) && (rightNorth == kCellOpen);

    uint8 tileNW;
    uint8 tileNE;
    uint8 tileSW;
    uint8 tileSE;

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

    if (_cellPassNorth == kCellOpen)
        tileSW = kTileFloor;
    else if (_cellPassNorth == kCellWall)
        tileSW = 16;
    else
        tileSW = 10;

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

void BattlefieldMap::generateDungeon(MapPos center) {
    for (_cellOffsetY = -2; _cellOffsetY < 3; ++_cellOffsetY) {
        for (_cellOffsetX = -6; _cellOffsetX < 7; ++_cellOffsetX) {
            _cellAbs.x = static_cast<int8>(_cellOffsetX + center.x);
            _cellAbs.y = static_cast<int8>(_cellOffsetY + center.y);

            _cellPassWest  = checkOpenPassage(_cellAbs, 6);
            _cellPassNorth = checkOpenPassage(_cellAbs, 0);
            _cellPassEast  = checkOpenPassage(_cellAbs, 2);

            setTilePatternWestSide();
            setTilePatternNorthSide();
            setTilePatternNWCorner();
            setTilePatternNECorner();
        }
    }
}

void BattlefieldMap::generateWilderness() {
    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            setRawTile(TilePos((uint8)col, (uint8)row), kTileOpenPlain + 1);
        }
    }

    int wx = static_cast<int>(_wild.col);
    if (_mapType == 3)
        wx += 13;
    if (_mapType == 4)
        wx += 26;
    int wy = static_cast<int>(_wild.row) * 44;

    if (wx + wy < kWildTilemapRows * kWildTilemapCols)
        _wildCell = kWildernessTilemap[0][wx + wy];
    else
        _wildCell = 0;

    setTilePatternRiver();
    setTilePatternTrees();
    setTilePatternCover();
}

uint8 BattlefieldMap::getTerrainFlags() const {
    uint8 flags;
    int wt = static_cast<int>(_wildCell);

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

void BattlefieldMap::markFordPair(int col, int row, int &pairCount) {
    if (col >= 0 && col < kPlayfieldCols - 1 &&
        row >= 0 && row < kPlayfieldRows) {
        setRawTile(TilePos((uint8)col, (uint8)row), kTileFordA + 1);
        setRawTile(TilePos((uint8)(col + 1), (uint8)row), kTileFordB + 1);
        pairCount++;
    }
}

void BattlefieldMap::setTilePatternRiver() {
    uint8 terrainFlags = getTerrainFlags();
    uint8 chanceThreshold = 0;

    if (terrainFlags & kTerrainRuins)
        chanceThreshold = 35;
    if (terrainFlags & kTerrainHills)
        chanceThreshold = 75;

    int diceRoll = g_engine->rollDice(1, 100);
    if (diceRoll > static_cast<int>(chanceThreshold))
        return;

    int startRoll = g_engine->rollDice(5, 4);
    int streamCol = 34 - startRoll;

    while (((streamCol + 2) % 7) != 0)
        streamCol--;

    int streamColStart = streamCol;
    int pairCount = 0;

    for (int row = 0; row < kPlayfieldRows; row++) {
        if (streamCol >= 0 && streamCol < kPlayfieldCols - 1) {
            setRawTile(TilePos((uint8)streamCol, (uint8)row), kTileStreamA + 1);
            setRawTile(TilePos((uint8)(streamCol + 1), (uint8)row), kTileStreamB + 1);

            if (g_engine->rollDice(1, 20) == 1) {
                markFordPair(streamCol, row, pairCount);
            } else if (pairCount / 2 == 0) {
                markFordPair(streamCol, row, pairCount);
            }
        }
        streamCol++;
    }

    if (pairCount == 0) {
        int fordsCol = 16 - g_engine->rollDice(1, 9);
        int fordsRow = fordsCol + streamColStart;
        if (fordsRow >= 0 &&
            fordsRow < kPlayfieldRows - 1) {
            markFordPair(fordsCol, fordsRow, pairCount);
            markFordPair(fordsCol + 1, fordsRow + 1, pairCount);
        }
    }
}

void BattlefieldMap::setTilePatternTrees() {
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

    const TilePropertyProvider *tileProps = _tileProps;

    for (int col = 0; col < kPlayfieldCols; col++) {
        for (int row = 1; row < kPlayfieldRows; row++) {
            uint8 rawCur = getRawTile(TilePos((uint8)col, (uint8)row));
            uint8 rawAbove = getRawTile(TilePos((uint8)col, (uint8)(row - 1)));
            if (rawCur == 0 || rawAbove == 0)
                continue;

            uint8 tidCur = rawCur - 1;
            uint8 tidAbove = rawAbove - 1;
            if (!tileProps || tidCur >= tileProps->getTilePropCount() ||
                tidAbove >= tileProps->getTilePropCount()) {
                continue;
            }
            if (tileProps->getGfxID(tidCur) != kTileOpenPlain)
                continue;
            if (tileProps->getGfxID(tidAbove) != kTileOpenPlain)
                continue;

            int diceRoll = g_engine->rollDice(1, 100);
            if (diceRoll > static_cast<int>(maxDensity))
                continue;

            uint8 treeVariant;
            int varRoll = g_engine->rollDice(1, 10);
            if (varRoll < 9)
                treeVariant = static_cast<uint8>((varRoll + 1) >> 1);
            else
                treeVariant = static_cast<uint8>(g_engine->rollDice(1, 2) + 4);

            if (getTerrainFlags() & kTerrainDesert)
                treeVariant = static_cast<uint8>(g_engine->rollDice(1, 3) + 3);

            setRawTile(TilePos((uint8)col, (uint8)row), (treeVariant + kTileTreeBotBase) + 1);

            if (treeVariant < 5)
                setRawTile(TilePos((uint8)col, (uint8)(row - 1)), (treeVariant + kTileTreeTopBase) + 1);
        }
    }
}

void BattlefieldMap::setTilePatternCover() {
    uint8 terrainFlags = getTerrainFlags();
    const TilePropertyProvider *tileProps = _tileProps;

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

    int threshA = static_cast<int>(bandBase);
    int threshB = threshA + 6;
    int threshC = threshB + static_cast<int>(bandWidth1);
    int threshD = threshC + static_cast<int>(bandWidth2);
    int threshE = threshD + static_cast<int>(bandWidth3);

    for (int col = 0; col < kPlayfieldCols; col++) {
        for (int row = 0; row < kPlayfieldRows; row++) {
            uint8 raw = getRawTile(TilePos((uint8)col, (uint8)row));
            if (raw == 0)
                continue;
            uint8 tileId = raw - 1;
            if (!tileProps || tileId >= tileProps->getTilePropCount())
                continue;
            if (tileProps->getGfxID(tileId) != kTileOpenPlain)
                continue;

            int roll = static_cast<int>(static_cast<uint8>(g_engine->rollDice(1, 255)));

            if (roll <= threshA) {
                int featureRoll = g_engine->rollDice(1, 4);
                if (featureRoll == 4 && row > 0) {
                    uint8 aboveRaw = getRawTile(TilePos((uint8)col, (uint8)(row - 1)));
                    if (aboveRaw != 0) {
                        uint8 aboveTid = aboveRaw - 1;
                        if (aboveTid < static_cast<uint8>(tileProps->getTilePropCount()) &&
                            tileProps->getGfxID(aboveTid) == kTileOpenPlain) {
                            setRawTile(TilePos((uint8)col, (uint8)(row - 1)), kTileWallTop + 1);
                            setRawTile(TilePos((uint8)col, (uint8)row), kTileWallBottom + 1);
                        }
                    }
                } else if (featureRoll < 4) {
                    setRawTile(TilePos((uint8)col, (uint8)row),
                               static_cast<uint8>(featureRoll + kTileZoneABase) + 1);
                }
            } else if (roll <= threshB) {
                int featureRoll = g_engine->rollDice(1, 3);
                setRawTile(TilePos((uint8)col, (uint8)row),
                           static_cast<uint8>(featureRoll + kTileScrubBase - 1) + 1);
            } else if (roll <= threshC) {
                int featureRoll = g_engine->rollDice(1, 4);
                setRawTile(TilePos((uint8)col, (uint8)row),
                           static_cast<uint8>(featureRoll + kTileCoverBase - 1) + 1);
            } else if (roll <= threshD) {
                int d10 = g_engine->rollDice(1, 10);
                int variant = ((d10 - 1) / 3) + 1;
                setRawTile(TilePos((uint8)col, (uint8)row),
                           static_cast<uint8>(variant + kTileCropBase - 1) + 1);
            } else if (roll <= threshE) {
                int featureRoll = g_engine->rollDice(1, 4);
                if (featureRoll == 4) {
                    if (!(getTerrainFlags() & kTerrainDesert))
                        featureRoll = g_engine->rollDice(1, 3);
                }
                setRawTile(TilePos((uint8)col, (uint8)row),
                           static_cast<uint8>(featureRoll + kTileCliffBase - 1) + 1);
            }
        }
    }
}

void BattlefieldMap::setRandomFloorTiles() {
    const TilePropertyProvider *tileProps = _tileProps;

    for (int col = 0; col < kPlayfieldCols; col++) {
        for (int row = 0; row < kPlayfieldRows; row++) {
            uint8 raw = getRawTile(TilePos((uint8)col, (uint8)row));
            if (raw == 0)
                continue;
            uint8 tileId = raw - 1;
            if (!tileProps || tileId >= tileProps->getTilePropCount())
                continue;
            if (tileProps->getGfxID(tileId) != kTileFloor)
                continue;

            int roll = g_engine->rollDice(1, 100);

            if (roll == 98) {
                setRawTile(TilePos((uint8)col, (uint8)row), kTileLightVegetation);
            } else if (roll == 99) {
                setRawTile(TilePos((uint8)col, (uint8)row), kTileDenseShrub);
            } else if (roll == 100 && _eclScriptId != 10) {
                if (g_engine->rollDice(1, 100) != 1)
                    continue;
                int d10 = g_engine->rollDice(1, 10);
                if (d10 <= 6)
                    setRawTile(TilePos((uint8)col, (uint8)row), kTileSmallRocks);
                else
                    setRawTile(TilePos((uint8)col, (uint8)row), kTileLargeBoulders);
            }
        }
    }
}

} // namespace Combat
} // namespace Goldbox
