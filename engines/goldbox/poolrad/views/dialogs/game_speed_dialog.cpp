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

#include "goldbox/poolrad/views/dialogs/game_speed_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/engine.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

GameSpeedDialog::GameSpeedDialog()
    : Dialog("GameSpeed"), _horizontalMenu(nullptr) {
    setBounds(Window(0, 24, 39, 24));
}

GameSpeedDialog::~GameSpeedDialog() {
    delete _horizontalMenu;
}

void GameSpeedDialog::rebuild() {
    delete _horizontalMenu;
    _horizontalMenu = nullptr;

    _menuModel.items.clear();
    _menuModel.currentSelection = 0;

    const uint speed = g_engine ? g_engine->getTextDelay() : 1;

    Common::Array<Common::String> labels;

    // S increments (slower); available below max (5).
    if (speed < 5)
        labels.push_back("Slower");

    // F decrements (faster); available above min (1).
    if (speed > 1)
        labels.push_back("Faster");

    labels.push_back("Exit");

    _menuModel.generateMenuItems(labels, true);

    const Common::String prompt = Common::String::format(
        "Game Speed: %u  ", speed);

    HorizontalMenuConfig cfg;
    cfg.promptTxt             = prompt;
    cfg.menuItemList          = &_menuModel;
    cfg.textColor             = 10;
    cfg.selectColor           = 15;
    cfg.promptColor           = 10;
    cfg.allowNumPad           = false;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor       = 8;
    cfg.singleItemMode        = false;

    _horizontalMenu = new HorizontalMenu("GameSpeedHMenu", cfg);
    _horizontalMenu->setParent(this);
    _horizontalMenu->activate();
}

void GameSpeedDialog::activate() {
    Dialog::activate();
    rebuild();
}

void GameSpeedDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void GameSpeedDialog::draw() {
    if (!_isVisible || !_horizontalMenu)
        return;
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 8);
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool GameSpeedDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;
    return _horizontalMenu->msgKeypress(msg);
}

void GameSpeedDialog::handleMenuResult(const MenuResultMessage &result) {
    if (!result._success) {
        // Escape — return to parent.
        deactivate();
        if (_parent)
            g_events->postMenuResult(_parent->getName(), true,
                Common::KEYCODE_RETURN, 0, Common::String(), false, false);
        return;
    }

    // HorizontalMenu posts _keyCode = the shortcut key that was pressed.
    char key = static_cast<char>(result._keyCode);
    if (key >= 'a' && key <= 'z')
        key = static_cast<char>(key - 32);

    if (key == 'S' && g_engine) {
        g_engine->setTextDelay(g_engine->getTextDelay() + 1);
        rebuild();
        return;
    }

    if (key == 'F' && g_engine) {
        g_engine->setTextDelay(g_engine->getTextDelay() - 1);
        rebuild();
        return;
    }

    // Exit (E), Return, or Space — return to parent.
    deactivate();
    if (_parent)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_RETURN, 0, Common::String(), false, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
