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
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/gfx/pic.h"

#include <string.h>

namespace Goldbox {
namespace Gfx {

BattlefieldTilemap::BattlefieldTilemap()
        : _built(false) {
    _surface.create(kSurfaceWidth, kSurfaceHeight);
    _surface.clear(0);
}

BattlefieldTilemap::~BattlefieldTilemap() {
    _surface.free();
}

void BattlefieldTilemap::clear() {
    _surface.clear(0);
    _built = false;
}

Common::Rect BattlefieldTilemap::tileToPixelRect(int col, int row,
                                                 int w, int h) const {
    return Common::Rect(col * kIconSize, row * kIconSize,
                        (col + w) * kIconSize, (row + h) * kIconSize);
}

Common::Rect BattlefieldTilemap::getActiveAreaRect(
        const Combat::BattlefieldMap &map) const {
    return tileToPixelRect(map.getViewportStartX(), map.getViewportStartY(),
                           kPlayfieldCols - map.getViewportStartX(),
                           kPlayfieldRows - map.getViewportStartY());
}

void BattlefieldTilemap::render(const Combat::BattlefieldMap &map,
                                const IconManager &iconMgr) {
    _surface.clear(0);

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            uint8 raw = map.getRawTile(col, row);
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

    _built = true;
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
