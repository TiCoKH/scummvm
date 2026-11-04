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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_SUBMENU_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_SUBMENU_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalMenu;
class GameSpeedDialog;

/**
 * DIALOG_CombatSubmenu — context-sensitive combat options interaction.
 *
 * Mirrors DIALOG_CombatSubmenu:
 *   - Guard (G): available when character has no ranged weapon, or ranged
 *     weapon is equipped. Sets guarding flag, ends turn.
 *   - Delay (D): sets initiative to 1, ends turn.
 *   - Quit (Q): ends actor turn via PA_DONE.
 *   - Bandage (B): available when COMBAT_CheckDoBandage(false). Bandages
 *     one dying party member, then ends turn.
 *   - Speed (S): opens GameSpeedDialog; returns to submenu on exit.
 *
 * Posts MenuResultMessage to "Combat" with _intValue = PA_DONE when the
 * turn is consumed, or PA_SUBMENU on cancel (returns to main menu).
 *
 * The interaction loop continues (rebuild) until an action ends the turn
 * or the player cancels back to the main menu.
 */
class CombatSubmenuDialog : public Dialog {
public:
    /**
     * Which nested child is currently active. A MenuResultMessage addressed to
     * this dialog is routed by this identity, NOT by the shared destination
     * string: the main HorizontalMenu and the nested GameSpeedDialog both post
     * their result to this view's name, so only the active child's result may
     * be interpreted as such.
     */
    enum ActiveChild {
        kChildNone = 0,   // nothing nested; a stray result is ignored
        kChildMainMenu,   // the submenu's own Guard/Delay/Quit/Bandage/Speed menu
        kChildSpeedDialog // the nested GameSpeedDialog
    };

    CombatSubmenuDialog();
    ~CombatSubmenuDialog() override;

    /** Called by CombatView before attaching. */
    void beginSubmenu(Goldbox::Data::PlayerCharacter *actor);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

    /**
     * Pure routing decision used by handleMenuResult, extracted so it can be
     * unit-tested without a live engine: a result is a speed-dialog result
     * only while the speed dialog is the active child; otherwise it is a
     * main-menu command.
     */
    static ActiveChild routeResult(ActiveChild activeChild);

private:
    Goldbox::Data::PlayerCharacter *_actor;
    MenuItemList   _menuModel;
    HorizontalMenu *_horizontalMenu;
    GameSpeedDialog *_speedDialog;
    ActiveChild    _activeChild;

    // Availability flags set by rebuild().
    bool _hasGuard;
    bool _hasBandage;

    void rebuild();
    void endTurn();
    void cancel();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_SUBMENU_DIALOG_H
