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

#ifndef GOLDBOX_DATA_ITEMS_BASE_ITEMS_H
#define GOLDBOX_DATA_ITEMS_BASE_ITEMS_H

#include "common/stream.h"
#include "common/array.h"

namespace Goldbox {
namespace Data {
namespace Items {

/// Which slot the item occupies on a character
enum class Slot : uint8 {
    S_MAIN_HAND  = 0,
    S_OFF_HAND   = 1,
    S_BODY_ARMOR = 2,
    S_GAUNTLETS  = 3,
    S_HELM       = 4,
    S_BELT       = 5,
    S_ROBE       = 6,
    S_CLOAK      = 7,
    S_BOOTS      = 8,
    S_RING       = 9,
    S_AMMO       = 10
};

/// Simple grouping of dice data 'dices d sides + bonus'
struct Damage {
    uint8 dices;   ///< Number of dice
    uint8 sides;   ///< Sides per die
    int8  bonus;   ///< Flat bonus
};

/// One 16-byte record from the ITEMS file
struct ItemProperty {
    Slot    slot;           ///< offset 0x00
    uint8   hands;          ///< offset 0x01
    Damage  dmgLarge;       ///< offsets 0x02–0x04
    uint8   fireRate;       ///< 0x05
    uint8   protect;        ///< 0x06
    uint8   wpnType;        ///< 0x07
    uint8   melType;        ///< 0x08
    Damage  dmgSmallMed;    ///< 0x09–0x0B
    uint8   range;          ///< 0x0C
    uint8   classMask;      ///< 0x0D
    uint8   missileType;    ///< 0x0E
};

/// Loads all 128 ItemProperty entries from the “ITEMS” file
class Storage {
public:
    /// Skip the 2-byte header, then read exactly 128 × 16 bytes
    void load(Common::SeekableReadStream &stream);

    /// All items in a contiguous array
    const Common::Array<ItemProperty> &all() const { return _items; }
    size_t count() const { return _items.size(); }

private:
    Common::Array<ItemProperty> _items;
};

} // namespace Items
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ITEMS_BASE_ITEMS_H
