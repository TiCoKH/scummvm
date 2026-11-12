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
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/shop_session.h"
#include "goldbox/poolrad/views/dialogs/valuable_transfer_dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalMenu;
class PartyList;
class PromptMessage;
class TextBoxDialog;

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
    // Colour of the second exit line (the first is light green).
    uint8 exitConfirmColor2 = 15;
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
 *  - Take (T): Select a pooled valuable, enter an amount, repeat until empty
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

    // --- GB13-025 explicit session lifecycle ---

    /**
     * GENUINE session entry (ST_ENTER_SHOP). Captures the caller location and
     * selected character, sets the service interface mode, and performs the
     * once-on-entry reset (pool/coin/session flags) EXACTLY ONCE. The view's
     * enterShop() calls this instead of activate() for a genuine entry.
     */
    void beginSession(const LocationContext &caller, uint16 selectedCharId);

    /**
     * REFOCUS / return from a pushed child (ViewCharacter or nested
     * selection). Rebuilds the menu and resumes the owning stage WITHOUT
     * re-running the entry reset, so the pooled money survives a View
     * round-trip. The view's msgFocus() refocus path calls this.
     */
    void resumeChild();

    /**
     * exit / cancel / quit. Resets every child and transaction stage,
     * restores the captured caller LocationContext and completes the host
     * request (kAsyncShopDone) exactly once. The view's exitShop() calls this.
     */
    void endSession();

    /** Read-only access to the session model (integration/tests). */
    const ShopSession &session() const { return _session; }

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
     * Query the shared treasure pool for money and item presence.
     * Direct equivalent of SHOP_getTreasurePoolFlags: checks all seven
     * coin denominations and whether the item list is non-empty.
     * Pure query — does not mutate pool state or draw anything.
     */
    void getTreasurePoolFlags(bool &hasMoney, bool &hasItems) const;

    /** Trace selected character and pool balances at debug level 1. */
    void debugShopMoney(const char *phase) const;

    /**
     * Check if detect magic spell is available (Treasure only).
     * Default returns false.
     */
    virtual bool hasDetectSpell() const { return false; }

    /**
     * Reset every type-specific child/transaction stage on
     * exit/cancel/quit. The base resets its shared transaction stages (Take,
     * Appraise, Exit-confirm); subclasses (Temple) override to also tear down
     * their service children (heal selector, Y/N prompts). Called from
     * endSession(). Default resets only the base stages.
     */
    virtual void resetServiceChildren();

    /** Session-ownership model (GB13-025). Shared by Store/Temple/Treasure. */
    ShopSession _session;

    /**
     * Called when the money Take dialog closes, after the menu rebuild.
     * Treasure uses it to return to its Money/Items choice.
     */
    virtual void onTakeClosed() {}

    /** Rebuild the main menu from the current pool flags. */
    void recreateHorizontalMenu();

    // --- Common action implementations ---
    void actionView();
    virtual void actionTake();
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
    TextBoxDialog *_textBox;
    PromptMessage *_promptMessage;
    bool _appraiseDone;
    bool _hasItems;
    bool _hasMoney;

    enum Stage {
        STAGE_MENU = 0,
        STAGE_TAKE,
        STAGE_APPRAISE_MENU,
        STAGE_APPRAISE_KEEP_SELL,
        STAGE_EXIT_CONFIRM
    };
    Stage _stage;

    HorizontalYesNo *_exitConfirmMenu;

    void buildMenuModel();
    void refreshScreen();
    void handleMenuKey(char key);
    void commitExit();
    void closeExitConfirm();

    // ACTION_TakeFromMoneyPool, hosted as a reusable child dialog.
    TreasurePoolTakeSource _takeSource;
    ValuableTransferDialog *_takeDialog;

    void handleTakeResult(const MenuResultMessage &result);

    // --- Appraise action state ---
    enum AppraiseType { APPRAISE_GEM = 0, APPRAISE_JEWELRY };
    HorizontalMenu *_appraiseMenu;
    HorizontalMenu *_appraiseKeepSellMenu;
    MenuItemList _appraiseMenuModel;
    MenuItemList _appraiseKSModel;
    uint16 _appraiseValue;
    AppraiseType _appraiseType;
    bool _appraiseSellOnly;

    void openAppraiseScreen();
    void closeAppraise();
    void buildAppraiseMenuModel();
    void handleAppraiseMenuResult(const MenuResultMessage &result);
    void appraiseItem(AppraiseType type);
    void buildKeepSellModel();
    void handleKeepSellResult(const MenuResultMessage &result);
    uint16 rollGemValue();
    uint16 rollJewelryValue();
    void showPromptMessage(const Common::String &msg);
    static void writeMessageWindow(Surface &s, int &col, int &row,
                                   uint8 color, const Common::String &text);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SHOP_BASE_DIALOG_H
