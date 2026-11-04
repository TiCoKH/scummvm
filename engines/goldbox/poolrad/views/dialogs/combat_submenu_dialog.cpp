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

#include "goldbox/poolrad/views/dialogs/combat_submenu_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/game_speed_dialog.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/events.h"
#include "goldbox/gfx/surface.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatSubmenuDialog::CombatSubmenuDialog()
    : Dialog("CombatSubmenu"), _actor(nullptr),
      _horizontalMenu(nullptr), _speedDialog(nullptr),
      _activeChild(kChildNone), _hasGuard(false), _hasBandage(false) {
    setBounds(Window(0, 24, 39, 24));
    _speedDialog = new GameSpeedDialog();
}

CombatSubmenuDialog::ActiveChild CombatSubmenuDialog::routeResult(
        ActiveChild activeChild) {
    // Only a result arriving while the speed dialog is the active child is a
    // speed-dialog result; everything else (including the main HorizontalMenu,
    // which posts to the same parent name) is a main-menu command.
    return activeChild == kChildSpeedDialog ? kChildSpeedDialog : kChildMainMenu;
}

CombatSubmenuDialog::~CombatSubmenuDialog() {
    delete _horizontalMenu;
    delete _speedDialog;
}

void CombatSubmenuDialog::beginSubmenu(Goldbox::Data::PlayerCharacter *actor) {
    _actor = actor;
}

void CombatSubmenuDialog::rebuild() {
    delete _horizontalMenu;
    _horizontalMenu = nullptr;

    _menuModel.items.clear();
    _menuModel.currentSelection = 0;

    // Guard: available when no ranged weapon, or ranged weapon is equipped.
    const Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<const Goldbox::Data::ADnDCharacter *>(_actor);
    _hasGuard = !adnd || !adnd->hasRangedWeapon() || adnd->isEquippedRangedWeapon();

    // Bandage: available when at least one dying party member exists.
    _hasBandage = Combat::checkDoBandage(false, nullptr);

    Common::Array<Common::String> labels;

    if (_hasGuard)
        labels.push_back("Guard");   // G

    labels.push_back("Delay");       // D
    labels.push_back("Quit");        // Q

    if (_hasBandage)
        labels.push_back("Bandage"); // B

    labels.push_back("Speed");       // S

    _menuModel.generateMenuItems(labels, true);

    HorizontalMenuConfig cfg;
    cfg.promptTxt             = "";
    cfg.menuItemList          = &_menuModel;
    cfg.textColor             = 10;
    cfg.selectColor           = 15;
    cfg.promptColor           = 15;
    cfg.allowNumPad           = false;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor       = 8;
    cfg.singleItemMode        = false;

    _horizontalMenu = new HorizontalMenu("CombatSubHMenu", cfg);
    _horizontalMenu->setParent(this);
    _horizontalMenu->activate();

    // The submenu's own menu is now the active child; a result addressed to
    // this view is a main-menu command until the speed dialog opens.
    _activeChild = kChildMainMenu;
}

void CombatSubmenuDialog::activate() {
    Dialog::activate();
    rebuild();
}

void CombatSubmenuDialog::deactivate() {
    if (_speedDialog->isActive())
        _speedDialog->deactivate();
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    _activeChild = kChildNone;
    Dialog::deactivate();
}

void CombatSubmenuDialog::draw() {
    if (!_isVisible)
        return;
    if (_speedDialog->isActive()) {
        _speedDialog->draw();
        return;
    }
    if (!_horizontalMenu)
        return;
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 8);
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool CombatSubmenuDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;
    debug(0, "[COMBAT_SUBMENU] keypress key=%d ascii=%d speedActive=%d",
        (int)msg.keycode, (int)msg.ascii, (int)_speedDialog->isActive());
    if (_speedDialog->isActive())
        return _speedDialog->msgKeypress(msg);
    if (!_horizontalMenu)
        return false;
    return _horizontalMenu->msgKeypress(msg);
}

void CombatSubmenuDialog::handleMenuResult(const MenuResultMessage &result) {
    debug(0, "[COMBAT_SUBMENU] result success=%d key=%d hasInt=%d int=%d",
        (int)result._success, (int)result._keyCode,
        (int)result._hasIntValue,
        result._hasIntValue ? result._intValue : -1);

    // Route by the active child, not by the shared destination string: the
    // main HorizontalMenu and the GameSpeedDialog both post their result to
    // this view's name, so only the speed dialog's own result may resume from
    // the speed branch. Returning from Speed recreates the submenu WITHOUT
    // replaying the previous command.
    if (routeResult(_activeChild) == kChildSpeedDialog) {
        if (_speedDialog->isActive())
            _speedDialog->deactivate();
        rebuild();
        return;
    }

    // Result from the submenu's own HorizontalMenu.
    if (!result._success) {
        cancel();
        return;
    }

    char key = 0;
    if (result._hasIntValue && result._intValue >= 0 &&
            result._intValue < (int)_menuModel.items.size()) {
        key = _menuModel.items[result._intValue].shortcut;
    } else {
        key = static_cast<char>(result._keyCode);
    }
    if (key >= 'a' && key <= 'z')
        key = static_cast<char>(key - 32);

    debug(0, "[COMBAT_SUBMENU] selected option='%c' actor='%s'",
        key ? key : '?', _actor ? _actor->name.c_str() : "NONE");

    if (key == 'G' && _hasGuard && _actor) {
        // Guard: endTurn() then set guarding. Presentation via CombatView.
        if (_actor->combatState) {
            _actor->combatState->endTurn();
            _actor->combatState->guarding = true;
        }
        endTurn();
        return;
    }

    if (key == 'D' && _actor) {
        // Delay: set initiative to 1 (lowest positive priority).
        if (_actor->combatState)
            _actor->combatState->initiative = 1;
        endTurn();
        return;
    }

    if (key == 'Q') {
        // Quit: end actor turn.
        if (_actor && _actor->combatState)
            _actor->combatState->endTurn();
        endTurn();
        return;
    }

    if (key == 'B' && _hasBandage) {
        // Bandage: perform bandage (ignoring its return value), then end turn.
        Combat::checkDoBandage(true, nullptr);
        if (_actor && _actor->combatState)
            _actor->combatState->endTurn();
        endTurn();
        return;
    }

    if (key == 'S') {
        // Speed: open nested game-speed dialog; submenu resumes on return
        // without consuming the actor action. Mark the speed dialog as the
        // active child so its result routes to the speed branch above.
        _activeChild = kChildSpeedDialog;
        _speedDialog->setParent(this);
        _speedDialog->activate();
        return;
    }

    // Unhandled key: rebuild and stay active.
    rebuild();
}

void CombatSubmenuDialog::endTurn() {
    deactivate();
    if (_parent)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_RETURN,
            static_cast<int>(Combat::CombatSession::PA_DONE),
            Common::String(), true, false);
}

void CombatSubmenuDialog::cancel() {
    deactivate();
    if (_parent)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_RETURN,
            static_cast<int>(Combat::CombatSession::PA_SUBMENU),
            Common::String(), true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
