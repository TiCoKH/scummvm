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

#ifndef GOLDBOX_COMBAT_TILE_PROPERTY_PROVIDER_H
#define GOLDBOX_COMBAT_TILE_PROPERTY_PROVIDER_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Combat {

/**
 * Tile property entry describing passability and obstacle type.
 *
 * Game-specific tile property tables map tile indices to their
 * combat-relevant attributes (passability, blocking behavior).
 */
struct TileProp {
    int8  passable;   // priority: 0x01 = walkable, -1 (0xFF) = blocked/impassable
    uint8 padding;    // reserved (always 0x00)
    uint8 tileSize;   // 0x02 = full tile obstacle, 0x00 = half-size
    uint8 blockID;    // graphic/sprite block ID
};

/**
 * Abstract interface for querying tile properties during combat.
 *
 * Each game provides its own implementation with game-specific
 * tile property tables. The combat system and tilemap renderer
 * query this interface rather than owning the data directly.
 */
class TilePropertyProvider {
public:
    virtual ~TilePropertyProvider() {}

    /** Number of entries in the tile property table. */
    virtual int getTilePropCount() const = 0;

    /** Get tile property by index. Returns nullptr if out of range. */
    virtual const TileProp *getTileProp(int index) const = 0;

    /** Convenience: check if a 1-based raw tile value is impassable. */
    bool isImpassable(uint8 rawTile) const {
        if (rawTile == 0 || rawTile > getTilePropCount())
            return true;
        const TileProp *prop = getTileProp(rawTile - 1);
        return prop && prop->passable == -1;
    }

    /** Convenience: get blockID for a 0-based tile index. Returns 0xFF on failure. */
    uint8 getGfxID(int index) const {
        const TileProp *prop = getTileProp(index);
        return prop ? prop->blockID : 0xFF;
    }
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_TILE_PROPERTY_PROVIDER_H
