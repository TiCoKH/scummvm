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

#include "goldbox/poolrad/views/dialogs/combat_move_dialog.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/player_character.h"
#include "goldbox/events.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

// Directions: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW  8=none
static const uint8 kDirNone = 8;

CombatMoveDialog::CombatMoveDialog()
    : Dialog("CombatMove"),
      _character(nullptr),
      _origMovePoints(0), _origDirection(0),
      _origPos(),
      _state(STATE_MOVE),
      _fleeYesNo(nullptr),
      _allyConfirm(nullptr),
      _pendingTarget(nullptr) {
    setBounds(Window(0, 24, 39, 24));

    HorizontalYesNoConfig cfg;
    cfg.promptTxt       = "Flee? ";
    cfg.promptColor     = 15;
    cfg.textColor       = 10;
    cfg.selectColor     = 15;
    cfg.backgroundColor = 8;
    _fleeYesNo = new HorizontalYesNo("CombatMoveFleeYesNo", cfg);

    cfg.promptTxt = "Attack Ally?";
    _allyConfirm = new HorizontalYesNo("CombatMoveAllyConfirm", cfg);
}

CombatMoveDialog::~CombatMoveDialog() {
    delete _fleeYesNo;
    delete _allyConfirm;
}

void CombatMoveDialog::beginMove(Goldbox::Data::PlayerCharacter *ch) {
    _character = ch;
    if (!ch || !ch->combatState)
        return;

    // Save original state for cancel/restore (mirrors orig_move/orig_facing/orig_x/orig_y).
    _origMovePoints = ch->combatState->movePoints;
    _origDirection  = ch->combatState->direction;
    _pendingTarget  = nullptr;

    const Combat::CombatantTable &table =
        Combat::g_combatSession->getTable();
    _origPos = table.getCharacterPos(ch);
}

void CombatMoveDialog::activate() {
    Dialog::activate();
    _state = STATE_MOVE;
}

void CombatMoveDialog::deactivate() {
    if (_fleeYesNo->isActive())
        _fleeYesNo->deactivate();
    if (_allyConfirm->isActive())
        _allyConfirm->deactivate();
    _pendingTarget = nullptr;
    Dialog::deactivate();
}

void CombatMoveDialog::draw() {
    if (!_isVisible)
        return;
    if (_state == STATE_FLEE && _fleeYesNo->isActive()) {
        _fleeYesNo->draw();
        return;
    }
    if (_state == STATE_ATTACK_CONFIRM && _allyConfirm->isActive()) {
        _allyConfirm->draw();
        return;
    }
    drawMovementMenu();
}

bool CombatMoveDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_state == STATE_FLEE)
        return _fleeYesNo->msgKeypress(msg);
    if (_state == STATE_ATTACK_CONFIRM)
        return _allyConfirm->msgKeypress(msg);

    if (!_character || !_character->combatState)
        return false;

    // Loop condition: move > 1 (mirrors while (move > 1 && ...)).
    if (_character->combatState->movePoints < 2) {
        finishAction(false);
        return true;
    }

    // RETURN / KP_ENTER: player voluntarily ends movement.
    if (msg.keycode == Common::KEYCODE_RETURN ||
            msg.keycode == Common::KEYCODE_KP_ENTER) {
        finishAction(false);
        return true;
    }

    // ESC / 0 on numpad: cancel and restore original position.
    if (msg.keycode == Common::KEYCODE_ESCAPE ||
            msg.keycode == Common::KEYCODE_KP0) {
        handleCancel();
        return true;
    }

    const uint8 dir = decodeDirection(msg);
    if (dir == kDirNone)
        return false;

    handleDirectionInput(dir);
    return true;
}

void CombatMoveDialog::handleMenuResult(const MenuResultMessage &result) {
    if (!Combat::g_combatSession || !_character)
        return;

    if (_state == STATE_FLEE) {
        _fleeYesNo->deactivate();
        _state = STATE_MOVE;

        if (result._hasIntValue && result._intValue == 1) {
            // YES — attempt to flee.
            const bool escaped = Combat::g_combatSession->flee(_character);
            // Fleeing ends the turn whether or not the attempt succeeds.
            finishAction(escaped);
        } else {
            redraw();
        }
        return;
    }

    if (_state != STATE_ATTACK_CONFIRM)
        return;

    _allyConfirm->deactivate();
    _state = STATE_MOVE;

    Goldbox::Data::PlayerCharacter *target = _pendingTarget;
    _pendingTarget = nullptr;
    if (result._success && result._hasIntValue && result._intValue == 1 &&
            target) {
        Combat::g_combatSession->makePartyHostile();
        if (Combat::g_combatSession->executeAttackOnTarget(
                    _character, target, true, true))
            finishAction(true);
        else
            redraw();
    } else {
        redraw();
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

// Numpad layout:
//   7(NW) 8(N) 9(NE)
//   4(W)  5    6(E)
//   1(SW) 2(S) 3(SE)
// Arrow keys map to cardinal directions.
uint8 CombatMoveDialog::decodeDirection(const KeypressMessage &msg) {
    switch (msg.keycode) {
    case Common::KEYCODE_KP8: case Common::KEYCODE_UP:    return 0; // N
    case Common::KEYCODE_KP9:                             return 1; // NE
    case Common::KEYCODE_KP6: case Common::KEYCODE_RIGHT: return 2; // E
    case Common::KEYCODE_KP3:                             return 3; // SE
    case Common::KEYCODE_KP2: case Common::KEYCODE_DOWN:  return 4; // S
    case Common::KEYCODE_KP1:                             return 5; // SW
    case Common::KEYCODE_KP4: case Common::KEYCODE_LEFT:  return 6; // W
    case Common::KEYCODE_KP7:                             return 7; // NW
    default:                                              return kDirNone;
    }
}

void CombatMoveDialog::handleDirectionInput(uint8 direction) {
    if (!Combat::g_combatSession || !_character)
        return;

    Combat::CombatSession &session = *Combat::g_combatSession;

    // Original: COMBAT_UpdateCharacterFacingAndRedraw() is called BEFORE
    // getGroundInfo(). The actor turns toward the requested direction even
    // if the destination is blocked or occupied.
    session.updateFacing(_character, direction);
    redraw();

    int occupant = 0;
    uint8 tileId = 0;
    session.queryGround(_character, direction, &occupant, &tileId);

    if (occupant != 0) {
        // Adjacent tile is occupied: enter action-select interaction.
        // The actor has already turned to face the target (above).
        handleActionSelect(occupant);
        return;
    }

    if (tileId == 0) {
        // ground_type == 0: special flee/run destination.
        enterFleePrompt();
        return;
    }

    if (_character->combatState->movePoints < session.getTilePassability(tileId)) {
        // Insufficient movement for this terrain cost.
        showBlockedMessage();
        return;
    }

    // Valid movement: perform the step (includes disengagement reactions,
    // movement cost deduction, occupancy rebuild, cloud/effect check).
    const Combat::CombatSession::MoveStepResult step =
        session.performMoveStep(_character, direction);

    switch (step.kind) {
    case Combat::CombatSession::MoveStepResult::MS_OK:
        if (_animateFn)
            _animateFn(step.fromPos, step.toPos, _character, _animateCtx);

        if (step.actionComplete || _character->hasNegativeEffect() ||
                !_character->enabled) {
            finishAction(true);
            return;
        }
        redraw();
        break;

    case Combat::CombatSession::MoveStepResult::MS_DISABLED:
        // Reactions or post-move effects may disable the actor after moving.
        if (_animateFn &&
                (step.fromPos.col != step.toPos.col ||
                 step.fromPos.row != step.toPos.row))
            _animateFn(step.fromPos, step.toPos, _character, _animateCtx);
        finishAction(true);
        break;

    case Combat::CombatSession::MoveStepResult::MS_BLOCKED:
        showBlockedMessage();
        break;

    case Combat::CombatSession::MoveStepResult::MS_OCCUPIED:
        // Should not reach here (handled above via queryGround), but guard it.
        finishAction(false);
        break;

    case Combat::CombatSession::MoveStepResult::MS_OUT_OF_BOUNDS:
        enterFleePrompt();
        break;
    }
}

void CombatMoveDialog::handleActionSelect(int occupantIndex) {
    if (!Combat::g_combatSession || !_character)
        return;

    Goldbox::Data::PlayerCharacter *target =
        Combat::g_combatSession->getTable().getCharacter(occupantIndex - 1);
    if (!target)
        return;

    Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<Goldbox::Data::ADnDCharacter *>(_character);
    if (adnd && adnd->hasRangedWeapon() &&
            !adnd->isEquippedRangedWeapon()) {
        PromptMessageConfig cfg;
        cfg.message = "Equip ranged weapon first.";
        cfg.textColor = 15;
        cfg.backgroundColor = 8;
        attachDialog(new PromptMessage("CombatMoveEquipRanged", cfg));
        return;
    }

    Combat::CombatSession &session = *Combat::g_combatSession;
    if (!session.confirmAttackNonHostile(_character, target)) {
        _pendingTarget = target;
        _state = STATE_ATTACK_CONFIRM;
        _allyConfirm->setParent(this);
        _allyConfirm->activate();
        redraw();
        return;
    }

    if (session.executeAttackOnTarget(_character, target, true))
        finishAction(true);
    else
        redraw();
}

void CombatMoveDialog::handleCancel() {
    if (!Combat::g_combatSession || !_character)
        return;

    // Restore original movement budget and position/facing.
    Combat::g_combatSession->cancelMove(
        _character, _origMovePoints, _origDirection, _origPos);

    // Cancel ends the move action (mirrors *result = !placed logic:
    // if restoration succeeds the action is not yet complete, so we
    // return false = "not done" to let CombatMain decide next step).
    finishAction(false);
}

void CombatMoveDialog::enterFleePrompt() {
    _state = STATE_FLEE;
    _fleeYesNo->setParent(this);
    _fleeYesNo->activate();
    redraw();
}

void CombatMoveDialog::finishAction(bool actionComplete) {
    if (!Combat::g_combatSession || !_character)
        return;

    // Normalize residual movement (mirrors: if (move < 2) move = 0).
    if (_character->combatState &&
            _character->combatState->movePoints < 2)
        _character->combatState->movePoints = 0;

    if (actionComplete)
        Combat::g_combatSession->finishMoveAction(_character);

    // Post result to CombatView to dismiss this dialog and advance the turn.
    if (g_events)
        g_events->postMenuResult("Combat", true, Common::KEYCODE_RETURN,
                                 static_cast<int>(Combat::CombatSession::PA_MOVE),
                                 Common::String(), true, false);
}

void CombatMoveDialog::showBlockedMessage() {
    PromptMessageConfig cfg;
    cfg.message = "Blocked";
    cfg.textColor = 15;
    cfg.backgroundColor = 8;
    attachDialog(new PromptMessage("CombatMoveBlocked", cfg));
}

void CombatMoveDialog::drawMovementMenu() {
    if (!_character || !_character->combatState)
        return;
    // Displayed budget is half the stored value (mirrors move >> 1).
    const uint8 budget = _character->combatState->movePoints >> 1;
    Surface s = getSurface();
    s.clearBox(0, 0, 39, 0, 8);
    s.writeStringC(0, 0, 10,
        Common::String::format("Move/Attack, Move Left = %d", budget));
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
