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


namespace Goldbox {
namespace Poolrad {
namespace Views {



AddCharacterView::AddCharacterView() : View("AddCharacter") {
    _rosterMenu = nullptr;
}

AddCharacterView::~AddCharacterView() {
    delete _rosterMenu;
}

bool AddCharacterView::msgKeypress(const KeypressMessage &msg) {
    if (_rosterMenu && _rosterMenu->isActive()) {
        return _rosterMenu->msgKeypress(msg);
    }
    return true;
}

void AddCharacterView::draw() {
	Surface s = getSurface();

	drawWindow(1, 1, 38, 22);
    loadRosterList();

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

    _rosterList.clear();
    while (!charListFile.eos()) {
        Common::String line = charListFile.readLine();
        if (!line.empty()) {
            _rosterList.push_back(line);
        }
    }
    charListFile.close();

    if (_rosterMenu) {
        delete _rosterMenu;
    }

    _rosterMenu = new Dialogs::VerticalMenu(
        "RosterMenu",
        "Select a character:",
        _rosterList,
        10, // Text color
        14, // Selection color
        12  // Prompt color
    );
}

void AddCharacterView::processRoster() {
    for (const auto &line : _rosterList) {
        Common::String chaFilename = line + ".cha";

        // Attempt to open the .CHA file
        Common::File chaFile;
        if (!chaFile.open(chaFilename.c_str())) {
            warning("Failed to open %s", chaFilename.c_str());
            continue; // Skip to the next line
        }

        // Successfully opened .CHA file
        debug(1, "Successfully opened file: %s", chaFilename.c_str());
        chaFile.close();
    }
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
