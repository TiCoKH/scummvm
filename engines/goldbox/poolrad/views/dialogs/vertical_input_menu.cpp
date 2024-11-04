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

 #include "goldbox/poolrad/views/dialogs/vertical_input_menu.h"
 #include "goldbox/poolrad/views/dialogs/horizontal_input_menu.h"
#include "vertical_input_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

VerticalInputMenu::VerticalInputMenu(const Common::String &name, const Common::String &promptTxt, 
                                     const Common::Array<Common::String> &menuStrings, int textColor, 
                                     int selectColor, int promptColor)
    : VerticalInput(name, promptColor, promptTxt), _textColor(textColor), _selectColor(selectColor) {
    
    _menuItems.generateMenuItems(menuStrings, false);

    // Instantiate HorizontalInputMenu tied to the vertical menu
    Common::Array<Common::String> horizontalMenuOptions = {"Exit", "Prev", "Next"};
    _horizontalMenu = new HorizontalInputMenu("Navigation", "Choose:", horizontalMenuOptions, _textColor, _selectColor, _promptColor);
}

VerticalInputMenu::~VerticalInputMenu() {
    delete _horizontalMenu;
}

void VerticalInputMenu::draw() {
    // Draw the vertical menu
    VerticalInput::draw();

    // Draw the horizontal menu tied to it
    _horizontalMenu->draw();
}

void VerticalInputMenu::drawText() {
    Surface s = getSurface();
    s.writeStringC(_promptTxt, _promptColor, 0, 24);

    // Draw the menu items vertically
    for (uint i = 0; i < _menuItems.items.size(); i++) {
        const MenuItem &item = _menuItems.items[i];
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
        s.setTextPos(0, 24 + (i + 1) * 2);  // Move cursor to the next line for vertical display
    }
}


void VerticalInputMenu::selectNextItem() {
    do {
        _menuItems.currentSelection = (_menuItems.currentSelection + 1) % _menuItems.items.size();
    } while (!_menuItems.items[_menuItems.currentSelection].active);
}

void VerticalInputMenu::selectPreviousItem() {
    do {
        _menuItems.currentSelection = (_menuItems.currentSelection > 0) ? _menuItems.currentSelection - 1 : _menuItems.items.size() - 1;
    } while (!_menuItems.items[_menuItems.currentSelection].active);
}

bool VerticalInputMenu::msgKeypress(const KeypressMessage &msg) {
    Common::KeyCode keyCode = msg.keycode;
    char asciiValue = msg.ascii;

    if (asciiValue >= 'a' && asciiValue <= 'z') {
        asciiValue -= 32;  // Convert to uppercase
    }

    if (keyCode == Common::KEYCODE_UP) {
        selectPreviousItem();
    } else if (keyCode == Common::KEYCODE_DOWN) {
        selectNextItem();
    } else if (keyCode == Common::KEYCODE_RETURN) {
        deactivate();  // Finalize selection
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
