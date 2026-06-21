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
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/party_selector.h"
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
      _partySelector(nullptr), _activePrompt(nullptr),
      _pendingItem(nullptr) {
}

StoreDialog::~StoreDialog() {
    delete _shopSelector;
    delete _partySelector;
    delete _activePrompt;
}

void StoreDialog::onShopActivate() {
    _buyStage = BUY_NONE;
    _pendingItem = nullptr;
}

void StoreDialog::getShopFlags(bool &hasItems, bool &hasMoney) {
    const TreasurePool &pool = VmInterface::getTreasurePool();
    hasItems = pool.hasItems();
    hasMoney = pool.hasAnyCoin();
}

void StoreDialog::actionPrimary() {
    openBuySelector();
}

void StoreDialog::buildShopItemList() {
    _shopMenuItems.items.clear();
    _shopMenuItems.currentSelection = 0;

    TreasurePool &pool = VmInterface::getTreasurePool();
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

void StoreDialog::restoreItemDisplayText() {
    // Rebuild display text on pool items to undo price formatting.
    // The pool items' name field is unchanged; only the MenuItemList
    // text was formatted, so nothing to restore on the items themselves.
}

void StoreDialog::openBuySelector() {
    _buyStage = BUY_SELECTOR;
    buildShopItemList();

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
    if (_shopSelector) {
        _shopSelector->deactivate();
        delete _shopSelector;
        _shopSelector = nullptr;
    }
    restoreItemDisplayText();
    _buyStage = BUY_NONE;
    _pendingItem = nullptr;

    // Restore base shop screen that was overwritten by the buy window
    Surface s = getSurface();
    s.clearBox(1, 1, 38, 22, 0);
    redraw();
}

void StoreDialog::attemptBuy() {
    TreasurePool &pool = VmInterface::getTreasurePool();
    Common::Array<Goldbox::Data::Items::CharacterItem> &items = pool.items();
    int sel = _shopMenuItems.currentSelection;
    // Menu is in reverse order relative to pool array
    int poolIdx = (int)items.size() - 1 - sel;
    if (poolIdx < 0 || poolIdx >= (int)items.size()) {
        closeBuySelector();
        return;
    }

    _pendingItem = &items[poolIdx];
    uint16 cost = _pendingItem->value;

    // Check party money first
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    uint32 partyGold = sumPartyGoldValue(*party);

    if (partyGold >= cost) {
        // Party can afford — open party selector for receive
        _buyStage = BUY_RECEIVE;

        if (_partySelector) {
            delete _partySelector;
            _partySelector = nullptr;
        }

        PartySelectorConfig psCfg;
        psCfg.promptText = "Give to";
        psCfg.allowExit = true;
        psCfg.excludedCharacter = nullptr;

        _partySelector = new PartySelector("StoreBuyReceive", psCfg);
        setDialogParent(_partySelector, this);
        _partySelector->activate();
        redraw();
    } else {
        // Check pool money
        TreasurePool &poolRef = VmInterface::getTreasurePool();
        uint32 poolGold = poolRef.coins().getGoldValue();
        if (poolGold >= cost) {
            // Pool can afford — open party selector for receive
            _buyStage = BUY_RECEIVE;

            if (_partySelector) {
                delete _partySelector;
                _partySelector = nullptr;
            }

            PartySelectorConfig psCfg;
            psCfg.promptText = "Give to";
            psCfg.allowExit = true;
            psCfg.excludedCharacter = nullptr;

            _partySelector = new PartySelector("StoreBuyReceive", psCfg);
            setDialogParent(_partySelector, this);
            _partySelector->activate();
            redraw();
        } else {
            showMessage("Not enough money");
        }
    }
}

void StoreDialog::handleReceiveResult(const MenuResultMessage &result) {
    if (_partySelector) {
        _partySelector->deactivate();
        delete _partySelector;
        _partySelector = nullptr;
    }

    if (!result._success || !_pendingItem) {
        // Cancelled — return to shop selector
        _buyStage = BUY_SELECTOR;
        if (_shopSelector)
            _shopSelector->activate();
        redraw();
        return;
    }

    // Try to give item to selected character (copy item before pool removal)
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);

    Goldbox::Data::Items::CharacterItem itemCopy = *_pendingItem;
    _pendingItem = nullptr;

    if (!ch || !ch->receiveItem(itemCopy)) {
        // Character can't carry — return to selector
        showMessage("Overloaded!");
        _buyStage = BUY_SELECTOR;
        if (_shopSelector)
            _shopSelector->activate();
        redraw();
        return;
    }

    // Item received — deduct cost
    uint16 cost = itemCopy.value;
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    uint32 partyGold = sumPartyGoldValue(*party);

    if (partyGold >= cost) {
        deductFromParty(cost);
    } else {
        deductFromPool(cost);
    }

    // Remove item from pool
    TreasurePool &pool = VmInterface::getTreasurePool();
    Common::Array<Goldbox::Data::Items::CharacterItem> &items = pool.items();
    int sel = _shopMenuItems.currentSelection;
    int poolIdx = (int)items.size() - 1 - sel;
    if (poolIdx >= 0 && poolIdx < (int)items.size())
        items.remove_at(poolIdx);

    // Rebuild and continue shopping
    _buyStage = BUY_SELECTOR;
    buildShopItemList();
    if (_shopMenuItems.items.empty()) {
        closeBuySelector();
    } else {
        if (_shopSelector)
            _shopSelector->rebuild(&_shopMenuItems, "Items for Sale");
        redraw();
    }
}

void StoreDialog::deductFromParty(uint32 cost) {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    uint32 partyGold = sumPartyGoldValue(*party);
    uint32 newGold = partyGold - cost;

    // Zero all party coins then store remainder on selected character
    for (uint i = 0; i < party->size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[i]);
        if (pc)
            pc->valuableItems.setFromGoldValue(0);
    }

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (ch)
        ch->valuableItems.setFromGoldValue(newGold);
}

void StoreDialog::deductFromPool(uint32 cost) {
    TreasurePool &pool = VmInterface::getTreasurePool();
    uint32 poolGold = pool.coins().getGoldValue();
    pool.coins().setFromGoldValue(poolGold - cost);
}

void StoreDialog::showMessage(const Common::String &msg) {
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

Goldbox::Data::ValuableItems StoreDialog::collectPartyCoins(
        const Common::Array<Goldbox::Data::PlayerCharacter *> &party) {
    Goldbox::Data::ValuableItems total;
    for (uint i = 0; i < party.size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *ch =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(party[i]);
        if (ch && !ch->isNpc())
            total.addCoins(ch->valuableItems);
    }
    return total;
}

uint32 StoreDialog::sumPartyGoldValue(
        const Common::Array<Goldbox::Data::PlayerCharacter *> &party) {
    return collectPartyCoins(party).getGoldValue();
}

bool StoreDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_activePrompt && _activePrompt->isActive())
        return _activePrompt->handleKeypress(msg);

    switch (_buyStage) {
    case BUY_SELECTOR:
        if (_shopSelector && _shopSelector->isActive())
            return _shopSelector->dispatchKeypress(msg);
        break;
    case BUY_RECEIVE:
        if (_partySelector && _partySelector->isActive())
            return _partySelector->handleKeypress(msg);
        break;
    default:
        return ShopBaseDialog::msgKeypress(msg);
    }
    return true;
}

void StoreDialog::handleMenuResult(const MenuResultMessage &result) {
    switch (_buyStage) {
    case BUY_SELECTOR:
        handleShopSelectorResult(result);
        break;
    case BUY_RECEIVE:
        handleReceiveResult(result);
        break;
    default:
        ShopBaseDialog::handleMenuResult(result);
        break;
    }
}

void StoreDialog::handleShopSelectorResult(const MenuResultMessage &result) {
    Common::KeyCode key = result._keyCode;

    if (key == Common::KEYCODE_b || key == Common::KEYCODE_RETURN) {
        // Buy selected item
        attemptBuy();
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
    case BUY_RECEIVE:
        if (_partySelector && _partySelector->isActive())
            _partySelector->draw();
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
