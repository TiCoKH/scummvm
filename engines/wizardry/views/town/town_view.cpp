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

#include "wizardry/views/town/town_view.h"
#include "wizardry/data/globals.h"

namespace Wizardry {
namespace Town {

void TownView::drawCells(TextWindow &window) {
	TownScreens screens(_G(cells), _G(messages), _G(party), _G(items));
	drawBackground(screens);

	// The menu is printed again by drawTown() where there is one
	_menu = Menu();
	drawTown(screens);

	if (!_notice.empty())
		screens.drawNotice(_notice);
}

void TownView::drawBackground(TownScreens &screens) {
	screens.drawBackground();
}

void TownView::notice(const Common::String &text) {
	_notice = text;
	delaySeconds(2);
	redraw();
}

void TownView::timeout() {
	_notice.clear();
	noticeDone();
	redraw();
}

uint16 TownView::keyToAscii(const KeypressMessage &msg) {
	uint16 ascii = msg.ascii;
	if (msg.keycode == Common::KEYCODE_RETURN || msg.keycode == Common::KEYCODE_KP_ENTER)
		ascii = kKeyReturn;
	else if (msg.keycode == Common::KEYCODE_BACKSPACE)
		ascii = kKeyBackspace;
	else if (msg.keycode == Common::KEYCODE_ESCAPE)
		ascii = kKeyEscape;
	else if (msg.keycode == Common::KEYCODE_UP)
		ascii = kKeyUp;
	else if (msg.keycode == Common::KEYCODE_DOWN)
		ascii = kKeyDown;
	else if (msg.keycode == Common::KEYCODE_LEFT)
		ascii = kKeyLeft;
	else if (msg.keycode == Common::KEYCODE_RIGHT)
		ascii = kKeyRight;
	else if (ascii >= 'a' && ascii <= 'z')
		ascii -= 0x20;

	return ascii;
}

bool TownView::msgKeypress(const KeypressMessage &msg) {
	if (!_notice.empty())
		return true;

	const uint16 ascii = keyToAscii(msg);
	if (!ascii)
		return true;

	choose(_menu.choose(ascii, _G(party).size()), ascii);
	return true;
}

bool TownView::msgMouseDown(const MouseDownMessage &msg) {
	if (!_notice.empty() || msg._button != MouseMessage::MB_LEFT)
		return true;

	const int col = msg._pos.x / _G(renderer).cellWidth();
	const int row = msg._pos.y / _G(renderer).cellHeight();
	const int result = _menu.chooseAt(col, row);
	if (result >= 0)
		choose(result, 0);
	return true;
}

} // End of namespace Town
} // End of namespace Wizardry
