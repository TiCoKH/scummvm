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

#include "goldbox/poolrad/views/dialogs/set_icon.h"
#include "common/keyboard.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Common::String;
using Common::KeyCode;
using Goldbox::Poolrad::Data::PoolradCharacter;

SetIcon::SetIcon(const String &name, PoolradCharacter *pc)
    : Dialog(name), _pc(pc) {
}

SetIcon::~SetIcon() {
}

bool SetIcon::msgKeypress(const KeypressMessage &msg) {
    // Handle icon selection keypresses here
    if (msg.keycode == Common::KEYCODE_ESCAPE) {
        if (_parent)
            _parent->handleMenuResult(false, msg.keycode, -1);
        return true;
    }
    if (msg.keycode == Common::KEYCODE_RETURN) {
        if (_parent)
            _parent->handleMenuResult(true, msg.keycode, -1);
        return true;
    }
    return true;
}

void SetIcon::draw() {
    Surface s = getSurface();
    s.clearBox(0, 0, 40, 25, kBackgroundColor);
    drawWindowC(kWindowLeft, kWindowTop, kWindowRight, kWindowBottom, kBackgroundColor);
    
    // Add some test text so we can see if drawing is working
    s.writeStringC("SELECT ICON", 15, 15, 10);
    s.writeStringC("Press RETURN to confirm", 10, 8, 15);
    s.writeStringC("Press ESC to cancel", 10, 9, 16);
}

void SetIcon::handleMenuResult(bool success, Common::KeyCode key, short value) {
    if (_parent)
        _parent->handleMenuResult(success, key, value);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
