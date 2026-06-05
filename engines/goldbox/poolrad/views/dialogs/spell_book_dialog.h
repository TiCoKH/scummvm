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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_SPELL_BOOK_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_SPELL_BOOK_DIALOG_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

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

/**
 * Dialog equivalent of legacy spell-book flow used from camp.
 */
class SpellBookDialog : public Dialog {
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

    SpellBookDialog();
    ~SpellBookDialog() override;

    void activate() override;
    void deactivate() override;
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
    HorizontalMenu *_horizontalMenu;

    // Child dialogs
    SpellsMenu *_spellsMenu;
    HorizontalYesNo *_confirmDialog;

    // Memorize/scribe pending state
    bool _pendingMemorizeSpells;
    bool _pendingScribeSpells;

    void buildMainMenu();
    void setStage(Stage stage);
    void returnToMainMenu();
    void handleMainMenuKey(char key);

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

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SPELL_BOOK_DIALOG_H