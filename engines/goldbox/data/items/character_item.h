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

#ifndef GOLDBOX_DATA_ITEMS_CHARACTER_ITEM_H
#define GOLDBOX_DATA_ITEMS_CHARACTER_ITEM_H

#include "common/str.h"
#include "goldbox/engine.h"
#include "goldbox/data/items/base_items.h"

namespace Goldbox {
namespace Data {
namespace Items {

/// One entry in a character's inventory (.ITM) file.
struct CharacterItem {
    Common::String name;       ///< Pascal‐string (1-byte length + up to 44 chars)
    uint8        typeIndex;    ///< index into gItemProps.all()
    uint8        nameCode1;    ///< raw name‐flag at 0x2E
    uint8        nameCode2;    ///< raw name‐flag at 0x2F
    uint8        nameCode3;    ///< raw name‐flag at 0x30
    uint8        bonus;        ///< enchantment bonus at 0x31
    uint8        saveBonus;    ///< saving‐throw bonus at 0x32
    uint8        readied;      ///< equipped/readied flags at 0x33
    uint8        hidden;       ///< hide‐name flags at 0x34
    uint8        cursed;       ///< cursed flag at 0x35
    uint16       weight;       ///< at 0x36–0x37 (LE)
    uint8        stackSize;    ///< at 0x38
    uint16       value;        ///< at 0x39–0x3A (LE)
    uint8        effect1;      ///< at 0x3B
    uint8        effect2;      ///< at 0x3C
    uint8        effect3;      ///< at 0x3D

    /// Fetch the static base‐item data this refers to.
    const ItemProperty &prop() const;

    /// Get the name component for this item by index (0-2)
    Common::String getNameComponent(int componentIndex) const;
    
    /// Returns the full display name considering hidden flags
    Common::String getDisplayName() const;
    
    /// Determines if this item is a scroll based on its type
    bool isScroll() const;
    
    /// Gets number of charges for wands, potions, etc.
    int getCharges() const;
    
    /// Checks if this item is equipped
    bool isEquipped() const;
    
    /// Determines if this is a special effect item (effect3 >= 128)
    bool hasSpecialEffect() const;
    
    /// Get effect description (for special items)
    Common::String getEffectDescription() const;
};

} // namespace Items
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ITEMS_CHARACTER_ITEM_H