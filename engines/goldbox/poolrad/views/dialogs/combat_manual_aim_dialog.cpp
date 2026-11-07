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

#include "goldbox/poolrad/views/dialogs/combat_manual_aim_dialog.h"
#include "goldbox/combat/combat_session.h"
#include "common/util.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

namespace {

const int kFieldMaxX = 49;
const int kFieldMaxY = 24;

// N, NE, E, SE, S, SW, W, NW
const int8 kDirDX[8] = {  0, 1, 1, 1, 0, -1, -1, -1 };
const int8 kDirDY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

} // namespace

CombatManualAimDialog::CombatManualAimDialog()
    : Dialog("CombatManualAim"), _listener(nullptr), _session(nullptr),
      _range(1), _cursor(0, 0), _horizontalMenu(nullptr) {
    setBounds(Window(0, 24, 39, 24));
}

CombatManualAimDialog::~CombatManualAimDialog() {
    delete _horizontalMenu;
}

void CombatManualAimDialog::beginManualAim(CombatAimListener *listener,
        Combat::CombatSession *session, const Combat::AimRequest &request,
        uint8 range, TilePos start) {
    _listener = listener;
    _session = session;
    _request = request;
    _range = range;
    _cursor = start;
    _cell = Combat::AimCell();
}

void CombatManualAimDialog::activate() {
    Dialog::activate();
    evaluateCursor();
    buildCommandMenu();
}

void CombatManualAimDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void CombatManualAimDialog::draw() {
    if (!_isVisible || !_horizontalMenu)
        return;
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool CombatManualAimDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    const int dir = decodeDirection(msg);
    if (dir >= 0) {
        _cursor.col = (uint8)CLIP<int>(_cursor.col + kDirDX[dir], 0, kFieldMaxX);
        _cursor.row = (uint8)CLIP<int>(_cursor.row + kDirDY[dir], 0, kFieldMaxY);
        evaluateCursor();
        buildCommandMenu();
        return true;
    }

    const bool handled = _horizontalMenu->msgKeypress(msg);
    if (_horizontalMenu->isActive())
        return handled;

    char key = 'E';
    if (msg.keycode != Common::KEYCODE_ESCAPE) {
        const int sel = _menuModel.currentSelection;
        if (sel >= 0 && sel < (int)_menuModel.items.size())
            key = _menuModel.items[sel].shortcut;
        if (key >= 'a' && key <= 'z')
            key = (char)(key - 32);
    }

    if (key == 'T') {
        finish(_cell.valid);
    } else if (key == 'C') {
        if (_listener)
            _listener->onAimCommand(AIM_CENTER);
        buildCommandMenu();
    } else {
        finish(false);
    }
    return true;
}

void CombatManualAimDialog::evaluateCursor() {
    if (_session)
        _cell = Combat::evaluateAimCell(*_session, _request, _range, _cursor);
    if (_listener)
        _listener->onManualAimCursor(_cursor, _cell);
}

void CombatManualAimDialog::buildCommandMenu() {
    Common::Array<Common::String> labels;
    if (_cell.valid)
        labels.push_back("Target");
    labels.push_back("Center");
    labels.push_back("Exit");

    _menuModel.items.clear();
    _menuModel.currentSelection = 0;
    _menuModel.generateMenuItems(labels, true);

    delete _horizontalMenu;

    HorizontalMenuConfig cfg;
    cfg.promptTxt             = "(Use Cursor keys) ";
    cfg.menuItemList          = &_menuModel;
    cfg.textColor             = 10;
    cfg.selectColor           = 15;
    cfg.promptColor           = 13;
    cfg.allowNumPad           = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor       = 8;
    cfg.singleItemMode        = false;

    _horizontalMenu = new HorizontalMenu("ManualAimMenu", cfg);
    _horizontalMenu->activate();
}

void CombatManualAimDialog::finish(bool picked) {
    Combat::AimPick pick;
    if (picked) {
        pick.target = _cell.occupant;
        pick.tile = _cursor;
    }
    CombatAimListener *listener = _listener;
    deactivate();
    if (listener)
        listener->onManualAimFinished(picked, pick);
}

int CombatManualAimDialog::decodeDirection(const KeypressMessage &msg) {
    switch (msg.keycode) {
    case Common::KEYCODE_UP:       case Common::KEYCODE_KP8: return 0;
    case Common::KEYCODE_PAGEUP:   case Common::KEYCODE_KP9: return 1;
    case Common::KEYCODE_RIGHT:    case Common::KEYCODE_KP6: return 2;
    case Common::KEYCODE_PAGEDOWN: case Common::KEYCODE_KP3: return 3;
    case Common::KEYCODE_DOWN:     case Common::KEYCODE_KP2: return 4;
    case Common::KEYCODE_END:      case Common::KEYCODE_KP1: return 5;
    case Common::KEYCODE_LEFT:     case Common::KEYCODE_KP4: return 6;
    case Common::KEYCODE_HOME:     case Common::KEYCODE_KP7: return 7;
    default:                                                 return -1;
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
