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

#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

HorizontalMenu::HorizontalMenu(const Common::String &name, const Common::String &promptTxt,
                                         const Common::Array<Common::String> &menuStrings, int textColor, int selectColor, int promptColor) 
                                             : Dialog(name), _textColor(textColor),
      _selectColor(selectColor), _promptColor(promptColor), _promptTxt(promptTxt) {
    _menuItems.generateMenuItems(menuStrings, true);
}

void HorizontalMenu::drawText() {
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 0);
    s.writeStringC(_promptTxt, _promptColor, 0, 24);

    // Draw the menu items with a space between each one
    for (uint i = 0; i < _menuItems.items.size(); i++) {
        const MenuItem &item = _menuItems.items[i];
        s.writeChar(' ');

        if (i == _menuItems.currentSelection) {
            s.writeCharC(item.shortcut, _selectColor);
            s.writeStringC(item.text, _selectColor);
        } else {
            if (item.shortcutFirst) {
                s.writeCharC(item.shortcut, _selectColor);
                s.writeStringC(item.text, _textColor);
            } else {
                s.writeStringC(item.text, _textColor);
                s.writeCharC(item.shortcut, _selectColor);
            }
        }
    }
}


void HorizontalMenu::selectNextItem() {
    do {
        _menuItems.currentSelection = (_menuItems.currentSelection + 1) % _menuItems.items.size();
    } while (!_menuItems.items[_menuItems.currentSelection].active);
}

void HorizontalMenu::selectPreviousItem() {
    do {
        _menuItems.currentSelection = (_menuItems.currentSelection > 0) ? _menuItems.currentSelection - 1 : _menuItems.items.size() - 1;
    } while (!_menuItems.items[_menuItems.currentSelection].active);
}

bool HorizontalMenu::msgKeypress(const KeypressMessage &msg) {
    Common::KeyCode keyCode = msg.keycode;
    char asciiValue = msg.ascii;

    if (asciiValue >= 'a' && asciiValue <= 'z') {
        asciiValue -= 32;
    }

    if (keyCode == Common::KEYCODE_UP) {
        selectPreviousItem();
    } else if (keyCode == Common::KEYCODE_DOWN) {
        selectNextItem();
    } else if (keyCode == Common::KEYCODE_RETURN) {
        deactivate();
    }

    if ((asciiValue >= 'A' && asciiValue <= 'Z') || (asciiValue >= '0' && asciiValue <= '9')) {
        int index = _menuItems.findByShortcut(asciiValue);
        if (index != -1) {
            _menuItems.currentSelection = index;
        }
    }

    drawText();
    return true;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

