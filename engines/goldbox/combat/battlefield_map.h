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

#ifndef GOLDBOX_COMBAT_BATTLEFIELD_MAP_H
#define GOLDBOX_COMBAT_BATTLEFIELD_MAP_H

#include "common/scummsys.h"
#include "goldbox/core/tile_pos.h"

namespace Goldbox {

class RuntimeGeoBlock;

namespace Combat {

class TilePropertyProvider;

class BattlefieldMap {
public:
    static const int kPlayfieldCols = 50;
    static const int kPlayfieldRows = 25;

    BattlefieldMap();

    void setTilePropertyProvider(const TilePropertyProvider *provider);
    const TilePropertyProvider *getTilePropertyProvider() const;

    void clear();

    void build(const RuntimeGeoBlock &geo,
               MapPos center, int8 playerY,
               bool isDungeon, uint8 eclScriptId,
               TilePos wild,
               uint8 mapType, uint8 terrainOverride);

    void regenerate(const RuntimeGeoBlock &geo,
                    MapPos center, int8 playerY,
                    bool isDungeon, uint8 eclScriptId,
                    TilePos wild,
                    uint8 mapType, uint8 terrainOverride);

    uint8 checkOpenPassage(MapPos mapPos, uint8 wireDir) const;
    uint8 getRawTile(TilePos pos) const;
    void setRawTile(TilePos pos, uint8 rawTile);
    uint8 getTileId(TilePos pos) const;
    TilePos getViewportStart() const;
    uint8 getSize() const;
    bool getTargetCursor() const;
    bool getIgnoreWalls() const;
    bool isDungeon() const;
    MapPos getCenter() const;

    // --- Dirty tile tracking ---

    /**
     * Returns true if any tile has been modified since last
     * acknowledgement. Used by the tilemap renderer to know when
     * to re-render affected tiles instead of the full surface.
     */
    bool hasDirtyTiles() const { return _dirtyCount > 0; }

    /** Number of individually dirty tiles since last ack. */
    int getDirtyCount() const { return _dirtyCount; }

    /**
     * Check if a specific tile is dirty.
     * @return true if tile at (col, row) was modified since last ack.
     */
    bool isTileDirty(TilePos pos) const;

    /**
     * Acknowledge all dirty tiles (renderer has redrawn them).
     * Resets the dirty set to empty.
     */
    void acknowledgeDirtyTiles();

private:
    struct PlayfieldState {
        uint8 unknown1;
        uint8 unknown2;
        TilePos viewportStart;
        bool targetCursor;
        uint8 size;
        bool ignoreWalls;
        uint8 fieldTileMap[kPlayfieldRows][kPlayfieldCols];
    } _playfield;

    enum CellState {
        kCellOpen = 0,
        kCellBlocked = 1,
        kCellWall = 3
    };

    static const int kMapBoundMin = 0;
    static const int kMapBoundMax = 15;
    static const int kCellTileCols = 6;
    static const int kCellTileRows = 5;
    static const int kWildTilemapRows = 36;
    static const int kWildTilemapCols = 44;

    static const uint8 kTileFloor = 22;
    static const uint8 kTileLightVegetation = 26;
    static const uint8 kTileDenseShrub = 27;
    static const uint8 kTileSmallRocks = 28;
    static const uint8 kTileLargeBoulders = 29;

    static const uint8 kTileOpenPlain = 22;
    static const uint8 kTileStreamA = 50;
    static const uint8 kTileStreamB = 51;
    static const uint8 kTileFordA = 52;
    static const uint8 kTileFordB = 53;
    static const uint8 kTileTreeTopBase = 0x1F;
    static const uint8 kTileTreeBotBase = 0x23;
    static const uint8 kTileScrubBase = 0x37;
    static const uint8 kTileCoverBase = 0x29;
    static const uint8 kTileCropBase = 0x38;
    static const uint8 kTileCliffBase = 0x2D;
    static const uint8 kTileWallBottom = 0x40;
    static const uint8 kTileWallTop = 0x41;
    static const uint8 kTileZoneABase = 0x3C;

    static const uint8 kTerrainWater = 0x01;
    static const uint8 kTerrainRiver = 0x02;
    static const uint8 kTerrainForest = 0x04;
    static const uint8 kTerrainMarsh = 0x08;
    static const uint8 kTerrainHills = 0x10;
    static const uint8 kTerrainRuins = 0x20;
    static const uint8 kTerrainDesert = 0x40;
    static const uint8 kTerrainUnderground = 0x80;

    static const uint8 kWildernessTilemap[kWildTilemapRows][kWildTilemapCols];

    const RuntimeGeoBlock *_geo;
    const TilePropertyProvider *_tileProps;
    bool _isDungeon;
    MapPos _center;
    int8 _playerY;
    uint8 _eclScriptId;
    uint8 _wildCell;
    uint8 _terrainOverrideFlags;
    uint8 _mapType;
    TilePos _wild;

    MapPos _cellAbs;
    int _cellOffsetX;
    int _cellOffsetY;
    uint8 _cellPassWest;
    uint8 _cellPassNorth;
    uint8 _cellPassEast;

    void writeTile(int localCol, int localRow, uint8 tileId);
    uint8 checkCell(MapPos mapPos, uint8 wireDir) const;

    // --- Dirty tile bitmap (1 bit per tile, 50*25 = 1250 bits = 157 bytes) ---
    static const int kDirtyBitmapSize = (kPlayfieldCols * kPlayfieldRows + 7) / 8;
    mutable uint8 _dirtyBitmap[kDirtyBitmapSize];
    mutable int _dirtyCount;

    void markTileDirty(TilePos pos) const;

    void generateDungeon(MapPos center);
    void generateWilderness();

    uint8 getTerrainFlags() const;
    void setTilePatternRiver();
    void setTilePatternTrees();
    void setTilePatternCover();
    void markFordPair(int col, int row, int &pairCount);

    void setTilePatternWestSide();
    void setTilePatternNorthSide();
    void setTilePatternNWCorner();
    void setTilePatternNECorner();

    void setRandomFloorTiles();
    void resetPlayfieldState();
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_BATTLEFIELD_MAP_H
