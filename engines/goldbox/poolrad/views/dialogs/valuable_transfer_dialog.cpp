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

#include "goldbox/poolrad/views/dialogs/valuable_transfer_dialog.h"
#include "common/debug.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/events.h"
#include "goldbox/poolrad/views/dialogs/horizontal_input.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Goldbox::Data::ValuableType;

namespace {

const char *const kValuableNames[Goldbox::Data::VALUABLE_COUNT] = {
    "Copper", "Silver", "Electrum", "Gold", "Platinum", "Gems", "Jewelry"
};

} // namespace

// --- TreasurePoolTakeSource ---

uint16 TreasurePoolTakeSource::getBalance(ValuableType type) const {
    if (type < 0 || type >= Goldbox::Data::VALUABLE_COUNT)
        return 0;
    return VmInterface::getTreasurePool().coin(type);
}

Common::String TreasurePoolTakeSource::transfer(ValuableType type,
        uint16 amount) {
    Goldbox::Data::ADnDCharacter *ch =
        dynamic_cast<Goldbox::Data::ADnDCharacter *>(
            VmInterface::getSelectedCharacter());
    if (!ch) {
        warning("TreasurePoolTakeSource: no character selected");
        return "No character selected.";
    }

    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    debug(1, "[ValuableTransfer:PoolTake] before: name='%s' type=%d "
        "amount=%u character=%u pool=%u encumbrance=%u",
        ch->name.c_str(), (int)type, (unsigned)amount,
        (unsigned)ch->valuableItems.values[type],
        (unsigned)pool.coin(type), (unsigned)ch->encumbrance);

    const Runtime::TreasurePool::TakeMoneyResult result =
        pool.takeMoney(*ch, type, amount);

    debug(1, "[ValuableTransfer:PoolTake] after: result=%d character=%u "
        "pool=%u encumbrance=%u", (int)result,
        (unsigned)ch->valuableItems.values[type],
        (unsigned)pool.coin(type), (unsigned)ch->encumbrance);

    switch (result) {
    case Runtime::TreasurePool::TAKE_MONEY_OK:
        return Common::String();
    case Runtime::TreasurePool::TAKE_MONEY_OVERLOADED:
        return "Overloaded";
    case Runtime::TreasurePool::TAKE_MONEY_OVERFLOW:
        return "Too many valuables.";
    case Runtime::TreasurePool::TAKE_MONEY_INVALID_TYPE:
    default:
        return "Invalid valuable type.";
    }
}

// --- ValuableTransferConfig ---

ValuableTransferConfig::ValuableTransferConfig()
    : windowX1(1), windowY1(1), windowX2(38), windowY2(22),
      windowTitleColor(28), listX1(2), listY1(2), listX2(38), listY2(8),
      headColor(13), textColor(10), selectColor(15), maxDigits(5),
      repeatWhileAvailable(true) {
}

ValuableTransferConfig ValuableTransferConfig::takeFromMoneyPool() {
    ValuableTransferConfig cfg;
    cfg.selectPrompt = "Select type of coin ";
    cfg.amountPromptFormat = "How much %s will you take? ";
    return cfg;
}

// --- ValuableTransferDialog ---

ValuableTransferDialog::ValuableTransferDialog(const Common::String &name,
        const ValuableTransferConfig &config)
    : Dialog(name), _config(config), _source(nullptr),
      _stage(STAGE_SELECT), _selector(nullptr), _amountInput(nullptr),
      _promptMessage(nullptr), _selectedType(Goldbox::Data::VAL_COPPER),
      _transferCount(0) {
    setBounds(Window(0, 0, 39, 24));

    VerticalMenuConfig cfg;
    cfg.promptTxt = _config.selectPrompt;
    cfg.promptOptions = &_promptOptions;
    cfg.menuItemList = &_menuItems;
    cfg.headColor = _config.headColor;
    cfg.textColor = _config.textColor;
    cfg.selectColor = _config.selectColor;
    cfg.xStart = _config.listX1;
    cfg.yStart = _config.listY1;
    cfg.xEnd = _config.listX2;
    cfg.yEnd = _config.listY2;
    cfg.title = "";
    cfg.addExit = true;

    _selector = new VerticalMenu(name + "Menu", cfg);
    setDialogParent(_selector, this);
    _isActive = false;
    _isVisible = false;
}

ValuableTransferDialog::~ValuableTransferDialog() {
    // Delete owned children before ~UIElement deletes remaining children.
    delete _amountInput;
    delete _promptMessage;
    delete _selector;
}

const char *ValuableTransferDialog::getValuableName(ValuableType type) {
    if (type < 0 || type >= Goldbox::Data::VALUABLE_COUNT)
        return "";
    return kValuableNames[type];
}

bool ValuableTransferDialog::hasAnyBalance() const {
    if (!_source)
        return false;
    for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i) {
        if (_source->getBalance((ValuableType)i) != 0)
            return true;
    }
    return false;
}

void ValuableTransferDialog::activate() {
    Dialog::activate();
    _transferCount = 0;
    _menuItems.currentSelection = 0;

    if (_promptMessage) {
        _promptMessage->deactivate();
        delete _promptMessage;
        _promptMessage = nullptr;
    }

    if (!_source) {
        warning("ValuableTransferDialog: activated without a source");
        finish(Common::KEYCODE_ESCAPE);
        return;
    }

    openSelector();
}

void ValuableTransferDialog::deactivate() {
    closeAmountInput();
    if (_selector)
        _selector->deactivate();
    if (_promptMessage)
        _promptMessage->deactivate();

    Surface s = getSurface();
    s.clearBox(_config.windowX1, _config.windowY1,
        _config.windowX2, _config.windowY2, 0);

    Dialog::deactivate();
}

void ValuableTransferDialog::draw() {
    if (!_isVisible)
        return;

    Surface s = getSurface();
    s.drawWindow(_config.windowX1, _config.windowY1, _config.windowX2,
        _config.windowY2, 0, _config.windowTitleColor, _config.windowTitle);

    if (_selector && _selector->isActive())
        _selector->draw();
    if (_stage == STAGE_AMOUNT && _amountInput && _amountInput->isActive())
        _amountInput->draw();
    if (_promptMessage && _promptMessage->isActive())
        _promptMessage->draw();
}

bool ValuableTransferDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_promptMessage && _promptMessage->isActive()) {
        _promptMessage->handleKeypress(msg);
        return true;
    }

    switch (_stage) {
    case STAGE_SELECT:
        if (_selector && _selector->isActive())
            return _selector->dispatchKeypress(msg);
        break;
    case STAGE_AMOUNT:
        if (_amountInput && _amountInput->isActive())
            return _amountInput->handleKeypress(msg);
        break;
    }
    return true;
}

void ValuableTransferDialog::handleMenuResult(
        const MenuResultMessage &result) {
    if (!_isActive)
        return;

    debug(1, "[ValuableTransfer:%s] result: stage=%d success=%d key=%d",
        getName().c_str(), (int)_stage, (int)result._success,
        (int)result._keyCode);

    if (_stage == STAGE_SELECT)
        handleSelectorResult(result);
    else
        handleAmountResult(result);
}

void ValuableTransferDialog::buildMenuItems() {
    const int selection = _menuItems.currentSelection;
    _menuItems.items.clear();
    _slotMap.clear();

    // The original iterates 6..0 but prepends, displaying Copper first.
    for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i) {
        const uint16 balance = _source->getBalance((ValuableType)i);
        if (balance == 0)
            continue;

        const Common::String amountTxt =
            Common::String::format("%u", (unsigned)balance);
        Common::String line = kValuableNames[i];

        // "Name    Amount" right-aligned in a 30-character field.
        const int fieldWidth = 30;
        int padding = fieldWidth - (int)line.size() - (int)amountTxt.size();
        if (padding < 1)
            padding = 1;
        for (int p = 0; p < padding; ++p)
            line += ' ';
        line += amountTxt;

        MenuItem mi;
        mi.text = line;
        mi.shortcut = 0;
        mi.active = true;
        mi.shortcutFirst = false;
        _menuItems.items.push_back(mi);
        _slotMap.push_back((ValuableType)i);
    }

    _menuItems.currentSelection = _slotMap.empty() ? 0 :
        CLIP(selection, 0, (int)_slotMap.size() - 1);
}

void ValuableTransferDialog::openSelector() {
    buildMenuItems();

    if (_slotMap.empty()) {
        finish(Common::KEYCODE_t);
        return;
    }

    _stage = STAGE_SELECT;
    // rebuild() resets the selection; keep the player's position.
    const int selection = _menuItems.currentSelection;
    _selector->rebuild(&_menuItems, "");
    _menuItems.currentSelection = selection;
    _selector->activate();
    redraw();
}

void ValuableTransferDialog::openAmountInput() {
    closeAmountInput();
    _stage = STAGE_AMOUNT;
    _selector->deactivateHorizontalMenu();

    HorizontalInputConfig cfg;
    cfg.promptTxt = Common::String::format(_config.amountPromptFormat.c_str(),
        kValuableNames[_selectedType]);
    cfg.promptColor = _config.textColor;
    cfg.maxInputLength = _config.maxDigits;

    _amountInput = new HorizontalInput(getName() + "Input", cfg);
    setDialogParent(_amountInput, this);
    _amountInput->activate();
    redraw();
}

void ValuableTransferDialog::closeAmountInput() {
    if (!_amountInput)
        return;

    _amountInput->deactivate();
    _amountInput->clear();
    delete _amountInput;
    _amountInput = nullptr;
}

void ValuableTransferDialog::finish(Common::KeyCode key) {
    UIElement *parent = _parent;
    deactivate();

    debug(1, "[ValuableTransfer:%s] finish: key=%d transfers=%d",
        getName().c_str(), (int)key, _transferCount);

    if (parent && g_events)
        g_events->postMenuResult(parent->getName(), _transferCount > 0,
            key, _transferCount, Common::String(), true, false);
}

void ValuableTransferDialog::showMessage(const Common::String &msg) {
    if (_promptMessage) {
        _promptMessage->deactivate();
        delete _promptMessage;
        _promptMessage = nullptr;
    }

    PromptMessageConfig cfg;
    cfg.message = msg;
    cfg.textColor = _config.textColor;
    _promptMessage = new PromptMessage(getName() + "Prompt", cfg);
    setDialogParent(_promptMessage, this);
    _promptMessage->activate();
}

void ValuableTransferDialog::handleSelectorResult(
        const MenuResultMessage &result) {
    const Common::KeyCode key = result._keyCode;

    if (!result._success ||
            (key != Common::KEYCODE_t && key != Common::KEYCODE_RETURN)) {
        finish(Common::KEYCODE_ESCAPE);
        return;
    }

    const int sel = _menuItems.currentSelection;
    if (sel < 0 || sel >= (int)_slotMap.size()) {
        warning("ValuableTransferDialog: invalid selection %d", sel);
        finish(Common::KEYCODE_ESCAPE);
        return;
    }

    _selectedType = _slotMap[sel];
    openAmountInput();
}

void ValuableTransferDialog::handleAmountResult(
        const MenuResultMessage &result) {
    const Common::String &inputStr = result._stringValue;
    const bool cancelled = !result._success ||
        result._keyCode == Common::KEYCODE_ESCAPE || inputStr.empty();

    uint32 amount = 0;
    if (!cancelled) {
        if (!result._hasStringValue || inputStr.size() > _config.maxDigits) {
            showMessage("Enter a number.");
            return;
        }
        for (uint i = 0; i < inputStr.size(); ++i) {
            if (inputStr[i] < '0' || inputStr[i] > '9') {
                showMessage("Enter a number.");
                return;
            }
            amount = amount * 10 + (uint32)(inputStr[i] - '0');
        }
    }

    closeAmountInput();

    if (cancelled) {
        _stage = STAGE_SELECT;
        _selector->activate();
        redraw();
        return;
    }

    // INPUT_getIntWithMax clamps to the offered balance.
    amount = MIN<uint32>(amount, _source->getBalance(_selectedType));
    const Common::String error = _source->transfer(_selectedType,
        (uint16)amount);
    if (error.empty())
        ++_transferCount;

    if (g_events)
        g_events->postEclStateMessage(EclVmMessage::ST_SCREEN_REFRESH, 0,
            EclVmMessage::VT_UINT8);

    if (!_config.repeatWhileAvailable && error.empty()) {
        finish(Common::KEYCODE_t);
        return;
    }

    openSelector();
    if (_isActive && !error.empty())
        showMessage(error);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
