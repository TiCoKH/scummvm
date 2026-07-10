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

#ifndef GOLDBOX_COMBAT_BATTLEFIELD_TILEMAP_LOGIC_H
#define GOLDBOX_COMBAT_BATTLEFIELD_TILEMAP_LOGIC_H

#include "common/scummsys.h"

namespace Goldbox {

class RuntimeGeoBlock;

namespace Gfx {
class BattlefieldTilemap;
}

namespace Combat {

class BattlefieldTilemapLogic {
public:
    explicit BattlefieldTilemapLogic(Gfx::BattlefieldTilemap &tilemap);

    void build(const RuntimeGeoBlock &geo,
               int8 centerX, int8 centerY, int8 playerY,
               bool isDungeon, uint8 eclScriptId,
               uint8 wildX, uint8 wildY,
               uint8 mapType, uint8 terrainOverride);

    void regenerate(const RuntimeGeoBlock &geo,
                    int8 centerX, int8 centerY, int8 playerY,
                    bool isDungeon, uint8 eclScriptId,
                    uint8 wildX, uint8 wildY,
                    uint8 mapType, uint8 terrainOverride);

    uint8 checkOpenPassage(int8 mapX, int8 mapY, uint8 wireDir) const;

private:
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

    Gfx::BattlefieldTilemap &_tilemap;
    const RuntimeGeoBlock *_geo;
    int8 _playerY;
    uint8 _eclScriptId;
    uint8 _wildCell;
    uint8 _terrainOverrideFlags;
    uint8 _mapType;
    uint8 _wildX;
    uint8 _wildY;

    int8 _cellAbsX;
    int8 _cellAbsY;
    int _cellOffsetX;
    int _cellOffsetY;
    uint8 _cellPassWest;
    uint8 _cellPassNorth;
    uint8 _cellPassEast;

    void writeTile(int localCol, int localRow, uint8 tileId);
    uint8 checkCell(int8 mapX, int8 mapY, uint8 wireDir) const;

    void generateDungeon(int8 centerX, int8 centerY);
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
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_BATTLEFIELD_TILEMAP_LOGIC_H
