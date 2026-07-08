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

#ifndef GOLDBOX_GFX_BATTLEFIELD_TILEMAP_H
#define GOLDBOX_GFX_BATTLEFIELD_TILEMAP_H

#include "common/scummsys.h"
#include "common/rect.h"
#include "graphics/managed_surface.h"
#include "goldbox/core/direction.h"

namespace Goldbox {

class RuntimeGeoBlock;

namespace Gfx {

class IconManager;

/**
 * Combat battlefield playfield.
 *
 * Models _PTR_COMBAT_PLAYFIELD:
 *   - 1257 bytes = 7-byte header + 50x25 tile buffer
 *   - Tile values are 1-based (0 = empty sentinel)
 *   - Header bytes [4..6] = {0, 1, 0}
 *
 * Tile IDs stored in the buffer are 0-based indices into the global
 * IconManager (128-slot icon store). COMBAT_BuildPlayfield loads
 * 24x24 terrain icons into IconManager via DAX_LoadIconBlock:
 *
 *   Dungeon:    DAX_LoadIconBlock("DungCom", 0, 24)  -> slots 0..23
 *   Wilderness: DAX_LoadIconBlock("WildCom", 0, 33)  -> slots 0..32
 *   Both:       DAX_LoadIconBlock("RandCom", 34, 6)  -> slots 34..39
 *
 * (DUNGCOM.DAX has 25 icons, WILDCOM.DAX has 34, RANDCOM.DAX has 6)
 *
 * Renders to an internal ManagedSurface by drawing icons from
 * IconManager at each tile position. The render pipeline extracts
 * regions via blitTo() for compositing with character icons.
 *
 * Directions use wire format from direction.h:
 *   0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
 */
class BattlefieldTilemap {
public:
    static const int kPlayfieldCols = 50;
    static const int kPlayfieldRows = 25;
    static const int kIconSize = 24;
    static const int kSurfaceWidth = kPlayfieldCols * kIconSize;
    static const int kSurfaceHeight = kPlayfieldRows * kIconSize;
    static const int kColMargin = 21;
    static const int kRowMargin = 10;
    static const int kHeaderSize = 7;
    static const int kBufferSize = kHeaderSize + kPlayfieldCols * kPlayfieldRows;
    static const int kCellTileCols = 6;
    static const int kCellTileRows = 5;
    static const int kSnapshotCols = 13;
    static const int kSnapshotRows = 5;
    static const int kMapBoundMin = 0;
    static const int kMapBoundMax = 15;

    // Icon store slot layout
    static const int kDungeonSlotStart = 0;
    static const int kDungeonLoadCount = 24;
    static const int kWildSlotStart = 0;
    static const int kWildLoadCount = 33;
    static const int kRandSlotStart = 34;
    static const int kRandLoadCount = 6;

    static const uint8 kTileFloor = 22;
    static const uint8 kTileLightVegetation = 26;
    static const uint8 kTileDenseShrub = 27;
    static const uint8 kTileSmallRocks = 28;
    static const uint8 kTileLargeBoulders = 29;

    // Wilderness tile IDs
    static const uint8 kTileOpenPlain = 22;
    static const uint8 kTileStreamA = 50;
    static const uint8 kTileStreamB = 51;
    static const uint8 kTileFordA = 52;
    static const uint8 kTileFordB = 53;
    static const uint8 kTileTreeTopBase = 0x1F;   // +variant(1..4)
    static const uint8 kTileTreeBotBase = 0x23;   // +variant(1..6)
    static const uint8 kTileScrubBase = 0x37;
    static const uint8 kTileCoverBase = 0x29;
    static const uint8 kTileCropBase = 0x38;
    static const uint8 kTileCliffBase = 0x2D;
    static const uint8 kTileWallBottom = 0x40;
    static const uint8 kTileWallTop = 0x41;
    static const uint8 kTileZoneABase = 0x3C;

    // Terrain flag bits
    static const uint8 kTerrainWater = 0x01;
    static const uint8 kTerrainRiver = 0x02;
    static const uint8 kTerrainForest = 0x04;
    static const uint8 kTerrainMarsh = 0x08;
    static const uint8 kTerrainHills = 0x10;
    static const uint8 kTerrainRuins = 0x20;
    static const uint8 kTerrainDesert = 0x40;
    static const uint8 kTerrainUnderground = 0x80;

    // Wilderness tilemap dimensions
    static const int kWildTilemapRows = 36;
    static const int kWildTilemapCols = 44;

    // Wilderness tilemap stub (TODO: populate from game data)
    static const uint8 kWildernessTilemap[kWildTilemapRows][kWildTilemapCols];

    static const int kTilePropTableCount = 65;

    /**
     * Tile property entry describing passability and obstacle type.
     */
    struct TileProp {
        uint8 gfxID;      // [0] canonical graphic/sprite ID
        int8  passable;   // [1] 0x01 = walkable, -1 (0xFF) = blocked
        uint8 padding;    // [2] always 0x00
        uint8 blockType;  // [3] 0x02 = hard obstacle, 0x00 = normal
    };

    // 65 entries (indices 0..64)
    // passable: 0x01 = walkable, -1 (0xFF) = blocked
    // blockType: 0x02 = hard obstacle, 0x00 = normal
    static const TileProp kTilePropTable[kTilePropTableCount];

    BattlefieldTilemap();
    ~BattlefieldTilemap();

    void build(const RuntimeGeoBlock &geo,
               int8 centerX, int8 centerY, int8 playerY,
               bool isDungeon, uint8 eclScriptId = 0,
               uint8 wildX = 0, uint8 wildY = 0,
               uint8 mapType = 1, uint8 terrainOverride = 0);

    void regenerate(const RuntimeGeoBlock &geo,
                    int8 centerX, int8 centerY, int8 playerY,
                    uint8 eclScriptId = 0,
                    uint8 wildX = 0, uint8 wildY = 0,
                    uint8 mapType = 1, uint8 terrainOverride = 0);

    void render(const IconManager &iconMgr);

    const Graphics::ManagedSurface *getSurface() const { return &_surface; }
    Graphics::ManagedSurface *getSurface() { return &_surface; }

    void blitTo(Graphics::ManagedSurface *dst,
                const Common::Point &dstPos,
                const Common::Rect &srcRect) const;

    void blitTo(Graphics::ManagedSurface *dst,
                const Common::Point &dstPos) const;

    Common::Rect tileToPixelRect(int col, int row, int w, int h) const;
    Common::Rect getActiveAreaRect() const;

    uint8 getRawTile(int col, int row) const;
    void setRawTile(int col, int row, uint8 rawTile);
    uint8 getTileId(int col, int row) const;

    void clear();
    bool isBuilt() const { return _built; }
    bool isDungeon() const { return _isDungeon; }

    int8 getCenterX() const { return _centerX; }
    int8 getCenterY() const { return _centerY; }

private:
    enum CellState {
        kCellOpen    = 0,
        kCellBlocked = 1,
        kCellWall    = 3
    };

    uint8 _header[kHeaderSize];
    uint8 _tileBuffer[kPlayfieldRows][kPlayfieldCols];
    Graphics::ManagedSurface _surface;
    const RuntimeGeoBlock *_geo;
    int8 _playerY;
    bool _built;
    bool _isDungeon;
    int8 _centerX;
    int8 _centerY;
    uint8 _eclScriptId;
    uint8 _wildCell;              // wilderness cell type for terrain lookup
    uint8 _terrainOverrideFlags;  // 0xFF = override active
    uint8 _mapType;               // VM map type (1=dungeon, 3/4=wilderness variants)
    uint8 _wildX;                 // wilderness X coordinate
    uint8 _wildY;                 // wilderness Y coordinate

    // Cell context state (mirrors Amiga global variables)
    int8  _cellAbsX;
    int8  _cellAbsY;
    int   _cellOffsetX;
    int   _cellOffsetY;
    uint8 _cellPassWest;
    uint8 _cellPassNorth;
    uint8 _cellPassEast;

    void writeTile(int localCol, int localRow, uint8 tileId);

    /** Check cell passage state. wireDir uses direction.h format (0=N,2=E,4=S,6=W). */
    uint8 checkCell(int8 mapX, int8 mapY, uint8 wireDir) const;

public:
    /**
     * Check bidirectional passability between two adjacent cells.
     * Returns 0 if both directions passable, non-zero if blocked.
     * Equivalent to original COMBAT_GetBidirectionalPassability.
     */
    uint8 checkOpenPassage(int8 mapX, int8 mapY, uint8 wireDir) const;

private:

    void generateDungeon(int8 centerX, int8 centerY);
    void generateWilderness(int8 centerX, int8 centerY);

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

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_BATTLEFIELD_TILEMAP_H
