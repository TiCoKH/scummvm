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
      _promptColor(config.promptColor),
      _promptTxt(config.promptTxt) {
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

    // Accept only Y as Yes
    if (asciiValue == 'Y') {
        if (_parent) {
            // value = 1 for Yes
            _parent->handleMenuResult(true, Common::KEYCODE_y, 1);
        }
        return true;
    }

    // Accept only N as No
    if (asciiValue == 'N') {
        if (_parent) {
            // value = 0 for No
            _parent->handleMenuResult(true, Common::KEYCODE_n, 0);
        }
        return true;
    }

    // Ignore other keys
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
    clear();
    // Show the prompt text; caller should include any punctuation like '?' if desired
    s.writeStringC(_promptTxt, _promptColor, 0, 24);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
