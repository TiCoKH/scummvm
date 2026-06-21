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
#include "goldbox/poolrad/views/dialogs/horizontal_input.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/party_list.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/vm_interface.h"
#include "goldbox/runtime/treasure_pool.h"
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
      _hasItems(false), _hasMoney(false), _stage(STAGE_MENU),
      _takeSelector(nullptr), _takeInput(nullptr),
      _takeSelectedType(Goldbox::Data::VAL_COPPER) {
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
    delete _takeSelector;
    delete _takeInput;
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

    switch (_stage) {
    case STAGE_TAKE_SELECTOR:
        if (_takeSelector && _takeSelector->isActive()) {
            Surface s = getSurface();
            s.drawWindow(1, 1, 38, 22, 0, 28, "");
            _takeSelector->draw();
        }
        return;
    case STAGE_TAKE_AMOUNT:
        if (_takeInput && _takeInput->isActive())
            _takeInput->draw();
        return;
    case STAGE_EXIT_CONFIRM:
        if (_exitConfirm && _exitConfirm->isActive()) {
            _exitConfirm->draw();
            return;
        }
        break;
    default:
        break;
    }

    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }
}

bool ShopBaseDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    switch (_stage) {
    case STAGE_EXIT_CONFIRM:
        if (_exitConfirm && _exitConfirm->isActive())
            return _exitConfirm->msgKeypress(msg);
        return true;
    case STAGE_TAKE_SELECTOR:
        if (_takeSelector && _takeSelector->isActive())
            return _takeSelector->dispatchKeypress(msg);
        return true;
    case STAGE_TAKE_AMOUNT:
        if (_takeInput && _takeInput->isActive())
            return _takeInput->handleKeypress(msg);
        return true;
    default:
        break;
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
    if (_parent)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_v, 0, Common::String(), true, false);
}

void ShopBaseDialog::actionTake() {
    openTakeSelector();
}

void ShopBaseDialog::actionPool() {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    TreasurePool &pool = VmInterface::getTreasurePool();
    pool.poolMoneyFromParty(*party);
    refreshScreen();
}

void ShopBaseDialog::actionShare() {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    TreasurePool &pool = VmInterface::getTreasurePool();
    pool.shareMoneyToParty(*party);
    refreshScreen();
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
    switch (_stage) {
    case STAGE_EXIT_CONFIRM:
        handleExitConfirmResult(result);
        return;
    case STAGE_TAKE_SELECTOR:
        handleTakeSelectorResult(result);
        return;
    case STAGE_TAKE_AMOUNT:
        handleTakeAmountResult(result);
        return;
    default:
        break;
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

// --- Take action implementation ---

const char *ShopBaseDialog::kValuableNames[Goldbox::Data::VALUABLE_COUNT] = {
    "Copper", "Silver", "Electrum", "Gold", "Platinum", "Gems", "Jewelry"
};

bool ShopBaseDialog::poolHasValuables() const {
    const TreasurePool &pool = VmInterface::getTreasurePool();
    const Goldbox::Data::ValuableItems &coins = pool.coins();
    for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i) {
        if (coins.values[i] != 0)
            return true;
    }
    return false;
}

void ShopBaseDialog::buildTakeMenuItems() {
    _takeMenuItems.items.clear();
    _takeMenuItems.currentSelection = 0;
    _takeSlotMap.clear();

    const TreasurePool &pool = VmInterface::getTreasurePool();
    const Goldbox::Data::ValuableItems &coins = pool.coins();

    // Iterate 6 downto 0 (Jewelry first) matching original build order
    for (int i = Goldbox::Data::VALUABLE_COUNT - 1; i >= 0; --i) {
        if (coins.values[i] == 0)
            continue;

        Common::String amountTxt = Common::String::format("%u", coins.values[i]);
        Common::String nameTxt = kValuableNames[i];

        // Format: "Name    Amount" right-aligned in 30-char field
        const int fieldWidth = 30;
        int padding = fieldWidth - (int)nameTxt.size() - (int)amountTxt.size();
        if (padding < 1)
            padding = 1;

        Common::String line = nameTxt;
        for (int p = 0; p < padding; ++p)
            line += ' ';
        line += amountTxt;

        MenuItem mi;
        mi.text = line;
        mi.shortcut = 0;
        mi.active = true;
        mi.shortcutFirst = false;
        _takeMenuItems.items.push_back(mi);
        _takeSlotMap.push_back((Goldbox::Data::ValuableType)i);
    }
}

void ShopBaseDialog::openTakeSelector() {
    buildTakeMenuItems();

    if (_takeMenuItems.items.empty()) {
        // Nothing to take
        return;
    }

    _stage = STAGE_TAKE_SELECTOR;

    if (_takeSelector) {
        delete _takeSelector;
        _takeSelector = nullptr;
    }

    _takePromptOpts.clear();

    VerticalMenuConfig cfg;
    cfg.promptTxt = "Select type of coin ";
    cfg.promptOptions = &_takePromptOpts;
    cfg.menuItemList = &_takeMenuItems;
    cfg.headColor = 13;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.xStart = 2;
    cfg.yStart = 2;
    cfg.xEnd = 38;
    cfg.yEnd = 8;
    cfg.title = "";
    cfg.addExit = true;

    _takeSelector = new VerticalMenu("ShopTakeMenu", cfg);
    setDialogParent(_takeSelector, this);
    _takeSelector->activate();
    redraw();
}

void ShopBaseDialog::closeTakeSelector() {
    if (_takeSelector) {
        _takeSelector->deactivate();
        delete _takeSelector;
        _takeSelector = nullptr;
    }
    _stage = STAGE_MENU;

    // Restore screen area used by the take window
    Surface s = getSurface();
    s.clearBox(1, 1, 38, 22, 0);
    recreateHorizontalMenu();
    redraw();
}

void ShopBaseDialog::openTakeAmountInput() {
    _stage = STAGE_TAKE_AMOUNT;

    if (_takeInput) {
        delete _takeInput;
        _takeInput = nullptr;
    }

    Common::String prompt = Common::String::format(
        "How much %s will you take? ", kValuableNames[_takeSelectedType]);

    HorizontalInputConfig cfg;
    cfg.promptTxt = prompt;
    cfg.promptColor = 10;
    cfg.maxInputLength = 5;

    _takeInput = new HorizontalInput("ShopTakeInput", cfg);
    setDialogParent(_takeInput, this);
    _takeInput->activate();
    redraw();
}

void ShopBaseDialog::handleTakeSelectorResult(const MenuResultMessage &result) {
    Common::KeyCode key = result._keyCode;

    if (key == Common::KEYCODE_t || key == Common::KEYCODE_RETURN) {
        // Take selected valuable
        int sel = _takeMenuItems.currentSelection;
        if (sel >= 0 && sel < (int)_takeSlotMap.size()) {
            _takeSelectedType = _takeSlotMap[sel];
            openTakeAmountInput();
        }
    } else {
        // Exit take menu
        closeTakeSelector();
    }
}

void ShopBaseDialog::handleTakeAmountResult(const MenuResultMessage &result) {
    Common::KeyCode key = result._keyCode;
    Common::String inputStr = result._stringValue;

    if (_takeInput) {
        _takeInput->deactivate();
        delete _takeInput;
        _takeInput = nullptr;
    }

    if (key == Common::KEYCODE_ESCAPE || inputStr.empty()) {
        // Cancelled — return to take selector
        _stage = STAGE_TAKE_SELECTOR;
        if (_takeSelector)
            _takeSelector->activate();
        redraw();
        return;
    }

    // Parse amount and clamp to available
    uint32 amount = (uint32)atoi(inputStr.c_str());
    TreasurePool &pool = VmInterface::getTreasurePool();
    Goldbox::Data::ValuableItems &poolCoins = pool.coins();
    uint16 available = poolCoins.values[_takeSelectedType];

    if (amount > available)
        amount = available;
    if (amount == 0) {
        _stage = STAGE_TAKE_SELECTOR;
        if (_takeSelector)
            _takeSelector->activate();
        redraw();
        return;
    }

    // Transfer to selected character
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (ch) {
        uint32 newVal = (uint32)ch->valuableItems.values[_takeSelectedType] + amount;
        ch->valuableItems.values[_takeSelectedType] =
            (newVal > 0xFFFF) ? (uint16)0xFFFF : (uint16)newVal;
    }

    // Subtract from pool
    poolCoins.values[_takeSelectedType] = available - (uint16)amount;

    // Check if pool still has valuables
    if (poolHasValuables()) {
        // Rebuild and continue take loop
        buildTakeMenuItems();
        if (_takeMenuItems.items.empty()) {
            closeTakeSelector();
        } else {
            _stage = STAGE_TAKE_SELECTOR;
            if (_takeSelector)
                _takeSelector->rebuild(&_takeMenuItems, "Valuables");
            else
                openTakeSelector();
            redraw();
        }
    } else {
        // Pool empty — exit take
        closeTakeSelector();
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
