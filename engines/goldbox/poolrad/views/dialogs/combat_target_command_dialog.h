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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_TARGET_COMMAND_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_TARGET_COMMAND_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data
namespace Combat {
class CombatSession;
} // namespace Combat
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * DIALOG_CombatTargetCommand — "Aim:" command menu for a selected target.
 *
 * Mirrors COMBAT_TargetCommandMenu:
 *   - Calculates target range and ranged-weapon eligibility.
 *   - Builds the available command set: Next / Previous / Manual /
 *     Target (or Aim for ranged) / Center / Exit.
 *   - Focuses the target in the combat info panel.
 *   - Runs DIALOG_HorizontalMenu and posts the result.
 *
 * Result codes posted to parent "CombatAttack":
 *   'N' = Next target
 *   'P' = Previous target
 *   'M' = Manual aim
 *   'T' = Confirm target (attack)
 *   'C' = Center viewport
 *   'E' = Exit / cancel
 */
class CombatTargetCommandDialog : public Dialog {
public:
    CombatTargetCommandDialog();
    ~CombatTargetCommandDialog() override;

    /**
     * Prepare for the given attacker/target pair.
     * Must be called before activate().
     * maxRange: maximum attack range (0xFF = unlimited).
     * showRange: true = ranged-weapon mode (validates ranged weapon,
     *            rejects self-targeting); false = normal melee mode.
     */
    void beginTargetCommand(Goldbox::Data::PlayerCharacter *attacker,
                            Goldbox::Data::PlayerCharacter *target,
                            uint8 maxRange,
                            bool showRange,
                            Combat::CombatSession *session);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    Goldbox::Data::PlayerCharacter *_attacker;
    Goldbox::Data::PlayerCharacter *_target;
    uint8                           _maxRange;
    bool                            _showRange;
    Combat::CombatSession          *_session;

    MenuItemList  _menuModel;
    HorizontalMenu *_horizontalMenu;

    // Command character for the target/aim slot (0 = none).
    char _targetCmd;

    void buildMenu();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_TARGET_COMMAND_DIALOG_H
