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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_SHOP_BASE_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_SHOP_BASE_DIALOG_H

#include "goldbox/core/menu_item.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalInput;
class HorizontalMenu;
class HorizontalYesNo;
class PartyList;
class TextBoxDialog;
class VerticalMenu;

/**
 * Enumeration of shop/service location types.
 * Each type has a unique primary action and exit confirmation text.
 */
enum ShopType {
    SHOP_STORE = 0,   // Item shop: Buy action
    SHOP_TEMPLE,      // Temple/healer: Heal action
    SHOP_TREASURE     // Post-combat treasure: Detect action (conditional)
};

/**
 * Configuration for ShopBaseDialog construction.
 * Provides type-specific strings and behavior flags.
 */
struct ShopBaseConfig {
    ShopType type;
    Common::String exitConfirmLine1;
    Common::String exitConfirmLine2;
    Common::String menuPrompt;
};

/**
 * Base dialog for Shop, Temple, and Treasure screens.
 *
 * All three share the same core loop:
 *  1. Query item/money flags → build dynamic menu
 *  2. Show horizontal menu (View/Take/Pool/Share/Appraise/Exit variants)
 *  3. Dispatch shortcut key to action handler
 *  4. Refresh screen after certain actions
 *  5. Repeat until exit confirmed
 *
 * Common actions handled by base:
 *  - View (V): View selected character
 *  - Take/Trade (T): Trade valuable / take treasure item
 *  - Pool (P): Pool party money
 *  - Share (S): Share party money
 *  - Appraise (A): Identify items (Shop/Temple only)
 *  - Exit (E): Exit with confirmation if items remain
 *  - Party navigation (numpad 1/7, arrow keys)
 *
 * Type-specific actions (override in subclass):
 *  - Buy (B): Shop only
 *  - Heal (H): Temple only
 *  - Detect (D): Treasure only (conditional on memorized spell)
 */
class ShopBaseDialog : public Dialog {
public:
    ShopBaseDialog(const Common::String &name, const ShopBaseConfig &config);
    ~ShopBaseDialog() override;

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

protected:
    // --- Type-specific hooks (override in subclass) ---

    /**
     * Primary action for this shop type.
     * Called when the type-specific shortcut is pressed (B/H).
     * Default implementation does nothing.
     */
    virtual void actionPrimary() {}

    /**
     * Detect action (Treasure only).
     * Called when 'D' is pressed and detect spell is available.
     */
    virtual void actionDetect() {}

    /**
     * Called after activate to perform type-specific initialization.
     * E.g., Shop pre-builds item display text list.
     */
    virtual void onShopActivate() {}

    /**
     * Query whether items/money remain in the shop/treasure pool.
     * Subclass must implement to check game state.
     * @param hasItems  Set true if unclaimed items exist
     * @param hasMoney  Set true if unclaimed money exists
     */
    virtual void getShopFlags(bool &hasItems, bool &hasMoney) = 0;

    /**
     * Check if detect magic spell is available (Treasure only).
     * Default returns false.
     */
    virtual bool hasDetectSpell() const { return false; }

    // --- Common action implementations ---
    void actionView();
    void actionTake();
    void actionPool();
    void actionShare();
    void actionAppraise();
    void actionExit();

    // --- State ---
    ShopType _shopType;
    bool _exitFlag;

private:
    ShopBaseConfig _config;
    MenuItemList _menuModel;
    HorizontalMenu *_horizontalMenu;
    HorizontalYesNo *_exitConfirm;
    TextBoxDialog *_textBox;
    bool _appraiseDone;
    bool _hasItems;
    bool _hasMoney;

    enum Stage {
        STAGE_MENU = 0,
        STAGE_EXIT_CONFIRM,
        STAGE_TAKE_SELECTOR,
        STAGE_TAKE_AMOUNT
    };
    Stage _stage;

    void buildMenuModel();
    void recreateHorizontalMenu();
    void refreshScreen();
    void handleMenuKey(char key);
    void handleExitConfirmResult(const MenuResultMessage &result);

    // --- Take action state ---
    static const char *kValuableNames[Goldbox::Data::VALUABLE_COUNT];
    MenuItemList _takeMenuItems;
    Common::Array<Common::String> _takePromptOpts;
    Common::Array<Goldbox::Data::ValuableType> _takeSlotMap;
    VerticalMenu *_takeSelector;
    HorizontalInput *_takeInput;
    Goldbox::Data::ValuableType _takeSelectedType;

    void openTakeSelector();
    void closeTakeSelector();
    void buildTakeMenuItems();
    void openTakeAmountInput();
    void handleTakeSelectorResult(const MenuResultMessage &result);
    void handleTakeAmountResult(const MenuResultMessage &result);
    bool poolHasValuables() const;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SHOP_BASE_DIALOG_H
