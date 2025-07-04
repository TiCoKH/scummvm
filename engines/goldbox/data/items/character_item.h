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
    Common::String name;       ///< 0x00 Pascal‐string (1-byte length + up to 41 chars)
    uint32       nextAddress;  ///< 0x2A-0x2D next item address segment (LE) (not needed)
    uint8        typeIndex;    ///< 0x2E index into gItemProps.all()
    uint8        nameCode1;    ///< 0x2F raw name‐flag1
    uint8        nameCode2;    ///< 0x30 raw name‐flag2 
    uint8        nameCode3;    ///< 0x31 raw name‐flag2 
    uint8        bonus;        ///< 0x32 enchantment bonus
    uint8        saveBonus;    ///< 0x33 saving‐throw bonus
    uint8        readied;      ///< 0x34 equipped/readied flags
    uint8        hidden;       ///< 0x35 hide‐name flags
    uint8        cursed;       ///< 0x36 cursed flag
    uint16       weight;       ///< 0x37–0x38 (LE)
    uint8        stackSize;    ///< 0x39
    uint16       value;        ///< 0x3A–0x3B (LE)
    uint8        effect1;      ///< 0x3C
    uint8        effect2;      ///< 0x3D
    uint8        effect3;      ///< 0x3E

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

    /// Debug print for CharacterItem  
    void debugPrint(int index = -1) const {  
        if (index >= 0)  
            debug("CharacterItem[%d]:", index);  
        debug("  Name: %s, TypeIndex: %u, Bonus: %u, Readied: %u, Hidden: %u, Cursed: %u, Weight: %u, Stack: %u, Value: %u, Effects: %u,%u,%u",  
            name.c_str(), typeIndex, bonus, readied, hidden, cursed, weight, stackSize, value, effect1, effect2, effect3);  
    }
};

} // namespace Items
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ITEMS_CHARACTER_ITEM_H
