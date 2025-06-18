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
    return Engine::gItemProps.all()[typeIndex];
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
    
    if (componentCode == 0)
        return "";
        
    return g_engine->getString(Common::String::format("itemnamecomponents.%d", componentCode));
}

Common::String CharacterItem::getDisplayName() const {
    // If name is explicitly defined, use it
    if (!name.empty())
        return name;
        
    Common::String result;
    
    // The first 3 bits of hidden control which name components are shown
    // If a bit is set (1), the component should be hidden
    for (int i = 0; i < 3; i++) {
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
    
    return result;
}

bool CharacterItem::isScroll() const {
    return typeIndex == 63;  // Index for scroll from your YAML
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