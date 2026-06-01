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

#include "goldbox/poolrad/views/dialogs/in_game_menu_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

InGameMenuDialog::InGameMenuDialog(const Common::String &name)
    : Dialog(name), _mode(kModeDungeon), _horizontalMenu(nullptr) {
    setBounds(Window(0, 24, 39, 24));
}

InGameMenuDialog::~InGameMenuDialog() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
}

void InGameMenuDialog::buildMenuModel() {
    _menuModel.items.clear();
    _menuModel.currentSelection = 0;

    Common::Array<Common::String> menuStrings;
    if (_mode == kModeDungeon)
        menuStrings.push_back("Area");
    menuStrings.push_back("Cast");
    menuStrings.push_back("View");
    menuStrings.push_back("Encamp");
    menuStrings.push_back("Search");
    menuStrings.push_back("Look");

    _menuModel.generateMenuItems(menuStrings, true);
}

void InGameMenuDialog::setMode(MapMode mode) {
    if (_mode != mode) {
        _mode = mode;
        if (_isActive)
            activate();
    }
}

void InGameMenuDialog::activate() {
    debug(0, "InGameMenuDialog::activate() called");
    Dialog::activate();

    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    buildMenuModel();

    HorizontalMenuConfig cfg;
    cfg.promptTxt = "";
    cfg.menuItemList = &_menuModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 15;
    cfg.allowNumPad = true;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;

    _horizontalMenu = new HorizontalMenu("InGameHMenu", cfg);
    _horizontalMenu->activate();
}

void InGameMenuDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void InGameMenuDialog::draw() {
    if (!_isVisible)
        return;
    debug(0, "InGameMenuDialog::draw() _isActive=%d _horizontalMenu=%p",
        (int)_isActive, (void*)_horizontalMenu);
    if (_horizontalMenu)
        _horizontalMenu->draw();
}

bool InGameMenuDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    InGameView *igv = dynamic_cast<InGameView *>(
        g_engine ? g_engine->findView("InGame") : nullptr);

    // Forward key to HorizontalMenu. It handles shortcuts and navigation.
    // Since HorizontalMenu will deactivate itself and post a result we can't
    // receive (not in tree), we instead check if it handled the key and
    // inspect what happened.
    bool wasActive = _horizontalMenu->isActive();
    bool handled = _horizontalMenu->msgKeypress(msg);

    if (!handled)
        return false;

    // HorizontalMenu handled the key. If it deactivated (selection made),
    // route the result to InGameView via handleMenuResult and reactivate.
    if (wasActive && !_horizontalMenu->isActive()) {
        Common::KeyCode resultKey = msg.keycode;
        bool hasResult = false;

        // Check arrow/numpad movement keys.
        switch (msg.keycode) {
        case Common::KEYCODE_UP:
        case Common::KEYCODE_KP8:
        case Common::KEYCODE_DOWN:
        case Common::KEYCODE_KP2:
        case Common::KEYCODE_LEFT:
        case Common::KEYCODE_KP4:
        case Common::KEYCODE_RIGHT:
        case Common::KEYCODE_KP6:
            resultKey = msg.keycode;
            hasResult = true;
            break;
        default:
            break;
        }

        // Check shortcut letter match.
        if (!hasResult) {
            char ascii = msg.ascii;
            if (ascii >= 'a' && ascii <= 'z')
                ascii = ascii - 32;
            for (int i = 0; i < (int)_menuModel.items.size(); ++i) {
                if (_menuModel.items[i].shortcut == ascii) {
                    resultKey = msg.keycode;
                    hasResult = true;
                    break;
                }
            }
        }

        // RETURN confirms the currently selected item.
        if (!hasResult && msg.keycode == Common::KEYCODE_RETURN) {
            int sel = _menuModel.currentSelection;
            if (sel >= 0 && sel < (int)_menuModel.items.size()) {
                // Convert shortcut char to keycode.
                char sc = _menuModel.items[sel].shortcut;
                if (sc >= 'A' && sc <= 'Z')
                    resultKey = static_cast<Common::KeyCode>(sc + 32);
                else
                    resultKey = static_cast<Common::KeyCode>(sc);
                hasResult = true;
            }
        }

        if (hasResult && igv) {
            MenuResultMessage result;
            result._targetViewName = igv->getName();
            result._success = true;
            result._keyCode = resultKey;
            result._intValue = 0;
            result._hasIntValue = false;
            result._hasStringValue = false;
            igv->handleMenuResult(result);
        }

        // Reactivate the menu to stay persistent (DIALOG_InGame loops).
        activate();
    }

    return true;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
