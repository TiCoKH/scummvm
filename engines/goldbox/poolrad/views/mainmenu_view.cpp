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

#include "goldbox/poolrad/views/mainmenu_view.h"
#include "goldbox/poolrad/views/party.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

//const Common::Rect MainmenuView::_area_party(1, 1, 28, 11);
//const Common::Rect MainmenuView::_area_menu(1, 12, 28, 22);

MainmenuView::MainmenuView() : View("Mainmenu") {
    
    Common::Array<Common::String> menuOptions;
    menuOptions.push_back("Create New Character");
    menuOptions.push_back("Drop Character");
    menuOptions.push_back("Modify Character");
    menuOptions.push_back("Train Character");
    menuOptions.push_back("View Character");
    menuOptions.push_back("Add Character to Party");
    menuOptions.push_back("Remove Character from Party");
    menuOptions.push_back("Load Saved Game");
    menuOptions.push_back("Save Current Game");
    menuOptions.push_back("BEGIN Adventuring");
    menuOptions.push_back("Exit to DOS");

    _menuItemList.generateMenuItems(menuOptions, true);
    _menuItemList.deactivate(1);
    _menuItemList.deactivate(2);
    _menuItemList.deactivate(3);
    _menuItemList.deactivate(4);
    _menuItemList.deactivate(6);
    _menuItemList.deactivate(8);
    _menuItemList.deactivate(9);
}

void MainmenuView::draw() {
    Surface s = getSurface();

    drawWindow( 1, 1, 38, 22);

    showMenu();
    drawPrompt();
}

bool MainmenuView::msgKeypress(const KeypressMessage &msg) {
    return true;
}

bool MainmenuView::msgFocus(const FocusMessage &msg) {
    View::msgFocus(msg);
    return true;
}

bool MainmenuView::msgUnfocus(const UnfocusMessage &msg) {
    return true;
}

void MainmenuView::timeout() {
}

void MainmenuView::showMenu() {
    Surface s = getSurface();
    s.clearBox(1, 12, 28, 22, 0);

	int line_off = 0;
	for (int i = 0; i < 11; i++) {
		if (_menuItemList.items[i].active) {
			s.writeCharC(_menuItemList.items[i].shortcut, 15, 2, 12 + line_off);
			s.writeStringC(_menuItemList.items[i].text, 10, 3, 12 + line_off);
			line_off++;
		}
	}
}

void MainmenuView::drawPrompt() {
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 0);
    s.writeStringC("Choose a function", 13, 0, 24);
}


} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
