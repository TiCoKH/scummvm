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

#include "wizardry/ui/town_screens.h"

namespace Wizardry {

// Message numbers of Wizardry I
enum {
	kMsgArmorClassLow = 400,     // "LO"
	kMsgPoison = 401,
	kMsgPartyHeader = 402,
	kMsgCastle = 403,
	kMsgCastleMenu1 = 404,
	kMsgCastleMenu2 = 405,
	kMsgLetterSelects = 406,
	kMsgRemoveWho = 413,
	kMsgReturnToLeave = 414,
	kMsgTavern = 415,
	kMsgTavernMenu = 416,
	kMsgAlignmentNames = 10009,  // + alignment
	kMsgClassNames = 10030,      // + class
	kMsgStatusNames = 10040      // + status
};

TextWindow TownScreens::openWindow(int x, int y, int width, int height) {
	TextWindow window(_screen, Common::Rect(x, y, x + width, y + height), true);
	window.clear();
	return window;
}

void TownScreens::drawBackground() {
	_screen.clear();

	// The main window is empty in town
	openWindow(1, 1, 38, 22);

	const Common::String &title = _messages.get(kMsgCastle);
	TextWindow heading(_screen, TextWindow::centeredBounds(title.size() - 1, 0, 3), true);
	heading.clear();
	heading.PRINTSTR(title);

	drawParty();
}

void TownScreens::drawParty() {
	TextWindow window = openWindow(0, 16, 40, 9);
	window.PRINTSTR(0, 0, _messages.get(kMsgPartyHeader));

	for (uint i = 0; i < _party.size(); ++i)
		printPartyLine(window, i);
}

void TownScreens::printPartyLine(TextWindow &window, uint index) {
	const Character &c = _party.member(index);

	window.MVCURSOR(0, index + 1);
	window.printInt(index + 1, 1);
	window.PRINTCHR(' ');
	window.PRINTSTR(c.name);
	while (window.cursorX() < 18)
		window.PRINTCHR(' ');

	window.PRINTSTR(_messages.get(kMsgAlignmentNames + c.alignment).substr(0, 1));
	window.PRINTCHR('-');
	window.PRINTSTR(_messages.get(kMsgClassNames + c.classType).substr(0, 3));
	window.PRINTCHR(' ');

	if (c.armorClass < -9)
		window.PRINTSTR(_messages.get(kMsgArmorClassLow));
	else
		window.printInt(c.armorClass, 2);

	window.printInt(c.hp, 5);
	window.PRINTCHR(' ');

	if (c.status != kStatusOk)
		window.PRINTSTR(_messages.get(kMsgStatusNames + c.status).substr(0, 6));
	else if (c.poison)
		window.PRINTSTR(_messages.get(kMsgPoison));
	else
		window.printInt(c.hpMax, 4);
}

void TownScreens::drawNotice(const Common::String &text) {
	TextWindow window(_screen, TextWindow::centeredBounds(text.size(), 9, 3), true);
	window.clear();
	window.printCenteredLine(text);
}

void TownScreens::drawCastleMenu(Menu &menu) {
	const Common::String options = _messages.get(kMsgCastleMenu1) + _messages.get(kMsgCastleMenu2);

	TextWindow window(_screen, TextWindow::centeredBounds(Menu::textWidth(options), 4, 7), true);
	window.clear();
	menu.print(window, options);
}

void TownScreens::drawTavernMenu(Menu &menu) {
	const Common::String &title = _messages.get(kMsgTavern);
	const Common::String &options = _messages.get(kMsgTavernMenu);
	const int width = MAX<int>(title.size() + 1, Menu::textWidth(options));

	TextWindow window(_screen, TextWindow::centeredBounds(width, 4, 9), true);
	window.clear();
	window.printCenteredLine(title);
	window.newLine();
	menu.print(window, options);
}

void TownScreens::drawTavernAdd(const Common::Array<uint> &available) {
	TextWindow prompt = openWindow(2, 6, 36, 4);
	prompt.printCenteredLine(_messages.get(kMsgLetterSelects));

	// Two columns of ten, lettered from A
	TextWindow list = openWindow(0, 9, 40, 12);
	for (uint i = 0; i < available.size(); ++i) {
		if (available[i] == kNobody)
			continue;
		list.MVCURSOR((i / 10) * 19, i % 10);
		list.PRINTCHR('A' + i);
		list.PRINTSTR(") ");
		list.PRINTSTR(_party.rosterEntry(available[i]).name);
	}
}

void TownScreens::drawTavernRemove() {
	TextWindow window = openWindow(8, 7, 24, 4);
	window.printCenteredLine(_messages.get(kMsgRemoveWho));
	window.printCenteredLine(_messages.get(kMsgReturnToLeave));
}

} // End of namespace Wizardry
