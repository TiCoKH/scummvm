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
#include "goldbox/poolrad/views/dialogs/rest_dialog.h"
#include "goldbox/poolrad/views/dialogs/spell_target_picker.h"
#include "goldbox/poolrad/views/dialogs/spells_menu.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/poolrad_spell_mapping.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/poolrad/noncombat_spell_cast.h"
#include "goldbox/poolrad/spell_preparation.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/spells/spell_metadata.h"
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
      _targetPicker(nullptr),
      _loseItDialog(nullptr) {

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
    dispCfg.headColor = 11;
    dispCfg.textColor = 10;
    dispCfg.selectColor = 15;
    dispCfg.xStart = 1;
    dispCfg.yStart = 4;
    dispCfg.xEnd = 38;
    dispCfg.yEnd = 22;
    dispCfg.title = "";
    dispCfg.addExit = true;
    _displayMenu = new VerticalMenu(name + "Display", dispCfg);
    _displayMenu->setParent(this);
    subView(_displayMenu);
    _displayMenu->deactivate();

    // Not a subView: ticked by this dialog only while resting.
    _restDialog = new RestDialog(name + "Rest");
    _restDialog->deactivate();

    _targetPicker = new SpellTargetPicker(name + "Target");
    subView(_targetPicker);
    _targetPicker->deactivate();

    // YesNoMenu('Lose it? ', LightMagenta, White, LightGreen).
    HorizontalYesNoConfig loseCfg;
    loseCfg.promptTxt = "Lose it? ";
    loseCfg.promptColor = 13;
    loseCfg.textColor = 15;
    loseCfg.selectColor = 10;
    loseCfg.backgroundColor = 0;
    _loseItDialog = new HorizontalYesNo(name + "LoseIt", loseCfg);
    subView(_loseItDialog);
    _loseItDialog->deactivate();
}

SpellBookDialog::~SpellBookDialog() {
    // Child dialogs are owned via subView (UIElement children).
    delete _restDialog;
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
    _prepFinalPass = false;
    _prepSelectionRan = false;

    refreshCharacter();

    // Enter main menu stage.
    returnToMainMenu();
}

// The party list can switch the selected character while the menu is up.
void SpellBookDialog::refreshCharacter() {
    _character = dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
        VmInterface::getSelectedCharacter());
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
    if (_targetPicker)
        _targetPicker->deactivate();
    if (_loseItDialog)
        _loseItDialog->deactivate();
    if (_restDialog && _restDialog->isActive())
        _restDialog->deactivate();
    _castMessage = CAST_MSG_NONE;

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

    refreshCharacter();
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
    if (_restDialog && _restDialog->isActive())
        return _restDialog->msgKeypress(msg);
    if (_targetPicker && _targetPicker->isActive())
        return _targetPicker->send(msg);
    if (_loseItDialog && _loseItDialog->isActive())
        return _loseItDialog->send(msg);
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

    if (_restDialog && _restDialog->isActive()) {
        _restDialog->draw();
        return;
    }

    // Stage-specific drawing.
    if (_stage == STAGE_MAIN_MENU && _horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }

    if (_spellsMenu && _spellsMenu->isActive()) {
        _spellsMenu->draw();
        if (_stage == STAGE_MEMORIZE)
            drawSpellSlots();
    }

    if (_confirmDialog && _confirmDialog->isActive())
        _confirmDialog->draw();

    if (_displayMenu && _displayMenu->isActive()) {
        Surface s = getSurface();
        s.drawWindow(1, 1, 38, 22, 0, 15, "Display");
        _displayMenu->draw();
    }

    drawCastMessage();

    if (_loseItDialog && _loseItDialog->isActive()) {
        _loseItDialog->setRedraw();
        _loseItDialog->draw();
    }

    if (_targetPicker && _targetPicker->isActive())
        _targetPicker->draw();
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
    case STAGE_MEMORIZE_REVIEW:
    case STAGE_SCRIBE_REVIEW:
        handleReviewResult(result);
        break;
    case STAGE_MEMORIZE_CONFIRM:
    case STAGE_SCRIBE_CONFIRM:
        handleConfirmResult(result);
        break;
    case STAGE_DISPLAY:
        handleDisplayResult(result);
        break;
    case STAGE_CAST_LOSE_IT:
        handleLoseItResult(result);
        break;
    case STAGE_REST:
        // CampMagicMenu leaves its loop (and the camp) on an interruption.
        _screenDirty = true;
        _interrupted = _restDialog && _restDialog->wasInterrupted();
        if (_interrupted)
            exitView();
        else
            returnToMainMenu();
        break;
    default:
        break;
    }
}

bool SpellBookDialog::tick() {
    if (_restDialog && _restDialog->isActive())
        _restDialog->tick();
    return Dialog::tick();
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
    refreshCharacter();
    if (refuseActivity(SpellPreparation::ACT_CAST))
        return;
    // A non-caster has nothing to cast.
    if (!Goldbox::Poolrad::SpellPreparation(_character)
            .canDoActivity(SpellPreparation::ACT_CAST)) {
        showNameMessage("has no spells memorized");
        _screenDirty = true;
        returnToMainMenu();
        return;
    }

    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    // NoncombatCastMenu entry: castTarget := nil.
    if (_targetPicker)
        _targetPicker->resetCastTarget();

    _spellsMenu->configure(SpellsMenu::SL_IN_MEMORY, SpellsMenu::SA_CAST);
    _spellsMenu->activate();
}

void SpellBookDialog::beginMemorize() {
    refreshCharacter();
    if (refuseActivity(SpellPreparation::ACT_MEMORIZE))
        return;
    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    // Camp.CampMemorizeMenu: offer the spells already waiting first.
    _prepFinalPass = false;
    _prepSelectionRan = false;
    if (Goldbox::Poolrad::SpellPreparation(_character)
            .countPendingMemorized() > 0)
        openPrepReview(true);
    else
        enterMemorizeSelection();
}

void SpellBookDialog::enterMemorizeSelection() {
    // Camp.DisplaySpellSlots: no spell allowance at all ends the menu.
    if (!Goldbox::Poolrad::SpellPreparation(_character)
            .canDoActivity(SpellPreparation::ACT_MEMORIZE)) {
        showNameMessage("cannot memorize any spells");
        finishPrep(true);
        return;
    }

    // An empty spell book returns at once without showing the list.
    bool anyKnown = false;
    for (int i = 0; i < Goldbox::Poolrad::Data::POOLRAD_PLAYER_KNOWN_SIZE; ++i)
        anyKnown = anyKnown || _character->spells.knownSpells[i] != 0;
    if (!anyKnown) {
        finishPrep(true);
        return;
    }

    _stage = STAGE_MEMORIZE;
    _spellsMenu->configure(SpellsMenu::SL_IN_SPELL_BOOK,
        SpellsMenu::SA_MEMORIZE);
    _prepSelectionRan = true;
    _spellsMenu->activate();
    redraw();
}

void SpellBookDialog::beginScribe() {
    refreshCharacter();
    if (refuseActivity(SpellPreparation::ACT_SCRIBE))
        return;
    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    // Camp.CampScribeMenu: offer the scroll spells already waiting first.
    _prepFinalPass = false;
    _prepSelectionRan = false;
    if (Goldbox::Poolrad::SpellPreparation(_character).hasPendingScribe())
        openPrepReview(false);
    else
        enterScribeSelection();
}

void SpellBookDialog::enterScribeSelection() {
    if (!Goldbox::Poolrad::SpellPreparation(_character).hasScrollSpells()) {
        showNameMessage("has no copyable scrolls");
        finishPrep(false);
        return;
    }

    _stage = STAGE_SCRIBE;
    _prepSelectionRan = true;
    collectScribeScrolls();
    _spellsMenu->configure(SpellsMenu::SL_ON_SCROLLS, SpellsMenu::SA_SCRIBE);
    _spellsMenu->setScrollItems(&_scribeScrollItems);
    _spellsMenu->activate();
    redraw();
}

void SpellBookDialog::openPrepReview(bool memorize) {
    _stage = memorize ? STAGE_MEMORIZE_REVIEW : STAGE_SCRIBE_REVIEW;
    _spellsMenu->resetSelection();
    _spellsMenu->configure(memorize ? SpellsMenu::SL_TO_BE_MEMORIZED
                                    : SpellsMenu::SL_TO_BE_SCRIBED,
        SpellsMenu::SA_NONE);
    _spellsMenu->activate();
    redraw();
}

void SpellBookDialog::handleReviewResult(const MenuResultMessage &result) {
    (void)result;
    if (_spellsMenu && _spellsMenu->isActive())
        _spellsMenu->deactivate();
    openPrepConfirm(_stage == STAGE_MEMORIZE_REVIEW);
}

// YesNoMenu('Memorize These Spells? ', Yellow, ...); the second confirm
// after the selection loop is lower-case.
void SpellBookDialog::openPrepConfirm(bool memorize) {
    _stage = memorize ? STAGE_MEMORIZE_CONFIRM : STAGE_SCRIBE_CONFIRM;
    Common::String prompt;
    if (memorize)
        prompt = _prepFinalPass ? "Memorize these spells? "
                                : "Memorize These Spells? ";
    else
        prompt = _prepFinalPass ? "Scribe these spells? "
                                : "Scribe These Spells? ";
    if (_confirmDialog) {
        _confirmDialog->setPrompt(prompt, 14);
        _confirmDialog->activate();
    }
    redraw();
}

// After the selection loop: show the queue again and confirm it.
void SpellBookDialog::finishPrep(bool memorize) {
    if (_prepSelectionRan && !_prepFinalPass && _character) {
        Goldbox::Poolrad::SpellPreparation prep(_character);
        const bool pending = memorize ? prep.countPendingMemorized() > 0
                                      : prep.hasPendingScribe();
        if (pending) {
            _prepFinalPass = true;
            openPrepReview(memorize);
            return;
        }
    }
    _screenDirty = true;
    returnToMainMenu();
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
    // Camp.CampRestMenu, the same rest menu as the camp's own Rest.
    if (_horizontalMenu)
        _horizontalMenu->deactivate();
    _restDialog->setParent(this);
    _restDialog->begin();
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

    const int legacyIndex = result._intValue;
    if (legacyIndex < 0 ||
            legacyIndex >= Goldbox::Poolrad::Data::POOLRAD_PLAYER_KNOWN_SIZE) {
        continueCastLoop();
        return;
    }
    _castSpell = Goldbox::Poolrad::Data::kPoolradSpellMapping[legacyIndex];
    _screenDirty = true;

    // CastSpellNC: target mode 0 is refused outside combat.
    if (NoncombatSpellCast::isCombatOnly(_castSpell)) {
        _stage = STAGE_CAST_LOSE_IT;
        setCastMessage(CAST_MSG_COMBAT_ONLY);
        if (_loseItDialog)
            _loseItDialog->activate();
        redraw();
        return;
    }

    showCastsMessage();
    castMemorizedSpell(legacyIndex);
    continueCastLoop();
}

void SpellBookDialog::handleLoseItResult(const MenuResultMessage &result) {
    // YesNoMenu only ends on Y or N.
    if (!result._success)
        return;

    if (_loseItDialog)
        _loseItDialog->deactivate();
    if (result._keyCode == Common::KEYCODE_y && _character)
        _character->removeMemorizedSpell(static_cast<uint8>(_castSpell));

    _stage = STAGE_CAST;
    continueCastLoop();
}

void SpellBookDialog::continueCastLoop() {
    // CastSpellNC ends with ClearMessageWindow.
    setCastMessage(CAST_MSG_NONE);

    // NoncombatCastMenu: an empty list ends the loop ("has no spells
    // memorized" is printed by the host).
    if (_castOnly && (!_character ||
            Goldbox::Poolrad::SpellPreparation(_character)
                .countUsableMemorized() == 0)) {
        _castListEmptied = true;
        exitView();
        return;
    }

    // idx := -1: the list is rebuilt with the cursor on its first spell.
    _spellsMenu->resetSelection();
    _spellsMenu->configure(SpellsMenu::SL_IN_MEMORY, SpellsMenu::SA_CAST);
    _spellsMenu->activate();
    if (g_events && g_events->focusedView())
        g_events->focusedView()->redraw();
}

void SpellBookDialog::showCastsMessage() {
    setCastMessage(CAST_MSG_CASTS);
    if (!g_events)
        return;

    // PauseOnly: gameDelay * 225 ms, keys ignored.
    const uint32 end = g_system->getMillis() +
        VmInterface::getTextDelay() * 225;
    while (g_system->getMillis() < end)
        if (!g_events->pumpModalInputFrame())
            break;

    setCastMessage(CAST_MSG_NONE);
}

// Camp.CanDoSpellActivity: print the refusal and stay in the magic menu.
bool SpellBookDialog::refuseActivity(int activity) {
    if (!_character) {
        returnToMainMenu();
        return true;
    }

    bool antimagic = false;
    if (g_engine) {
        const ECL::AddressSpace *mem = g_engine->getEclMemory();
        if (mem)
            antimagic = mem->read8(Data::getPoolradVmLayout()
                .field(kVmFieldNoMagicFlag).vmAddr) != 0;
    }

    const char *refusal = Goldbox::Poolrad::SpellPreparation(_character)
        .activityRefusal(static_cast<SpellPreparation::Activity>(activity),
            antimagic);
    if (!refusal)
        return false;

    showNameMessage(refusal);
    _screenDirty = true;
    returnToMainMenu();
    return true;
}

// CharHlp.PrintNameString(curChar, text, 10, True): name and text in the
// message window, then a pause.
void SpellBookDialog::showNameMessage(const Common::String &text) {
    _nameMessage = text;
    setCastMessage(CAST_MSG_NAME);
    pauseMessage();
    setCastMessage(CAST_MSG_NONE);
}

// CharHlp.PrintBottomLine: the text on row 24, a pause, then cleared.
void SpellBookDialog::showBottomMessage(const Common::String &text) {
    _nameMessage = text;
    setCastMessage(CAST_MSG_BOTTOM);
    pauseMessage();
    setCastMessage(CAST_MSG_NONE);
}

// TextRend.PauseOnly: gameDelay * 225 ms, keys ignored.
void SpellBookDialog::pauseMessage() {
    if (!g_events)
        return;
    const uint32 end = g_system->getMillis() +
        VmInterface::getTextDelay() * 225;
    while (g_system->getMillis() < end)
        if (!g_events->pumpModalInputFrame())
            break;
}

void SpellBookDialog::setCastMessage(CastMessage message) {
    _castMessage = message;
    redraw();
}

void SpellBookDialog::drawCastMessage() {
    if (_castMessage == CAST_MSG_NONE)
        return;

    if (_castMessage == CAST_MSG_BOTTOM) {
        Surface s = getSurface();
        s.clearBox(0, 24, 39, 24, 0);
        s.writeStringC(0, 24, 10, _nameMessage);
        return;
    }

    // NoncombatCastMenu frames and clears the message window first.
    drawWindow(1, 17, 38, 22);
    Surface s = getSurface();
    const Common::String spellName =
        Goldbox::Spells::getSpellName(_castSpell);

    if (_castMessage == CAST_MSG_COMBAT_ONLY) {
        s.writeStringC(1, 19, 10, spellName);
        s.writeStringC(1, 20, 10, "is a combat-only spell...");
        return;
    }

    if (_castMessage == CAST_MSG_NAME) {
        // PrintNameString: the name on the window's first line, the text
        // below it.
        if (_character) {
            s.writeStringC(1, 18, _character->enabled ? 11 : 12,
                _character->name);
            s.writeStringC(1, 19, 10, _nameMessage);
        }
        return;
    }

    if (_character) {
        // DisplayCreatureName: LightRed when disabled, else LightCyan.
        s.writeStringC(1, 19, _character->enabled ? 11 : 12,
            _character->name);
        s.writeStringC(_character->name.size() + 2, 19, 10, "casts");
    }
    s.writeStringC(1, 20, 10, spellName);
}

void SpellBookDialog::castMemorizedSpell(int legacyIndex) {
    if (!_character || !Goldbox::Spells::g_spellCasting)
        return;
    if (legacyIndex < 0 ||
            legacyIndex >= Goldbox::Poolrad::Data::POOLRAD_PLAYER_KNOWN_SIZE)
        return;

    const Goldbox::Data::Spells::Spells spell =
        Goldbox::Poolrad::Data::kPoolradSpellMapping[legacyIndex];

    // castSpell spends the memorized slot once a target is chosen; a
    // cancelled pick keeps it, as castUI returning sel = False does.
    NoncombatSpellCast cast(_character, false, _targetPicker);
    cast.cast(spell);
}

void SpellBookDialog::handleMemorizeResult(const MenuResultMessage &result) {
    debug(3, "SpellBookDialog::handleMemorizeResult success=%d hasInt=%d intVal=%d",
        (int)result._success, (int)result._hasIntValue,
        (int)(result._hasIntValue ? result._intValue : -1));
    if (!result._success || !result._hasIntValue) {
        finishPrep(true);
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
        // Queued only while a slot of its class and level is left.
        Goldbox::Poolrad::SpellPreparation(_character).queueMemorize(spell);
    }

    _screenDirty = true;
    enterMemorizeSelection();
}

void SpellBookDialog::handleScribeResult(const MenuResultMessage &result) {
    debug(3, "SpellBookDialog::handleScribeResult success=%d hasInt=%d intVal=%d",
        (int)result._success, (int)result._hasIntValue,
        (int)(result._hasIntValue ? result._intValue : -1));
    if (!result._success || !result._hasIntValue) {
        finishPrep(false);
        return;
    }

    const int legacyIndex = result._intValue;
    _screenDirty = true;

    // Camp.CampScribeMenu: refuse known, already queued or unscribable
    // spells; otherwise mark the first matching scroll payload. Rest
    // completes the scribe later.
    if (_character && legacyIndex >= 0 &&
            legacyIndex < Goldbox::Poolrad::Data::POOLRAD_PLAYER_KNOWN_SIZE) {
        const Goldbox::Data::Spells::Spells spell =
            Goldbox::Poolrad::Data::kPoolradSpellMapping[legacyIndex];
        Goldbox::Poolrad::SpellPreparation prep(_character);
        if (_character->spells.knownSpells[legacyIndex] != 0)
            showBottomMessage("You already know that spell");
        else if (prep.isScribing(spell))
            showBottomMessage("You are already scibing that spell");
        else if (prep.slotCapacity(spell) > 0)
            prep.markScribe(spell);
        else
            showBottomMessage("You can not scribe that spell.");
    }

    enterScribeSelection();
}

void SpellBookDialog::handleConfirmResult(const MenuResultMessage &result) {
    // Y/N result for "Memorize/Scribe these spells?". "No" (success false,
    // or the N shortcut) discards the pending selection without touching
    // usable slots; "Yes" keeps the pending work for rest to complete.
    const bool keep = result._success &&
        result._keyCode != Common::KEYCODE_n;
    const bool memorize = (_stage == STAGE_MEMORIZE_CONFIRM);

    if (_confirmDialog)
        _confirmDialog->deactivate();

    if (_character && !keep) {
        Goldbox::Poolrad::SpellPreparation prep(_character);
        if (memorize)
            prep.cancelPendingMemorize();
        else
            prep.cancelPendingScribe();
    }

    // Before the selection: Yes keeps the queue and skips selecting, No
    // clears it and goes on to select. After it, either answer ends.
    if (!_prepFinalPass && !keep) {
        if (memorize)
            enterMemorizeSelection();
        else
            enterScribeSelection();
        return;
    }

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

    // Magic.AddAllScrollSpells: SpellsMenu lists every payload of each
    // scroll, so each scroll is passed once.
    Goldbox::Data::Items::CharacterInventory &inv = _character->getInventory();
    const int itemCount = inv.count();
    for (int i = 0; i < itemCount; ++i) {
        Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (item.isMissileOrScroll() &&
                (item.effect1 || item.effect2 || item.effect3))
            _scribeScrollItems.push_back(&item);
    }
}

void SpellBookDialog::buildActiveEffectDisplay() {
    _displayModel.items.clear();
    _displayModel.currentSelection = 0;

    // Camp.CampDisplayMenu: every creature with a heading line, then its
    // visible effects by name.
    struct Local {
        static void add(MenuItemList &list, bool title,
                const Common::String &text) {
            MenuItem item;
            item.text = text;
            item.active = !title;
            item.shortcut = 0;
            list.items.push_back(item);
        }
    };
    Local::add(_displayModel, false, " ");

    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    if (!party)
        return;

    for (Goldbox::Data::PlayerCharacter *ch : *party) {
        if (!ch)
            continue;
        Local::add(_displayModel, true, ch->name);
        uint count = 0;
        Goldbox::Data::Effects::CharacterEffects *fx = ch->getEffects();
        if (fx) {
            for (const Goldbox::Data::Effects::Effect &e : fx->effects()) {
                const Common::String name = effectDisplayName(e.id);
                if (name.empty())
                    continue;
                ++count;
                Local::add(_displayModel, false, " " + name);
            }
        }
        if (count == 0)
            Local::add(_displayModel, false, " <No Spell Effects>");
        Local::add(_displayModel, false, " ");
    }
}

// The name Camp.CampDisplayMenu shows for an effect, or empty to hide it.
Common::String SpellBookDialog::effectDisplayName(uint8 id) {
    switch (id) {
    case 4:  return "Studying Manual of Bodily Health";
    case 7:  return "Training with Manual of Bodily Health";
    case 27: return "Feather Fall";
    case 31: return "Helpless";
    case 35: return "Prayer";
    case 44: return "Cause Disease";
    case 50: return "Dreaded Mummy Disease";
    case 54: return "Repulsed";
    case 55: return "Poisoned";
    case 59: return "Regenerating";
    case 61: return "Fire Resistance";
    case 71: return "Invisible";
    case 72: return "Camouflaged";
    case 89: return "Displaced";
    default:
        break;
    }

    // Spell effects: named after the first spell (1..56) producing them.
    const bool spellEffect = id == 1 || id == 2 || id == 5 ||
        (id >= 8 && id <= 14) || id == 16 || id == 17 ||
        (id >= 19 && id <= 25) || id == 28 || id == 29 ||
        (id >= 32 && id <= 34) || (id >= 36 && id <= 39) ||
        id == 41 || id == 42 || id == 45 || id == 46 || id == 49 ||
        (id >= 51 && id <= 53);
    if (!spellEffect)
        return Common::String();

    const Common::Array<Goldbox::Data::Spells::SpellEntry> &entries =
        Goldbox::Data::Rules::getSpellEntries();
    for (uint k = 1; k <= 56 && k < entries.size(); ++k) {
        if (entries[k].effectId == id)
            return Goldbox::Spells::getSpellName(
                static_cast<Goldbox::Data::Spells::Spells>(k));
    }
    return Common::String::format("Funky--%u", (unsigned)id);
}

// Camp.DisplaySpellSlots: "<name> can memorize:" and the slots left per
// class and level; a level without allowance is blank.
void SpellBookDialog::drawSpellSlots() {
    if (!_character)
        return;

    drawWindow(1, 17, 38, 22);
    Surface s = getSurface();
    s.writeStringC(1, 18, _character->enabled ? 11 : 12, _character->name);
    s.writeStringC(1, 19, 10, "can memorize:");

    Goldbox::Poolrad::SpellPreparation prep(_character);
    static const Goldbox::Data::Spells::SpellClass kClasses[2] = {
        Goldbox::Data::Spells::SC_CLERIC, Goldbox::Data::Spells::SC_MAGICUSER
    };
    static const char *const kLabels[2] = {
        "    Cleric Spells:", "Magic-User Spells:"
    };
    int row = 20;
    for (int typ = 0; typ < 2; ++typ) {
        bool any = false;
        for (uint8 lvl = 1; lvl <= 3; ++lvl)
            any = any || prep.slotAllowance(kClasses[typ], lvl) != 0;
        if (!any)
            continue;
        s.writeStringC(1, row, 10, kLabels[typ]);
        int col = 20;
        for (uint8 lvl = 1; lvl <= 3; ++lvl) {
            if (prep.slotAllowance(kClasses[typ], lvl) != 0)
                s.writeStringC(col, row, 10, Common::String::format("%u",
                    (unsigned)prep.slotsRemaining(kClasses[typ], lvl)));
            col += 3;
        }
        ++row;
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