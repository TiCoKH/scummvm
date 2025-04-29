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
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/views/add_character_view.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"

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
    subView(_rosterMenu);
}

AddCharacterView::~AddCharacterView() {
    delete _rosterMenu;
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


void AddCharacterView::handleMenuResult(bool success, Common::KeyCode key, short value) {
    if (!success) return;

    switch (key) {
        case Common::KEYCODE_RETURN:
        case Common::KEYCODE_a:
            loadCharacter(value);
            break;

        case Common::KEYCODE_ESCAPE:
        case Common::KEYCODE_e:
            // handle cancelation logic here
            break;

        default:
            break;
    }
}

Common::String AddCharacterView::formatFilename(const Common::String &name) {
    Common::String formattedName;

    for (uint i = 0; i < name.size() && formattedName.size() < 8; ++i) {
        if (name[i] != ' ') {
            formattedName += name[i];
        }
    }
    return formattedName;
}

void AddCharacterView::loadCharacter(int selectedIndex) {
    Common::String characterName = _rosterList->items[selectedIndex].text;
    Common::String baseFilename = formatFilename(characterName);

    // Load .CHR
    Common::String chrFilename = baseFilename + ".CHR";
    Common::File characterFile;
    if (!characterFile.open(chrFilename.c_str())) {
        warning("Failed to open character file: %s", chrFilename.c_str());
        return;
    }

    Goldbox::Poolrad::Data::PoolradCharacter *character = new Goldbox::Poolrad::Data::PoolradCharacter();
    character->load(characterFile);
    characterFile.close();

    // Attempt to load .ITM (items file)
    Common::String itmFilename = baseFilename + ".ITM";
    Common::File itemsFile;
    if (itemsFile.open(itmFilename.c_str())) {
        // TODO: load items here (you would probably call character.loadItems(itemsFile) or similar)
        debug("Loaded items file: %s", itmFilename.c_str());
        itemsFile.close();
    } else {
        debug("Items file not found: %s", itmFilename.c_str());
    }

    Common::String spcFilename = baseFilename + ".SPC";
    Common::File spellsFile;
    if (spellsFile.open(spcFilename.c_str())) {
        // TODO: load spells here (you would probably call character.loadItems(itemsFile) or similar)
        spellsFile.close();
        debug("Loaded spells file: %s", spcFilename.c_str());
    } else {
        debug("Spells file not found: %s", spcFilename.c_str());
    }

    Goldbox::VmInterface::getParty().push_back(character);

    // Mark as loaded
    _rosterList->items[selectedIndex].text = "* " + characterName;
    _rosterMenu->redraw();

    // Check party full
    if (Goldbox::VmInterface::getParty().size() >= 6) {
        close();
    }
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
