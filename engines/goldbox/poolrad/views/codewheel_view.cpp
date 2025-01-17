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

namespace Goldbox {
namespace Poolrad {
namespace Views {

const char USE_THE[] = "USE THE TRANSLATION WHEEL TO DECIPHER";
const char THE_CODE[] = "THE CODE WORD.";
const char OUTER_RING[] = "MATCH THE ESPRUAR RUNE (OUTER RING)";
const char INNER_RING[] = "WITH THE DETHEK RUNE (INNER RING)";
const char THE_PATH[] = "AND READ THE CODE WORD UNDER THE PATH:";
const char READ_FROM[] = "READ FROM THE INSIDE TO THE OUTSIDE.";
const char INPUT[] = "INPUT THE CODE WORD: ";
const char INNCORRECT[] = "Sorry, that's incorrect.";

CodewheelView::CodewheelView() : View("Codewheel"), _retry(0){

	Dialogs::HorizontalInputConfig inputConfig = {
        INPUT,
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
			replaceView("Mainmenu");
		} else {
			_inputPrompt->clearInput();
			s.clearBox(0, 24, 39, 24, 0);
			s.writeStringC(INNCORRECT, 14, 0, 24);
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
    _code_index = getRandomNumber(12);

	drawWindow( 1, 1, 38, 22);
	s.writeStringC(USE_THE, 10, 1, 1);
    s.writeStringC(THE_CODE, 10, 1, 2);
    s.writeStringC(OUTER_RING, 10, 1, 5);
    s.writeGlyphC(passcodes[_code_index].outer[0]+0x40, 12, 8, 6);
    s.writeGlyphC(passcodes[_code_index].outer[1]+0x40, 12, 9, 6);
    s.writeStringC(INNER_RING, 10, 1, 8);
    s.writeGlyphC(passcodes[_code_index].inner[0]+0x40, 12, 8, 9);
    if (passcodes[_code_index].inner[1] != 0xFF) {
        s.writeGlyphC(passcodes[_code_index].inner[1]+0x40, 12, 9, 9);
    }
    s.writeStringC(THE_PATH, 10, 1, 11);
    s.writeStringC(passcodes[_code_index].path, 12, 6, 13);
    s.writeStringC(READ_FROM, 10, 1, 14);

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
