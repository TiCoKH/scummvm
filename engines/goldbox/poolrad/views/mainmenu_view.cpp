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

#include "common/tokenizer.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/poolrad.h"
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

using Common::String;
using Common::Array;
using Common::StringTokenizer;

namespace {

static bool hasPartyMembers() {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    return party && party->size() > 0;
}

} // anonymous namespace

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

    _partyList = new Dialogs::PartyList();
    _partyList->deactivate();
    _partyList->setParent(nullptr);

    _loadSaveDialog = new Dialogs::LoadSaveDialog("LoadSaveDialog");
    _loadSaveDialog->deactivate();
    _loadSaveDialog->setParent(nullptr);

    Dialogs::HorizontalYesNoConfig exitCfg = { "Quit to DOS ", 13, 10, 15, 0 };
    _exitConfirmDialog = new Dialogs::HorizontalYesNo("MainmenuExitConfirm", exitCfg);
    _exitConfirmDialog->deactivate();
    _exitConfirmDialog->setParent(nullptr);
}

MainmenuView::~MainmenuView() {
    if (_activeSubView) {
        _activeSubView->deactivate();
        _activeSubView->setParent(nullptr);
        _activeSubView = nullptr;
    }
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

bool MainmenuView::msgFocus(const FocusMessage &msg) {
    View::msgFocus(msg);
    refreshPartyState();
    return true;
}

bool MainmenuView::msgUnfocus(const UnfocusMessage &msg) {
    return true;
}

void MainmenuView::refreshPartyState() {
    _party = Goldbox::VmInterface::getParty();
    updateMenuState();
    if (_partyList) {
        if (_party && _party->size() > 0) {
            _partyList->activate();
        } else {
            _partyList->deactivate();
        }
    }
}

void MainmenuView::draw() {
    Surface s = getSurface();

    drawWindow(1, 1, 38, 22);
    if (_partyList && _partyList->isActive()) {
        _partyList->draw();
    }
    drawMenu();
    drawPrompt();
    if (_activeSubView && _activeSubView->isActive())
        _activeSubView->draw();
}

bool MainmenuView::msgKeypress(const KeypressMessage &msg) {
    // Keep active dialog pointer in sync.
    if (_activeSubView && !_activeSubView->isActive()) {
        if (_state == MM_STATE_EXIT_CONFIRM)
            _state = MM_STATE_NORMAL;
        _activeSubView = nullptr;
    }

    // Forward to active dialog first.
    if (_activeSubView) {
        // In EXIT_SAVE state, ESC on load/save dialog means quit without saving.
        if (_state == MM_STATE_EXIT_SAVE && _activeSubView == _loadSaveDialog &&
                msg.keycode == Common::KEYCODE_ESCAPE) {
            setActiveSubView(nullptr);
            _state = MM_STATE_NORMAL;
            requestQuit();
            return true;
        }

        // Ignore repeated 'E' while exit confirm is open.
        if (_activeSubView == _exitConfirmDialog &&
                (msg.keycode == Common::KEYCODE_e ||
                 msg.ascii == 'e' || msg.ascii == 'E')) {
            return true;
        }

        const bool handled = _activeSubView->send(msg);
        if (handled)
            return true;
    }

    if (_partyList && _partyList->isActive() && _partyList->send(msg))
        return true;

    // Keep command availability in sync with runtime state.
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
            if (_menuItemList.isActive(LOAD) && _loadSaveDialog) {
                _state = MM_STATE_NORMAL;
                _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeLoad);
                setActiveSubView(_loadSaveDialog);
                redraw();
            }
            break;
        case Common::KEYCODE_s:
            if (_menuItemList.isActive(SAVE) && _loadSaveDialog) {
                _state = MM_STATE_NORMAL;
                _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeSave);
                setActiveSubView(_loadSaveDialog);
                redraw();
            }
            break;
        case Common::KEYCODE_b:
            if (_menuItemList.isActive(BEGIN)) {
                // Original: BYTE_GAME_STATE = GS_DUNGEON_MAP; return;
                // setGameState triggers onGameStateEnter → replaceView("InGame")
                VmInterface::setGameStatus(GS_DUNGEON_MAP);
            }
            break;
        case Common::KEYCODE_e:
            if (_menuItemList.isActive(EXIT) && _exitConfirmDialog) {
                _state = MM_STATE_EXIT_CONFIRM;
                setActiveSubView(_exitConfirmDialog);
                redraw();
            }
            break;
        default:
            break;
    }
    return true;
}

void MainmenuView::handleMenuResult(const MenuResultMessage &result) {
    // Exit confirm path
    if (_activeSubView == _exitConfirmDialog || (_exitConfirmDialog &&
            _exitConfirmDialog->isActive()) || _state == MM_STATE_EXIT_CONFIRM) {
        if (!result._success) {
            setActiveSubView(nullptr);
            _state = MM_STATE_NORMAL;
            redraw();
            return;
        }

        const bool isNo = (result._keyCode == Common::KEYCODE_n ||
            result._keyCode == Common::KEYCODE_ESCAPE ||
            (result._hasIntValue && result._intValue == 0));
        if (isNo) {
            setActiveSubView(nullptr);
            _state = MM_STATE_NORMAL;
            redraw();
            return;
        }

        const bool isYes = (result._keyCode == Common::KEYCODE_y ||
            (result._hasIntValue && result._intValue == 1));
        if (isYes) {
            setActiveSubView(nullptr);
            if (_loadSaveDialog && hasPartyMembers()) {
                _state = MM_STATE_EXIT_SAVE;
                _loadSaveDialog->setMode(Dialogs::LoadSaveDialog::kModeSave);
                setActiveSubView(_loadSaveDialog);
                redraw();
                return;
            }
            _state = MM_STATE_NORMAL;
            requestQuit();
            return;
        }
        return;
    }

    // Load/Save path
    if (!_loadSaveDialog)
        return;

    if (!result._success) {
        if (result._keyCode == Common::KEYCODE_ESCAPE) {
            setActiveSubView(nullptr);
            if (_state == MM_STATE_EXIT_SAVE) {
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

    setActiveSubView(nullptr);

    Poolrad::PoolradEngine *engine = Poolrad::g_engine;
    if (!engine) {
        redraw();
        return;
    }

    if (_state == MM_STATE_EXIT_SAVE &&
            _loadSaveDialog->getMode() == Dialogs::LoadSaveDialog::kModeSave) {
        _state = MM_STATE_NORMAL;
        requestQuit();
        return;
    }

    // After successful load, check if engine transitioned away from START_MENU.
    // loadGameSlotX86 calls setGameState() internally which triggers
    // onGameStateEnter → replaceView("InGame") for non-menu states.
    // If loaded state was GS_START_MENU, we stay in mainmenu with loaded party.
    if (VmInterface::getGameStatus() == GS_START_MENU) {
        refreshPartyState();
        redraw();
    }
    // Otherwise the engine already replaced this view — nothing to do.
}

void MainmenuView::setActiveSubView(Dialogs::Dialog *dlg) {
    if (_activeSubView && _activeSubView != dlg) {
        _activeSubView->deactivate();
        _activeSubView->setParent(nullptr);
    }
    _activeSubView = dlg;
    if (_activeSubView) {
        _activeSubView->setParent(this);
        _activeSubView->activate();
    }
}

void MainmenuView::requestQuit() {
    Poolrad::PoolradEngine *engine = Poolrad::g_engine;
    if (!engine)
        return;
    engine->quitGame();
}

void MainmenuView::timeout() {
}

void MainmenuView::drawMenu() {
    Surface s = getSurface();
    s.clearBox(1, 12, 28, 22, 0);

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

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
