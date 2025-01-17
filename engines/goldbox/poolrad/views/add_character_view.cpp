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
#include "common/file.h"
#include "graphics/palette.h"
#include "goldbox/poolrad/views/add_character_view.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

AddCharacterView::AddCharacterView() : View("AddCharacter") {
    
    loadRosterList();
    Common::Array<Common::String> promptOptions = {"Add", "Exit"};
    Dialogs::VerticalMenuConfig menuConfig = {
        "Add a character:",  // promptTxt
        promptOptions,       // promptOptions
        _rosterList,         // menuItemList (initialized later)
        13,                  // headColor
        10,                  // textColor
        15,                  // selectColor
        1, 2, 38, 22,        // bounds
        false                // isAddExit
    };
    _rosterMenu = new Dialogs::VerticalMenu("RosterMenu", menuConfig);
    _children.push_back(_rosterMenu);
}

AddCharacterView::~AddCharacterView() {
    delete _rosterMenu;
}

bool AddCharacterView::msgMenu(const MenuMessage &msg) {
    return true;
}

void AddCharacterView::draw() {
	Surface s = getSurface();

	drawWindow(1, 1, 38, 22);

    if (_rosterMenu) {
        _rosterMenu->draw();
    }

	delaySeconds(10);

}

bool AddCharacterView::msgFocus(const FocusMessage &msg) {
	return true;
}

bool AddCharacterView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void AddCharacterView::timeout() {
//	replaceView("Codewheel");
}

void AddCharacterView::loadRosterList() {
    Common::File charListFile;
    if (!charListFile.open("CHARLIST.TXT")) {
        warning("Failed to open CHARLIST.TXT");
        return;
    }

    if (_rosterList) {
        delete _rosterList;
    }
    _rosterList = new Goldbox::MenuItemList();

    while (!charListFile.eos()) {
        Common::String line = charListFile.readLine();
        if (!line.empty()) {
            Goldbox::MenuItem item;
            item.text = line;
            item.active = true;
            _rosterList->items.push_back(item);
        }
    }
    charListFile.close();
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
