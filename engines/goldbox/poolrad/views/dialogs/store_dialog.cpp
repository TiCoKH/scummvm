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

#include "goldbox/poolrad/views/dialogs/store_dialog.h"
#include "common/debug.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/vm_interface.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

static ShopBaseConfig makeStoreConfig() {
    ShopBaseConfig cfg;
    cfg.type = SHOP_STORE;
    cfg.menuPrompt = "";
    cfg.exitConfirmLine1 = "As you Leave the Shopkeeper says,"
        " 'Excuse me but you left some things here.'";
    cfg.exitConfirmLine2 = "Do you want to go back and get your stuff?";
    return cfg;
}

StoreDialog::StoreDialog(const Common::String &name)
    : ShopBaseDialog(name, makeStoreConfig()),
      _buyStage(BUY_NONE), _shopSelector(nullptr),
      _activePrompt(nullptr) {
}

StoreDialog::~StoreDialog() {
    delete _shopSelector;
    delete _activePrompt;
}

void StoreDialog::deactivate() {
    if (_shopSelector) {
        _shopSelector->deactivate();
        delete _shopSelector;
        _shopSelector = nullptr;
    }
    if (_activePrompt) {
        _activePrompt->deactivate();
        delete _activePrompt;
        _activePrompt = nullptr;
    }
    _buyStage = BUY_NONE;
    ShopBaseDialog::deactivate();
}

void StoreDialog::onShopActivate() {
    _buyStage = BUY_NONE;
    buildShopItemList();
    debug(1, "[Shop:%s] store-activate: stockItems=%u",
        getName().c_str(), (unsigned)_shopMenuItems.items.size());
}

void StoreDialog::actionPrimary() {
    openBuySelector();
}

void StoreDialog::buildShopItemList() {
    _shopMenuItems.items.clear();
    _shopMenuItems.currentSelection = 0;

    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    Common::Array<Goldbox::Data::Items::CharacterItem> &items = pool.items();

    // Original uses a prepend-to-head linked list, so iteration order is
    // reversed compared to load order.  Iterate backwards to match.
    for (int i = (int)items.size() - 1; i >= 0; --i) {
        Goldbox::Data::Items::CharacterItem &item = items[i];
        if (item.value == 0)
            item.value = 1;

        // Format: "ItemName         Price" right-aligned in 30-char field
        Common::String priceTxt = Common::String::format("%u", item.value);
        Common::String nameTxt = item.getDisplayName();

        // Build a 30-character buffer with name left-aligned and price right-aligned
        const int fieldWidth = 30;
        int padding = fieldWidth - (int)nameTxt.size() - (int)priceTxt.size();
        if (padding < 1)
            padding = 1;

        Common::String line = nameTxt;
        for (int p = 0; p < padding; ++p)
            line += ' ';
        line += priceTxt;

        MenuItem mi;
        mi.text = line;
        mi.shortcut = 0;
        mi.active = true;
        mi.shortcutFirst = false;
        _shopMenuItems.items.push_back(mi);
    }
}

void StoreDialog::openBuySelector() {
    _buyStage = BUY_SELECTOR;
    buildShopItemList();
    debug(1, "[Shop:%s] buy-open: stockItems=%u",
        getName().c_str(), (unsigned)_shopMenuItems.items.size());
    debugShopMoney("buy:open");

    if (_shopMenuItems.items.empty()) {
        _buyStage = BUY_NONE;
        return;
    }

    if (_shopSelector) {
        delete _shopSelector;
        _shopSelector = nullptr;
    }

    _buyPromptOpts.clear();
    _buyPromptOpts.push_back("Buy");

    VerticalMenuConfig cfg;
    cfg.promptTxt = "Items: ";
    cfg.promptOptions = &_buyPromptOpts;
    cfg.menuItemList = &_shopMenuItems;
    cfg.headColor = 15;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.xStart = 1;
    cfg.yStart = 4;
    cfg.xEnd = 38;
    cfg.yEnd = 22;
    cfg.title = "";
    cfg.addExit = true;

    _shopSelector = new VerticalMenu("StoreBuyMenu", cfg);
    setDialogParent(_shopSelector, this);
    _shopSelector->activate();
    redraw();
}

void StoreDialog::closeBuySelector() {
    debugShopMoney("buy:close");
    if (_shopSelector) {
        _shopSelector->deactivate();
        delete _shopSelector;
        _shopSelector = nullptr;
    }
    _buyStage = BUY_NONE;

    if (g_events)
        g_events->postEclStateMessage(EclVmMessage::ST_SCREEN_REFRESH, 0,
            EclVmMessage::VT_UINT8);
    redraw();
}

void StoreDialog::attemptBuy() {
    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    Common::Array<Goldbox::Data::Items::CharacterItem> &items = pool.items();
    int sel = _shopMenuItems.currentSelection;
    // Menu is built in reverse order relative to pool array.
    int poolIdx = (int)items.size() - 1 - sel;
    debug(1, "[Shop:%s] buy-attempt: selection=%d poolIndex=%d stock=%u",
        getName().c_str(), sel, poolIdx, (unsigned)items.size());
    debugShopMoney("buy:before-receive");
    if (poolIdx < 0 || poolIdx >= (int)items.size()) {
        warning("StoreDialog: invalid stock selection %d", sel);
        closeBuySelector();
        return;
    }

    const Goldbox::Data::Items::CharacterItem &shopItem = items[poolIdx];
    const uint16 price = shopItem.value;
    debug(1, "[Shop:%s] buy-item: name='%s' type=%u price=%u "
        "weight=%u stack=%u",
        getName().c_str(), shopItem.getDisplayName().c_str(),
        (unsigned)shopItem.typeIndex, (unsigned)price,
        (unsigned)shopItem.weight, (unsigned)shopItem.stackSize);

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch) {
        warning("StoreDialog: no Poolrad character selected for purchase");
        showMessage("No character selected");
        return;
    }

    const uint32 charMoney = ch->valuableItems.getGoldValue();
    const bool payFromCharacter = charMoney >= price;
    Goldbox::Data::ValuableItems &payment = payFromCharacter
        ? ch->valuableItems : pool.coins();
    const uint32 availableMoney = payment.getGoldValue();
    debug(1, "[Shop:%s] buy-payment: source=%s characterGold=%u "
        "availableGold=%u price=%u",
        getName().c_str(), payFromCharacter ? "character" : "pool",
        (unsigned)charMoney, (unsigned)availableMoney, (unsigned)price);
    if (availableMoney < price) {
        showMessage("Not enough Money.");
        return;
    }

    // SHOP_BuyItem copies stock; it never unlinks the selected item.
    const bool received = ch->receiveItem(shopItem);
    debug(1, "[Shop:%s] buy-receive: success=%d",
        getName().c_str(), (int)received);
    debugShopMoney("buy:after-receive");
    if (!received) {
        showMessage("Overloaded");
        return;
    }

    const uint32 remainingMoney = availableMoney - price;
    debug(1, "[Shop:%s] buy-deduct: source=%s before=%u price=%u "
        "expectedRemainder=%u",
        getName().c_str(), payFromCharacter ? "character" : "pool",
        (unsigned)availableMoney, (unsigned)price, (unsigned)remainingMoney);
    payment.setFromGoldValue(remainingMoney);
    debugShopMoney("buy:after-payment");
    ch->recalcCombatStats();
    debugShopMoney("buy:after-recalc");
}

void StoreDialog::showMessage(const Common::String &msg) {
    debug(1, "[Shop:%s] message: %s", getName().c_str(), msg.c_str());
    if (_activePrompt) {
        delete _activePrompt;
        _activePrompt = nullptr;
    }

    PromptMessageConfig cfg;
    cfg.message = msg;
    cfg.textColor = 10;
    _activePrompt = new PromptMessage("StoreBuyMsg", cfg);
    setDialogParent(_activePrompt, this);
    _activePrompt->activate();
}

bool StoreDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    debug(1, "[Shop:%s] store-keypress: buyStage=%d key=%d ascii=%u",
        getName().c_str(), (int)_buyStage, (int)msg.keycode,
        (unsigned)msg.ascii);

    if (_activePrompt && _activePrompt->isActive()) {
        _activePrompt->handleKeypress(msg);
        return true;
    }

    switch (_buyStage) {
    case BUY_SELECTOR:
        if (_shopSelector && _shopSelector->isActive())
            return _shopSelector->dispatchKeypress(msg);
        break;
    default:
        return ShopBaseDialog::msgKeypress(msg);
    }
    return true;
}

void StoreDialog::handleMenuResult(const MenuResultMessage &result) {
    debug(1, "[Shop:%s] store-result: buyStage=%d success=%d key=%d "
        "hasValue=%d value=%d selection=%d",
        getName().c_str(), (int)_buyStage, (int)result._success,
        (int)result._keyCode, (int)result._hasIntValue,
        result._hasIntValue ? (int)result._intValue : -1,
        _shopMenuItems.currentSelection);
    switch (_buyStage) {
    case BUY_SELECTOR:
        handleShopSelectorResult(result);
        break;
    default:
        ShopBaseDialog::handleMenuResult(result);
        break;
    }
}

void StoreDialog::handleShopSelectorResult(const MenuResultMessage &result) {
    Common::KeyCode key = result._keyCode;

    if (result._success &&
            (key == Common::KEYCODE_b || key == Common::KEYCODE_RETURN)) {
        attemptBuy();
        if (_shopSelector) {
            _shopSelector->activateHorizontalMenu();
            redraw();
        }
    } else {
        // Exit buy menu (Escape or 'E')
        closeBuySelector();
    }
}

void StoreDialog::draw() {
    if (!_isVisible)
        return;

    switch (_buyStage) {
    case BUY_SELECTOR:
        if (_shopSelector && _shopSelector->isActive()) {
            Surface s = getSurface();
            s.drawWindow(1, 1, 38, 22, 0, 15, "Shop");
            _shopSelector->draw();
        }
        break;
    default:
        ShopBaseDialog::draw();
        break;
    }

    if (_activePrompt && _activePrompt->isActive())
        _activePrompt->draw();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
