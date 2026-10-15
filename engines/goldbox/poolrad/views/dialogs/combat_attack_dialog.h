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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_ATTACK_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_ATTACK_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combat_globals.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data
namespace Poolrad {
namespace Views {
namespace Dialogs {

class CombatTargetCommandDialog;
class CombatManualAimDialog;
class HorizontalYesNo;

/**
 * DIALOG_CombatAttack — attack target-selection interaction.
 *
 * Mirrors the attack branch of DIALOG_CombatMain:
 *   1. Builds the facing target list via CombatSession::buildFacingTargetList().
 *   2. Cycles through targets (Next/Previous).
 *   3. Delegates to CombatTargetCommandDialog for the "Aim:" command menu.
 *   4. Delegates to CombatManualAimDialog for free-cursor targeting.
 *   5. On confirmation, calls CombatSession::executeAttackOnTarget().
 *
 * Posts MenuResultMessage to "Combat" with _intValue = PA_ATTACK when done,
 * or PA_NONE on cancel.
 */
class CombatAttackDialog : public Dialog {
public:
    CombatAttackDialog();
    ~CombatAttackDialog() override;

    /** Called by CombatView before attaching. */
    void beginAttack(Goldbox::Data::PlayerCharacter *attacker,
                     Combat::CombatSession *session);

    /**
     * Aim for a spell instead of an attack (mirrors the COMBAT_AttackTarget
     * call in COMBAT_spellTargetSelection): nothing is executed, the chosen
     * combatant/tile is recorded. tileMode starts in free-cursor mode so an
     * empty tile can be chosen for a burst.
     */
    void beginSpellAim(Goldbox::Data::PlayerCharacter *caster,
                       Combat::CombatSession *session,
                       uint8 range, bool tileMode);

    Goldbox::Data::PlayerCharacter *getAimTarget() const { return _aimTarget; }
    TilePos getAimTile() const { return _aimTile; }

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    enum State {
        STATE_TARGET_COMMAND,  // CombatTargetCommandDialog active
        STATE_MANUAL_AIM       // CombatManualAimDialog active
    };

    Goldbox::Data::PlayerCharacter *_attacker;
    Combat::CombatSession          *_session;

    Combat::TargetList _targetList;
    int                _targetIndex;  // 1-based, 0 = none

    State _state;

    CombatTargetCommandDialog *_targetCommand;
    CombatManualAimDialog     *_manualAim;
    HorizontalYesNo           *_allyConfirm;
    Goldbox::Data::PlayerCharacter *_pendingTarget;  // target awaiting ally-attack confirmation

    // Spell aiming mode (see beginSpellAim).
    bool  _spellMode;
    bool  _tileMode;
    uint8 _spellRange;
    Goldbox::Data::PlayerCharacter *_aimTarget;
    TilePos _aimTile;

    // --- Helpers ---
    void cycleTarget(int direction);  // +1 = next, -1 = previous
    Goldbox::Data::PlayerCharacter *currentTarget() const;

    void enterTargetCommand();
    void enterManualAim();
    void attackTarget(Goldbox::Data::PlayerCharacter *target);

    void finishAttack(bool actionComplete);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_ATTACK_DIALOG_H
