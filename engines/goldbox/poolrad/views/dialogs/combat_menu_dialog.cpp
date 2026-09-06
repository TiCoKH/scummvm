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

#include "goldbox/poolrad/views/dialogs/combat_menu_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/events.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatMenuDialog::CombatMenuDialog()
    : Dialog("CombatMenu"), _horizontalMenu(nullptr) {
    setBounds(Window(0, 24, 39, 24));
}

CombatMenuDialog::~CombatMenuDialog() {
    delete _horizontalMenu;
}

void CombatMenuDialog::buildMenuModel() {
    _menuModel.items.clear();
    _menuModel.currentSelection = 0;

    Common::Array<Common::String> items;
    items.push_back("Attack");
    items.push_back("Cast");
    items.push_back("Use");
    items.push_back("Move");
    items.push_back("Guard");
    items.push_back("Flee");
    _menuModel.generateMenuItems(items, true);
}

void CombatMenuDialog::activate() {
    if (_isActive && _horizontalMenu)
        return;

    Dialog::activate();

    delete _horizontalMenu;
    _horizontalMenu = nullptr;

    buildMenuModel();

    HorizontalMenuConfig cfg;
    cfg.promptTxt            = "";
    cfg.menuItemList         = &_menuModel;
    cfg.textColor            = 10;
    cfg.selectColor          = 15;
    cfg.promptColor          = 15;
    cfg.allowNumPad          = true;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor      = 8;  // match combat view kBackgroundColor
    cfg.singleItemMode       = false;

    _horizontalMenu = new HorizontalMenu("CombatHMenu", cfg);
    _horizontalMenu->activate();
}

void CombatMenuDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void CombatMenuDialog::draw() {
    if (!_isVisible || !_horizontalMenu)
        return;
    // Clear the menu row with the combat background color before drawing text.
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 8);
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool CombatMenuDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    const bool wasActive = _horizontalMenu->isActive();
    const bool handled   = _horizontalMenu->msgKeypress(msg);

    if (!handled)
        return false;

    if (wasActive && !_horizontalMenu->isActive()) {
        // Determine which action was selected by matching the shortcut key.
        char ascii = msg.ascii;
        if (ascii >= 'a' && ascii <= 'z')
            ascii = static_cast<char>(ascii - 32);

        // Map shortcut letter to PlayerAction index (order matches buildMenuModel).
        static const char kShortcuts[] = { 'A', 'C', 'U', 'M', 'G', 'F' };
        for (int i = 0; i < 6; ++i) {
            if (ascii == kShortcuts[i]) {
                postActionResult(static_cast<Combat::CombatSession::PlayerAction>(i));
                return true;
            }
        }

        // RETURN confirms current selection.
        if (msg.keycode == Common::KEYCODE_RETURN) {
            const int sel = _menuModel.currentSelection;
            if (sel >= 0 && sel < 6)
                postActionResult(static_cast<Combat::CombatSession::PlayerAction>(sel));
            return true;
        }

        // Reactivate to stay persistent until a valid action is chosen.
        _horizontalMenu->activate();
        _horizontalMenu->setRedraw();
    }

    return true;
}

void CombatMenuDialog::postActionResult(Combat::CombatSession::PlayerAction action) {
    if (g_events)
        g_events->postMenuResult("Combat", true, Common::KEYCODE_RETURN,
                                 static_cast<int>(action), Common::String(),
                                 true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
