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

#include "goldbox/poolrad/views/dialogs/horizontal_input.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

HorizontalInput::HorizontalInput(const Common::String &name, const HorizontalInputConfig &config)
    : Dialog(name),
      _promptTxt(config.promptTxt),
      _promptColor(config.promptColor),
      _maxInputLength(config.maxInputLength),
      _inputText("") {
    activate();
    _text_offset = _promptTxt.empty() ? 0 : _promptTxt.size();
}

bool HorizontalInput::msgKeypress(const KeypressMessage &msg) {
    char asciiValue = msg.ascii;
    Common::KeyCode keyCode = msg.keycode;

    if (_isActive) {
        if (keyCode == Common::KEYCODE_RETURN || keyCode == Common::KEYCODE_ESCAPE) {
            if (_parent) {
                _parent->handleMenuResult(true, keyCode, 0);
            }
            return true;
        }
        if (asciiValue >= 'a' && asciiValue <= 'z') {
            asciiValue -= 32;
        }

        if (keyCode == Common::KEYCODE_BACKSPACE && !_inputText.empty()) {
            _inputText.deleteChar(_inputText.size() - 1);
        } else if (asciiValue >= ' ' && asciiValue <= '~' && _inputText.size() < _maxInputLength) {
            _inputText += asciiValue;
        }
        drawText();
    }
    return true;
}

void HorizontalInput::draw() {
    if (_isActive) {
        drawText();
    }
}

void HorizontalInput::clear() {
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 0);
}

void HorizontalInput::drawText() {
    Surface s = getSurface();
    clear();
    s.writeStringC(_promptTxt, _promptColor, 0, 24);
    s.writeStringC(_inputText, 15, _text_offset, 24);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
