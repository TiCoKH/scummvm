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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_CAMP_MENU_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_CAMP_MENU_DIALOG_H

#include "common/array.h"
#include "common/ptr.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Gfx {
class Pic;
}

namespace Data {
class PlayerCharacter;
}

namespace Poolrad {
namespace Views {

namespace Dialogs {

class GameSpeedDialog;
class HorizontalMenu;
class HorizontalYesNo;
class LoadSaveDialog;
class RestDialog;
class SetIcon;
class SpellBookDialog;

/**
 * Self-contained camping dialog (Camp.CampMainMenu).
 *
 * Owns:
 *  - Campfire picture display (PIC 29, animated multi-frame)
 *  - Horizontal menu: Save / View / Magic / Rest / Alter / Exit
 *  - The Save and Alter (Order, Drop, Speed, Icon, Pics) sub-menus
 *  - "The party makes camp..." text output
 *
 * Posts MenuResult to parent InGameView on exit:
 *  - _success=true, _intValue=1 if rest was interrupted (encounter)
 *  - _success=true, _intValue=0 on normal exit ('E')
 *
 * Blocks movement/navigation keys — only camp menu keys are accepted.
 */
class CampMenuDialog : public Dialog {
public:
    static const uint8 kCampfirePicId = 29;

private:
    enum CampStage {
        CAMP_MENU = 0,
        CAMP_SAVE,          // SaveLoad.SaveGame
        CAMP_QUIT_CONFIRM,  // "Quit TO DOS "
        CAMP_ALTER,         // "Alter: Order Drop Speed Icon Pics Exit"
        CAMP_ORDER,         // "Party Order: Select|Place Exit"
        CAMP_DROP_CONFIRM,  // "Drop from party? "
        CAMP_DROP_QUIT,     // last member: "quit TO DOS: "
        CAMP_SPEED,
        CAMP_ICON,
        CAMP_PICS,          // "Monsters on  Portraits on  Exit"
        CAMP_REST           // GameTime.CampRest
    };

    MenuItemList _menuModel;
    HorizontalMenu *_horizontalMenu;
    SpellBookDialog *_spellBookDialog;

    CampStage _stage = CAMP_MENU;
    // Sub-menu of the current stage (Alter, Order or Pics).
    MenuItemList _subModel;
    HorizontalMenu *_subMenu = nullptr;
    HorizontalYesNo *_yesNo = nullptr;
    LoadSaveDialog *_loadSaveDialog = nullptr;
    GameSpeedDialog *_speedDialog = nullptr;
    SetIcon *_iconDialog = nullptr;
    RestDialog *_restDialog = nullptr;
    // CampOrderMenu: Place mode (Home/End move the character).
    bool _orderPlacing = false;

    // PrintNameString text in the message window; empty shows the camp line.
    Common::String _messageText;
    bool _showCampText = true;

    // Animated campfire frames.
    Common::Array<Common::SharedPtr<Goldbox::Gfx::Pic>> _campfireFrames;
    uint8 _currentFrame;
    uint32 _lastFrameTime;
    uint16 _frameIntervalMs;

    bool _isInterrupted;
    /** The Magic sub-menu (SpellBookDialog) was opened from the camp menu. */
    bool _inSpellBook = false;

    void buildMenuModel();
    void loadCampfireFrames();
    void exitCamp();
    void handleMenuKey(char key);
    /**
     * Return to the camp loop once the spell book has closed, possibly from
     * its own queued menu result; an interrupted rest ends the camp.
     */
    void resumeAfterSpellBook();

    // Sub-menus.
    void openSubMenu(CampStage stage, const Common::String &prompt,
                     const Common::Array<Common::String> &labels);
    void closeSubMenu();
    void openYesNo(CampStage stage, const Common::String &prompt);
    void closeYesNo();
    void returnToCampMenu();
    void openAlterMenu();
    void openOrderMenu();
    void openPicsMenu();
    void beginSave();
    void beginDrop();
    void beginSpeed();
    void beginIcon();
    void handleSubMenuKey(char key);
    void handleYesNo(bool yes);
    void handleOrderMoveKey(bool up);

    // CharHlp.PrintNameString(curChar, text, ...); pause waits a moment and
    // then clears it.
    void showNameMessage(const Common::String &text, bool pause);
    Goldbox::Data::PlayerCharacter *selectedCharacter() const;

public:
    CampMenuDialog(const Common::String &name);
    ~CampMenuDialog() override;

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;
    bool tick() override;
    void timeout() override;

    bool isInterrupted() const { return _isInterrupted; }

    /**
     * Party Order's Place mode takes Home/End to move the character, so the
     * party list must not use them to change the selection.
     */
    bool capturesPartyKeys() const {
        return _stage == CAMP_ORDER && _orderPlacing;
    }
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_CAMP_MENU_DIALOG_H
