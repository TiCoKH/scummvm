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

#ifndef GOLDBOX_GFX_COMBAT_TILE_CACHE_H
#define GOLDBOX_GFX_COMBAT_TILE_CACHE_H

#include "common/scummsys.h"

namespace Goldbox {

namespace Data {
class DaxBlockContainer;
}

namespace Gfx {

class Pic;

/**
 * Cache of decoded 24x24 terrain tile graphics for combat rendering.
 *
 * Mirrors original DAX_LoadIconBlock behavior:
 *   Dungeon:    DUNGCOM blocks 0-24  -> tile slots 0..24
 *   Wilderness: WILDCOM blocks 0-33  -> tile slots 0..33 (replaces dungeon)
 *   Both:       RANDCOM blocks 34-39 -> tile slots 34..39
 *
 * The BattlefieldMap stores raw tile values as (slot + 1), so the
 * renderer subtracts 1 to get the cache index.
 *
 * Tiles are decoded lazily on first access and cached for the
 * duration of the combat encounter.
 */
class CombatTileCache {
public:
    static const int MAX_TILES = 64;

    CombatTileCache();
    ~CombatTileCache();

    /**
     * Load terrain tiles for a dungeon encounter.
     * Loads DUNGCOM blocks 0-24 into slots 0-24,
     * then RANDCOM blocks 0-5 into slots 34-39.
     */
    void loadDungeon(Data::DaxBlockContainer &dungcom,
                     Data::DaxBlockContainer &randcom);

    /**
     * Load terrain tiles for a wilderness encounter.
     * Loads WILDCOM blocks 0-33 into slots 0-33,
     * then RANDCOM blocks 0-5 into slots 34-39.
     */
    void loadWilderness(Data::DaxBlockContainer &wildcom,
                        Data::DaxBlockContainer &randcom);

    /**
     * Get a decoded terrain tile by slot index.
     * @param slotId Tile slot (0-based, from BattlefieldMap raw value - 1)
     * @return Pic pointer or nullptr if slot is empty/invalid
     */
    const Pic *getTile(uint8 slotId) const;

    /** Release all cached tiles. */
    void clear();

    /** Check if tiles have been loaded. */
    bool isLoaded() const { return _loaded; }

private:
    Pic *_tiles[MAX_TILES];
    bool _loaded;

    void loadFromContainer(Data::DaxBlockContainer &container,
                           int startBlock, int count, int destSlot);
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_COMBAT_TILE_CACHE_H
