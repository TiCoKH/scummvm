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

namespace Goldbox {

class RuntimeGeoBlock;

namespace Combat {
class BattlefieldMap;
class TilePropertyProvider;
}

namespace Gfx {

class IconManager;
class CombatTileCache;

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
    static const int kHeaderSize = 7;
    static const int kBufferSize = kHeaderSize + kPlayfieldCols * kPlayfieldRows;

    BattlefieldTilemap();
    ~BattlefieldTilemap();

    void render(const Combat::BattlefieldMap &map,
                const CombatTileCache &tileCache,
                const Combat::TilePropertyProvider *tileProps = nullptr);

    /**
     * Incrementally re-render only tiles marked dirty in the map.
     * Much cheaper than a full render() when only a few tiles changed
     * (e.g. downed-member stamp, spell cloud placement).
     *
     * @param map  Battlefield map (dirty tiles are acknowledged after redraw)
     * @param tileCache  Terrain tile cache for tile graphics
     * @param tileProps  Tile property provider for blockID lookup (optional)
     * @return Number of tiles actually redrawn (0 if nothing was dirty)
     */
    int renderDirtyTiles(Combat::BattlefieldMap &map,
                         const CombatTileCache &tileCache,
                         const Combat::TilePropertyProvider *tileProps = nullptr);

    /**
     * @deprecated Use CombatTileCache overload instead.
     * Kept for backward compatibility during transition.
     */
    void render(const Combat::BattlefieldMap &map,
                const IconManager &iconMgr);
    int renderDirtyTiles(Combat::BattlefieldMap &map,
                         const IconManager &iconMgr);

    const Graphics::ManagedSurface *getSurface() const { return &_surface; }
    Graphics::ManagedSurface *getSurface() { return &_surface; }

    void blitTo(Graphics::ManagedSurface *dst,
                const Common::Point &dstPos,
                const Common::Rect &srcRect) const;

    void blitTo(Graphics::ManagedSurface *dst,
                const Common::Point &dstPos) const;

    Common::Rect tileToPixelRect(int col, int row, int w, int h) const;
    Common::Rect getActiveAreaRect(const Combat::BattlefieldMap &map) const;

    void clear();
    bool isBuilt() const { return _built; }

private:
    Graphics::ManagedSurface _surface;
    bool _built;
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_BATTLEFIELD_TILEMAP_H
