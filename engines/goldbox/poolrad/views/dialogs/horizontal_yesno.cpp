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

#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

HorizontalYesNo::HorizontalYesNo(const Common::String &name, const HorizontalYesNoConfig &config)
        : Dialog(name),
            _textColor(config.textColor),
            _selectColor(config.selectColor),
            _promptColor(config.promptColor),
            _promptTxt(config.promptTxt) {
    // Build internal YES/NO menu using MenuItemList like HorizontalMenu
    Common::Array<Common::String> items;
    items.push_back("YES");
    items.push_back("NO");
    _menuItems.generateMenuItems(items, true); // generate shortcuts 'Y' and 'N'
    _menuItems.currentSelection = 0; // default to YES
    activate();
}

bool HorizontalYesNo::msgKeypress(const KeypressMessage &msg) {
    char asciiValue = msg.ascii;
    Common::KeyCode keyCode = msg.keycode;

    if (!_isActive)
        return true;

    // Normalize a-z to A-Z for comparison
    if (asciiValue >= 'a' && asciiValue <= 'z')
        asciiValue -= 32;

    // Left/Right style navigation similar to HorizontalMenu
    if (keyCode == Common::KEYCODE_LEFT || keyCode == Common::KEYCODE_COMMA) {
        _menuItems.prevActive();
        _redraw = true;
    } else if (keyCode == Common::KEYCODE_RIGHT || keyCode == Common::KEYCODE_PERIOD) {
        _menuItems.nextActive();
        _redraw = true;
    } else if (keyCode == Common::KEYCODE_SPACE) {
        _menuItems.currentSelection = 0; // default to YES
        _redraw = true;
    } else if (keyCode == Common::KEYCODE_RETURN) {
        // Map selection to y/n and send to parent
        if (_parent) {
            if (_menuItems.currentSelection == 0)
                _parent->handleMenuResult(true, Common::KEYCODE_y, 1);
            else
                _parent->handleMenuResult(true, Common::KEYCODE_n, 0);
        }
        return true;
    } else if (asciiValue == 'Y') {
        _menuItems.currentSelection = 0;
        if (_parent)
            _parent->handleMenuResult(true, Common::KEYCODE_y, 1);
        return true;
    } else if (asciiValue == 'N') {
        _menuItems.currentSelection = 1;
        if (_parent)
            _parent->handleMenuResult(true, Common::KEYCODE_n, 0);
        return true;
    } else {
        // Ignore other keys
        return true;
    }

    if (_redraw)
        drawText();
    return true;
}

void HorizontalYesNo::draw() {
    if (_isActive)
        drawText();
}

void HorizontalYesNo::clear() {
    Surface s = getSurface();
    // Clear the prompt line (same placement as HorizontalInput)
    s.clearBox(0, 24, 39, 24, 0);
}

void HorizontalYesNo::drawText() {
    Surface s = getSurface();
    if (!_redraw)
        return;
    clear();
    // Show the prompt text; caller should include any punctuation like '?' if desired
    s.writeStringC(_promptTxt, _promptColor, 0, 24);
    s.setTextPos(_promptTxt.size(), 24);
    // Render options using MenuItemList like HorizontalMenu
    for (uint i = 0; i < _menuItems.items.size(); i++) {
        const MenuItem &item = _menuItems.items[i];
        s.writeChar(' ');
        if (i == (uint)_menuItems.currentSelection) {
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
    _redraw = false;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
