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

#include "goldbox/poolrad/views/dialogs/treasure_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/poolrad/noncombat_spell_cast.h"
#include "goldbox/spells/spell_casting.h"
#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_handlers.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

static const uint8 SPELL_DETECT_MAGIC = 5;
static const uint8 SPELL_KNOW_ALIGNMENT = 11;
static const uint8 MAX_MEMORIZED_SPELLS = 21;

static ShopBaseConfig makeTreasureConfig() {
    ShopBaseConfig cfg;
    cfg.type = SHOP_TREASURE;
    cfg.menuPrompt = "";
    cfg.exitConfirmLine1 = "There is still treasure left.  ";
    cfg.exitConfirmLine2 = "Do you want to go back and claim your treasure?";
    cfg.exitConfirmColor2 = 15;
    return cfg;
}

TreasureDialog::TreasureDialog(const Common::String &name)
    : ShopBaseDialog(name, makeTreasureConfig()) {
}

TreasureDialog::~TreasureDialog() {
    delete _takeChoiceMenu;
    delete _lootSelector;
    delete _lootPrompt;
}

void TreasureDialog::deactivate() {
    clearTakeChildren();
    ShopBaseDialog::deactivate();
}

void TreasureDialog::resetServiceChildren() {
    clearTakeChildren();
    ShopBaseDialog::resetServiceChildren();
}

void TreasureDialog::clearTakeChildren() {
    closeTakeChoice();
    if (_lootSelector) {
        _lootSelector->deactivate();
        delete _lootSelector;
        _lootSelector = nullptr;
    }
    if (_lootPrompt) {
        _lootPrompt->deactivate();
        delete _lootPrompt;
        _lootPrompt = nullptr;
    }
    _takeStage = TAKE_NONE;
    _inTakeChoice = false;
}

// PostCbt.LootTakeMenu: both kinds left loop on a Money/Items/Exit menu,
// otherwise go straight to the money or the item menu.
void TreasureDialog::actionTake() {
    bool hasMoney = false;
    bool hasItems = false;
    getTreasurePoolFlags(hasMoney, hasItems);
    if (hasMoney && hasItems) {
        _inTakeChoice = true;
        openTakeChoice();
    } else if (hasMoney) {
        ShopBaseDialog::actionTake();
    } else if (hasItems) {
        openLootSelector();
    }
}

void TreasureDialog::onTakeClosed() {
    if (_inTakeChoice)
        continueTakeChoice();
}

void TreasureDialog::openTakeChoice() {
    closeTakeChoice();
    _takeStage = TAKE_CHOICE;

    Common::Array<Common::String> opts;
    opts.push_back("Money");
    opts.push_back("Items");
    opts.push_back("Exit");
    _takeChoiceModel.items.clear();
    _takeChoiceModel.currentSelection = 0;
    _takeChoiceModel.generateMenuItems(opts, true);

    HorizontalMenuConfig cfg;
    cfg.promptTxt = "Take: ";
    cfg.menuItemList = &_takeChoiceModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 13;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;

    _takeChoiceMenu = new HorizontalMenu("LootTakeMenu", cfg);
    setDialogParent(_takeChoiceMenu, this);
    _takeChoiceMenu->activate();
    redraw();
}

void TreasureDialog::closeTakeChoice() {
    if (_takeChoiceMenu) {
        _takeChoiceMenu->deactivate();
        delete _takeChoiceMenu;
        _takeChoiceMenu = nullptr;
    }
    if (_takeStage == TAKE_CHOICE)
        _takeStage = TAKE_NONE;
}

void TreasureDialog::handleTakeChoiceResult(const MenuResultMessage &result) {
    char key = 0;
    if (result._keyCode == Common::KEYCODE_RETURN) {
        const int sel = _takeChoiceModel.currentSelection;
        if (sel >= 0 && sel < (int)_takeChoiceModel.items.size())
            key = _takeChoiceModel.items[sel].shortcut;
    } else {
        key = (char)result._keyCode;
        if (key >= 'a' && key <= 'z')
            key -= 32;
    }

    closeTakeChoice();
    if (key == 'M') {
        ShopBaseDialog::actionTake();
    } else if (key == 'I') {
        openLootSelector();
    } else {
        // 'E' or Escape leaves the loop.
        _inTakeChoice = false;
        returnToLootMenu();
    }
}

void TreasureDialog::continueTakeChoice() {
    bool hasMoney = false;
    bool hasItems = false;
    getTreasurePoolFlags(hasMoney, hasItems);
    if (hasMoney && hasItems) {
        openTakeChoice();
        return;
    }
    _inTakeChoice = false;
    returnToLootMenu();
}

void TreasureDialog::returnToLootMenu() {
    _takeStage = TAKE_NONE;
    recreateHorizontalMenu();
    if (g_events)
        g_events->postEclStateMessage(EclVmMessage::ST_SCREEN_REFRESH, 0,
            EclVmMessage::VT_UINT8);
    redraw();
}

// The loot list is built by prepending, so it runs newest first.
void TreasureDialog::buildLootItemList() {
    const int keep = _lootItems.currentSelection;
    _lootItems.items.clear();

    const Common::Array<Goldbox::Data::Items::CharacterItem> &items =
        VmInterface::getTreasurePool().items();
    for (int i = (int)items.size() - 1; i >= 0; --i) {
        MenuItem mi;
        mi.text = items[i].getDisplayName();
        mi.shortcut = 0;
        mi.active = true;
        mi.shortcutFirst = false;
        _lootItems.items.push_back(mi);
    }

    _lootItems.currentSelection = 0;
    if (keep > 0 && !_lootItems.items.empty())
        _lootItems.currentSelection =
            MIN<int>(keep, (int)_lootItems.items.size() - 1);
}

// PostCbt.LootTakeItemSelection: the "Loot" window with an Items/Take menu.
void TreasureDialog::openLootSelector() {
    if (_lootSelector) {
        _lootSelector->deactivate();
        delete _lootSelector;
        _lootSelector = nullptr;
    }

    buildLootItemList();
    if (_lootItems.items.empty()) {
        closeLootSelector();
        return;
    }
    _takeStage = TAKE_ITEMS;

    _lootPromptOpts.clear();
    _lootPromptOpts.push_back("Take");

    VerticalMenuConfig cfg;
    cfg.promptTxt = "Items: ";
    cfg.promptOptions = &_lootPromptOpts;
    cfg.menuItemList = &_lootItems;
    cfg.headColor = 15;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.xStart = 1;
    cfg.yStart = 4;
    cfg.xEnd = 38;
    cfg.yEnd = 22;
    cfg.title = "";
    cfg.addExit = true;

    _lootSelector = new VerticalMenu("LootItemMenu", cfg);
    setDialogParent(_lootSelector, this);
    _lootSelector->activate();
    redraw();
}

void TreasureDialog::closeLootSelector() {
    if (_lootSelector) {
        _lootSelector->deactivate();
        delete _lootSelector;
        _lootSelector = nullptr;
    }
    _lootItems.currentSelection = 0;
    _takeStage = TAKE_NONE;

    if (_inTakeChoice)
        continueTakeChoice();
    else
        returnToLootMenu();
}

void TreasureDialog::handleLootSelectorResult(const MenuResultMessage &result) {
    const Common::KeyCode key = result._keyCode;
    if (result._success &&
            (key == Common::KEYCODE_t || key == Common::KEYCODE_RETURN))
        takeLootItem();
    else
        closeLootSelector();
}

// PostCbt.LootTakeItemMenu: give a copy to the current character (or print
// OverLoaded), then drop it from the loot; the menu ends when none is left.
void TreasureDialog::takeLootItem() {
    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    Common::Array<Goldbox::Data::Items::CharacterItem> &items = pool.items();
    const int sel = _lootItems.currentSelection;
    const int poolIdx = (int)items.size() - 1 - sel;
    if (poolIdx < 0 || poolIdx >= (int)items.size()) {
        closeLootSelector();
        return;
    }

    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
            VmInterface::getSelectedCharacter());
    if (!ch) {
        closeLootSelector();
        return;
    }

    Goldbox::Data::Items::CharacterItem item = items[poolIdx];
    item.readied = 0;
    if (!ch->receiveItem(item)) {
        if (_lootPrompt) {
            delete _lootPrompt;
            _lootPrompt = nullptr;
        }
        PromptMessageConfig cfg;
        cfg.message = "OverLoaded";
        cfg.textColor = 10;
        _lootPrompt = new PromptMessage("LootTakeMsg", cfg);
        setDialogParent(_lootPrompt, this);
        _lootPrompt->activate();
        if (_lootSelector)
            _lootSelector->activateHorizontalMenu();
        redraw();
        return;
    }

    items.remove_at(poolIdx);
    if (items.empty())
        closeLootSelector();
    else
        openLootSelector();
}

bool TreasureDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_lootPrompt && _lootPrompt->isActive()) {
        _lootPrompt->handleKeypress(msg);
        return true;
    }

    switch (_takeStage) {
    case TAKE_CHOICE:
        if (_takeChoiceMenu && _takeChoiceMenu->isActive())
            return _takeChoiceMenu->msgKeypress(msg);
        return true;
    case TAKE_ITEMS:
        if (_lootSelector && _lootSelector->isActive())
            return _lootSelector->dispatchKeypress(msg);
        return true;
    default:
        break;
    }
    return ShopBaseDialog::msgKeypress(msg);
}

void TreasureDialog::handleMenuResult(const MenuResultMessage &result) {
    switch (_takeStage) {
    case TAKE_CHOICE:
        handleTakeChoiceResult(result);
        return;
    case TAKE_ITEMS:
        handleLootSelectorResult(result);
        return;
    default:
        break;
    }
    ShopBaseDialog::handleMenuResult(result);
}

void TreasureDialog::draw() {
    if (!_isVisible)
        return;

    switch (_takeStage) {
    case TAKE_CHOICE:
        if (_takeChoiceMenu) {
            _takeChoiceMenu->setRedraw();
            _takeChoiceMenu->draw();
        }
        break;
    case TAKE_ITEMS:
        if (_lootSelector && _lootSelector->isActive()) {
            Surface s = getSurface();
            s.drawWindow(1, 1, 38, 22, 0, 15, "Loot");
            _lootSelector->draw();
        }
        break;
    default:
        ShopBaseDialog::draw();
        break;
    }

    if (_lootPrompt && _lootPrompt->isActive())
        _lootPrompt->draw();
}

bool TreasureDialog::hasDetectSpell() const {
    // Scan selected character's memorized spells for Detect Magic or
    // Know Alignment (matches original scan in DIALOG_Treasure).
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch)
        return false;

    bool hasItems = false;
    bool hasMoney = false;
    getTreasurePoolFlags(hasMoney, hasItems);
    if (!hasItems)
        return false;

    for (uint8 i = 0; i < MAX_MEMORIZED_SPELLS; ++i) {
        uint8 spellId = ch->spells.memorizedSpells[i];
        if (spellId == SPELL_DETECT_MAGIC || spellId == SPELL_KNOW_ALIGNMENT) {
            const_cast<TreasureDialog *>(this)->_detectSpellType = spellId;
            return true;
        }
    }
    return false;
}

void TreasureDialog::actionDetect() {
    // Resolve the selected character the same way hasDetectSpell() does; the
    // menu only offers Detect when hasDetectSpell() returned true, so a valid
    // caster with a memorized spell id 5/11 is expected here.
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    Goldbox::Poolrad::Data::PoolradCharacter *ch =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(base);
    if (!ch || !Goldbox::Spells::g_spellCasting)
        return;
    if (_detectSpellType != SPELL_DETECT_MAGIC &&
            _detectSpellType != SPELL_KNOW_ALIGNMENT)
        return;

    // The raw memorized id maps DIRECTLY to the Spells enum: id 5 ==
    // SP_CL1_DETECT_MAGIC, id 11 == SP_MUL1_DETECT_MAGIC (data/spells/spell.h).
    // (Know Alignment id 11 shares the Mage Detect Magic slot in the legacy
    // detect scan.) No table lookup is needed.
    const Goldbox::Data::Spells::Spells spell =
        static_cast<Goldbox::Data::Spells::Spells>(_detectSpellType);

    // Noncombat cast origin, identical to SpellBookDialog::castMemorizedSpell:
    // a memorized slot is spent (fromMagicItem false), never an item/temple
    // charge, and distinct from the combat path. castSpell consumes exactly
    // ONE memorized slot on CAST_OK and leaves it intact on CAST_ABORTED.
    Goldbox::Poolrad::NoncombatSpellCast cast(ch, false);
    const Goldbox::Spells::SpellCastResult result = cast.cast(spell);
    if (result.status != Goldbox::Spells::CAST_OK)
        return; // cancelled / failed: nothing consumed, no state change

    // Confirmed cast: change the detection display state and refresh WITHOUT
    // implicitly identifying every hidden property and WITHOUT rerolling the
    // generated items. We only flip the dialog's identify-display flag so the
    // item list re-renders with the Detect marker (getListDisplayText's
    // identifyActive path); the per-item `hidden` masks are left untouched.
    //
    // REVIEW-OPEN: the exact original "detection state" global (the loot
    // detect flag DIALOG_Treasure sets) could not be byte-confirmed. The
    // confirmable effects are implemented here: one spell consumed via the
    // noncombat path + display refresh with the Detect marker active. The
    // specific persistent detection-state bit is flagged for review.
    _detectActive = true;
    debug(3, "TreasureDialog::actionDetect: cast spell %u, detectActive=%d",
        (unsigned)_detectSpellType, (int)_detectActive);

    if (g_events)
        g_events->postEclStateMessage(EclVmMessage::ST_SCREEN_REFRESH, 0,
            EclVmMessage::VT_UINT8);
    redraw();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
