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

#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/poolrad/views/dialogs/party_list.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

ShopBaseDialog::ShopBaseDialog(const Common::String &name,
        const ShopBaseConfig &config)
    : Dialog(name), _shopType(config.type), _exitFlag(false),
      _config(config), _horizontalMenu(nullptr), _exitConfirm(nullptr),
      _textBox(nullptr), _appraiseDone(false),
      _hasItems(false), _hasMoney(false), _stage(STAGE_MENU) {
    setBounds(Window(0, 0, 39, 24));

    HorizontalYesNoConfig ynCfg;
    ynCfg.promptTxt = "";
    ynCfg.promptColor = 13;
    ynCfg.textColor = 10;
    ynCfg.selectColor = 15;
    _exitConfirm = new HorizontalYesNo("ShopExitConfirm", ynCfg);
    _exitConfirm->deactivate();
    subView(_exitConfirm);
}

ShopBaseDialog::~ShopBaseDialog() {
    delete _horizontalMenu;
    delete _exitConfirm;
}

void ShopBaseDialog::activate() {
    Dialog::activate();
    _exitFlag = false;
    _appraiseDone = false;
    _stage = STAGE_MENU;

    if (_exitConfirm)
        _exitConfirm->deactivate();

    onShopActivate();
    recreateHorizontalMenu();
}

void ShopBaseDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    if (_exitConfirm)
        _exitConfirm->deactivate();

    Dialog::deactivate();
}

void ShopBaseDialog::buildMenuModel() {
    _menuModel.items.clear();
    _menuModel.currentSelection = 0;

    getShopFlags(_hasItems, _hasMoney);

    Common::Array<Common::String> menuStrings;

    switch (_shopType) {
    case SHOP_STORE:
        menuStrings.push_back("Buy");
        menuStrings.push_back("View");
        if (_hasMoney) {
            menuStrings.push_back("Take");
            menuStrings.push_back("Pool");
            menuStrings.push_back("Share");
        } else {
            menuStrings.push_back("Pool");
        }
        menuStrings.push_back("Appraise");
        menuStrings.push_back("Exit");
        break;

    case SHOP_TEMPLE:
        menuStrings.push_back("Heal");
        menuStrings.push_back("View");
        if (_hasMoney) {
            menuStrings.push_back("Take");
            menuStrings.push_back("Pool");
            menuStrings.push_back("Share");
        } else {
            menuStrings.push_back("Pool");
        }
        menuStrings.push_back("Appraise");
        menuStrings.push_back("Exit");
        break;

    case SHOP_TREASURE:
        menuStrings.push_back("View");
        if (_hasMoney) {
            menuStrings.push_back("Take");
            menuStrings.push_back("Pool");
            menuStrings.push_back("Share");
        } else if (_hasItems) {
            menuStrings.push_back("Take");
            menuStrings.push_back("Pool");
        } else {
            menuStrings.push_back("Pool");
        }
        if (hasDetectSpell())
            menuStrings.push_back("Detect");
        menuStrings.push_back("Exit");
        break;
    }

    _menuModel.generateMenuItems(menuStrings, true);
}

void ShopBaseDialog::recreateHorizontalMenu() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    buildMenuModel();

    HorizontalMenuConfig cfg;
    cfg.promptTxt = _config.menuPrompt;
    cfg.menuItemList = &_menuModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 13;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor = 0;

    _horizontalMenu = new HorizontalMenu("ShopHMenu", cfg);
    _horizontalMenu->activate();
}

void ShopBaseDialog::draw() {
    if (!_isVisible)
        return;

    if (_exitConfirm && _exitConfirm->isActive()) {
        _exitConfirm->draw();
        return;
    }

    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }
}

bool ShopBaseDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_stage == STAGE_EXIT_CONFIRM) {
        if (_exitConfirm && _exitConfirm->isActive())
            return _exitConfirm->msgKeypress(msg);
        return true;
    }

    if (!_horizontalMenu)
        return false;

    bool wasActive = _horizontalMenu->isActive();
    bool handled = _horizontalMenu->msgKeypress(msg);

    if (wasActive && !_horizontalMenu->isActive()) {
        if (msg.keycode == Common::KEYCODE_ESCAPE) {
            actionExit();
            redraw();
            return true;
        }

        // Party navigation keys: let them bubble up to InGameView.
        if (msg.keycode >= Common::KEYCODE_UP &&
                msg.keycode <= Common::KEYCODE_PAGEDOWN) {
            recreateHorizontalMenu();
            return false;
        }

        char ascii = msg.ascii;
        if (ascii >= 'a' && ascii <= 'z')
            ascii -= 32;

        bool validKey = false;
        for (uint i = 0; i < _menuModel.items.size(); ++i) {
            if (_menuModel.items[i].shortcut == ascii) {
                validKey = true;
                break;
            }
        }

        if (!validKey && msg.keycode == Common::KEYCODE_RETURN) {
            int sel = _menuModel.currentSelection;
            if (sel >= 0 && sel < (int)_menuModel.items.size()) {
                ascii = _menuModel.items[sel].shortcut;
                validKey = true;
            }
        }

        if (validKey)
            handleMenuKey(ascii);

        // Rebuild and reactivate menu unless exiting.
        if (_isActive && !_exitFlag && _stage == STAGE_MENU)
            recreateHorizontalMenu();
    }

    redraw();
    return handled;
}

void ShopBaseDialog::handleMenuKey(char key) {
    switch (key) {
    case 'B':
        if (_shopType == SHOP_STORE)
            actionPrimary();
        break;
    case 'H':
        if (_shopType == SHOP_TEMPLE)
            actionPrimary();
        break;
    case 'V':
        actionView();
        break;
    case 'T':
        actionTake();
        break;
    case 'P':
        actionPool();
        break;
    case 'S':
        actionShare();
        break;
    case 'A':
        actionAppraise();
        break;
    case 'D':
        if (_shopType == SHOP_TREASURE)
            actionDetect();
        break;
    case 'E':
        actionExit();
        break;
    default:
        break;
    }
}

void ShopBaseDialog::actionView() {
    addView("ViewCharacter");
}

void ShopBaseDialog::actionTake() {
    // TODO: Dispatch to DIALOG_TradeValuable (Shop/Temple)
    // or ACTION_Take (Treasure) based on _shopType.
}

void ShopBaseDialog::actionPool() {
    // TODO: ACTION_PoolMoney
}

void ShopBaseDialog::actionShare() {
    // TODO: ACTION_ShareMoney
}

void ShopBaseDialog::actionAppraise() {
    // TODO: SHOP_Appraise → set _appraiseDone on success
    _appraiseDone = true;
}

void ShopBaseDialog::actionExit() {
    getShopFlags(_hasItems, _hasMoney);

    // If nothing remains, exit immediately.
    if (!_hasItems && !_hasMoney) {
        _exitFlag = true;
        if (_parent)
            g_events->postMenuResult(_parent->getName(), true,
                Common::KEYCODE_e, 0, Common::String(), true, false);
        deactivate();
        return;
    }

    // Show exit confirmation prompt.
    _stage = STAGE_EXIT_CONFIRM;

    // Display confirmation text in text area.
    Surface s = getSurface();
    s.clearBox(1, 17, 38, 22, 0);
    s.writeStringC(1, 18, 10, _config.exitConfirmLine1);
    s.writeStringC(1, 20, 10, _config.exitConfirmLine2);

    if (_exitConfirm) {
        setDialogParent(_exitConfirm, this);
        _exitConfirm->activate();
    }
}

void ShopBaseDialog::handleMenuResult(const MenuResultMessage &result) {
    if (_stage == STAGE_EXIT_CONFIRM) {
        handleExitConfirmResult(result);
        return;
    }
}

void ShopBaseDialog::handleExitConfirmResult(const MenuResultMessage &result) {
    _stage = STAGE_MENU;

    if (_exitConfirm)
        _exitConfirm->deactivate();

    if (result._keyCode == Common::KEYCODE_y) {
        // User confirmed exit despite remaining items.
        _exitFlag = true;
        if (_parent)
            g_events->postMenuResult(_parent->getName(), true,
                Common::KEYCODE_e, 0, Common::String(), true, false);
        deactivate();
    } else {
        // User chose to go back.
        Surface s = getSurface();
        s.clearBox(1, 17, 38, 22, 0);
        recreateHorizontalMenu();
        redraw();
    }
}

void ShopBaseDialog::refreshScreen() {
    // Redraw main screen layout after actions that modify display
    // (Buy, Take, Appraise success).
    redraw();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
