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

#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/core/tile_pos.h"

#include <string.h>

namespace Goldbox {
namespace Gfx {

BattlefieldTilemap::BattlefieldTilemap()
        : _built(false) {
    _surface.create(kSurfaceWidth, kSurfaceHeight);
    _surface.clear(kBackgroundColor);
}

BattlefieldTilemap::~BattlefieldTilemap() {
    _surface.free();
}

void BattlefieldTilemap::clear() {
    _surface.clear(kBackgroundColor);
    _built = false;
}

Common::Rect BattlefieldTilemap::tileToPixelRect(TilePos pos, int w, int h) const {
    return Common::Rect(pos.col * kIconSize, pos.row * kIconSize,
                        (pos.col + w) * kIconSize, (pos.row + h) * kIconSize);
}

Common::Rect BattlefieldTilemap::getActiveAreaRect(
        const Combat::BattlefieldMap &map) const {
    TilePos vs = map.getViewportStart();
    return tileToPixelRect(vs,
                           kPlayfieldCols - vs.col,
                           kPlayfieldRows - vs.row);
}

void BattlefieldTilemap::render(const Combat::BattlefieldMap &map,
                                const CombatTileCache &tileCache,
                                const Combat::TilePropertyProvider *tileProps) {
    _surface.clear(kBackgroundColor);

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            uint8 raw = map.getRawTile(TilePos((uint8)col, (uint8)row));
            if (raw == 0)
                continue;

            uint8 slotId;
            if (tileProps && (raw - 1) < tileProps->getTilePropCount()) {
                slotId = tileProps->getGfxID(raw - 1);
            } else {
                slotId = raw - 1;
            }

            const Pic *pic = tileCache.getTile(slotId);
            if (!pic)
                continue;

            int pixX = col * kIconSize;
            int pixY = row * kIconSize;
            pic->trDraw(&_surface, pixX, pixY, 255);
        }
    }

    _built = true;
}

int BattlefieldTilemap::renderDirtyTiles(Combat::BattlefieldMap &map,
                                         const CombatTileCache &tileCache,
                                         const Combat::TilePropertyProvider *tileProps) {
    if (!map.hasDirtyTiles())
        return 0;

    int redrawn = 0;

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            if (!map.isTileDirty(TilePos((uint8)col, (uint8)row)))
                continue;

            int pixX = col * kIconSize;
            int pixY = row * kIconSize;

            Common::Rect tileRect(pixX, pixY,
                                  pixX + kIconSize, pixY + kIconSize);
            _surface.fillRect(tileRect, kBackgroundColor);

            uint8 raw = map.getRawTile(TilePos((uint8)col, (uint8)row));
            if (raw != 0) {
                uint8 slotId;
                if (tileProps && (raw - 1) < tileProps->getTilePropCount()) {
                    slotId = tileProps->getGfxID(raw - 1);
                } else {
                    slotId = raw - 1;
                }
                const Pic *pic = tileCache.getTile(slotId);
                if (pic)
                    pic->trDraw(&_surface, pixX, pixY, 255);
            }

            redrawn++;
        }
    }

    map.acknowledgeDirtyTiles();
    return redrawn;
}

void BattlefieldTilemap::render(const Combat::BattlefieldMap &map,
                                const IconManager &iconMgr) {
    _surface.clear(kBackgroundColor);

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            uint8 raw = map.getRawTile(TilePos((uint8)col, (uint8)row));
            if (raw == 0)
                continue;

            uint8 slotId = raw - 1;
            const Pic *pic = iconMgr.getPic(slotId);
            if (!pic)
                continue;

            int pixX = col * kIconSize;
            int pixY = row * kIconSize;
            pic->trDraw(&_surface, pixX, pixY, 255);
        }
    }

    _built = true;
}

int BattlefieldTilemap::renderDirtyTiles(Combat::BattlefieldMap &map,
                                         const IconManager &iconMgr) {
    if (!map.hasDirtyTiles())
        return 0;

    int redrawn = 0;

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            if (!map.isTileDirty(TilePos((uint8)col, (uint8)row)))
                continue;

            int pixX = col * kIconSize;
            int pixY = row * kIconSize;

            Common::Rect tileRect(pixX, pixY,
                                  pixX + kIconSize, pixY + kIconSize);
            _surface.fillRect(tileRect, kBackgroundColor);

            uint8 raw = map.getRawTile(TilePos((uint8)col, (uint8)row));
            if (raw != 0) {
                uint8 slotId = raw - 1;
                const Pic *pic = iconMgr.getPic(slotId);
                if (pic)
                    pic->trDraw(&_surface, pixX, pixY, 255);
            }

            redrawn++;
        }
    }

    map.acknowledgeDirtyTiles();
    return redrawn;
}

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
