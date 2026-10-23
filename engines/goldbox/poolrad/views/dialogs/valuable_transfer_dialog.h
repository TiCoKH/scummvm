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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_VALUABLE_TRANSFER_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_VALUABLE_TRANSFER_DIALOG_H

#include "common/array.h"
#include "common/str.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalInput;
class PromptMessage;
class VerticalMenu;

/**
 * Source/target abstraction for ValuableTransferDialog.
 *
 * The dialog only handles presentation and input. The source owns the
 * domain side: which balances are offered and how a transfer is applied.
 */
class ValuableTransferSource {
public:
    virtual ~ValuableTransferSource() {}

    /** Balance offered for a denomination (0 hides it from the list). */
    virtual uint16 getBalance(Goldbox::Data::ValuableType type) const = 0;

    /**
     * Apply a transfer of an already clamped amount.
     * Returns an empty string on success, or a message to show the player.
     */
    virtual Common::String transfer(Goldbox::Data::ValuableType type,
        uint16 amount) = 0;
};

/**
 * Moves valuables from the shared treasure pool to the selected character.
 * Domain side of ACTION_TakeFromMoneyPool (CHARACTER_addCoin).
 */
class TreasurePoolTakeSource : public ValuableTransferSource {
public:
    uint16 getBalance(Goldbox::Data::ValuableType type) const override;
    Common::String transfer(Goldbox::Data::ValuableType type,
        uint16 amount) override;
};

struct ValuableTransferConfig {
    Common::String selectPrompt;
    // printf format receiving the denomination name.
    Common::String amountPromptFormat;
    int windowX1, windowY1, windowX2, windowY2;
    int windowTitleColor;
    Common::String windowTitle;
    int listX1, listY1, listX2, listY2;
    int headColor;
    int textColor;
    int selectColor;
    uint8 maxDigits;
    // Return to the selector after a transfer while any balance remains.
    bool repeatWhileAvailable;

    ValuableTransferConfig();

    /** Layout and strings of the original ACTION_TakeFromMoneyPool. */
    static ValuableTransferConfig takeFromMoneyPool();
};

/**
 * Reusable "select a valuable type, enter an amount, transfer" dialog.
 *
 * Flow (ACTION_TakeFromMoneyPool):
 *  1. Draw the window and list every denomination with a nonzero balance.
 *  2. The player selects a denomination (ESC/Exit closes the dialog).
 *  3. The player enters an amount, clamped to the balance
 *     (INPUT_getIntWithMax). ESC/empty returns to the list.
 *  4. The source applies the transfer and may report a message.
 *  5. The list is rebuilt; the dialog closes when no balance remains.
 *
 * On close it posts a MenuResult to its parent:
 *  _success = at least one transfer succeeded, _intValue = transfer count.
 */
class ValuableTransferDialog : public Dialog {
public:
    ValuableTransferDialog(const Common::String &name,
        const ValuableTransferConfig &config);
    ~ValuableTransferDialog() override;

    /** Must be set before activate(); not owned. */
    void setSource(ValuableTransferSource *source) { _source = source; }

    /** True if the source currently has any nonzero balance. */
    bool hasAnyBalance() const;

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

    static const char *getValuableName(Goldbox::Data::ValuableType type);

private:
    enum Stage {
        STAGE_SELECT = 0,
        STAGE_AMOUNT
    };

    ValuableTransferConfig _config;
    ValuableTransferSource *_source;
    Stage _stage;
    MenuItemList _menuItems;
    Common::Array<Common::String> _promptOptions;
    // Replaces ITEM_getValuableType's parsing of the menu text.
    Common::Array<Goldbox::Data::ValuableType> _slotMap;
    VerticalMenu *_selector;
    HorizontalInput *_amountInput;
    PromptMessage *_promptMessage;
    Goldbox::Data::ValuableType _selectedType;
    int _transferCount;

    void buildMenuItems();
    void openSelector();
    void openAmountInput();
    void closeAmountInput();
    void finish(Common::KeyCode key);
    void showMessage(const Common::String &msg);
    void handleSelectorResult(const MenuResultMessage &result);
    void handleAmountResult(const MenuResultMessage &result);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_VALUABLE_TRANSFER_DIALOG_H
