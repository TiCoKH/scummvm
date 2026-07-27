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
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/party_list.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/vm_interface.h"
#include "goldbox/engine.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

ShopBaseDialog::ShopBaseDialog(const Common::String &name,
        const ShopBaseConfig &config)
    : Dialog(name), _shopType(config.type), _exitFlag(false),
      _config(config), _horizontalMenu(nullptr),
      _textBox(nullptr), _promptMessage(nullptr), _appraiseDone(false),
      _hasItems(false), _hasMoney(false), _stage(STAGE_MENU),
      _takeSelector(nullptr), _takeInput(nullptr),
      _takeSelectedType(Goldbox::Data::VAL_COPPER),
      _appraiseMenu(nullptr), _appraiseKeepSellMenu(nullptr),
      _appraiseValue(0), _appraiseType(APPRAISE_GEM),
      _appraiseSellOnly(false) {
    setBounds(Window(0, 0, 39, 24));
}

ShopBaseDialog::~ShopBaseDialog() {
    delete _horizontalMenu;
    delete _takeSelector;
    delete _takeInput;
    delete _appraiseMenu;
    delete _appraiseKeepSellMenu;
    delete _promptMessage;
}

void ShopBaseDialog::activate() {
    Dialog::activate();
    _exitFlag = false;
    _appraiseDone = false;
    _stage = STAGE_MENU;

    onShopActivate();
    recreateHorizontalMenu();
}

void ShopBaseDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

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
    case STAGE_APPRAISE_MENU:
        if (_appraiseMenu) {
            _appraiseMenu->setRedraw();
            _appraiseMenu->draw();
        }
        return;
    case STAGE_APPRAISE_KEEP_SELL:
        if (_appraiseKeepSellMenu) {
            _appraiseKeepSellMenu->setRedraw();
            _appraiseKeepSellMenu->draw();
        }
        return;
    default:
        break;
    }

    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }

    if (_promptMessage && _promptMessage->isActive())
        _promptMessage->draw();
}

bool ShopBaseDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_promptMessage && _promptMessage->isActive()) {
        _promptMessage->handleKeypress(msg);
        return true;
    }

    switch (_stage) {
    case STAGE_TAKE_SELECTOR:
        if (_takeSelector && _takeSelector->isActive())
            return _takeSelector->dispatchKeypress(msg);
        return true;
    case STAGE_TAKE_AMOUNT:
        if (_takeInput && _takeInput->isActive())
            return _takeInput->handleKeypress(msg);
        return true;
    case STAGE_APPRAISE_MENU:
        if (_appraiseMenu && _appraiseMenu->isActive())
            return _appraiseMenu->msgKeypress(msg);
        return true;
    case STAGE_APPRAISE_KEEP_SELL:
        if (_appraiseKeepSellMenu && _appraiseKeepSellMenu->isActive())
            return _appraiseKeepSellMenu->msgKeypress(msg);
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
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    TreasurePool &pool = VmInterface::getTreasurePool();
    pool.poolMoneyFromParty(*party);
    refreshScreen();
}

void ShopBaseDialog::actionShare() {
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    TreasurePool &pool = VmInterface::getTreasurePool();
    pool.shareMoneyToParty(*party);
    refreshScreen();
}

void ShopBaseDialog::actionAppraise() {
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch)
        return;

    uint16 gems = ch->valuableItems.values[Goldbox::Data::VAL_GEMS];
    uint16 jewelry = ch->valuableItems.values[Goldbox::Data::VAL_JEWELRY];

    if (gems == 0 && jewelry == 0) {
        showPromptMessage("No Gems or Jewelry");
        return;
    }

    _appraiseDone = true;
    openAppraiseScreen();
}

void ShopBaseDialog::actionExit() {
    _exitFlag = true;
    if (_parent)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_e, 0, Common::String(), true, false);
    deactivate();
}

void ShopBaseDialog::handleMenuResult(const MenuResultMessage &result) {
    switch (_stage) {
    case STAGE_TAKE_SELECTOR:
        handleTakeSelectorResult(result);
        return;
    case STAGE_TAKE_AMOUNT:
        handleTakeAmountResult(result);
        return;
    case STAGE_APPRAISE_MENU:
        handleAppraiseMenuResult(result);
        return;
    case STAGE_APPRAISE_KEEP_SELL:
        handleKeepSellResult(result);
        return;
    default:
        break;
    }
}

void ShopBaseDialog::refreshScreen() {
    // Redraw main screen layout after actions that modify display
    // (Buy, Take, Appraise success).
    redraw();
}

void ShopBaseDialog::showPromptMessage(const Common::String &msg) {
    if (_promptMessage) {
        delete _promptMessage;
        _promptMessage = nullptr;
    }

    PromptMessageConfig cfg;
    cfg.message = msg;
    cfg.textColor = 10;
    _promptMessage = new PromptMessage("ShopPrompt", cfg);
    setDialogParent(_promptMessage, this);
    _promptMessage->activate();
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

// --- Appraise action implementation ---

uint16 ShopBaseDialog::rollGemValue() {
    int roll = VmInterface::rollDice(1, 100);
    if (roll <= 25)       return 10;
    else if (roll <= 50)  return 50;
    else if (roll <= 70)  return 100;
    else if (roll <= 90)  return 500;
    else if (roll <= 99)  return 1000;
    else                  return 5000;
}

uint16 ShopBaseDialog::rollJewelryValue() {
    int roll = VmInterface::rollDice(1, 100);
    if (roll <= 10)       return (uint16)(g_engine->getRandomNumber(899) + 100);
    else if (roll <= 20)  return (uint16)(g_engine->getRandomNumber(999) + 200);
    else if (roll <= 40)  return (uint16)(g_engine->getRandomNumber(1499) + 300);
    else if (roll <= 50)  return (uint16)(g_engine->getRandomNumber(2499) + 500);
    else if (roll <= 70)  return (uint16)(g_engine->getRandomNumber(4999) + 1000);
    else if (roll <= 90)  return (uint16)(g_engine->getRandomNumber(5999) + 2000);
    else                  return (uint16)(g_engine->getRandomNumber(9999) + 2000);
}

void ShopBaseDialog::openAppraiseScreen() {
    _stage = STAGE_APPRAISE_MENU;

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch) {
        closeAppraise();
        return;
    }

    uint16 gems = ch->valuableItems.values[Goldbox::Data::VAL_GEMS];
    uint16 jewelry = ch->valuableItems.values[Goldbox::Data::VAL_JEWELRY];

    // Draw info screen
    Surface s = getSurface();
    s.clearBox(1, 1, 38, 22, 0);
    s.writeStringC(1, 1, 15, ch->name);
    s.writeStringC(1, 7, 10, "You have a fine collection of:");

    if (gems > 0) {
        Common::String gemStr = Common::String::format("%u %s",
            gems, (gems == 1) ? "Gem" : "Gems");
        s.writeStringC(1, 9, 10, gemStr);
    }
    if (jewelry > 0) {
        Common::String jewStr = Common::String::format("%u %s",
            jewelry, (jewelry == 1) ? "piece of Jewelry" : "pieces of Jewelry");
        s.writeStringC(1, 10, 10, jewStr);
    }

    buildAppraiseMenuModel();

    if (_appraiseMenu) {
        _appraiseMenu->deactivate();
        delete _appraiseMenu;
        _appraiseMenu = nullptr;
    }

    HorizontalMenuConfig cfg;
    cfg.promptTxt = "Appraise :";
    cfg.menuItemList = &_appraiseMenuModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 13;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;

    _appraiseMenu = new HorizontalMenu("AppraiseMenu", cfg);
    setDialogParent(_appraiseMenu, this);
    _appraiseMenu->activate();
    redraw();
}

void ShopBaseDialog::buildAppraiseMenuModel() {
    _appraiseMenuModel.items.clear();
    _appraiseMenuModel.currentSelection = 0;

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch)
        return;

    Common::Array<Common::String> opts;
    if (ch->valuableItems.values[Goldbox::Data::VAL_GEMS] > 0)
        opts.push_back("Gems");
    if (ch->valuableItems.values[Goldbox::Data::VAL_JEWELRY] > 0)
        opts.push_back("Jewelry");
    opts.push_back("Exit");

    _appraiseMenuModel.generateMenuItems(opts, true);
}

void ShopBaseDialog::closeAppraise() {
    if (_appraiseMenu) {
        _appraiseMenu->deactivate();
        delete _appraiseMenu;
        _appraiseMenu = nullptr;
    }
    if (_appraiseKeepSellMenu) {
        _appraiseKeepSellMenu->deactivate();
        delete _appraiseKeepSellMenu;
        _appraiseKeepSellMenu = nullptr;
    }
    _stage = STAGE_MENU;

    Surface s = getSurface();
    s.clearBox(1, 1, 38, 22, 0);
    recreateHorizontalMenu();
    redraw();
}

void ShopBaseDialog::handleAppraiseMenuResult(const MenuResultMessage &result) {
    char key = 0;
    if (result._keyCode == Common::KEYCODE_RETURN) {
        int sel = _appraiseMenuModel.currentSelection;
        if (sel >= 0 && sel < (int)_appraiseMenuModel.items.size())
            key = _appraiseMenuModel.items[sel].shortcut;
    } else {
        key = (char)result._keyCode;
        if (key >= 'a' && key <= 'z')
            key -= 32;
        // Match by shortcut
        bool found = false;
        for (uint i = 0; i < _appraiseMenuModel.items.size(); ++i) {
            if (_appraiseMenuModel.items[i].shortcut == key) {
                found = true;
                break;
            }
        }
        if (!found)
            key = 0;
    }

    if (key == 'G') {
        appraiseItem(APPRAISE_GEM);
    } else if (key == 'J') {
        appraiseItem(APPRAISE_JEWELRY);
    } else {
        // Exit or Escape
        closeAppraise();
    }
}

void ShopBaseDialog::appraiseItem(AppraiseType type) {
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch) {
        closeAppraise();
        return;
    }

    _appraiseType = type;

    // Consume one gem or jewelry
    if (type == APPRAISE_GEM) {
        if (ch->valuableItems.values[Goldbox::Data::VAL_GEMS] == 0) {
            openAppraiseScreen();
            return;
        }
        ch->valuableItems.values[Goldbox::Data::VAL_GEMS]--;
        _appraiseValue = rollGemValue();
    } else {
        if (ch->valuableItems.values[Goldbox::Data::VAL_JEWELRY] == 0) {
            openAppraiseScreen();
            return;
        }
        ch->valuableItems.values[Goldbox::Data::VAL_JEWELRY]--;
        _appraiseValue = rollJewelryValue();
    }

    // Display appraisal result
    Surface s = getSurface();
    Common::String valueStr = Common::String::format("The %s is Valued at %u gp.",
        (type == APPRAISE_GEM) ? "Gem" : "Jewel", _appraiseValue);
    s.writeStringC(1, 12, 10, valueStr);

    // Determine keep/sell options
    buildKeepSellModel();

    _stage = STAGE_APPRAISE_KEEP_SELL;

    if (_appraiseKeepSellMenu) {
        _appraiseKeepSellMenu->deactivate();
        delete _appraiseKeepSellMenu;
        _appraiseKeepSellMenu = nullptr;
    }

    HorizontalMenuConfig cfg;
    cfg.promptTxt = "You can :";
    cfg.menuItemList = &_appraiseKSModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 13;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;

    _appraiseKeepSellMenu = new HorizontalMenu("AppraiseKSMenu", cfg);
    setDialogParent(_appraiseKeepSellMenu, this);
    _appraiseKeepSellMenu->activate();
    redraw();
}

void ShopBaseDialog::buildKeepSellModel() {
    _appraiseKSModel.items.clear();
    _appraiseKSModel.currentSelection = 0;

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);

    // Check if character can hold the item
    _appraiseSellOnly = false;
    if (ch) {
        Goldbox::Data::Items::CharacterItem testItem;
        testItem.name = (_appraiseType == APPRAISE_GEM) ? "Gem" : "Jewelry";
        testItem.nextAddress = 0;
        testItem.typeIndex = 70;
        testItem.nameCode1 = 0;
        testItem.nameCode2 = 0;
        testItem.nameCode3 = (_appraiseType == APPRAISE_GEM) ? (uint8)101 : (uint8)214;
        testItem.bonus = 0;
        testItem.saveBonus = 0;
        testItem.readied = 0;
        testItem.hidden = 0;
        testItem.cursed = 0;
        testItem.weight = 1;
        testItem.stackSize = 0;
        testItem.value = _appraiseValue;
        testItem.effect1 = 0;
        testItem.effect2 = 0;
        testItem.effect3 = 0;

        if (!ch->canReceiveItemLegacy(testItem))
            _appraiseSellOnly = true;
    }

    Common::Array<Common::String> opts;
    opts.push_back("Sell");
    if (!_appraiseSellOnly)
        opts.push_back("Keep");
    _appraiseKSModel.generateMenuItems(opts, true);
}

void ShopBaseDialog::handleKeepSellResult(const MenuResultMessage &result) {
    char key = 0;
    if (result._keyCode == Common::KEYCODE_RETURN) {
        int sel = _appraiseKSModel.currentSelection;
        if (sel >= 0 && sel < (int)_appraiseKSModel.items.size())
            key = _appraiseKSModel.items[sel].shortcut;
    } else {
        key = (char)result._keyCode;
        if (key >= 'a' && key <= 'z')
            key -= 32;
    }

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);

    if (key == 'K' && !_appraiseSellOnly && ch) {
        // Keep: add item to inventory
        Goldbox::Data::Items::CharacterItem newItem;
        newItem.name = (_appraiseType == APPRAISE_GEM) ? "Gem" : "Jewelry";
        newItem.nextAddress = 0;
        newItem.typeIndex = 70;
        newItem.nameCode1 = 0;
        newItem.nameCode2 = 0;
        newItem.nameCode3 = (_appraiseType == APPRAISE_GEM) ? (uint8)101 : (uint8)214;
        newItem.bonus = 0;
        newItem.saveBonus = 0;
        newItem.readied = 0;
        newItem.hidden = 0;
        newItem.cursed = 0;
        newItem.weight = 1;
        newItem.stackSize = 0;
        newItem.value = _appraiseValue;
        newItem.effect1 = 0;
        newItem.effect2 = 0;
        newItem.effect3 = 0;

        ch->receiveItem(newItem);
    } else {
        // Sell: receive 1/5 value as gold to character
        if (ch) {
            uint16 sellValue = _appraiseValue / 5;
            uint32 newGold = (uint32)ch->valuableItems.values[Goldbox::Data::VAL_GOLD] + sellValue;
            ch->valuableItems.values[Goldbox::Data::VAL_GOLD] =
                (newGold > 0xFFFF) ? (uint16)0xFFFF : (uint16)newGold;
        }
    }

    // Recalc combat stats after inventory/weight change
    if (ch)
        ch->recalcCombatStats();

    // Clean up keep/sell menu
    if (_appraiseKeepSellMenu) {
        _appraiseKeepSellMenu->deactivate();
        delete _appraiseKeepSellMenu;
        _appraiseKeepSellMenu = nullptr;
    }

    // Check if more gems/jewelry remain; if so, loop back
    if (ch) {
        uint16 gems = ch->valuableItems.values[Goldbox::Data::VAL_GEMS];
        uint16 jewelry = ch->valuableItems.values[Goldbox::Data::VAL_JEWELRY];
        if (gems > 0 || jewelry > 0) {
            openAppraiseScreen();
            return;
        }
    }

    closeAppraise();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
