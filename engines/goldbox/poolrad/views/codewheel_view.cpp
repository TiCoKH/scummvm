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
#include "goldbox/keymapping.h"
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

CodewheelView::CodewheelView() : View("Codewheel"){}

bool CodewheelView::msgKeypress(const KeypressMessage &msg) {
	// Any keypress to close the view
	close();
	return true;
}

void CodewheelView::draw() {
	Surface s = getSurface();

    _retry++;
    int rnd = getRandomNumber(12);

	drawWindow(22, 38, 1, 1);
	s.writeStringC(USE_THE, 10, 1, 1);
    s.writeStringC(THE_CODE, 10, 2, 1);
    s.writeStringC(OUTER_RING, 10, 5, 1);
    s.writeGlyphC(passcodes[rnd].outer[0]+0x40, 12, 6, 8);
    s.writeGlyphC(passcodes[rnd].outer[1]+0x40, 12, 6, 9);
    s.writeStringC(INNER_RING, 10, 8, 1);
    s.writeGlyphC(passcodes[rnd].inner[0]+0x40, 12, 9, 8);
    if (passcodes[rnd].inner[1] != 0xFF) {
        s.writeGlyphC(passcodes[rnd].inner[1]+0x40, 12, 9, 9);
    }
    s.writeStringC(THE_PATH, 10, 11, 1);
    s.writeStringC(passcodes[rnd].path, 12, 13, 6);
    s.writeStringC(READ_FROM, 10, 14, 1);
//    s.writeStringC(INPUT, 13, 24, 1);
    //s.writeStringC(INNCORRECT, 14, 7, 2);
}

bool CodewheelView::msgFocus(const FocusMessage &msg) {
	return true;
}

bool CodewheelView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void CodewheelView::timeout() {
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
