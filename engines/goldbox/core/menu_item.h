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

#ifndef GOLDBOX_MENU_ITEM_H
#define GOLDBOX_MENU_ITEM_H

#include "common/array.h"
#include "common/str.h"

namespace Goldbox {

struct MenuItem {
    char shortcut;
    Common::String text;
    bool active = true;
    bool shortcutFirst = true;
};

struct MenuItemList {
    Common::Array<MenuItem> items;
    uint currentSelection;

    MenuItemList() : currentSelection(0) {}

    MenuItem getCurrentSelection() const;

    void push_back(const Common::String &text);

    void generateMenuItems(const Common::Array<Common::String> &menuStrings, bool generateShortcuts);
    void generateShortcut(uint index);
    void setShortcutToLast(uint index);
    void activate(uint index);
    void deactivate(uint index);
    bool isActive(uint index);
    int findByShortcut(char shortcut) const;
    bool isActiveShortcut(char shortcut) const;
    void next();
    void prev();
    void nextActive();
    void prevActive();
};

} // namespace Goldbox

#endif // GOLDBOX_MENU_ITEM_H
