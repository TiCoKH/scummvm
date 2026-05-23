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
#include "goldbox/poolrad/poolrad.h"
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

MainmenuView::MainmenuView() : View("Mainmenu"), _partyList(nullptr), _loadSaveDialog(nullptr), _party(nullptr) {
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

	_loadSaveDialog = new Dialogs::LoadSaveDialog("LoadSaveDialog");
}

MainmenuView::~MainmenuView() {
    if (_partyList) {
        delete _partyList;
        _partyList = nullptr;
    }
    if (_loadSaveDialog) {
        delete _loadSaveDialog;
        _loadSaveDialog = nullptr;
    }
}

void MainmenuView::draw() {
    Surface s = getSurface();

    _party = Goldbox::VmInterface::getParty();

    if (!_partyList && _party && _party->size() > 0) {
        _partyList = new Dialogs::PartyList();
        _partyList->activate();
    }

    // Deactivate PartyList if party becomes empty
    if (_partyList && (!_party || _party->size() == 0)) {
        _partyList->deactivate();
    }
    // Activate PartyList if party has members
    if (_partyList && _party && _party->size() > 0 && !_partyList->isActive()) {
        _partyList->activate();
    }

    drawWindow( 1, 1, 38, 22);
    updateMenuState();
    if (_party && _party->size() > 0 && _partyList) {
        _partyList->draw();
    }
    drawMenu();
    drawPrompt();
    if (_loadSaveDialog && _loadSaveDialog->isActive())
        _loadSaveDialog->draw();
}

bool MainmenuView::msgKeypress(const KeypressMessage &msg) {
    if (_activeDialog) {
        const bool handledByDialog = _activeDialog->send(msg);
        debug("MainmenuView::msgKeypress activeDialog=%s key=%d ascii=%d handled=%d",
            _activeDialog->getName().c_str(), (int)msg.keycode,
            (int)msg.ascii, handledByDialog ? 1 : 0);
        if (handledByDialog)
            return true;
    }
    if (_partyList && _partyList->isActive() && _partyList->send(msg))
        return true;

    // Keep command availability in sync with runtime state even between draws.
    _party = Goldbox::VmInterface::getParty();
    updateMenuState();

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
            if (_menuItemList.isActive(VIEW)) {
                replaceView("ViewCharacter");
            }
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
                if (_loadSaveDialog) {
                    debug("MainmenuView::msgKeypress opening load dialog");
                    _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeLoad);
                    _loadSaveDialog->activate();
                    _activeDialog = _loadSaveDialog;
                    redraw();
                }
            break;
        case Common::KEYCODE_s:
            if (_menuItemList.isActive(SAVE)) {
                if (_loadSaveDialog) {
                    _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeSave);
                    _loadSaveDialog->activate();
                    _activeDialog = _loadSaveDialog;
                    redraw();
                }
			}
            break;
        case Common::KEYCODE_b:
            if (_menuItemList.isActive(BEGIN))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_e:
            replaceView("Mainmenu");
            break;
        default:
            break;
    }
    return true;
}

void MainmenuView::handleMenuResult(const MenuResultMessage &result) {
    if (!_loadSaveDialog)
        return;

    debug("MainmenuView::handleMenuResult success=%d key=%d hasInt=%d int=%d mode=%d",
        result._success ? 1 : 0, (int)result._keyCode,
        result._hasIntValue ? 1 : 0, result._hasIntValue ? result._intValue : -1,
        (int)_loadSaveDialog->getMode());

    if (!result._success) {
        // ESC/cancel closes the dialog; operation failures keep it open so
        // the dialog can display status text and allow retry.
        if (result._keyCode == Common::KEYCODE_ESCAPE) {
            _loadSaveDialog->deactivate();
            _activeDialog = nullptr;
            redraw();
        }
        return;
    }

    if (!result._hasIntValue)
        return;

    const int slotIndex = result._intValue;
    if (slotIndex < 0 || slotIndex > 9)
        return;

    const char slotLetter = static_cast<char>('A' + slotIndex);
    debug("MainmenuView::handleMenuResult operation succeeded for slot %c", slotLetter);

    _loadSaveDialog->deactivate();
    _activeDialog = nullptr;

    Poolrad::PoolradEngine *engine = Poolrad::g_engine;
    if (!engine) {
        redraw();
        return;
    }

    if (_loadSaveDialog->getMode() == Dialogs::LoadSaveDialog::kModeLoad) {
        debug("MainmenuView::handleMenuResult post-load gameState=%d (staying on main menu)",
            (int)engine->getGameState());
    }

    // Save/load succeeded: stay on main menu and refresh visual state.
    _party = Goldbox::VmInterface::getParty();
    updateMenuState();
    if (_partyList) {
        if (_party && _party->size() > 0) {
            _partyList->activate();
        } else {
            _partyList->deactivate();
        }
    }
    redraw();
}

bool MainmenuView::msgFocus(const FocusMessage &msg) {
    View::msgFocus(msg);

    // Setup when view gets focus (called by replaceView/addView)
    _party = Goldbox::VmInterface::getParty();
    if (!_partyList) {
        _partyList = new Dialogs::PartyList();
    }
    if (_partyList) {
        if (_party && _party->size() > 0) {
            _partyList->activate();
        } else {
            _partyList->deactivate();
        }
    }

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
			s.writeCharC(2, 12 + line_off, 15, _menuItemList.items[i].shortcut);
			s.writeStringC(3, 12 + line_off, 10, _menuItemList.items[i].text);
			line_off++;
		}
	}
}

void MainmenuView::updateMenuState() {
    int partySize = _party ? _party->size() : 0;

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
    s.writeStringC(0, 24, 13, "Choose a function");
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
