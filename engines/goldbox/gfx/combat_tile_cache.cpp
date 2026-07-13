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

#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "common/debug.h"

#include <string.h>

namespace Goldbox {
namespace Gfx {

CombatTileCache::CombatTileCache() : _loaded(false) {
    memset(_tiles, 0, sizeof(_tiles));
}

CombatTileCache::~CombatTileCache() {
    clear();
}

void CombatTileCache::clear() {
    for (int i = 0; i < MAX_TILES; i++) {
        delete _tiles[i];
        _tiles[i] = nullptr;
    }
    _loaded = false;
}

void CombatTileCache::loadDungeon(Data::DaxBlockContainer &dungcom,
                                  Data::DaxBlockContainer &randcom) {
    clear();
    // DUNGCOM blocks 0-24 -> slots 0-24 (25 dungeon terrain tiles)
    loadFromContainer(dungcom, 0, 25, 0);
    // RANDCOM blocks 0-5 -> slots 34-39 (6 random decoration tiles)
    loadFromContainer(randcom, 0, 6, 34);
    _loaded = true;
    debug(3, "CombatTileCache::loadDungeon: loaded dungeon terrain tiles");
}

void CombatTileCache::loadWilderness(Data::DaxBlockContainer &wildcom,
                                     Data::DaxBlockContainer &randcom) {
    clear();
    // WILDCOM blocks 0-33 -> slots 0-33 (34 wilderness terrain tiles)
    loadFromContainer(wildcom, 0, 34, 0);
    // RANDCOM blocks 0-5 -> slots 34-39 (6 random decoration tiles)
    loadFromContainer(randcom, 0, 6, 34);
    _loaded = true;
    debug(3, "CombatTileCache::loadWilderness: loaded wilderness terrain tiles");
}

const Pic *CombatTileCache::getTile(uint8 slotId) const {
    if (slotId >= MAX_TILES)
        return nullptr;
    return _tiles[slotId];
}

void CombatTileCache::loadFromContainer(Data::DaxBlockContainer &container,
                                        int startBlock, int count,
                                        int destSlot) {
    for (int i = 0; i < count; i++) {
        int blockId = startBlock + i;
        int slot = destSlot + i;
        if (slot >= MAX_TILES)
            break;

        Data::DaxBlock *rawBlock = container.getBlockById(
            static_cast<uint8>(blockId));
        if (!rawBlock)
            continue;

        // DUNGCOM/WILDCOM/RANDCOM blocks are DaxBlockPic (via DaxBlock8x8D)
        Data::DaxBlockPic *picBlock =
            dynamic_cast<Data::DaxBlockPic *>(rawBlock);
        if (picBlock) {
            _tiles[slot] = Pic::read(picBlock);
            if (!_tiles[slot]) {
                debug(5, "CombatTileCache: failed to decode tile block %d -> slot %d",
                    blockId, slot);
            }
            continue;
        }

        debug(5, "CombatTileCache: block %d is not DaxBlockPic type", blockId);
    }
}

} // namespace Gfx
} // namespace Goldbox
