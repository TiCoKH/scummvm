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
#include "common/debug.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
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

namespace {

void debugShopCoins(const Common::String &shopName, const char *phase,
        const char *owner, const Goldbox::Data::ValuableItems &coins) {
    using namespace Goldbox::Data;
    debug(1, "[Shop:%s] %s %s: copper=%u silver=%u electrum=%u "
        "gold=%u platinum=%u gems=%u jewelry=%u goldValue=%u weight=%u",
        shopName.c_str(), phase, owner,
        (unsigned)coins[VAL_COPPER], (unsigned)coins[VAL_SILVER],
        (unsigned)coins[VAL_ELECTRUM], (unsigned)coins[VAL_GOLD],
        (unsigned)coins[VAL_PLATINUM], (unsigned)coins[VAL_GEMS],
        (unsigned)coins[VAL_JEWELRY], (unsigned)coins.getGoldValue(),
        (unsigned)coins.getTotalWeight());
}

} // namespace

ShopBaseDialog::ShopBaseDialog(const Common::String &name,
        const ShopBaseConfig &config)
    : Dialog(name), _shopType(config.type), _exitFlag(false),
      _config(config), _horizontalMenu(nullptr),
      _textBox(nullptr), _promptMessage(nullptr), _appraiseDone(false),
      _hasItems(false), _hasMoney(false), _stage(STAGE_MENU),
      _takeDialog(nullptr),
      _appraiseMenu(nullptr), _appraiseKeepSellMenu(nullptr),
      _appraiseValue(0), _appraiseType(APPRAISE_GEM),
      _appraiseSellOnly(false), _exitConfirmMenu(nullptr) {
    setBounds(Window(0, 0, 39, 24));
    _takeDialog = new ValuableTransferDialog(name + "Take",
        ValuableTransferConfig::takeFromMoneyPool());
    _takeDialog->setSource(&_takeSource);
}

ShopBaseDialog::~ShopBaseDialog() {
    delete _horizontalMenu;
    delete _takeDialog;
    delete _appraiseMenu;
    delete _appraiseKeepSellMenu;
    delete _promptMessage;
    delete _exitConfirmMenu;
}

void ShopBaseDialog::activate() {
    Dialog::activate();
    _exitFlag = false;
    _appraiseDone = false;
    _stage = STAGE_MENU;

    if (_exitConfirmMenu) {
        delete _exitConfirmMenu;
        _exitConfirmMenu = nullptr;
    }

    onShopActivate();
    recreateHorizontalMenu();
    debugShopMoney("activate");
}

void ShopBaseDialog::deactivate() {
    debugShopMoney("deactivate");
    if (_takeDialog) {
        if (_takeDialog->isActive())
            _takeDialog->deactivate();
        detachDialog(_takeDialog);
    }
    if (_promptMessage)
        _promptMessage->deactivate();
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

    getTreasurePoolFlags(_hasMoney, _hasItems);

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
    cfg.singleItemMode = false;

    _horizontalMenu = new HorizontalMenu("ShopHMenu", cfg);
    _horizontalMenu->activate();
}

void ShopBaseDialog::draw() {
    if (!_isVisible)
        return;

    switch (_stage) {
    case STAGE_TAKE:
        if (_takeDialog && _takeDialog->isVisible())
            _takeDialog->draw();
        break;
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
    case STAGE_EXIT_CONFIRM:
        if (_exitConfirmMenu && _exitConfirmMenu->isActive())
            _exitConfirmMenu->draw();
        return;
    default:
        if (_horizontalMenu) {
            _horizontalMenu->setRedraw();
            _horizontalMenu->draw();
        }
        break;
    }

    if (_promptMessage && _promptMessage->isActive())
        _promptMessage->draw();
}

bool ShopBaseDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    debug(1, "[Shop:%s] keypress: stage=%d key=%d ascii=%u",
        getName().c_str(), (int)_stage, (int)msg.keycode,
        (unsigned)msg.ascii);

    if (_promptMessage && _promptMessage->isActive()) {
        _promptMessage->handleKeypress(msg);
        return true;
    }

    switch (_stage) {
    case STAGE_TAKE:
        if (_takeDialog && _takeDialog->isActive())
            return _takeDialog->msgKeypress(msg);
        return true;
    case STAGE_APPRAISE_MENU:
        if (_appraiseMenu && _appraiseMenu->isActive())
            return _appraiseMenu->msgKeypress(msg);
        return true;
    case STAGE_APPRAISE_KEEP_SELL:
        if (_appraiseKeepSellMenu && _appraiseKeepSellMenu->isActive())
            return _appraiseKeepSellMenu->msgKeypress(msg);
        return true;
    case STAGE_EXIT_CONFIRM:
        if (_exitConfirmMenu && _exitConfirmMenu->isActive())
            return _exitConfirmMenu->msgKeypress(msg);
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
    debug(1, "[Shop:%s] action: key='%c' stage=%d",
        getName().c_str(), key, (int)_stage);
    debugShopMoney("before-action");
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
    debugShopMoney("view-character");
    if (_parent)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_v, 0, Common::String(), true, false);
}

void ShopBaseDialog::actionTake() {
    if (!_takeDialog->hasAnyBalance())
        return;

    debugShopMoney("take:open");
    _stage = STAGE_TAKE;
    attachDialog(_takeDialog);
    _takeDialog->activate();
    redraw();
}

void ShopBaseDialog::actionPool() {
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    pool.poolMoneyFromParty(*party);
    debugShopMoney("after-pool");
    refreshScreen();
}

void ShopBaseDialog::actionShare() {
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    pool.shareMoneyToParty(*party);
    debugShopMoney("after-share");
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
    bool hasItems, hasMoney;
    getTreasurePoolFlags(hasMoney, hasItems);

    if (!hasMoney) {
        commitExit();
        return;
    }

    // Pool/items remain — show shopkeeper warning and Y/N confirmation.
    _stage = STAGE_EXIT_CONFIRM;

    Surface s = getSurface();
    s.clearBox(0, 17, 39, 22, 0);
    s.writeStringC(0, 17, 10, _config.exitConfirmLine1);
    s.writeStringC(0, 18, 10, _config.exitConfirmLine2);

    if (_exitConfirmMenu) {
        delete _exitConfirmMenu;
        _exitConfirmMenu = nullptr;
    }

    HorizontalYesNoConfig cfg;
    cfg.promptTxt = "";
    cfg.promptColor = 13;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.backgroundColor = 0;

    _exitConfirmMenu = new HorizontalYesNo("ShopExitConfirm", cfg);
    setDialogParent(_exitConfirmMenu, this);
    _exitConfirmMenu->activate();
    redraw();
}

void ShopBaseDialog::commitExit() {
    debugShopMoney("exit");
    _exitFlag = true;
    if (_exitConfirmMenu) {
        _exitConfirmMenu->deactivate();
        delete _exitConfirmMenu;
        _exitConfirmMenu = nullptr;
    }
    if (_parent)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_e, 0, Common::String(), true, false);
    deactivate();
}

void ShopBaseDialog::closeExitConfirm() {
    if (_exitConfirmMenu) {
        _exitConfirmMenu->deactivate();
        delete _exitConfirmMenu;
        _exitConfirmMenu = nullptr;
    }
    _stage = STAGE_MENU;

    // Clear the confirmation text area (rows 17-22).
    Surface s = getSurface();
    s.clearBox(0, 17, 39, 22, 0);

    recreateHorizontalMenu();
    redraw();
}

void ShopBaseDialog::handleMenuResult(const MenuResultMessage &result) {
    debug(1, "[Shop:%s] menu-result: stage=%d success=%d key=%d "
        "hasValue=%d value=%d",
        getName().c_str(), (int)_stage, (int)result._success,
        (int)result._keyCode, (int)result._hasIntValue,
        result._hasIntValue ? (int)result._intValue : -1);
    switch (_stage) {
    case STAGE_TAKE:
        handleTakeResult(result);
        return;
    case STAGE_APPRAISE_MENU:
        handleAppraiseMenuResult(result);
        return;
    case STAGE_APPRAISE_KEEP_SELL:
        handleKeepSellResult(result);
        return;
    case STAGE_EXIT_CONFIRM: {
        // Y (intValue==1) => leave; N/ESC => cancel and clear confirmation UI
        bool confirmed = result._hasIntValue && result._intValue == 1;
        if (confirmed)
            commitExit();
        else
            closeExitConfirm();
        return;
    }
    default:
        break;
    }
}

void ShopBaseDialog::getTreasurePoolFlags(bool &hasMoney, bool &hasItems) const {
    const Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    hasMoney = pool.hasAnyCoin();
    hasItems = pool.hasItems();
}

void ShopBaseDialog::debugShopMoney(const char *phase) const {
    if (!g_engine) {
        debug(1, "[Shop:%s] %s: no engine", getName().c_str(), phase);
        return;
    }

    Goldbox::Poolrad::Data::PoolradCharacter *character =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
            VmInterface::getSelectedCharacter());
    if (character) {
        debug(1, "[Shop:%s] %s character=%p name='%s' items=%d "
            "encumbrance=%u",
            getName().c_str(), phase, static_cast<void *>(character),
            character->name.c_str(), character->getInventory().count(),
            (unsigned)character->encumbrance);
        debugShopCoins(getName(), phase, "character",
            character->valuableItems);
    } else {
        debug(1, "[Shop:%s] %s: no Poolrad character selected",
            getName().c_str(), phase);
    }
    debugShopCoins(getName(), phase, "pool",
        VmInterface::getTreasurePool().coins());
}

void ShopBaseDialog::refreshScreen() {
    if (g_events)
        g_events->postEclStateMessage(EclVmMessage::ST_SCREEN_REFRESH, 0,
            EclVmMessage::VT_UINT8);
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

void ShopBaseDialog::handleTakeResult(const MenuResultMessage &result) {
    debug(1, "[Shop:%s] take:closed transfers=%d", getName().c_str(),
        result._hasIntValue ? (int)result._intValue : 0);
    detachDialog(_takeDialog);
    _stage = STAGE_MENU;
    debugShopMoney("take:close");
    recreateHorizontalMenu();
    refreshScreen();
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
    cfg.singleItemMode = false;

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
    cfg.singleItemMode = false;

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
