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

#include "goldbox/data/items/character_item.h"

namespace Goldbox {
namespace Data {
namespace Items {

const ItemProperty &CharacterItem::prop() const {
    const auto &all = Engine::gItemProps.all();
    if (typeIndex >= all.size()) {
        warning("CharacterItem::prop: typeIndex %u out of range (max %u)",
                (unsigned)typeIndex, (unsigned)all.size());
        // Return first item as fallback
        static ItemProperty dummy;
        return dummy;
    }
    const ItemProperty &p = all[typeIndex];
    return p;
}

Common::String CharacterItem::getNameComponent(int componentIndex) const {
    uint8 componentCode = 0;

    switch (componentIndex) {
    case 0:
        componentCode = nameCode1;
        break;
    case 1:
        componentCode = nameCode2;
        break;
    case 2:
        componentCode = nameCode3;
        break;
    default:
        return "";
    }

    return ItemNameComponents::getComponent(componentCode);
}

Common::String CharacterItem::getDisplayName() const {
    Common::String result;

    // Build name exclusively from nameCode components and hidden flags.
    // The 'name' field in .ITM is just a cached display buffer from the original engine
    // and should NOT be used for display in our implementation.

    // Bits 0-2 of 'hidden' control which name components are shown
    // If a bit is set (1), that component should be hidden
    // Use reverse order (2,1,0) to match original item naming layout.
    for (int i = 2; i >= 0; --i) {
        if (hidden & (1 << i))
            continue;  // Skip this component if hidden bit is set

        Common::String component = getNameComponent(i);
        if (!component.empty()) {
            if (!result.empty())
                result += " ";
            result += component;
        }
    }

    // Add bonus indicator if it's greater than 0 and not hidden by bit 3
    if (bonus > 0 && !(hidden & 0x8)) {
        result += Common::String::format(" +%d", bonus);
    }

    // Prefix stack count for stackable items (e.g., "11 Darts")
    if (stackSize > 1 && !result.empty()) {
        // Simple pluralization: append 's' if not already ending with s/S
        const char lastChar = result[result.size() - 1];
        if (lastChar != 's' && lastChar != 'S')
            result += "s";
        result = Common::String::format("%u ", (unsigned)stackSize) + result;
    }

    return result;
}

constexpr uint8 SCROLL_TYPE_INDEX = 63;

bool CharacterItem::isScroll() const {
    return typeIndex == SCROLL_TYPE_INDEX;
}

int CharacterItem::getCharges() const {
    // For scrolls, the number of charges typically indicates number of spells
    if (isScroll())
        return effect1;

    // For wands and similar items that use charges
    if (effect3 < 128)
        return effect1;

    return 0;
}

bool CharacterItem::isEquipped() const {
    return readied != 0;
}

bool CharacterItem::hasSpecialEffect() const {
    return effect3 >= 128;
}

Common::String CharacterItem::getEffectDescription() const {
    if (hasSpecialEffect()) {
        // Lookup specific effects based on effect2 and your YAML data
        // This would need additional entries in your YAML for effects
        return g_engine->getString(Common::String::format("itemeffects.%d", effect2));
    }
    return "";
}

} // namespace Items
} // namespace Data
} // namespace Goldbox