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

#ifndef GOLDBOX_DATA_ITEMS_CHARACTER_INVENTORY_H
#define GOLDBOX_DATA_ITEMS_CHARACTER_INVENTORY_H

#include "common/array.h"
#include "common/str.h"
#include "goldbox/data/items/character_item.h"

namespace Goldbox {
namespace Data {
namespace Items {

/// Manages a character’s dynamic inventory file (.ITM).
class CharacterInventory {
public:
    enum { kDefaultMaxItems = 16 };

    struct Rules {
        uint8 allowedClassMask;   ///< Bitmask of classes the character can use
        uint16 maxEncumbrance;    ///< Base carry capacity (0 = ignore)
        int capacityModifier;     ///< Strength tier modifier (can be negative)
        uint8 maxItems;           ///< Max items (default 16)

        Rules() : allowedClassMask(0xFF), maxEncumbrance(0),
            capacityModifier(0), maxItems(kDefaultMaxItems) {}
    };

    /// Reads every 63‐byte record from `filename`; leaves `_items` empty if the file is missing.
    bool load(const Common::String &filename);

    /// Writes back in the same layout (optional).
    bool save(const Common::String &filename) const;

    void clear() { _items.clear(); }

    void debugPrint() const;

    /// Inventory logic helpers (modernized behavior with legacy rules)
    bool canCarryItem(const CharacterItem &item, const Rules &rules) const;
    bool addItem(const CharacterItem &item, const Rules &rules,
                 Common::Array<CharacterItem *> *equippedSlots);
    bool removeItem(CharacterItem *item,
                    Common::Array<CharacterItem *> *equippedSlots);

    bool equipItem(CharacterItem *item, Slot slot,
                   Common::Array<CharacterItem *> &equippedSlots,
                   uint8 *handsUsed, uint8 *saveBonus);
    bool unequipItem(Slot slot,
                     Common::Array<CharacterItem *> &equippedSlots,
                     uint8 *handsUsed, uint8 *saveBonus);

    CharacterItem *findByLegacyAddress(uint32 legacyAddr);
    const CharacterItem *findByLegacyAddress(uint32 legacyAddr) const;

    uint16 totalEncumbrance() const;
    void recomputeEquippedTotals(
        const Common::Array<CharacterItem *> &equippedSlots,
        uint8 *handsUsed, uint8 *saveBonus) const;

    /// Access loaded items.
    const Common::Array<CharacterItem> &all() const { return _items; }
    int                                count() const { return _items.size(); }
    CharacterItem                      &operator[](size_t i) {
        return _items[i];
    }
    const CharacterItem                &operator[](size_t i) const {
        return _items[i];
    }
    const Common::Array<CharacterItem> &items() const { return _items; }
    Common::Array<CharacterItem>       &items() { return _items; }

private:
    static uint16 calcItemEncumbrance(const CharacterItem &item);
    static bool meetsClassRestriction(const CharacterItem &item,
                                      const Rules &rules);
    static void captureEquippedLegacyOffsets(
        const Common::Array<CharacterItem *> &equippedSlots,
        Common::Array<uint32> &offsetsOut);
    void remapEquippedByLegacyOffsets(
        Common::Array<CharacterItem *> *equippedSlots,
        const Common::Array<uint32> &offsets);

    Common::Array<CharacterItem> _items;
};

} // namespace Items
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_ITEMS_CHARACTER_INVENTORY_H
