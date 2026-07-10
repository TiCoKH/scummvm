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

void BattlefieldTilemap::setTilePropertyProvider(
        const Combat::TilePropertyProvider *provider) {
    _tileProps = provider;
    if (_cicon)
        _cicon->setTilePropertyProvider(provider);
}

const Combat::TilePropertyProvider *BattlefieldTilemap::getTilePropertyProvider() const {
    return _cicon ? _cicon->getTilePropertyProvider() : _tileProps;
}

BattlefieldTilemap::BattlefieldTilemap()
        : _tileProps(nullptr), _cicon(nullptr), _built(false) {
    _surface.create(kSurfaceWidth, kSurfaceHeight);
    _surface.clear(0);
        _cicon = new Combat::BattlefieldMap();
}

BattlefieldTilemap::~BattlefieldTilemap() {
    delete _cicon;
    _surface.free();
}

void BattlefieldTilemap::clear() {
    _cicon->clear();
    _surface.clear(0);
    _built = false;
}

uint8 BattlefieldTilemap::getRawTile(int col, int row) const {
    return _cicon->getRawTile(col, row);
}

void BattlefieldTilemap::setRawTile(int col, int row, uint8 rawTile) {
    _cicon->setRawTile(col, row, rawTile);
}

uint8 BattlefieldTilemap::getTileId(int col, int row) const {
    return _cicon->getTileId(col, row);
}

bool BattlefieldTilemap::isDungeon() const {
    return _cicon->isDungeon();
}

int8 BattlefieldTilemap::getCenterX() const {
    return _cicon->getCenterX();
}

int8 BattlefieldTilemap::getCenterY() const {
    return _cicon->getCenterY();
}

Common::Rect BattlefieldTilemap::tileToPixelRect(int col, int row,
                                                 int w, int h) const {
    return Common::Rect(col * kIconSize, row * kIconSize,
                        (col + w) * kIconSize, (row + h) * kIconSize);
}

Common::Rect BattlefieldTilemap::getActiveAreaRect() const {
    return tileToPixelRect(_cicon->getViewportStartX(), _cicon->getViewportStartY(),
                           kPlayfieldCols - _cicon->getViewportStartX(),
                           kPlayfieldRows - _cicon->getViewportStartY());
}

void BattlefieldTilemap::build(const RuntimeGeoBlock &geo,
                               int8 centerX, int8 centerY, int8 playerY,
                               bool isDungeon, uint8 eclScriptId,
                               uint8 wildX, uint8 wildY,
                               uint8 mapType, uint8 terrainOverride) {
    clear();

    _cicon->build(geo, centerX, centerY, playerY, isDungeon, eclScriptId,
                  wildX, wildY, mapType, terrainOverride);
    _built = true;
}

void BattlefieldTilemap::regenerate(const RuntimeGeoBlock &geo,
                                    int8 centerX, int8 centerY, int8 playerY,
                                    uint8 eclScriptId,
                                    uint8 wildX, uint8 wildY,
                                    uint8 mapType, uint8 terrainOverride) {
    _cicon->regenerate(geo, centerX, centerY, playerY, _cicon->isDungeon(),
                       eclScriptId, wildX, wildY, mapType, terrainOverride);
}

uint8 BattlefieldTilemap::checkOpenPassage(int8 mapX, int8 mapY,
                                           uint8 wireDir) const {
    return _cicon->checkOpenPassage(mapX, mapY, wireDir);
}

void BattlefieldTilemap::render(const IconManager &iconMgr) {
    _surface.clear(0);

    for (int row = 0; row < kPlayfieldRows; row++) {
        for (int col = 0; col < kPlayfieldCols; col++) {
            uint8 raw = _cicon->getRawTile(col, row);
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
