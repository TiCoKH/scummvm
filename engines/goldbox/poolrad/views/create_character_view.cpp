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
#include "goldbox/poolrad/views/create_character_view.h"


namespace Goldbox {
namespace Poolrad {
namespace Views {

const char PICK_RACE[] = "Pick Race";
const char PICK_GENDER[] = "Pick Gender";
const char PICK_CLASS[] = "Pick Class";
const char PICK_ALIGNMENT[] = "Pick Alignment";
const char KEEP_THIS[] = "Keep this character? ";
const char CHAR_NAME[] = "Character name: ";

CreateCharacterView::CreateCharacterView() : View("CreatCharacter"){}

bool CreateCharacterView::msgKeypress(const KeypressMessage &msg) {
	// Any keypress to close the view
	replaceView("Mainmenu");
	return true;
}

void CreateCharacterView::draw() {
	Surface s = getSurface();

    int rnd = getRandomNumber(12);

	drawWindow( 1, 1, 38, 22);

}

bool CreateCharacterView::msgFocus(const FocusMessage &msg) {
	return true;
}

bool CreateCharacterView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void CreateCharacterView::timeout() {
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
