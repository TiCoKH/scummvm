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
        MenuItem item = {shortcut, menuText, true, generateShortcuts};
        items.push_back(item);
    }
    if (generateShortcuts) {
        for (uint i = 0; i < items.size(); i++) {
            generateShortcut(i);
        }
    }
}

void MenuItemList::generateShortcut(uint index) {
    if (index >= items.size())
        return;

    MenuItem &item = items[index];
    item.shortcut = item.text.c_str()[0];
    item.text = item.text.substr(1);
}

void MenuItemList::setShortcutToLast(uint index) {
    if (index >= 0 && index < items.size()) {
        MenuItem &item = items[index];
        size_t length = item.text.size();

        if (length > 0) {
            Common::String newText = Common::String(item.shortcut) + item.text;
            item.shortcut = newText[newText.size() - 1];
            item.text = newText.substr(0, newText.size() - 1);
            item.shortcutFirst = false;
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
            return i;
        }
    }
    return -1;
}

bool MenuItemList::isActiveShortcut(char shortcut) const {
    for (uint i = 0; i < items.size(); i++) {
        if (items[i].shortcut == shortcut && items[i].active) {
            return true;
        }
    }
    return false;
}

void MenuItemList::next() {
    currentSelection = (currentSelection + 1) % items.size();
}

void MenuItemList::prev() {
    currentSelection = (currentSelection == 0) ? items.size() - 1 : currentSelection - 1;

}

void MenuItemList::nextActive() {
    do {
        currentSelection = (currentSelection + 1) % items.size();
    } while (!items[currentSelection].active);
}

void MenuItemList::prevActive() {
    do {
        currentSelection = (currentSelection == 0) ? items.size() - 1 : currentSelection - 1;
    } while (!items[currentSelection].active);
}

void MenuItemList::push_back(const Common::String &text) {
    MenuItem item;
    item.text = text;
    item.shortcut = '\0';
    item.active = true;
    item.shortcutFirst = true;
    items.push_back(item);
}

} // namespace Goldbox
