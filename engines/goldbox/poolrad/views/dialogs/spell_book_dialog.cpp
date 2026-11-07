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

#include "goldbox/poolrad/views/dialogs/spell_book_dialog.h"

#include "goldbox/events.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/spells_menu.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/poolrad_spell_mapping.h"
#include "goldbox/poolrad/spell_preparation.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/spells/spell_casting.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Dialogs::HorizontalMenu;
using Dialogs::HorizontalMenuConfig;
using Dialogs::HorizontalYesNo;
using Dialogs::HorizontalYesNoConfig;
using Dialogs::SpellsMenu;

SpellBookDialog::SpellBookDialog(const Common::String &name)
    : Dialog(name),
      _stage(STAGE_MAIN_MENU),
      _interrupted(false),
      _screenDirty(false),
      _character(nullptr),
      _horizontalMenu(nullptr),
      _spellsMenu(nullptr),
      _confirmDialog(nullptr),
      _displayMenu(nullptr),
      _pendingMemorizeSpells(false),
      _pendingScribeSpells(false) {

    buildMainMenu();

    HorizontalMenuConfig cfg;
    cfg.promptTxt = "";
    cfg.menuItemList = &_mainMenuModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 15;
    cfg.allowNumPad = true;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;

    _horizontalMenu = new HorizontalMenu(name + "HMenu", cfg);
    _horizontalMenu->setParent(this);
    subView(_horizontalMenu);
    _horizontalMenu->deactivate();

    // SpellsMenu — reused for cast/memorize/scribe sub-stages.
    _spellsMenu = new SpellsMenu(name + "SpellsMenu");
    _spellsMenu->setParent(this);
    subView(_spellsMenu);
    _spellsMenu->deactivate();

    HorizontalYesNoConfig ynCfg;
    ynCfg.promptTxt = "";
    ynCfg.promptColor = 15;
    ynCfg.textColor = 10;
    ynCfg.selectColor = 15;
    ynCfg.backgroundColor = 0;
    _confirmDialog = new HorizontalYesNo(name + "Confirm", ynCfg);
    _confirmDialog->setParent(this);
    subView(_confirmDialog);
    _confirmDialog->deactivate();

    // Active-effect display (SPELL_ScrollRead): a scrollable read-only list.
    VerticalMenuConfig dispCfg;
    dispCfg.promptTxt = "";
    dispCfg.promptOptions = nullptr;
    dispCfg.menuItemList = &_displayModel;
    dispCfg.headColor = 13;
    dispCfg.textColor = 10;
    dispCfg.selectColor = 15;
    dispCfg.xStart = 1;
    dispCfg.yStart = 3;
    dispCfg.xEnd = 38;
    dispCfg.yEnd = 22;
    dispCfg.title = "";
    dispCfg.addExit = true;
    _displayMenu = new VerticalMenu(name + "Display", dispCfg);
    _displayMenu->setParent(this);
    subView(_displayMenu);
    _displayMenu->deactivate();
}

SpellBookDialog::~SpellBookDialog() {
    // Child dialogs are owned via subView (UIElement children).
}

void SpellBookDialog::buildMainMenu() {
    _mainMenuModel.items.clear();
    _mainMenuModel.currentSelection = 0;

    Common::Array<Common::String> labels;
    labels.push_back("Cast");
    labels.push_back("Memorize");
    labels.push_back("Scribe");
    labels.push_back("Display");
    labels.push_back("Rest");
    labels.push_back("Exit");

    _mainMenuModel.generateMenuItems(labels, true);
}

// --- Dialog lifecycle ---

void SpellBookDialog::activate() {
    Dialog::activate();

    _interrupted = false;
    _screenDirty = false;
    _castOnly = false;
    _castListEmptied = false;
    _pendingMemorizeSpells = false;
    _pendingScribeSpells = false;

    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
        VmInterface::getSelectedCharacter());

    // Enter main menu stage.
    returnToMainMenu();
}

void SpellBookDialog::deactivate() {
    if (_horizontalMenu)
        _horizontalMenu->deactivate();
    if (_spellsMenu)
        _spellsMenu->deactivate();
    if (_confirmDialog)
        _confirmDialog->deactivate();
    if (_displayMenu)
        _displayMenu->deactivate();

    Dialog::deactivate();
}

void SpellBookDialog::beginCastOnly() {
    activate();
    _castOnly = true;
    setStage(STAGE_CAST);
}

// --- Main menu ---

void SpellBookDialog::returnToMainMenu() {
    // The map cast menu has no spell-book main menu to return to.
    if (_castOnly) {
        exitView();
        return;
    }

    debug(3, "SpellBookDialog::returnToMainMenu() stage was=%d spellsMenuActive=%d",
        (int)_stage, (int)(_spellsMenu && _spellsMenu->isActive()));

    if (_horizontalMenu)
        _horizontalMenu->deactivate();
    if (_spellsMenu)
        _spellsMenu->deactivate();
    if (_confirmDialog)
        _confirmDialog->deactivate();
    if (_displayMenu)
        _displayMenu->deactivate();

    buildMainMenu();

    if (_horizontalMenu)
        _horizontalMenu->activate();
    _stage = STAGE_MAIN_MENU;

    // Force full redraw from the top-level focused view so the campfire
    // viewport clears any full-screen spell sub-dialog content (rows 1-22).
    if (g_events && g_events->focusedView())
        g_events->focusedView()->redraw();
    else
        redraw();
}

void SpellBookDialog::setStage(Stage stage) {
    _stage = stage;

    switch (_stage) {
    case STAGE_MAIN_MENU:
        returnToMainMenu();
        break;
    case STAGE_CAST:
        beginCast();
        break;
    case STAGE_MEMORIZE:
        beginMemorize();
        break;
    case STAGE_SCRIBE:
        beginScribe();
        break;
    case STAGE_DISPLAY:
        beginDisplay();
        break;
    case STAGE_REST:
        beginRest();
        break;
    default:
        break;
    }
}

// --- Input handling ---

bool SpellBookDialog::msgKeypress(const KeypressMessage &msg) {
    if (!isActive())
        return false;

    // Party navigation keys: unwind sub-menus if needed, then pass
    // back to parent (InGameView) which owns the PartyList. The map cast
    // list (SpellSelection) keeps them for paging instead.
    if (!_castOnly) {
        switch (msg.keycode) {
        case Common::KEYCODE_KP_PLUS:
        case Common::KEYCODE_KP_MINUS:
        case Common::KEYCODE_PAGEUP:
        case Common::KEYCODE_PAGEDOWN:
            if (_stage != STAGE_MAIN_MENU)
                returnToMainMenu();
            return false; // let InGameView's PartyList handle cycling
        default:
            break;
        }
    }

    // Only the current stage's menu takes input. HorizontalMenu does not
    // check its own active flag, so offering the key to every child let the
    // hidden main menu turn 'E' or Enter into a bogus spell pick.
    if (_spellsMenu && _spellsMenu->isActive())
        return _spellsMenu->send(msg);
    if (_confirmDialog && _confirmDialog->isActive())
        return _confirmDialog->send(msg);
    if (_displayMenu && _displayMenu->isActive())
        return _displayMenu->send(msg);
    if (_horizontalMenu && _horizontalMenu->isActive())
        return _horizontalMenu->send(msg);
    return true;
}

void SpellBookDialog::handleMainMenuKey(char key) {
    switch (key) {
    case 'C': setStage(STAGE_CAST); break;
    case 'M': setStage(STAGE_MEMORIZE); break;
    case 'S': setStage(STAGE_SCRIBE); break;
    case 'D': setStage(STAGE_DISPLAY); break;
    case 'R': setStage(STAGE_REST); break;
    case 'E': exitView(); break;
    default:
        if (_horizontalMenu)
            _horizontalMenu->activate();
        break;
    }
}

// --- Draw ---

void SpellBookDialog::draw() {
    if (!isVisible())
        return;

    // Stage-specific drawing.
    if (_stage == STAGE_MAIN_MENU && _horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }

    if (_spellsMenu && _spellsMenu->isActive())
        _spellsMenu->draw();

    if (_confirmDialog && _confirmDialog->isActive())
        _confirmDialog->draw();

    if (_displayMenu && _displayMenu->isActive())
        _displayMenu->draw();
}

// --- MenuResult routing ---

void SpellBookDialog::handleMenuResult(const MenuResultMessage &result) {
    debug(3, "SpellBookDialog::handleMenuResult stage=%d success=%d key=%d hasInt=%d intVal=%d",
        (int)_stage, (int)result._success, (int)result._keyCode,
        (int)result._hasIntValue, (int)(result._hasIntValue ? result._intValue : -1));
    switch (_stage) {
    case STAGE_MAIN_MENU:
        handleMainMenuResult(result);
        break;
    case STAGE_CAST:
        handleCastResult(result);
        break;
    case STAGE_MEMORIZE:
        handleMemorizeResult(result);
        break;
    case STAGE_SCRIBE:
        handleScribeResult(result);
        break;
    case STAGE_MEMORIZE_CONFIRM:
    case STAGE_SCRIBE_CONFIRM:
        handleConfirmResult(result);
        break;
    case STAGE_DISPLAY:
        handleDisplayResult(result);
        break;
    default:
        break;
    }
}

void SpellBookDialog::handleMainMenuResult(const MenuResultMessage &result) {
    if (!result._success) {
        exitView();
        return;
    }

    if (!result._hasIntValue)
        return;

    const int sel = result._intValue;
    if (sel < 0 || sel >= (int)_mainMenuModel.items.size()) {
        if (_horizontalMenu)
            _horizontalMenu->activate();
        return;
    }

    handleMainMenuKey(_mainMenuModel.items[sel].shortcut);
}

// --- Sub-action entry points ---

void SpellBookDialog::beginCast() {
    // ACTION_CheckSpellActionType(SA_CAST): a non-caster cannot cast.
    if (!_character ||
            !Goldbox::Poolrad::SpellPreparation(_character)
                .canDoActivity(SpellPreparation::ACT_CAST)) {
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    _spellsMenu->configure(SpellsMenu::SL_IN_MEMORY, SpellsMenu::SA_CAST);
    _spellsMenu->activate();
}

void SpellBookDialog::beginMemorize() {
    // ACTION_CheckSpellActionType(SA_MEMORIZE): only casters with slots.
    if (!_character ||
            !Goldbox::Poolrad::SpellPreparation(_character)
                .canDoActivity(SpellPreparation::ACT_MEMORIZE)) {
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    _spellsMenu->configure(SpellsMenu::SL_IN_SPELL_BOOK,
        SpellsMenu::SA_MEMORIZE);
    _spellsMenu->activate();
}

void SpellBookDialog::beginScribe() {
    // ACTION_CheckSpellActionType(SA_SCRIBE): only magic-users with a book.
    if (!_character ||
            !Goldbox::Poolrad::SpellPreparation(_character)
                .canDoActivity(SpellPreparation::ACT_SCRIBE)) {
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    collectScribeScrolls();
    _spellsMenu->configure(SpellsMenu::SL_ON_SCROLLS, SpellsMenu::SA_SCRIBE);
    _spellsMenu->setScrollItems(&_scribeScrollItems);
    _spellsMenu->activate();
}

void SpellBookDialog::beginDisplay() {
    // SPELL_ScrollRead: build the selected character's active-effect list and
    // present it read-only. The selected character/location are preserved —
    // nothing here mutates spell state or the party selection.
    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    buildActiveEffectDisplay();

    if (_displayMenu) {
        _displayMenu->rebuild(&_displayModel, "");
        _displayMenu->activate();
    }
}

void SpellBookDialog::beginRest() {
    // Original ACTION_Rest: calculate max regain time from party, populate
    // the rest time struct, call DIALOG_RestInterrupt. Both the SpellBook and
    // Camp entry points drive the SAME shared rest operation; the engine owns
    // all timers/counters. The interruption result is reported back to the
    // caller the same way camp does (the ONRESTINTR ECL entry is run by the
    // wiring, not here).
    if (Goldbox::Poolrad::g_engine)
        _interrupted = Goldbox::Poolrad::g_engine->runRest();

    _screenDirty = true;
    // CampMagicMenu leaves its loop (and the camp) on an interruption.
    if (_interrupted) {
        exitView();
        return;
    }
    returnToMainMenu();
}

// --- Sub-action result handlers ---

void SpellBookDialog::handleCastResult(const MenuResultMessage &result) {
    debug(3, "SpellBookDialog::handleCastResult success=%d hasInt=%d intVal=%d",
        (int)result._success, (int)result._hasIntValue,
        (int)(result._hasIntValue ? result._intValue : -1));
    if (!result._success || !result._hasIntValue) {
        debug(3, "SpellBookDialog::handleCastResult -> returnToMainMenu");
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    int legacyIndex = result._intValue;
    debug(3, "SpellBookDialog: cast spell legacyIndex=%d -> re-entering loop", legacyIndex);

    // ACTION_Execute: cast through the shared spell service. A confirmed cast
    // consumes exactly the memorized slot (inside castSpell); a cancelled aim
    // returns CAST_ABORTED and consumes nothing.
    castMemorizedSpell(legacyIndex);

    _screenDirty = true;

    // NoncombatCastMenu: an empty list ends the loop ("has no spells
    // memorized" is printed by the host).
    if (_castOnly && (!_character ||
            Goldbox::Poolrad::SpellPreparation(_character)
                .countUsableMemorized() == 0)) {
        _castListEmptied = true;
        exitView();
        return;
    }

    // Re-enter cast loop (original loops until spell==0).
    _spellsMenu->configure(SpellsMenu::SL_IN_MEMORY, SpellsMenu::SA_CAST);
    _spellsMenu->activate();
}

void SpellBookDialog::castMemorizedSpell(int legacyIndex) {
    if (!_character || !Goldbox::Spells::g_spellCasting)
        return;
    if (legacyIndex < 0 ||
            legacyIndex >= Goldbox::Poolrad::Data::POOLRAD_PLAYER_KNOWN_SIZE)
        return;

    const Goldbox::Data::Spells::Spells spell =
        Goldbox::Poolrad::Data::kPoolradSpellMapping[legacyIndex];

    // Noncombat cast origin: a memorized slot is spent (fromMagicItem false),
    // never an item or temple charge, and distinct from the combat path.
    Goldbox::Spells::SpellContext ctx;
    ctx.caster = _character;
    ctx.spellBook = &_character->spellBook;
    ctx.inCombat = false;
    ctx.fromMagicItem = false;

    // castSpell consumes the memorized slot itself on success and leaves it
    // intact on CAST_ABORTED (cancel) / CAST_INVALID_TARGET.
    Goldbox::Spells::g_spellCasting->castSpell(ctx, spell);
}

void SpellBookDialog::handleMemorizeResult(const MenuResultMessage &result) {
    debug(3, "SpellBookDialog::handleMemorizeResult success=%d hasInt=%d intVal=%d",
        (int)result._success, (int)result._hasIntValue,
        (int)(result._hasIntValue ? result._intValue : -1));
    if (!result._success || !result._hasIntValue) {
        debug(3, "SpellBookDialog::handleMemorizeResult -> confirm/returnToMainMenu");
        if (_pendingMemorizeSpells && _confirmDialog) {
            // Ask whether to keep the queued (pending) memorization. A "No"
            // discards the pending marks without touching usable slots.
            _stage = STAGE_MEMORIZE_CONFIRM;
            _confirmDialog->activate();
            return;
        }
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    int legacyIndex = result._intValue;
    debug(5, "SpellBookDialog: memorize spell legacyIndex=%d", legacyIndex);

    // CHARACTER_GetRemainingSpellSlots + find empty slot + assign spellId|0x80
    // (pending) + CHARACTER_SortMemorizedSpells, all in SpellPreparation.
    // At capacity the queue is rejected and nothing changes.
    if (_character && legacyIndex >= 0 &&
            legacyIndex < Goldbox::Poolrad::Data::POOLRAD_PLAYER_KNOWN_SIZE) {
        const Goldbox::Data::Spells::Spells spell =
            Goldbox::Poolrad::Data::kPoolradSpellMapping[legacyIndex];
        Goldbox::Poolrad::SpellPreparation prep(_character);
        if (prep.queueMemorize(spell))
            _pendingMemorizeSpells = true;
    }

    _screenDirty = true;

    // Re-enter memorize loop.
    _spellsMenu->configure(SpellsMenu::SL_IN_SPELL_BOOK,
        SpellsMenu::SA_MEMORIZE);
    _spellsMenu->activate();
}

void SpellBookDialog::handleScribeResult(const MenuResultMessage &result) {
    debug(3, "SpellBookDialog::handleScribeResult success=%d hasInt=%d intVal=%d",
        (int)result._success, (int)result._hasIntValue,
        (int)(result._hasIntValue ? result._intValue : -1));
    if (!result._success || !result._hasIntValue) {
        debug(3, "SpellBookDialog::handleScribeResult -> confirm/returnToMainMenu");
        if (_pendingScribeSpells && _confirmDialog) {
            _stage = STAGE_SCRIBE_CONFIRM;
            _confirmDialog->activate();
            return;
        }
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    int legacyIndex = result._intValue;
    debug(5, "SpellBookDialog: scribe spell legacyIndex=%d", legacyIndex);

    // Mark the selected scroll payload pending (bit 7). The source scroll and
    // the matching effect slot come from the SpellsMenu selection. Rest
    // completes the scribe later; nothing is learned or removed here.
    Goldbox::Data::Items::CharacterItem *scroll =
        _spellsMenu ? _spellsMenu->getScribeSourceItem() : nullptr;
    if (_character && scroll &&
            legacyIndex >= 0 &&
            legacyIndex < Goldbox::Poolrad::Data::POOLRAD_PLAYER_KNOWN_SIZE) {
        const uint8 spellId = (uint8)(legacyIndex + 1);
        // Find the payload slot on the scroll that matches this spell id.
        uint8 *effects[3] = {
            &scroll->effect1, &scroll->effect2, &scroll->effect3
        };
        int payloadSlot = -1;
        for (int e = 0; e < 3; ++e) {
            if ((*effects[e] & 0x7F) == spellId && *effects[e] < 0x80) {
                payloadSlot = e;
                break;
            }
        }
        Goldbox::Poolrad::SpellPreparation prep(_character);
        if (payloadSlot >= 0 && prep.queueScribe(scroll, payloadSlot))
            _pendingScribeSpells = true;
    }

    _screenDirty = true;

    // Re-enter scribe loop (refresh the scroll list so pending payloads drop).
    collectScribeScrolls();
    _spellsMenu->configure(SpellsMenu::SL_ON_SCROLLS, SpellsMenu::SA_SCRIBE);
    _spellsMenu->setScrollItems(&_scribeScrollItems);
    _spellsMenu->activate();
}

void SpellBookDialog::handleConfirmResult(const MenuResultMessage &result) {
    // Y/N result for "Memorize/Scribe these spells?". "No" (success false,
    // or the N shortcut) discards the pending selection without touching
    // usable slots; "Yes" keeps the pending work for rest to complete.
    const bool keep = result._success &&
        result._keyCode != Common::KEYCODE_n;

    if (_character && !keep) {
        Goldbox::Poolrad::SpellPreparation prep(_character);
        if (_stage == STAGE_MEMORIZE_CONFIRM)
            prep.cancelPendingMemorize();
        else if (_stage == STAGE_SCRIBE_CONFIRM)
            prep.cancelPendingScribe();
    }

    _pendingMemorizeSpells = false;
    _pendingScribeSpells = false;

    if (_confirmDialog)
        _confirmDialog->deactivate();

    _screenDirty = true;
    returnToMainMenu();
}

void SpellBookDialog::handleDisplayResult(const MenuResultMessage &result) {
    (void)result;
    if (_displayMenu)
        _displayMenu->deactivate();
    _screenDirty = true;
    returnToMainMenu();
}

void SpellBookDialog::collectScribeScrolls() {
    _scribeScrollItems.clear();
    if (!_character)
        return;

    // SpellsMenu builds one SL_ON_SCROLLS list entry per non-empty payload
    // slot, so the scroll-item array must carry one pointer PER PAYLOAD to
    // stay 1:1 with those entries (getScribeSourceItem indexes it by entry).
    Goldbox::Data::Items::CharacterInventory &inv = _character->getInventory();
    const int itemCount = inv.count();
    for (int i = 0; i < itemCount; ++i) {
        Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;
        const uint8 effects[3] = { item.effect1, item.effect2, item.effect3 };
        for (int e = 0; e < 3; ++e) {
            if ((effects[e] & 0x7F) != 0)
                _scribeScrollItems.push_back(&item);
        }
    }
}

void SpellBookDialog::buildActiveEffectDisplay() {
    _displayModel.items.clear();
    _displayModel.currentSelection = 0;

    if (!_character)
        return;

    Goldbox::Data::Effects::CharacterEffects *fx = _character->getEffects();
    if (!fx || fx->isEmpty()) {
        MenuItem item;
        item.text = "  No active effects";
        item.active = false;
        item.shortcut = 0;
        _displayModel.items.push_back(item);
        return;
    }

    for (const Goldbox::Data::Effects::Effect &e : fx->effects()) {
        MenuItem item;
        item.text = Common::String::format("  Effect %u", (unsigned)e.id);
        item.active = false;
        item.shortcut = 0;
        _displayModel.items.push_back(item);
    }
}

void SpellBookDialog::exitView() {
    if (_screenDirty) {
        // Original: GAME_ScreenByState() — parent view redraws on refocus.
    }

    // As in-camp overlay dialog, close by deactivation.
    // Keep fallback pop behavior when this object is pushed as a top-level view.
    if (isFocused())
        close();
    else
        deactivate();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox