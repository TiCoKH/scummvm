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

namespace {

static Common::String dialogName(const Dialogs::Dialog *dlg) {
    return dlg ? dlg->getName() : Common::String("<none>");
}

static bool hasPartyMembers() {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    return party && party->size() > 0;
}

} // namespace

MainmenuView::MainmenuView() : View("Mainmenu"), _partyList(nullptr), _loadSaveDialog(nullptr), _exitConfirmDialog(nullptr), _party(nullptr) {
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
    setDialogParent(_loadSaveDialog, this);

    Dialogs::HorizontalYesNoConfig exitCfg = { "Quit to DOS ", 13, 10, 15, 0 };
    _exitConfirmDialog = new Dialogs::HorizontalYesNo("MainmenuExitConfirm", exitCfg);
    setDialogParent(_exitConfirmDialog, this);
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
    if (_exitConfirmDialog) {
        delete _exitConfirmDialog;
        _exitConfirmDialog = nullptr;
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
    if (_activeDialog && _activeDialog->isActive())
        _activeDialog->draw();
}

bool MainmenuView::msgKeypress(const KeypressMessage &msg) {
	debug("MainmenuView::msgKeypress BEGIN key=%d ascii=%d state=%s activeDialog=%s",
        (int)msg.keycode, (int)msg.ascii, Common::String::format("%d", (int)_state).c_str(),
        dialogName(_activeDialog).c_str());

    // Keep active dialog pointer in sync.
    if (_activeDialog && !_activeDialog->isActive()) {
		debug("MainmenuView::msgKeypress active dialog '%s' is inactive; clearing",
            dialogName(_activeDialog).c_str());
        if (_state == MM_STATE_EXIT_CONFIRM)
            _state = MM_STATE_NORMAL;
        _activeDialog = nullptr;
    }

    // Exit confirm is modal while it is the active dialog.
    if (_exitConfirmDialog && _activeDialog == _exitConfirmDialog &&
            _exitConfirmDialog->isActive()) {
        // Opening the confirm dialog is bound to E in the main menu.
        // Ignore E while confirm is already open, otherwise key-repeat can
        // immediately re-trigger and collapse the modal flow.
        if (msg.keycode == Common::KEYCODE_e ||
                msg.ascii == 'e' || msg.ascii == 'E') {
			debug("MainmenuView::msgKeypress EXIT_CONFIRM ignoring repeated E (key=%d ascii=%d)",
				(int)msg.keycode, (int)msg.ascii);
            return true;
        }

        const bool isNoKey = (msg.keycode == Common::KEYCODE_n ||
            msg.keycode == Common::KEYCODE_ESCAPE ||
            msg.ascii == 'n' || msg.ascii == 'N');
        if (isNoKey) {
			debug("MainmenuView::msgKeypress EXIT_CONFIRM -> NO/CANCEL (key=%d ascii=%d)",
				(int)msg.keycode, (int)msg.ascii);
            setActiveDialog(nullptr);
            _state = MM_STATE_NORMAL;
            redraw();
            return true;
        }

        const bool isYesKey = (msg.keycode == Common::KEYCODE_y ||
            msg.ascii == 'y' || msg.ascii == 'Y');
        if (isYesKey) {
			debug("MainmenuView::msgKeypress EXIT_CONFIRM -> YES (key=%d ascii=%d)",
				(int)msg.keycode, (int)msg.ascii);
            setActiveDialog(nullptr);
            if (_loadSaveDialog && hasPartyMembers()) {
                _state = MM_STATE_EXIT_SAVE;
                _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeSave);
                setActiveDialog(_loadSaveDialog);
				debug("MainmenuView::msgKeypress EXIT_CONFIRM -> EXIT_SAVE dialog=%s",
                        dialogName(_activeDialog).c_str());
                redraw();
                return true;
            }

            _state = MM_STATE_NORMAL;
			debug("MainmenuView::msgKeypress EXIT_CONFIRM YES with empty party -> quit");
            requestQuit();
            return true;
        }

        const bool handledByDialog = _activeDialog->send(msg);
        debug("MainmenuView::msgKeypress activeDialog=%s key=%d ascii=%d handled=%d",
            _activeDialog->getName().c_str(), (int)msg.keycode,
            (int)msg.ascii, handledByDialog ? 1 : 0);
        return true;
    }

    if (_activeDialog) {
        if (_state == MM_STATE_EXIT_SAVE && _activeDialog == _loadSaveDialog &&
                msg.keycode == Common::KEYCODE_ESCAPE) {
            debug("MainmenuView::msgKeypress EXIT_SAVE ESC direct fallback -> quit");
            setActiveDialog(nullptr);
            _state = MM_STATE_NORMAL;
            requestQuit();
            return true;
        }

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
                    _state = MM_STATE_NORMAL;
                    _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeLoad);
                    setActiveDialog(_loadSaveDialog);
                    debug("MainmenuView::msgKeypress LOAD activeDialog=%s state=%s",
                        dialogName(_activeDialog).c_str(), Common::String::format("%d", (int)_state).c_str());
                    redraw();
                }
            break;
        case Common::KEYCODE_s:
            if (_menuItemList.isActive(SAVE)) {
                if (_loadSaveDialog) {
                    _state = MM_STATE_NORMAL;
                    _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeSave);
                    setActiveDialog(_loadSaveDialog);
                    debug("MainmenuView::msgKeypress SAVE activeDialog=%s state=%s",
                        dialogName(_activeDialog).c_str(), Common::String::format("%d", (int)_state).c_str());
                    redraw();
                }
			}
            break;
        case Common::KEYCODE_b:
            if (_menuItemList.isActive(BEGIN))
                replaceView("Mainmenu");
            break;
        case Common::KEYCODE_e:
            if (_menuItemList.isActive(EXIT) && _exitConfirmDialog) {
                _state = MM_STATE_EXIT_CONFIRM;
                setActiveDialog(_exitConfirmDialog);
                debug("MainmenuView::msgKeypress EXIT activeDialog=%s state=%s",
                        dialogName(_activeDialog).c_str(), Common::String::format("%d", (int)_state).c_str());
                redraw();
            }
            break;
        default:
            break;
    }
    return true;
}

void MainmenuView::handleMenuResult(const MenuResultMessage &result) {
	debug("MainmenuView::handleMenuResult BEGIN success=%d key=%d hasInt=%d int=%d state=%s activeDialog=%s",
		result._success ? 1 : 0, (int)result._keyCode,
		result._hasIntValue ? 1 : 0, result._hasIntValue ? result._intValue : -1,
        Common::String::format("%d", (int)_state).c_str(), dialogName(_activeDialog).c_str());

    if (_activeDialog == _exitConfirmDialog || (_exitConfirmDialog &&
            _exitConfirmDialog->isActive()) || _state == MM_STATE_EXIT_CONFIRM) {
		debug("MainmenuView::handleMenuResult processing EXIT_CONFIRM path");
        if (!result._success) {
			debug("MainmenuView::handleMenuResult EXIT_CONFIRM !success -> close confirm");
            setActiveDialog(nullptr);
            _state = MM_STATE_NORMAL;
            redraw();
            return;
        }

        const bool isNo = (result._keyCode == Common::KEYCODE_n ||
            result._keyCode == Common::KEYCODE_ESCAPE ||
            (result._hasIntValue && result._intValue == 0));
        if (isNo) {
			debug("MainmenuView::handleMenuResult EXIT_CONFIRM -> NO/CANCEL");
            setActiveDialog(nullptr);
            _state = MM_STATE_NORMAL;
            redraw();
            return;
        }

        const bool isYes = (result._keyCode == Common::KEYCODE_y ||
            (result._hasIntValue && result._intValue == 1));
        if (isYes) {
			debug("MainmenuView::handleMenuResult EXIT_CONFIRM -> YES");
            setActiveDialog(nullptr);
            if (_loadSaveDialog && hasPartyMembers()) {
                _state = MM_STATE_EXIT_SAVE;
                _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeSave);
                setActiveDialog(_loadSaveDialog);
				debug("MainmenuView::handleMenuResult EXIT_CONFIRM YES -> EXIT_SAVE activeDialog=%s",
                    dialogName(_activeDialog).c_str());
                redraw();
                return;
            }

            _state = MM_STATE_NORMAL;
			debug("MainmenuView::handleMenuResult EXIT_CONFIRM YES with empty party -> quit");
            requestQuit();
            return;
        }

        return;
    }

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
			debug("MainmenuView::handleMenuResult LOAD/SAVE ESC activeDialog=%s state=%s",
                dialogName(_activeDialog).c_str(), Common::String::format("%d", (int)_state).c_str());
            setActiveDialog(nullptr);

            if (_state == MM_STATE_EXIT_SAVE) {
				debug("MainmenuView::handleMenuResult EXIT_SAVE ESC -> quit");
                _state = MM_STATE_NORMAL;
                requestQuit();
                return;
            }

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

    setActiveDialog(nullptr);
    debug("MainmenuView::handleMenuResult slot operation complete; state=%s activeDialog=%s",
        Common::String::format("%d", (int)_state).c_str(), dialogName(_activeDialog).c_str());

    Poolrad::PoolradEngine *engine = Poolrad::g_engine;
    if (!engine) {
        redraw();
        return;
    }

    if (_loadSaveDialog->getMode() == Dialogs::LoadSaveDialog::kModeLoad) {
        debug("MainmenuView::handleMenuResult post-load gameState=%d (staying on main menu)",
            (int)engine->getGameState());
    }

    if (_state == MM_STATE_EXIT_SAVE &&
            _loadSaveDialog->getMode() == Dialogs::LoadSaveDialog::kModeSave) {
		debug("MainmenuView::handleMenuResult EXIT_SAVE save complete -> quit");
        _state = MM_STATE_NORMAL;
        requestQuit();
        return;
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

void MainmenuView::setActiveDialog(Dialogs::Dialog *dlg) {
    debug("MainmenuView::setActiveDialog from=%s to=%s state=%s",
        dialogName(_activeDialog).c_str(), dialogName(dlg).c_str(),
        Common::String::format("%d", (int)_state).c_str());
    switchActiveDialog(_activeDialog, dlg);
    debug("MainmenuView::setActiveDialog done active=%s",
        dialogName(_activeDialog).c_str());
}

void MainmenuView::requestQuit() {
	debug("MainmenuView::requestQuit state=%s activeDialog=%s",
        Common::String::format("%d", (int)_state).c_str(), dialogName(_activeDialog).c_str());
    Poolrad::PoolradEngine *engine = Poolrad::g_engine;
    if (!engine)
        return;

    engine->quitGame();
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
