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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MENU_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MENU_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalMenu;

/**
 * Player action menu shown during a party member's combat turn.
 *
 * Mirrors DIALOG_CombatMenu: runs Effect Set 15 on the actor, then
 * dynamically builds the available commands from character/world state.
 * Posts a MenuResultMessage to "Combat" with _intValue = PlayerAction.
 */
class CombatMenuDialog : public Dialog {
public:
    CombatMenuDialog();
    ~CombatMenuDialog() override;

    /**
     * Prepare the menu for the given actor before activate().
     * Runs Effect Set 15 and computes which commands are available.
     * Must be called each time a new actor's turn begins.
     */
    void beginMenu(Goldbox::Data::PlayerCharacter *actor,
                   Combat::CombatSession *session);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;

private:
    // Availability flags set by beginMenu().
    bool _hasMove;
    bool _hasUse;
    bool _hasCast;
    bool _hasTurn;

    // Parallel arrays: action enum and shortcut key for each built entry.
    Common::Array<Combat::CombatSession::PlayerAction> _actions;
    Common::Array<char> _shortcuts;

    MenuItemList   _menuModel;
    HorizontalMenu *_horizontalMenu;

    void buildMenuModel();
    void postActionResult(Combat::CombatSession::PlayerAction action);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MENU_DIALOG_H
