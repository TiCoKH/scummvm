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
    int loadedCount = 0;
    int missingStreak = 0;

    // Flatten tile sources into one logical frame stream:
    // - one block with many frames
    // - many blocks with one frame each
    // - many blocks with many frames each
    for (int blockId = startBlock; blockId <= 255 && loadedCount < count;
         ++blockId) {
        const int slot = destSlot + loadedCount;
        if (slot >= MAX_TILES)
            break;

        Data::DaxBlock *rawBlock = container.getBlockById(
            static_cast<uint8>(blockId));
        if (!rawBlock) {
            ++missingStreak;
            if (missingStreak >= 8)
                break;
            continue;
        }
        missingStreak = 0;

        Data::DaxBlockPic *picBlock =
            dynamic_cast<Data::DaxBlockPic *>(rawBlock);
        if (!picBlock)
            continue;

        const int frameSize = (picBlock->width * picBlock->height) / 2;
        if (frameSize <= 0)
            continue;

        int frameCount = static_cast<int>(picBlock->_data.size()) / frameSize;

        Data::DaxBlock8x8D *tileBlock =
            dynamic_cast<Data::DaxBlock8x8D *>(picBlock);
        if (tileBlock && tileBlock->item_count > 0
            && tileBlock->item_count < frameCount) {
            frameCount = tileBlock->item_count;
        } else if (!tileBlock && picBlock->frameCount > 0
                   && picBlock->frameCount < frameCount) {
            frameCount = picBlock->frameCount;
        }

        for (int frame = 0; frame < frameCount && loadedCount < count;
             ++frame) {
            int frameSlot = destSlot + loadedCount;
            if (frameSlot >= MAX_TILES)
                break;

            Pic *tilePic = Pic::readTileFrame(picBlock, frame);
            if (!tilePic)
                continue;

            _tiles[frameSlot] = tilePic;
            ++loadedCount;
        }
    }

    debug(4, "CombatTileCache: loaded %d/%d tiles from container "
        "(startBlock=%d, destSlot=%d)",
        loadedCount, count, startBlock, destSlot);
}

} // namespace Gfx
} // namespace Goldbox
