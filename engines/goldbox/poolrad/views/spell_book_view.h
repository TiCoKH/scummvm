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

#ifndef GOLDBOX_POOLRAD_VIEWS_SPELL_BOOK_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_SPELL_BOOK_VIEW_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
namespace Poolrad {

namespace Data {
class PoolradCharacter;
}

namespace Views {
namespace Dialogs {
class HorizontalMenu;
class HorizontalYesNo;
class SpellsMenu;
class PartyList;
}

/**
 * VIEW_SpellBook equivalent — top-level spell management view.
 *
 * Pushed onto the view stack from CampMenuDialog ('M' key) or any
 * other caller that needs the spellbook interface. Returns by
 * popping itself from the stack.
 *
 * Original signatures:
 *   x86:  void VIEW_SpellBook(bool *result)
 *   m68k: void VIEW_SpellBook(bool *b_res)
 *
 * Main loop presents a horizontal menu:
 *   Cast / Memorize / Scribe / Display / Rest
 *
 * Sub-actions dispatched:
 *   'C' -> SPELL_CastSpell     (SpellsMenu SL_IN_MEMORY / SA_CAST)
 *   'M' -> SPELL_SpellMemorize (SpellsMenu SL_IN_SPELL_BOOK / SA_MEMORIZE)
 *   'S' -> SPELL_Scribe        (SpellsMenu SL_ON_SCROLLS / SA_SCRIBE)
 *   'D' -> SPELL_ScrollRead    (display active effects on party)
 *   'R' -> ACTION_Rest          (rest and regain spells)
 *
 * Party navigation keys (+/-/PgUp/PgDn) cycle the active character
 * and redraw the party list (DIALOG_PartyListNavigate + DIALOG_ShowParty).
 *
 * Exits on 'E'/ESC, or when rest is interrupted (encounter).
 * The original returns a bool *result indicating interruption; here
 * the caller can query wasInterrupted() after the view pops.
 */
class SpellBookView : public View {
public:
    enum Stage {
        STAGE_MAIN_MENU = 0,
        STAGE_CAST,
        STAGE_MEMORIZE,
        STAGE_MEMORIZE_CONFIRM,
        STAGE_SCRIBE,
        STAGE_SCRIBE_CONFIRM,
        STAGE_DISPLAY,
        STAGE_REST
    };

    SpellBookView();
    ~SpellBookView() override;

    void onEnter(Goldbox::GameState state) override;
    bool msgFocus(const FocusMessage &msg) override;
    bool msgUnfocus(const UnfocusMessage &msg) override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void handleMenuResult(const MenuResultMessage &result) override;

    bool wasInterrupted() const { return _interrupted; }

private:
    Stage _stage;
    bool _interrupted;
    bool _screenDirty;
    Goldbox::Poolrad::Data::PoolradCharacter *_character;

    // Main horizontal menu: Cast / Memorize / Scribe / Display / Rest
    MenuItemList _mainMenuModel;
    Dialogs::HorizontalMenu *_horizontalMenu;

    // Child dialogs
    Dialogs::SpellsMenu *_spellsMenu;
    Dialogs::PartyList *_partyList;
    Dialogs::HorizontalYesNo *_confirmDialog;

    // Memorize/scribe pending state
    bool _pendingMemorizeSpells;
    bool _pendingScribeSpells;

    void buildMainMenu();
    void setStage(Stage stage);
    void returnToMainMenu();
    void handleMainMenuKey(char key);
    void handlePartyNavigation(const KeypressMessage &msg);

    // Sub-action entry points (mirror original function names)
    void beginCast();
    void beginMemorize();
    void beginScribe();
    void beginDisplay();
    void beginRest();

    // Sub-action result handlers
    void handleCastResult(const MenuResultMessage &result);
    void handleMemorizeResult(const MenuResultMessage &result);
    void handleScribeResult(const MenuResultMessage &result);
    void handleConfirmResult(const MenuResultMessage &result);

    void exitView();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_SPELL_BOOK_VIEW_H
