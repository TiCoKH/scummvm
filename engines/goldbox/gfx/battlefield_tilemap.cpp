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

#include "goldbox/combat/battlefield_tilemap_logic.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/gfx/pic.h"

#include <string.h>

namespace Goldbox {
namespace Gfx {

BattlefieldTilemap::BattlefieldTilemap()
    : _tileProps(nullptr), _logic(nullptr), _built(false), _isDungeon(true),
      _centerX(0), _centerY(0) {
    memset(_header, 0, sizeof(_header));
    memset(_tileBuffer, 0, sizeof(_tileBuffer));
    _surface.create(kSurfaceWidth, kSurfaceHeight);
    _surface.clear(0);
    _logic = new Combat::BattlefieldTilemapLogic(*this);
}

BattlefieldTilemap::~BattlefieldTilemap() {
    delete _logic;
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
        row < 0 || row >= kPlayfieldRows) {
        return 0;
    }

    return _tileBuffer[row][col];
}

void BattlefieldTilemap::setRawTile(int col, int row, uint8 rawTile) {
    if (col < 0 || col >= kPlayfieldCols ||
        row < 0 || row >= kPlayfieldRows) {
        return;
    }

    _tileBuffer[row][col] = rawTile;
}

uint8 BattlefieldTilemap::getTileId(int col, int row) const {
    uint8 raw = getRawTile(col, row);
    return (raw == 0) ? 0xFF : static_cast<uint8>(raw - 1);
}

Common::Rect BattlefieldTilemap::tileToPixelRect(int col, int row,
                                                 int w, int h) const {
    return Common::Rect(col * kIconSize, row * kIconSize,
                        (col + w) * kIconSize, (row + h) * kIconSize);
}

Common::Rect BattlefieldTilemap::getActiveAreaRect() const {
    return tileToPixelRect(kColMargin, kRowMargin,
                           kPlayfieldCols - kColMargin,
                           kPlayfieldRows - kRowMargin);
}

void BattlefieldTilemap::build(const RuntimeGeoBlock &geo,
                               int8 centerX, int8 centerY, int8 playerY,
                               bool isDungeon, uint8 eclScriptId,
                               uint8 wildX, uint8 wildY,
                               uint8 mapType, uint8 terrainOverride) {
    clear();

    _centerX = centerX;
    _centerY = centerY;
    _isDungeon = isDungeon;

    memset(_header, 0, sizeof(_header));
    _header[4] = 0;
    _header[5] = 1;
    _header[6] = 0;

    _logic->build(geo, centerX, centerY, playerY, isDungeon, eclScriptId,
                  wildX, wildY, mapType, terrainOverride);
    _built = true;
}

void BattlefieldTilemap::regenerate(const RuntimeGeoBlock &geo,
                                    int8 centerX, int8 centerY, int8 playerY,
                                    uint8 eclScriptId,
                                    uint8 wildX, uint8 wildY,
                                    uint8 mapType, uint8 terrainOverride) {
    _centerX = centerX;
    _centerY = centerY;

    _logic->regenerate(geo, centerX, centerY, playerY, _isDungeon,
                       eclScriptId, wildX, wildY, mapType, terrainOverride);
}

uint8 BattlefieldTilemap::checkOpenPassage(int8 mapX, int8 mapY,
                                           uint8 wireDir) const {
    return _logic->checkOpenPassage(mapX, mapY, wireDir);
}

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
