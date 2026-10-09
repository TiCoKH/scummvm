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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MANUAL_AIM_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MANUAL_AIM_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/core/tile_pos.h"

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
 * DIALOG_CombatManualAim — free-cursor targeting interaction.
 *
 * Mirrors COMBAT_manualAim:
 *   - Cursor starts at the initial target's tile.
 *   - Numpad/arrow keys move the cursor in 8 directions.
 *   - Each step: LOS check, range check, tile passability, character lookup,
 *     ranged-weapon validation (when withWeapon=true).
 *   - "Target" command confirms; "Exit" cancels; "Center" re-centers viewport.
 *   - On confirmation with withWeapon=true, calls
 *     CombatSession::executeAttackOnTarget().
 *
 * Posts MenuResultMessage to "CombatAttack":
 *   _success=true  + _intValue=1  → attack executed / target selected
 *   _success=false               → cancelled
 */
class CombatManualAimDialog : public Dialog {
public:
    /** Result of a manual aim session. */
    struct ManualAimResult {
        Goldbox::Data::PlayerCharacter *target;
        uint8 x;
        uint8 y;
        ManualAimResult() : target(nullptr), x(0), y(0) {}
    };

    CombatManualAimDialog();
    ~CombatManualAimDialog() override;

    /**
     * Prepare for a manual aim session.
     * initialTarget: cursor starts here (may be nullptr → use attacker pos).
     * withWeapon:    true = ranged-weapon validation + attack execution.
     * emptyAllowed:  true = empty tile is a valid selection.
     * maxRange:      maximum attack range.
     */
    void beginManualAim(Goldbox::Data::PlayerCharacter *attacker,
                        Goldbox::Data::PlayerCharacter *initialTarget,
                        uint8 maxRange,
                        bool withWeapon,
                        bool emptyAllowed,
                        Combat::CombatSession *session);

    const ManualAimResult &getResult() const { return _result; }

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    Goldbox::Data::PlayerCharacter *_attacker;
    uint8                           _maxRange;
    bool                            _withWeapon;
    bool                            _emptyAllowed;
    Combat::CombatSession          *_session;

    // Cursor state.
    uint8 _cursorX;
    uint8 _cursorY;
    uint8 _direction;  // 0-7 = directional, 8 = center/idle

    // Per-frame evaluation result.
    bool                            _valid;
    Goldbox::Data::PlayerCharacter *_selectedCharacter;
    uint8                           _targetRange;

    ManualAimResult _result;

    MenuItemList   _menuModel;
    HorizontalMenu *_horizontalMenu;

    // --- Helpers ---
    void evaluateCursor();
    void buildCommandMenu();
    void confirmTarget();
    void cancelAim();

    /** Decode a keypress to a direction index (0-7), or 8 for none. */
    static uint8 decodeDirection(const KeypressMessage &msg);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MANUAL_AIM_DIALOG_H
