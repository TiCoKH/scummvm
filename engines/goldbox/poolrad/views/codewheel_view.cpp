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

#include "common/system.h"
#include "graphics/palette.h"
//#include "goldbox/keymapping.h"
#include "goldbox/poolrad/views/codewheel_view.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/views/credits_view.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {


CodewheelView::CodewheelView() : View("Codewheel"), _retry(0){

	Dialogs::HorizontalInputConfig inputConfig = {
        VmInterface::getString("codewheel.input"),
        13,
        6
    };
    _inputPrompt = new Dialogs::HorizontalInput("InputPrompt", inputConfig);
}

CodewheelView::~CodewheelView() {
    delete _inputPrompt;
}

bool CodewheelView::msgKeypress(const KeypressMessage &msg) {

	Surface s = getSurface();

	if ((msg.keycode == Common::KEYCODE_RETURN) || (msg.keycode == Common::KEYCODE_KP_ENTER)) {
		_inputPrompt->deactivate();
		if (_inputPrompt->getInput() == passcodes[_code_index].answer) {
			s.clearBox(0, 24, 39, 24, 0);
			replaceView(viewAfterCodewheel());
		} else {
			_inputPrompt->clearInput();
			s.clearBox(0, 24, 39, 24, 0);
			s.writeStringC(0, 24, 14, VmInterface::getString("codewheel.incorrect"));
            delaySeconds(2);
		}
    } else {
		if (_inputPrompt->isActive()) {
			_inputPrompt->msgKeypress(msg);
		}
    }
    return true;
}

void CodewheelView::draw() {
	Surface s = getSurface();

    _retry++;
    if (_retry >= 3) {/*TODO:Quit*/};
    // GDConsts.wheel: 13 entries; the PC-98 table has no SAVIOR.
    if (g_engine->getPlatform() == Common::kPlatformPC98) {
        _code_index = getRandomNumber(11);
        if (_code_index >= kSaviorIndex)
            ++_code_index;
    } else {
        _code_index = getRandomNumber(12);
    }

	drawWindow( 1, 1, 38, 22);
	s.writeStringC(1, 1, 10, VmInterface::getString("codewheel.1"));
	s.writeStringC(1, 2, 10, VmInterface::getString("codewheel.2"));
	s.writeStringC(1, 5, 10, VmInterface::getString("codewheel.3"));
	s.writeRune(8, 6, 12, passcodes [_code_index].outer[0] + 0x40);
	s.writeRune(9, 6, 12, passcodes [_code_index].outer[1] + 0x40);
	s.writeStringC(1, 8, 10, VmInterface::getString("codewheel.4"));
	s.writeRune(8, 9, 12, passcodes [_code_index].inner[0] + 0x40);
    if (passcodes[_code_index].inner[1] != 0xFF) {
		s.writeRune(9, 9, 12, passcodes [_code_index].inner[1] + 0x40);
    }
	s.writeStringC(1, 11, 10, VmInterface::getString("codewheel.5"));
	s.writeStringC(6, 13, 12, passcodes [_code_index].path);
	s.writeStringC(1, 14, 10, VmInterface::getString("codewheel.6"));

	_inputPrompt->activate();
	_inputPrompt->draw();

}

bool CodewheelView::msgFocus(const FocusMessage &msg) {
	return true;
}

bool CodewheelView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void CodewheelView::timeout() {
	redraw();
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
