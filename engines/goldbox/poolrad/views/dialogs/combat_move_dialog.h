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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MOVE_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MOVE_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/core/tile_pos.h"

// Forward declaration in the correct namespace — mirrors combat_view.h pattern.
namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data
} // namespace Goldbox

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * DIALOG_CombatMove — movement interaction controller.
 *
 * Owns the full movement state machine for one character's move action:
 *   - Displays the movement budget menu each step.
 *   - Reads directional input (numpad / arrow keys).
 *   - Handles cancel (restores original position/facing).
 *   - Handles flee (nested Yes/No prompt).
 *   - Delegates to CombatSession for all data mutations.
 *   - Posts MenuResultMessage to "Combat" when the action ends.
 *
 * Sub-states:
 *   STATE_MOVE    — waiting for direction key
 *   STATE_FLEE    — waiting for Yes/No flee confirmation
 */
class CombatMoveDialog : public Dialog {
public:
    CombatMoveDialog();
    ~CombatMoveDialog() override;

    /** Called by CombatView before attaching; captures snapshot of original state. */
    void beginMove(Goldbox::Data::PlayerCharacter *ch);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    enum State { STATE_MOVE, STATE_FLEE };

    Goldbox::Data::PlayerCharacter *_character;

    // Snapshot saved at move start for cancel/restore.
    uint8 _origMovePoints;
    uint8 _origDirection;
    TilePos _origPos;

    State _state;

    HorizontalYesNo *_fleeYesNo;

    // --- Helpers ---

    /** Decode a keypress to an 8-way direction index (0-7), or 8 for none. */
    static uint8 decodeDirection(const KeypressMessage &msg);

    void handleDirectionInput(uint8 direction);
    void handleCancel();
    void enterFleePrompt();
    void finishAction(bool actionComplete);

    void drawMovementMenu();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MOVE_DIALOG_H
