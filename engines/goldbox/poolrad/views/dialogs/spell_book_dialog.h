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

#include "common/array.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Data {
namespace Items {
struct CharacterItem;
}
}

namespace Poolrad {

namespace Data {
class PoolradCharacter;
}

class SpellPreparation;

namespace Views {
namespace Dialogs {
class HorizontalMenu;
class HorizontalYesNo;
class RestDialog;
class SpellsMenu;
class SpellTargetPicker;
class VerticalMenu;

/**
 * Dialog equivalent of legacy spell-book flow used from camp.
 */
class SpellBookDialog : public Dialog {
public:
    enum Stage {
        STAGE_MAIN_MENU = 0,
        STAGE_CAST,
        STAGE_MEMORIZE,
        STAGE_MEMORIZE_REVIEW,     // view-only "to be memorized" list
        STAGE_MEMORIZE_CONFIRM,
        STAGE_SCRIBE,
        STAGE_SCRIBE_REVIEW,       // view-only "to be scribed" list
        STAGE_SCRIBE_CONFIRM,
        STAGE_DISPLAY,
        STAGE_REST,
        STAGE_CAST_LOSE_IT
    };

    explicit SpellBookDialog(const Common::String &name = "SpellBook");
    ~SpellBookDialog() override;

    void activate() override;
    void deactivate() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void handleMenuResult(const MenuResultMessage &result) override;
    bool tick() override;

    bool wasInterrupted() const { return _interrupted; }

    /**
     * Camp.NoncombatCastMenu: open straight into the memorized-spell cast
     * loop; leaving the list closes the dialog instead of the main menu.
     */
    void beginCastOnly();

    /** The cast-only loop ended because nothing is memorized any more. */
    bool castListEmptied() const { return _castListEmptied; }

private:
    Stage _stage;
    bool _interrupted;
    bool _screenDirty;
    bool _castOnly = false;
    bool _castListEmptied = false;
    Goldbox::Poolrad::Data::PoolradCharacter *_character;

    // Main horizontal menu: Cast / Memorize / Scribe / Display / Rest
    MenuItemList _mainMenuModel;
    HorizontalMenu *_horizontalMenu;

    // Child dialogs
    SpellsMenu *_spellsMenu;
    HorizontalYesNo *_confirmDialog;
    VerticalMenu *_displayMenu;
    RestDialog *_restDialog = nullptr;
    SpellTargetPicker *_targetPicker;
    HorizontalYesNo *_loseItDialog;

    // Text shown in the message window during a cast.
    enum CastMessage {
        CAST_MSG_NONE,
        CAST_MSG_CASTS,
        CAST_MSG_COMBAT_ONLY,
        CAST_MSG_NAME,         // "<name> <_nameMessage>"
        CAST_MSG_BOTTOM        // _nameMessage on the bottom line
    };
    CastMessage _castMessage = CAST_MSG_NONE;
    Common::String _nameMessage;
    Goldbox::Data::Spells::Spells _castSpell = Goldbox::Data::Spells::SP_NONE;

    // Camp.CampMemorizeMenu / CampScribeMenu: the confirm after the
    // selection loop (idx <> -1) rather than the one before it.
    bool _prepFinalPass = false;
    // The spell selection list was shown (idx <> -1).
    bool _prepSelectionRan = false;

    // Scroll items passed to SpellsMenu for scribe selection (kept alive for
    // the duration of the scribe sub-stage).
    Common::Array<Goldbox::Data::Items::CharacterItem *> _scribeScrollItems;

    // Active-effect display model.
    MenuItemList _displayModel;

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
    void handleMainMenuResult(const MenuResultMessage &result);
    void handleCastResult(const MenuResultMessage &result);
    void handleMemorizeResult(const MenuResultMessage &result);
    void handleScribeResult(const MenuResultMessage &result);
    void handleConfirmResult(const MenuResultMessage &result);
    void handleDisplayResult(const MenuResultMessage &result);
    void handleLoseItResult(const MenuResultMessage &result);

    // PrintCastSpellMsg(curChar, 'casts', spell): shown, paused, cleared.
    void showCastsMessage();
    void showNameMessage(const Common::String &text);
    void showBottomMessage(const Common::String &text);
    void pauseMessage();
    void openPrepReview(bool memorize);
    void openPrepConfirm(bool memorize);
    void enterMemorizeSelection();
    void enterScribeSelection();
    void finishPrep(bool memorize);
    void handleReviewResult(const MenuResultMessage &result);
    void drawSpellSlots();
    bool refuseActivity(int activity);
    void refreshCharacter();
    void setCastMessage(CastMessage message);
    void drawCastMessage();
    // Back to the cast list after a cast or a refused combat-only spell.
    void continueCastLoop();

    // Immediate noncombat cast through g_spellCasting for the chosen slot.
    void castMemorizedSpell(int legacyIndex);
    // Collect inventory scrolls carrying scribe payloads for the current
    // character into _scribeScrollItems.
    void collectScribeScrolls();
    // Build the per-character active-effect list for STAGE_DISPLAY.
    void buildActiveEffectDisplay();
    static Common::String effectDisplayName(uint8 id);

    void exitView();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SPELL_BOOK_DIALOG_H