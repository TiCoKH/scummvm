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

HorizontalMenu::HorizontalMenu(const Common::String &name, const HorizontalMenuConfig &config)
    : Dialog(name),
      _menuItems(config.menuItemList),
      _textColor(config.textColor),
      _selectColor(config.selectColor),
      _promptColor(config.promptColor),
      _allowNumPad(config.allowNumPad),
      _promptTxt(config.promptTxt) {
    assert(_menuItems != nullptr);
}

void HorizontalMenu::draw() {
    drawText();
}

void HorizontalMenu::drawText() {

    if (!_redraw) return;

    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 0);
    s.writeStringC(_promptTxt, _promptColor, 0, 24);

	s.setTextPos(_promptTxt.size(), 24);
    for (uint i = 0; i < _menuItems->items.size(); i++) {
        const MenuItem &item = _menuItems->items[i];
        s.writeChar(' ');

        if (i == _menuItems->currentSelection) {
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

bool HorizontalMenu::msgKeypress(const KeypressMessage &msg) {
    Common::KeyCode keyCode = msg.keycode;
    char asciiValue = (msg.ascii >= 'a' && msg.ascii <= 'z') ? msg.ascii - 32 : msg.ascii;

    switch (keyCode) {
        case Common::KEYCODE_COMMA: {
			_menuItems->prevActive();
            _redraw = true;
            break;
        }
        case Common::KEYCODE_PERIOD: {
			_menuItems->nextActive();
            _redraw = true;
            break;
        }
        case Common::KEYCODE_SPACE: {
            _menuItems->currentSelection = 0;
            _redraw = true;
            break;
        }
        case Common::KEYCODE_RETURN: {
            _parent->handleMenuResult(true, keyCode, 0);
            return true;
        }
        case Common::KEYCODE_ESCAPE: {
            _parent->handleMenuResult(false, keyCode, 0);
            return true;
        }
    }

    if ((asciiValue >= 'A' && asciiValue <= 'Z') || (asciiValue >= '0' && asciiValue <= '9')) {
        int index = _menuItems->findByShortcut(asciiValue);
        if (index != -1 && _menuItems->items[index].active) {
            _menuItems->currentSelection = index;
            _parent->handleMenuResult(true, keyCode, index);
            deactivate();
            return true;
        }
        if (_allowNumPad) {
        switch (asciiValue) {
            case '1': {
                keyCode = Common::KEYCODE_END;
                break;
            }
            case '2': {
                keyCode = Common::KEYCODE_DOWN;
                break;
            }
            case '3': {
                keyCode = Common::KEYCODE_PAGEDOWN;
                break;
            }
            case '4': {
                keyCode = Common::KEYCODE_LEFT;
                break;
            }
            case '5': {
                keyCode = Common::KEYCODE_SPACE;
                asciiValue = ' ';
                break;
            }
            case '6': {
                keyCode = Common::KEYCODE_RIGHT;
                break;
            }
            case '7': {
                keyCode = Common::KEYCODE_HOME;
                break;
            }
            case '8': {
                keyCode = Common::KEYCODE_UP;
                break;
            }
            case '9': {
                keyCode = Common::KEYCODE_PAGEUP;
                break;
            }
        }
        _parent->handleMenuResult(true, keyCode, 0);
        deactivate();
        return true;
        }
    }


    if (keyCode >= Common::KEYCODE_UP && keyCode <= Common::KEYCODE_PAGEDOWN) {
        _parent->handleMenuResult(true, keyCode, 0);
        deactivate();
        return true;
    }

    if (_redraw) drawText();
    return true;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

