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
#include "common/fs.h"
#include "common/path.h"
#include "graphics/palette.h"
#include "goldbox/engine.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/views/add_character_view.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

namespace {

static Common::Path getCharacterSavePath() {
    if (Goldbox::g_engine)
        return Goldbox::g_engine->resolveSavePath();

    return Common::Path();
}

} // namespace

AddCharacterView::AddCharacterView()
        : View("AddCharacter"),
            _rosterList(new Goldbox::MenuItemList()),
            _rosterMenu(nullptr) {

    loadRosterList();
    _promptOptions.push_back("Add");
    _promptOptions.push_back("Exit");
    
    Dialogs::VerticalMenuConfig menuConfig = {
        "Add a character:",  // promptTxt
        &_promptOptions,     // promptOptions (pointer)
        _rosterList,         // menuItemList (initialized later)
        13,                  // headColor
        10,                  // textColor
        15,                  // selectColor
        1, 2, 38, 22,        // bounds
        "",                  // title
        false                // asHeader
    };
    _rosterMenu = new Dialogs::VerticalMenu("RosterMenu", menuConfig);
    subView(_rosterMenu);
}

AddCharacterView::~AddCharacterView() {
    delete _rosterMenu;
    delete _rosterList;
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
    loadRosterList();
    if (_rosterMenu) {
		_rosterMenu->rebuild(_rosterList, "");
        _rosterMenu->activate();
    }
	return true;
}

bool AddCharacterView::msgUnfocus(const UnfocusMessage &msg) {
    if (_rosterMenu) {
        _rosterMenu->deactivate();
    }
	return true;
}

void AddCharacterView::timeout() {
//	replaceView("Codewheel");
}

void AddCharacterView::handleMenuResult(const MenuResultMessage &result) {
    short value = result._hasIntValue ? (short)result._intValue : 0;
    Common::KeyCode key = result._keyCode;

	switch (key) {
	case Common::KEYCODE_RETURN:
	case Common::KEYCODE_a:
		loadCharacter(value);
		break;

	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_e:
		replaceView("Mainmenu");
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
    if (!_rosterList || selectedIndex < 0 ||
            selectedIndex >= (int)_rosterList->items.size()) {
        warning("Invalid roster selection index: %d", selectedIndex);
        return;
    }

    Common::String characterName = _rosterList->items[selectedIndex].text;
    if (characterName.hasPrefix("*")) {
        debug(4, "Character already added: %s", characterName.c_str());
        return;
    }

    Common::String baseFilename = formatFilename(characterName);
    Common::Path savePath = _characterDataPath;
    if (savePath.empty())
        savePath = getCharacterSavePath();

    // Load .CHR
    Common::String chrFilename = baseFilename + ".CHA";
    Common::File characterFile;
    const Common::Path chrPath = savePath / chrFilename;
    Common::FSNode chrNode(chrPath);
    if (!characterFile.open(chrNode)) {
        warning("Failed to open character file: %s",
            chrPath.toString().c_str());
        return;
    }

    Goldbox::Poolrad::Data::PoolradCharacter *character = new Goldbox::Poolrad::Data::PoolradCharacter();
    character->load(characterFile);
    characterFile.close();

    Common::String itmFilename = baseFilename + ".ITM";
    const Common::Path itmPath = savePath / itmFilename;
    if (character->inventory.load(itmPath.toString())) {
        debug(4, "Loaded items file: %s", itmFilename.c_str());
        for (const auto &item : character->inventory.items()) {
            debug(4, "Item: %s", item.name.c_str());
        }
        character->resolveEquippedItems();
    } else {
        debug(4, "Items file not found or failed to load: %s", itmFilename.c_str());
    }

    Common::String spcFilename = baseFilename + ".SPC";
    const Common::Path spcPath = savePath / spcFilename;
    if (character->effects.load(spcPath.toString())) {
        debug(4, "Loaded spells file: %s", spcFilename.c_str());
    } else {
        debug(4, "Spells file not found or failed to load: %s", spcFilename.c_str());
    }

    Goldbox::VmInterface::getParty()->push_back(character);

    // Mark as loaded
    _rosterList->items[selectedIndex].text = "* " + characterName;
    _rosterMenu->redrawLine(selectedIndex);

    // Check party full
    if (Goldbox::VmInterface::getParty()->size() >= 6) {
        replaceView("Mainmenu");
    }
}
void AddCharacterView::loadRosterList() {
    _rosterList->items.clear();

    _characterDataPath = getCharacterSavePath();
    if (_characterDataPath.empty()) {
        warning("Failed to resolve legacy save directory for CHARLIST.TXT");
        return;
    }

    Common::FSNode saveNode(_characterDataPath);
    if (!saveNode.exists() || !saveNode.isDirectory()) {
        warning("Legacy save directory missing: %s",
            _characterDataPath.toString().c_str());
        return;
    }

    Common::Path charListPath = _characterDataPath / "CHARLIST.TXT";
    Common::File charListFile;
    if (!charListFile.open(Common::FSNode(charListPath))) {
        charListPath = _characterDataPath / "charlist.txt";
        if (!charListFile.open(Common::FSNode(charListPath))) {
            warning("Failed to open CHARLIST.TXT in legacy save directory: %s",
                _characterDataPath.toString().c_str());
            return;
        }
    }

    while (!charListFile.eos()) {
        Common::String line = charListFile.readLine();
        line.trim();
        if (line.empty())
            continue;

        Goldbox::MenuItem item;
        item.text = line;
        item.active = true;
        _rosterList->items.push_back(item);
    }
    charListFile.close();

    debug(4, "Loaded CHARLIST.TXT from %s (%d entries)",
        charListPath.toString().c_str(),
        (int)_rosterList->items.size());
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
