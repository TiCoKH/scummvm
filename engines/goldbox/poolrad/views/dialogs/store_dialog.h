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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_STORE_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_STORE_DIALOG_H

#include "goldbox/core/menu_item.h"
#include "common/list.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
namespace Items {
struct CharacterItem;
}
}
namespace Poolrad {
namespace Views {
namespace Dialogs {

class PartySelector;
class PromptMessage;
class VerticalMenu;

/**
 * Item shop dialog (DIALOG_Shop equivalent).
 * Primary action: Buy (B) — opens item purchase sub-menu.
 * Exit text references "the Shopkeeper".
 */
class StoreDialog : public ShopBaseDialog {
public:
    StoreDialog(const Common::String &name = "Store");
    ~StoreDialog() override;

    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;
    void draw() override;

protected:
    void actionPrimary() override;
    void onShopActivate() override;
    void getShopFlags(bool &hasItems, bool &hasMoney) override;

private:
    // Exchange rates and gold conversion now live in ValuableItems.

    enum BuyStage {
        BUY_NONE = 0,
        BUY_SELECTOR,
        BUY_RECEIVE
    };
    BuyStage _buyStage;

    MenuItemList _shopMenuItems;
    Common::Array<Common::String> _buyPromptOpts;
    VerticalMenu *_shopSelector;
    PartySelector *_partySelector;
    PromptMessage *_activePrompt;
    Goldbox::Data::Items::CharacterItem *_pendingItem;

    void buildShopItemList();
    void restoreItemDisplayText();
    void openBuySelector();
    void closeBuySelector();
    void handleShopSelectorResult(const MenuResultMessage &result);
    void attemptBuy();
    void handleReceiveResult(const MenuResultMessage &result);
    void deductFromParty(uint32 cost);
    void deductFromPool(uint32 cost);
    void showMessage(const Common::String &msg);

    static uint32 sumPartyGoldValue(
        const Common::List<Goldbox::Data::PlayerCharacter *> &party);
    static Goldbox::Data::ValuableItems collectPartyCoins(
        const Common::List<Goldbox::Data::PlayerCharacter *> &party);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_STORE_DIALOG_H
