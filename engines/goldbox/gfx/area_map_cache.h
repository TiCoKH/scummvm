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

#ifndef GOLDBOX_GFX_AREA_MAP_CACHE_H
#define GOLDBOX_GFX_AREA_MAP_CACHE_H

#include "common/scummsys.h"
#include "graphics/managed_surface.h"

namespace Goldbox {

class RuntimeGeoBlock;

namespace Gfx {

class Tile8x8Cache;

/**
 * Cached 2D top-down area map surface (16x16 cells, 8px each = 128x128).
 *
 * Built once per map load from RuntimeGeoBlock wall nibbles using tile slot 4
 * (global tile IDs 0x100-0x11E). The viewport blits an 88x88 (11x11 cell)
 * window centered on the party, plus a directional cursor stamp.
 *
 * Tile mapping:
 *   Base tile: 0x104
 *   Wall mask: bit 0 = north wall, bit 1 = east, bit 2 = south, bit 3 = west
 *   Final tile ID = 0x104 + mask
 *
 * Cursor tiles (facing arrow):
 *   0x100 = North, 0x101 = East, 0x102 = South, 0x103 = West
 *   Index = facing / 2  (wire format 0/2/4/6)
 */
class AreaMapCache {
public:
    static const int kGridSize = 16;
    static const int kCellSize = 8;
    static const int kSurfaceSize = kGridSize * kCellSize; // 128

    static const int kViewCells = 11;
    static const int kViewSize = kViewCells * kCellSize; // 88

    static const uint16 kBaseTileId = 0x104;
    static const uint16 kCursorTileBase = 0x100;

    AreaMapCache();

    /**
     * Rebuild the entire 128x128 cached surface from current map state.
     * Call on map load or when map data changes (door open, etc.).
     */
    void rebuild(const RuntimeGeoBlock &geo, const Tile8x8Cache &tileCache);

    /**
     * Invalidate and rebuild a single cell (e.g. after door state change).
     */
    void rebuildCell(int x, int y, const RuntimeGeoBlock &geo,
            const Tile8x8Cache &tileCache);

    /**
     * Draw the area map viewport onto dst at the standard 3D viewport
     * position, centered on (playerX, playerY) with a facing cursor.
     *
     * @param dst       Target surface (full screen)
     * @param playerX   Party X position (0-15)
     * @param playerY   Party Y position (0-15)
     * @param facing    Party facing direction (wire: 0/2/4/6)
     * @param tileCache Tile cache for cursor stamp
     */
    void drawViewport(Graphics::ManagedSurface &dst,
            int playerX, int playerY, uint8 facing,
            const Tile8x8Cache &tileCache) const;

    /** True if the cache has been built at least once. */
    bool isBuilt() const { return _built; }

private:
    void blitTileToCache(int cellX, int cellY, uint16 globalTileId,
            const Tile8x8Cache &tileCache);

    Graphics::ManagedSurface _surface;
    bool _built;
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_AREA_MAP_CACHE_H
