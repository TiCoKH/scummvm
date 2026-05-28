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

#include "goldbox/gfx/area_map_cache.h"

#include "common/algorithm.h"
#include "goldbox/gfx/walldef_surface_builder.h"
#include "goldbox/gfx/viewport_background.h"
#include "goldbox/runtime/runtime_geo.h"

namespace Goldbox {
namespace Gfx {

AreaMapCache::AreaMapCache() : _built(false) {
    _surface.create(kSurfaceSize, kSurfaceSize,
            Graphics::PixelFormat::createFormatCLUT8());
    _surface.fillRect(Common::Rect(0, 0, kSurfaceSize, kSurfaceSize), 0);
}

void AreaMapCache::rebuild(const RuntimeGeoBlock &geo,
        const Tile8x8Cache &tileCache) {
    _surface.fillRect(Common::Rect(0, 0, kSurfaceSize, kSurfaceSize), 0);

    for (int y = 0; y < kGridSize; ++y) {
        for (int x = 0; x < kGridSize; ++x) {
            uint8 mask = 0;
            if (geo.getMapNibble(x, y, 0) != 0) mask |= 1; // north
            if (geo.getMapNibble(x, y, 2) != 0) mask |= 2; // east
            if (geo.getMapNibble(x, y, 4) != 0) mask |= 4; // south
            if (geo.getMapNibble(x, y, 6) != 0) mask |= 8; // west

            blitTileToCache(x, y, kBaseTileId + mask, tileCache);
        }
    }

    _built = true;
}

void AreaMapCache::rebuildCell(int x, int y, const RuntimeGeoBlock &geo,
        const Tile8x8Cache &tileCache) {
    if (x < 0 || x >= kGridSize || y < 0 || y >= kGridSize)
        return;

    uint8 mask = 0;
    if (geo.getMapNibble(x, y, 0) != 0) mask |= 1;
    if (geo.getMapNibble(x, y, 2) != 0) mask |= 2;
    if (geo.getMapNibble(x, y, 4) != 0) mask |= 4;
    if (geo.getMapNibble(x, y, 6) != 0) mask |= 8;

    blitTileToCache(x, y, kBaseTileId + mask, tileCache);
}

void AreaMapCache::blitTileToCache(int cellX, int cellY,
        uint16 globalTileId, const Tile8x8Cache &tileCache) {
    const Graphics::ManagedSurface *tile = tileCache.tileSurface(globalTileId);
    if (!tile)
        return;

    const int dstX = cellX * kCellSize;
    const int dstY = cellY * kCellSize;

    for (int py = 0; py < kCellSize; ++py) {
        const byte *srcRow = (const byte *)tile->getBasePtr(0, py);
        byte *dstRow = (byte *)_surface.getBasePtr(dstX, dstY + py);
        for (int px = 0; px < kCellSize; ++px)
            dstRow[px] = srcRow[px];
    }
}

void AreaMapCache::drawViewport(Graphics::ManagedSurface &dst,
        int playerX, int playerY, uint8 facing,
        const Tile8x8Cache &tileCache) const {
    if (!_built)
        return;

    // Viewport origin: clamp so the 11-cell window stays within 16-cell map.
    int viewOriginX = playerX - 5;
    int viewOriginY = playerY - 5;
    if (viewOriginX < 0) viewOriginX = 0;
    if (viewOriginX > 5) viewOriginX = 5;
    if (viewOriginY < 0) viewOriginY = 0;
    if (viewOriginY > 5) viewOriginY = 5;

    // Blit 88x88 region from cached surface to viewport position.
    const int srcX = viewOriginX * kCellSize;
    const int srcY = viewOriginY * kCellSize;
    const int vpX = ViewportBackground::kViewportX;
    const int vpY = ViewportBackground::kViewportY;

    dst.blitFrom(_surface,
            Common::Rect(srcX, srcY, srcX + kViewSize, srcY + kViewSize),
            Common::Point(vpX, vpY));

    // Draw cursor arrow at party position within viewport.
    const int cursorCol = playerX - viewOriginX;
    const int cursorRow = playerY - viewOriginY;
    const uint16 cursorTileId = kCursorTileBase + (facing / 2);
    const Graphics::ManagedSurface *cursorTile =
            tileCache.tileSurface(cursorTileId);
    if (!cursorTile)
        return;

    const int cursorDstX = vpX + cursorCol * kCellSize;
    const int cursorDstY = vpY + cursorRow * kCellSize;

    for (int py = 0; py < kCellSize; ++py) {
        const byte *srcRow = (const byte *)cursorTile->getBasePtr(0, py);
        byte *dstRow = (byte *)dst.getBasePtr(cursorDstX, cursorDstY + py);
        for (int px = 0; px < kCellSize; ++px) {
            if (srcRow[px] != 0)
                dstRow[px] = srcRow[px];
        }
    }
}

} // namespace Gfx
} // namespace Goldbox
