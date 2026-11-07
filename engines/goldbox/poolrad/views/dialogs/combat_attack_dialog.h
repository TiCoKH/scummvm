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
#include "goldbox/poolrad/views/dialogs/combat_aim_events.h"
#include "goldbox/combat/combat_globals.h"

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

class CombatTargetCommandDialog;
class CombatManualAimDialog;
class HorizontalYesNo;

/**
 * COMBAT_AttackTarget as an event-driven controller, shared by the combat
 * Aim command (weapon mode) and spell target picks.
 *
 * The run cycles through every creature in reach, skipping those the actor
 * may not engage, and offers the Aim menu on each. Manual hands over to the
 * free cursor; its Exit returns here. In weapon mode Target attacks (after
 * the Attack Ally prompt for a same-side target); while the actor still has
 * attacks left the run stays open on the rebuilt list. The host gets
 * exactly one aimFinished() per begin.
 */
class CombatAttackDialog : public Dialog, public CombatAimListener {
public:
    CombatAttackDialog();
    ~CombatAttackDialog() override;

    void setHost(CombatAimHost *host) { _host = host; }

    /** The combat Aim command: a weapon attack at weapon range. */
    void beginAttack(::Goldbox::Data::PlayerCharacter *attacker,
                     Combat::CombatSession *session);

    /** A spell target pick; groundOk allows a bare cell (bursts). */
    void beginSpellAim(::Goldbox::Data::PlayerCharacter *caster,
                       Combat::CombatSession *session,
                       uint8 range, bool groundOk);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

    // CombatAimListener
    void onAimCommand(AimCommand command) override;
    void onManualAimCursor(TilePos cell, const Combat::AimCell &info) override;
    void onManualAimFinished(bool picked,
                             const Combat::AimPick &pick) override;

private:
    enum State {
        STATE_IDLE,
        STATE_TARGET_MENU,
        STATE_MANUAL_AIM,
        STATE_ALLY_CONFIRM,
        STATE_FINISHED
    };

    CombatAimHost *_host;
    Combat::CombatSession *_session;
    Combat::AimRequest _request;
    uint8 _range;
    State _state;

    Combat::TargetList _targets;
    int _index;            // 0-based into _targets.targetOrder
    int _skipStep;         // direction used to skip unengageable entries
    TilePos _manualCell;
    ::Goldbox::Data::PlayerCharacter *_pendingTarget;

    CombatTargetCommandDialog *_targetCommand;
    CombatManualAimDialog *_manualAim;
    HorizontalYesNo *_allyConfirm;

    void begin(const Combat::AimRequest &request,
               Combat::CombatSession *session);
    void buildTargets();
    ::Goldbox::Data::PlayerCharacter *currentTarget() const;
    TilePos currentTile() const;
    void advance(int step, bool animate);
    void showTargetMenu();
    void returnToMenu();
    void confirmTarget(::Goldbox::Data::PlayerCharacter *target);
    void attack(::Goldbox::Data::PlayerCharacter *target, bool allyConfirmed);
    void closeChildren();
    void finish(bool committed, bool turnComplete,
                const Combat::AimPick &pick = Combat::AimPick());
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_ATTACK_DIALOG_H
