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

#include "goldbox/poolrad/views/dialogs/combat_target_command_dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatTargetCommandDialog::CombatTargetCommandDialog()
    : Dialog("CombatTargetCmd"), _listener(nullptr), _targetOffered(false),
      _horizontalMenu(nullptr) {
    setBounds(Window(0, 24, 39, 24));
}

CombatTargetCommandDialog::~CombatTargetCommandDialog() {
    delete _horizontalMenu;
}

void CombatTargetCommandDialog::beginTargetCommand(CombatAimListener *listener,
                                                   bool targetOffered) {
    _listener = listener;
    _targetOffered = targetOffered;
}

void CombatTargetCommandDialog::activate() {
    Dialog::activate();
    buildMenu();
}

void CombatTargetCommandDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void CombatTargetCommandDialog::buildMenu() {
    Common::Array<Common::String> labels;
    labels.push_back("Next");
    labels.push_back("Prev");
    labels.push_back("Manual");
    if (_targetOffered)
        labels.push_back("Target");
    labels.push_back("Center");
    labels.push_back("Exit");

    _menuModel.items.clear();
    _menuModel.currentSelection = 0;
    _menuModel.generateMenuItems(labels, true);

    delete _horizontalMenu;

    HorizontalMenuConfig cfg;
    cfg.promptTxt             = "Aim:";
    cfg.menuItemList          = &_menuModel;
    cfg.textColor             = 10;
    cfg.selectColor           = 15;
    cfg.promptColor           = 13;
    cfg.allowNumPad           = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor       = 8;
    cfg.singleItemMode        = false;

    _horizontalMenu = new HorizontalMenu("AimMenu", cfg);
    _horizontalMenu->activate();
}

void CombatTargetCommandDialog::draw() {
    if (!_isVisible || !_horizontalMenu)
        return;
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool CombatTargetCommandDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    const bool handled = _horizontalMenu->msgKeypress(msg);
    if (_horizontalMenu->isActive())
        return handled;

    // The menu closed: Escape exits, anything else picked an item.
    AimCommand command = AIM_EXIT;
    if (msg.keycode != Common::KEYCODE_ESCAPE) {
        const int sel = _menuModel.currentSelection;
        if (sel >= 0 && sel < (int)_menuModel.items.size()) {
            char key = _menuModel.items[sel].shortcut;
            if (key >= 'a' && key <= 'z')
                key = (char)(key - 32);
            command = static_cast<AimCommand>(key);
        }
    }

    CombatAimListener *listener = _listener;
    deactivate();
    if (listener)
        listener->onAimCommand(command);
    return true;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
