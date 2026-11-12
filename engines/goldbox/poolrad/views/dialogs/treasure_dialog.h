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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_TREASURE_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_TREASURE_DIALOG_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalMenu;
class PromptMessage;
class VerticalMenu;

/**
 * Post-combat treasure division dialog (DIALOG_Treasure equivalent).
 * No primary Buy/Heal action. Conditional Detect (D) action if the
 * selected character has Detect Magic (spell id 5) or Know Alignment
 * (spell id 11) memorized.
 * Exit text: "There is still treasure left."
 */
class TreasureDialog : public ShopBaseDialog {
public:
    TreasureDialog(const Common::String &name = "Treasure");
    ~TreasureDialog() override;

    void deactivate() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;
    void draw() override;

protected:
    void actionDetect() override;
    bool hasDetectSpell() const override;
    void actionTake() override;
    void onTakeClosed() override;
    void resetServiceChildren() override;

private:
    // PostCbt.LootTakeMenu / LootTakeItemMenu.
    enum TakeStage {
        TAKE_NONE = 0,
        TAKE_CHOICE,    // "Take: Money Items Exit"
        TAKE_ITEMS      // the Loot item list
    };
    TakeStage _takeStage = TAKE_NONE;
    // The Money/Items/Exit loop runs while both kinds are left.
    bool _inTakeChoice = false;
    MenuItemList _takeChoiceModel;
    HorizontalMenu *_takeChoiceMenu = nullptr;
    MenuItemList _lootItems;
    Common::Array<Common::String> _lootPromptOpts;
    VerticalMenu *_lootSelector = nullptr;
    PromptMessage *_lootPrompt = nullptr;

    void openTakeChoice();
    void closeTakeChoice();
    void handleTakeChoiceResult(const MenuResultMessage &result);
    void continueTakeChoice();
    void buildLootItemList();
    void openLootSelector();
    void closeLootSelector();
    void handleLootSelectorResult(const MenuResultMessage &result);
    void takeLootItem();
    void returnToLootMenu();
    void clearTakeChildren();

    uint8 _detectSpellType = 0;
    // True once a Detect spell has been confirmed in this treasure session;
    // drives the identify-marker display refresh without mutating per-item
    // identification (hidden) masks or rerolling generated items.
    bool _detectActive = false;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_TREASURE_DIALOG_H
