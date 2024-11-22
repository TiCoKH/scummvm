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

#include "goldbox/core/menu_item.h"

namespace Goldbox {

MenuItem MenuItemList::getCurrentSelection() const {
    return items[currentSelection];
}

void MenuItemList::generateMenuItems(const Common::Array<Common::String> &menuStrings, bool generateShortcuts) {
    for (uint8 i = 0; i < menuStrings.size(); i++) {
        char shortcut = '\0';
        Common::String menuText = menuStrings[i];

        if (generateShortcuts) {
            shortcut = menuStrings[i].c_str()[0];
            menuText = menuStrings[i].substr(1);
        }

        MenuItem item = {shortcut, menuText, true, generateShortcuts};
        items.push_back(item);
    }
}

void MenuItemList::setShortcutToLast(uint index) {
    if (index >= 0 && index < items.size()) {
        MenuItem &item = items[index];
        size_t length = item.text.size();

        if (length > 0) {
            // Merge current shortcut with the rest of the text
            Common::String newText = Common::String(item.shortcut) + item.text;

            // Set last character as shortcut and remove it from the text
            item.shortcut = newText[newText.size() - 1];
            item.text = newText.substr(0, newText.size() - 1);

            item.shortcutFirst = false;  // Set shortcutFirst to false
        }
    }
}

void MenuItemList::activate(uint index) {
    items[index].active = true;
}

void MenuItemList::deactivate(uint index) {
    items[index].active = false;
}

bool MenuItemList::isActive(uint index) {
    return items[index].active;
}

int MenuItemList::findByShortcut(char shortcut) const {
    for (uint i = 0; i < items.size(); i++) {
        if (items[i].shortcut == shortcut && items[i].active) {
            return i;  // Return index if found and active
        }
    }
    return -1;  // Return -1 if not found
}

bool MenuItemList::isActiveShortcut(char shortcut) const {
    for (uint i = 0; i < items.size(); i++) {
        if (items[i].shortcut == shortcut && items[i].active) {
            return true;
        }
    }
    return false;
}

} // namespace Goldbox
