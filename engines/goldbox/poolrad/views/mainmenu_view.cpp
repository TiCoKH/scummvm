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
#include "goldbox/poolrad/views/dialogs/party_list.h"

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

using Common::String;
using Common::Array;
using Common::StringTokenizer;

MainmenuView::MainmenuView() : View("Mainmenu") {
    
    Array<String> menuOptions;

    const String shortcuts = VmInterface::getString("mainmenu.0");
    StringTokenizer tokenizer(shortcuts, " ");
    while (!tokenizer.empty()) {
        const String shortcut = tokenizer.nextToken();
        const String descriptionKey = "mainmenu." + shortcut;
        const String description = VmInterface::getString(descriptionKey);
        menuOptions.push_back(description);
    }
    _menuItemList.generateMenuItems(menuOptions, true);
    _menuItemList.activate(CREATE);
    _menuItemList.activate(EXIT);
    _party = Goldbox::VmInterface::getParty();
    _partyList = new Views::Dialogs::PartyList();
}

MainmenuView::~MainmenuView() {
    delete _partyList;
}

void MainmenuView::draw() {
    Surface s = getSurface();

    drawWindow( 1, 1, 38, 22);
    updateMenuState();
    if (_party->size() > 0 && _partyList) {
        _partyList->setSelectedCharIndex(_party->size());
        Goldbox::VmInterface::setSelectedCharacter((*_party)[_partyList->getSelectedCharIndex()-1]);
        _partyList->draw();
    }
    drawMenu();
    drawPrompt();
}

bool MainmenuView::msgKeypress(const KeypressMessage &msg) {
    switch (msg.keycode) {
        case Common::KEYCODE_c:
            if (_menuItemList.isActive(CREATE))
                replaceView("CreatCharacter");
            break;
        case Common::KEYCODE_d:
            if (_menuItemList.isActive(DROP))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_m:
            if (_menuItemList.isActive(MODIFY))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_t:
            if (_menuItemList.isActive(TRAIN))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_v:
            if (_menuItemList.isActive(VIEW))
                replaceView("ViewCharacter");
            break;
        case Common::KEYCODE_a:
            if (_menuItemList.isActive(ADD))
                replaceView("AddCharacter");
            break;
        case Common::KEYCODE_r:
            if (_menuItemList.isActive(REMOVE))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_l:
            if (_menuItemList.isActive(LOAD))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_s:
            if (_menuItemList.isActive(SAVE))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_b:
            if (_menuItemList.isActive(BEGIN))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_e:
            replaceView("Mainmenu");
            break;
        case Common::KEYCODE_END	:
            if (_party->size()>1) 
                _partyList->nextChar();
                Goldbox::VmInterface::setSelectedCharacter((*_party)[_partyList->getSelectedCharIndex()-1]);
            break;
        case Common::KEYCODE_HOME:
            if (_party->size()>1) 
                _partyList->prevChar();
                Goldbox::VmInterface::setSelectedCharacter((*_party)[_partyList->getSelectedCharIndex()-1]);
            break;
        default:
            break;
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
