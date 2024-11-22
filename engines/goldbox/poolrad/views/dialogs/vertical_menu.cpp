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

#include "goldbox/poolrad/views/dialogs/vertical_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

VerticalMenu::VerticalMenu(const Common::String &name, const Common::String &promptTxt,
                             const Common::Array<Common::String> &menuStrings, int textColor,
                             int selectColor, int promptColor)
    : Dialog(name), _horizontalMenu(nullptr), _textColor(textColor),
      _selectColor(selectColor), _promptColor(promptColor), _promptTxt(promptTxt) {
    activate();
    _menuItems.generateMenuItems(menuStrings, false);

    // Optional horizontal menu
    Common::Array<Common::String> horizontalMenuOptions = {"Exit", "Prev", "Next"};
    _horizontalMenu = new HorizontalMenu("Navigation", "Choose:", horizontalMenuOptions, _textColor, _selectColor, _promptColor);
}

VerticalMenu::~VerticalMenu() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
}

void VerticalMenu::draw() {
    if (_isActive) {
        Surface s = getSurface();
        s.writeStringC(_promptTxt, _promptColor, 0, 24);
        drawText();

        if (_horizontalMenu) {
            _horizontalMenu->draw();
        }
    }
}

void VerticalMenu::drawText() {
    Surface s = getSurface();

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

void VerticalMenu::selectNextItem() {
    do {
        _menuItems.currentSelection = (_menuItems.currentSelection + 1) % _menuItems.items.size();
    } while (!_menuItems.items[_menuItems.currentSelection].active);
}

void VerticalMenu::selectPreviousItem() {
    do {
        _menuItems.currentSelection = (_menuItems.currentSelection > 0) ? _menuItems.currentSelection - 1 : _menuItems.items.size() - 1;
    } while (!_menuItems.items[_menuItems.currentSelection].active);
}

bool VerticalMenu::msgKeypress(const KeypressMessage &msg) {
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

    draw();
    return true;
}

void VerticalMenu::setMenuItemShortcut(int index, char newShortcut) {
    if (index >= 0 && index < _menuItems.items.size()) {
        _menuItems.items[index].shortcut = newShortcut;
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

