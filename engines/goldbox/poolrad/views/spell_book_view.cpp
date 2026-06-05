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

#include "goldbox/poolrad/views/spell_book_view.h"

#include "goldbox/events.h"
#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/party_list.h"
#include "goldbox/poolrad/views/dialogs/spells_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

using Dialogs::HorizontalMenu;
using Dialogs::HorizontalMenuConfig;
using Dialogs::HorizontalYesNo;
using Dialogs::HorizontalYesNoConfig;
using Dialogs::PartyList;
using Dialogs::SpellsMenu;

SpellBookView::SpellBookView()
    : View("SpellBook"),
      _stage(STAGE_MAIN_MENU),
      _interrupted(false),
      _screenDirty(false),
      _character(nullptr),
      _horizontalMenu(nullptr),
      _spellsMenu(nullptr),
      _partyList(nullptr),
      _confirmDialog(nullptr),
      _pendingMemorizeSpells(false),
      _pendingScribeSpells(false) {

    // PartyList — always-visible party display (right side).
    _partyList = new PartyList("SpellBookPartyList");
    subView(_partyList);

    // SpellsMenu — reused for cast/memorize/scribe sub-stages.
    _spellsMenu = new SpellsMenu("SpellBookSpellsMenu");
    _spellsMenu->setParent(this);
    subView(_spellsMenu);
    _spellsMenu->deactivate();
}

SpellBookView::~SpellBookView() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    if (_confirmDialog) {
        delete _confirmDialog;
        _confirmDialog = nullptr;
    }
    // _spellsMenu and _partyList are owned via subView (UIElement children).
}

void SpellBookView::buildMainMenu() {
    _mainMenuModel.items.clear();
    _mainMenuModel.currentSelection = 0;

    Common::Array<Common::String> labels;
    labels.push_back("Cast");
    labels.push_back("Memorize");
    labels.push_back("Scribe");
    labels.push_back("Display");
    labels.push_back("Rest");

    _mainMenuModel.generateMenuItems(labels, true);
}

// --- View lifecycle ---

void SpellBookView::onEnter(Goldbox::GameState /*state*/) {
    _interrupted = false;
    _screenDirty = false;
    _pendingMemorizeSpells = false;
    _pendingScribeSpells = false;
}

bool SpellBookView::msgFocus(const FocusMessage &msg) {
    View::msgFocus(msg);

    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
        VmInterface::getSelectedCharacter());

    // Activate party list.
    if (_partyList)
        _partyList->activate();

    // Enter main menu stage.
    returnToMainMenu();
    return true;
}

bool SpellBookView::msgUnfocus(const UnfocusMessage &msg) {
    if (_spellsMenu)
        _spellsMenu->deactivate();
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    if (_confirmDialog) {
        _confirmDialog->deactivate();
        delete _confirmDialog;
        _confirmDialog = nullptr;
    }
    if (_partyList)
        _partyList->deactivate();

    return View::msgUnfocus(msg);
}

// --- Main menu ---

void SpellBookView::returnToMainMenu() {
    if (_spellsMenu)
        _spellsMenu->deactivate();
    if (_confirmDialog) {
        _confirmDialog->deactivate();
        delete _confirmDialog;
        _confirmDialog = nullptr;
    }

    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

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

    _horizontalMenu = new HorizontalMenu("SpellBookHMenu", cfg);
    _horizontalMenu->activate();
    _stage = STAGE_MAIN_MENU;

    redraw();
}

void SpellBookView::setStage(Stage stage) {
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

bool SpellBookView::msgKeypress(const KeypressMessage &msg) {
    // Party navigation keys pass through in all stages.
    switch (msg.keycode) {
    case Common::KEYCODE_KP_PLUS:
    case Common::KEYCODE_KP_MINUS:
    case Common::KEYCODE_PAGEUP:
    case Common::KEYCODE_PAGEDOWN:
        handlePartyNavigation(msg);
        return true;
    default:
        break;
    }

    // Forward to active sub-dialog.
    if (_stage != STAGE_MAIN_MENU && _spellsMenu && _spellsMenu->isActive()) {
        if (_spellsMenu->msgKeypress(msg))
            return true;
    }
    if (_confirmDialog && _confirmDialog->isActive()) {
        if (_confirmDialog->msgKeypress(msg))
            return true;
    }

    // Main menu input.
    if (_stage == STAGE_MAIN_MENU && _horizontalMenu) {
        bool wasActive = _horizontalMenu->isActive();
        bool handled = _horizontalMenu->msgKeypress(msg);

        if (handled && wasActive && !_horizontalMenu->isActive()) {
            char ascii = msg.ascii;
            if (ascii >= 'a' && ascii <= 'z')
                ascii = ascii - 32;

            // Exit check.
            if (msg.keycode == Common::KEYCODE_ESCAPE || ascii == 'E') {
                exitView();
                return true;
            }

            bool validKey = false;
            for (uint i = 0; i < _mainMenuModel.items.size(); ++i) {
                if (_mainMenuModel.items[i].shortcut == ascii) {
                    validKey = true;
                    break;
                }
            }
            if (!validKey && msg.keycode == Common::KEYCODE_RETURN) {
                int sel = _mainMenuModel.currentSelection;
                if (sel >= 0 && sel < (int)_mainMenuModel.items.size()) {
                    ascii = _mainMenuModel.items[sel].shortcut;
                    validKey = true;
                }
            }

            if (validKey) {
                handleMainMenuKey(ascii);
            } else {
                _horizontalMenu->activate();
            }
            return true;
        }
        if (handled)
            return true;
    }

    return View::msgKeypress(msg);
}

void SpellBookView::handleMainMenuKey(char key) {
    switch (key) {
    case 'C': setStage(STAGE_CAST); break;
    case 'M': setStage(STAGE_MEMORIZE); break;
    case 'S': setStage(STAGE_SCRIBE); break;
    case 'D': setStage(STAGE_DISPLAY); break;
    case 'R': setStage(STAGE_REST); break;
    default:
        if (_horizontalMenu)
            _horizontalMenu->activate();
        break;
    }
}

void SpellBookView::handlePartyNavigation(const KeypressMessage &msg) {
    // Original: DIALOG_PartyListNavigate(keypress) + DIALOG_ShowParty()
    if (_partyList) {
        _partyList->handleKeypressDirect(msg);
    }

    // Refresh character pointer after cycling.
    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
        VmInterface::getSelectedCharacter());

    redraw();
}

// --- Draw ---

void SpellBookView::draw() {
    Surface s = getSurface();

    // Draw main windows (party area + message area).
    drawMainScreenWindows(false);

    // Party list draws itself.
    if (_partyList)
        _partyList->draw();

    // Stage-specific drawing.
    if (_stage == STAGE_MAIN_MENU && _horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }

    if (_spellsMenu && _spellsMenu->isActive())
        _spellsMenu->draw();

    if (_confirmDialog && _confirmDialog->isActive())
        _confirmDialog->draw();
}

// --- MenuResult routing ---

void SpellBookView::handleMenuResult(const MenuResultMessage &result) {
    debug(5, "SpellBookView::handleMenuResult stage=%d success=%d key=%d",
        (int)_stage, (int)result._success, (int)result._keyCode);

    switch (_stage) {
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
    default:
        break;
    }
}

// --- Sub-action entry points ---

void SpellBookView::beginCast() {
    // Original: ACTION_CheckSpellActionType(SA_CAST) then loop
    // DIALOG_Spells(SL_IN_MEMORY, SA_CAST) until spell==0.
    // TODO: ACTION_CheckSpellActionType validation.

    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    _spellsMenu->configure(SpellsMenu::SL_IN_MEMORY, SpellsMenu::SA_CAST);
    _spellsMenu->activate();
}

void SpellBookView::beginMemorize() {
    // Original: ACTION_CheckSpellActionType(SA_MEMORIZE), shows
    // pending-memorize list, asks Y/N, then loops spell book selection.
    // TODO: Show pending list + confirmation first pass.

    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    _spellsMenu->configure(SpellsMenu::SL_IN_SPELL_BOOK,
        SpellsMenu::SA_MEMORIZE);
    _spellsMenu->activate();
}

void SpellBookView::beginScribe() {
    // Original: ACTION_CheckSpellActionType(SA_SCRIBE), shows
    // pending-scribe list, asks Y/N, then loops scroll selection.
    // TODO: Show pending list + confirmation first pass.

    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    _spellsMenu->configure(SpellsMenu::SL_ON_SCROLLS, SpellsMenu::SA_SCRIBE);
    _spellsMenu->activate();
}

void SpellBookView::beginDisplay() {
    // Original SPELL_ScrollRead: iterate party characters, build effect
    // list per character, present in a VerticalMenu scroll window.
    // TODO: Build effect list + vertical scroll display.

    _screenDirty = true;
    returnToMainMenu();
}

void SpellBookView::beginRest() {
    // Original ACTION_Rest: calculate max regain time from party,
    // populate rest time struct, call DIALOG_RestInterrupt.
    // TODO: Implement rest logic + encounter check.

    _screenDirty = true;
    returnToMainMenu();
}

// --- Sub-action result handlers ---

void SpellBookView::handleCastResult(const MenuResultMessage &result) {
    if (!result._success || !result._hasIntValue) {
        // User exited without casting (ESC/no-spells).
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    int legacyIndex = result._intValue;
    debug(5, "SpellBookView: cast spell legacyIndex=%d", legacyIndex);

    // Original: SCREEN_drawBitmapBorder + SCREEN_ClearRect + ACTION_Execute
    // TODO: Execute the spell via ACTION_Execute.

    _screenDirty = true;

    // Re-enter cast loop (original loops until spell==0).
    _spellsMenu->configure(SpellsMenu::SL_IN_MEMORY, SpellsMenu::SA_CAST);
    _spellsMenu->activate();
}

void SpellBookView::handleMemorizeResult(const MenuResultMessage &result) {
    if (!result._success || !result._hasIntValue) {
        // Exited memorize. If spells were queued, show confirmation.
        if (_pendingMemorizeSpells) {
            _stage = STAGE_MEMORIZE_CONFIRM;
            // TODO: Show "Memorize these spells?" HorizontalYesNo.
            _pendingMemorizeSpells = false;
        }
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    int legacyIndex = result._intValue;
    debug(5, "SpellBookView: memorize spell legacyIndex=%d", legacyIndex);

    // Original: CHARACTER_GetRemainingSpellSlots, find empty slot,
    // assign spellId | 0x80 (pending bit), CHARACTER_SortMemorizedSpells.
    // TODO: Actually queue the spell into character data.

    _pendingMemorizeSpells = true;
    _screenDirty = true;

    // Re-enter memorize loop.
    _spellsMenu->configure(SpellsMenu::SL_IN_SPELL_BOOK,
        SpellsMenu::SA_MEMORIZE);
    _spellsMenu->activate();
}

void SpellBookView::handleScribeResult(const MenuResultMessage &result) {
    if (!result._success || !result._hasIntValue) {
        if (_pendingScribeSpells) {
            _stage = STAGE_SCRIBE_CONFIRM;
            // TODO: Show "Scribe these spells?" HorizontalYesNo.
            _pendingScribeSpells = false;
        }
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    int legacyIndex = result._intValue;
    debug(5, "SpellBookView: scribe spell legacyIndex=%d", legacyIndex);

    // Original: check mem_spells[spellId + 0x1b] ("already know"),
    // traverse items for ITEM_isMissileOrScroll, mark scroll bit 7.
    // TODO: Implement scribe logic.

    _pendingScribeSpells = true;
    _screenDirty = true;

    // Re-enter scribe loop.
    _spellsMenu->configure(SpellsMenu::SL_ON_SCROLLS, SpellsMenu::SA_SCRIBE);
    _spellsMenu->activate();
}

void SpellBookView::handleConfirmResult(const MenuResultMessage &result) {
    // Y/N result for "Memorize/Scribe these spells?"
    // Original: 'N' -> CHARACTER_ClearMemorizedSpells or
    //                   CHARACTER_clearItemMemorizedSpellFlags.
    // TODO: Wire up character spell clearing on 'N'.

    if (_confirmDialog) {
        _confirmDialog->deactivate();
        delete _confirmDialog;
        _confirmDialog = nullptr;
    }

    _screenDirty = true;
    returnToMainMenu();
}

void SpellBookView::exitView() {
    if (_screenDirty) {
        // Original: GAME_ScreenByState() — parent view redraws on refocus.
    }

    // Pop this view from the stack, returning to the previous view.
    close();
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
