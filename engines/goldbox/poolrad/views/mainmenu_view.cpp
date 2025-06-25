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

#include "common/file.h"
#include "common/tokenizer.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/views/mainmenu_view.h"

#define CREATE 0
#define DROP   1
#define MODIFY 2
#define TRAIN  3
#define VIEW   4
#define ADD    5
#define REMOVE 6
#define LOAD   7 
#define SAVE   8
#define BEGIN  9
#define EXIT  10

namespace Goldbox {
namespace Poolrad {
namespace Views {

MainmenuView::MainmenuView() : View("Mainmenu") {
    
    Common::Array<Common::String> menuOptions;

    const Common::String shortcuts = VmInterface::getString("mainmenu.0");
    Common::StringTokenizer tokenizer(shortcuts, " ");
    while (!tokenizer.empty()) {
        const Common::String shortcut = tokenizer.nextToken();
        const Common::String descriptionKey = "mainmenu." + shortcut;
        const Common::String description = VmInterface::getString(descriptionKey);
        menuOptions.push_back(description);
    }
    _menuItemList.generateMenuItems(menuOptions, true);
    _menuItemList.activate(CREATE);
    _menuItemList.activate(EXIT);
    _party = Goldbox::VmInterface::getParty();
    updateMenuState();
}

void MainmenuView::draw() {
    Surface s = getSurface();

    drawWindow( 1, 1, 38, 22);
	updateMenuState();
    if (_party->size() > 0) {
		if (_selectedCharIndex == 0) {
			_selectedCharIndex = _party->size();
		}
        drawParty();
    }
    drawMenu();
    drawPrompt();
}

bool MainmenuView::msgKeypress(const KeypressMessage &msg) {
    	
    if (msg.keycode == Common::KEYCODE_c) {
        replaceView("CreatCharacter");
    }
    if ((msg.keycode == Common::KEYCODE_d) && _menuItemList.isActive(DROP)) {
        replaceView("Mainmenu");
    }
    if ((msg.keycode == Common::KEYCODE_m) && _menuItemList.isActive(MODIFY)) {
        replaceView("Mainmenu");
    }
    if ((msg.keycode == Common::KEYCODE_t) && _menuItemList.isActive(TRAIN)) {
        replaceView("Mainmenu");
    }
    if ((msg.keycode == Common::KEYCODE_v) && _menuItemList.isActive(VIEW)) {
        replaceView("Mainmenu");
    }
    if ((msg.keycode == Common::KEYCODE_a) && _menuItemList.isActive(ADD)) {
        replaceView("AddCharacter");
    }
    if ((msg.keycode == Common::KEYCODE_r) && _menuItemList.isActive(REMOVE)) {
        replaceView("Mainmenu");
    }
    if ((msg.keycode == Common::KEYCODE_l) && _menuItemList.isActive(LOAD)) {
        replaceView("Mainmenu");
    }
    if ((msg.keycode == Common::KEYCODE_s) && _menuItemList.isActive(SAVE)) {
        replaceView("Mainmenu");
    }
    if ((msg.keycode == Common::KEYCODE_b) && _menuItemList.isActive(BEGIN)) {
        replaceView("Mainmenu");
    }
    if (msg.keycode == Common::KEYCODE_e) {
        replaceView("Mainmenu");
    }

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

void MainmenuView::drawMenu() {
    Surface s = getSurface();
	s.clearBox(1, 12, 28, 22, 0); // Clear the menu area

	int line_off = 0;
	for (int i = 0; i < 11; i++) {
		if (_menuItemList.items[i].active) {
			s.writeCharC(_menuItemList.items[i].shortcut, 15, 2, 12 + line_off);
			s.writeStringC(_menuItemList.items[i].text, 10, 3, 12 + line_off);
			line_off++;
		}
	}
}

void MainmenuView::drawParty() {
	Surface s = getSurface();
	s.clearBox(1, 1, 28, 11, 0); // Clear the party area
    int x_n = 1;  int x_ac = 33; int y = 2;
    s.writeStringC("Name", 15, x_n, y);
    s.writeStringC("AC  HP", 15, x_ac, y);
    y += 2;
    for (uint _partyIndex = 0; _partyIndex < _party->size(); _partyIndex++) {
        Data::PlayerCharacter *pc = (*_party)[_partyIndex];
        if (pc) {
			if (_partyIndex == _selectedCharIndex - 1) {
				s.writeStringC(pc->name, 15, x_n, y);
			} else {
				s.writeStringC(pc->name, 11, x_n, y);
			}
			if (pc->hitPoints.max > 0) {
				s.writeStringC(Common::String::format("%d", pc->armorClass.current), 10, x_ac, y);
				s.writeStringC(Common::String::format("%d", pc->hitPoints.max), 10, x_ac + 4, y);
			} else {
				s.writeStringC(Common::String::format("%d", pc->armorClass.current), 12, x_ac, y);
				s.writeStringC(Common::String::format("%d", pc->hitPoints.max), 12, x_ac + 4, y);
			}			
            y ++;
        }
    }
}

void MainmenuView::updateMenuState() {
    int partySize = _party->size();

    if (partySize > 0) {
        _menuItemList.activate(DROP);
        _menuItemList.activate(MODIFY);
		_menuItemList.activate(TRAIN);
        _menuItemList.activate(VIEW);
        _menuItemList.activate(REMOVE);
        _menuItemList.deactivate(LOAD);
        _menuItemList.activate(SAVE);
        _menuItemList.activate(BEGIN);
    } else {
        _menuItemList.deactivate(DROP);
        _menuItemList.deactivate(MODIFY);
		_menuItemList.deactivate(TRAIN);
        _menuItemList.deactivate(VIEW);
        _menuItemList.deactivate(REMOVE);
        _menuItemList.activate(LOAD);
        _menuItemList.deactivate(SAVE);
        _menuItemList.deactivate(BEGIN);
    }

    if (partySize >= 6) {
        _menuItemList.deactivate(ADD);
    } else {
        _menuItemList.activate(ADD);
    }
}

void MainmenuView::drawPrompt() {
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 0);
    s.writeStringC("Choose a function", 13, 0, 24);
}

void MainmenuView::loadCharList() {

    charList.clear();
    Common::File charListFile;
    if (!charListFile.open("CHARLIST.TXT")) {
        warning("Failed to open CHARLIST.TXT");
        return;
    }
    while (!charListFile.eos()) {
        Common::String line = charListFile.readLine();
        if (!line.empty()) {
            charList.push_back(line);
        }
    }
    charListFile.close();
}


} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
