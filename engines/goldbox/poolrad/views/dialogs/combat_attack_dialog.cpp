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

#include "goldbox/poolrad/views/dialogs/combat_attack_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_target_command_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_manual_aim_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/data/player_character.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatAttackDialog::CombatAttackDialog()
    : Dialog("CombatAttack"), _attacker(nullptr), _session(nullptr),
      _targetIndex(0), _state(STATE_TARGET_COMMAND),
      _pendingTarget(nullptr) {
    _targetCommand = new CombatTargetCommandDialog();
    _manualAim     = new CombatManualAimDialog();

    HorizontalYesNoConfig cfg;
    cfg.promptTxt       = "Attack Ally?";
    cfg.promptColor     = 15;
    cfg.textColor       = 10;
    cfg.selectColor     = 15;
    cfg.backgroundColor = 0;
    _allyConfirm = new HorizontalYesNo("CombatAllyConfirm", cfg);
}

CombatAttackDialog::~CombatAttackDialog() {
    delete _targetCommand;
    delete _manualAim;
    delete _allyConfirm;
}

void CombatAttackDialog::beginAttack(Goldbox::Data::PlayerCharacter *attacker,
                                     Combat::CombatSession *session) {
    _attacker    = attacker;
    _session     = session;
    _targetIndex = 0;

    _targetList.targetOrder.clear();
    if (_attacker && _session)
        _session->buildFacingTargetList(_attacker, _targetList);

    // Start on the first target (1-based).
    if (!_targetList.targetOrder.empty())
        _targetIndex = 1;
}

void CombatAttackDialog::activate() {
    Dialog::activate();
    if (_targetIndex > 0)
        enterTargetCommand();
    else
        finishAttack(false);  // no targets — cancel immediately
}

void CombatAttackDialog::deactivate() {
    if (_targetCommand->isActive()) {
        _targetCommand->deactivate();
        detachDialog(_targetCommand);
    }
    if (_manualAim->isActive()) {
        _manualAim->deactivate();
        detachDialog(_manualAim);
    }
    if (_allyConfirm->isActive()) {
        _allyConfirm->deactivate();
        detachDialog(_allyConfirm);
    }
    _pendingTarget = nullptr;
    Dialog::deactivate();
}

void CombatAttackDialog::draw() {
    if (!_isVisible)
        return;
    if (_allyConfirm->isActive())
        _allyConfirm->draw();
    else if (_state == STATE_TARGET_COMMAND && _targetCommand->isActive())
        _targetCommand->draw();
    else if (_state == STATE_MANUAL_AIM && _manualAim->isActive())
        _manualAim->draw();
}

bool CombatAttackDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;
    if (_allyConfirm->isActive())
        return _allyConfirm->msgKeypress(msg);
    if (_state == STATE_TARGET_COMMAND && _targetCommand->isActive())
        return _targetCommand->msgKeypress(msg);
    if (_state == STATE_MANUAL_AIM && _manualAim->isActive())
        return _manualAim->msgKeypress(msg);
    return false;
}

void CombatAttackDialog::handleMenuResult(const MenuResultMessage &result) {
    if (!result._hasIntValue)
        return;

    if (_state == STATE_TARGET_COMMAND) {
        // Results from CombatTargetCommandDialog.
        const char cmd = (char)result._intValue;
        switch (cmd) {
        case 'N':
            cycleTarget(+1);
            enterTargetCommand();
            break;
        case 'P':
            cycleTarget(-1);
            enterTargetCommand();
            break;
        case 'M':
            enterManualAim();
            break;
        case 'T': {
            // Confirm: check if target requires ally-attack confirmation.
            Goldbox::Data::PlayerCharacter *target = currentTarget();
            if (target && _session &&
                    !_session->confirmAttackNonHostile(_attacker, target)) {
                // Cross-side attack by player: show "Attack Ally?" prompt.
                _pendingTarget = target;
                attachDialog(_allyConfirm);
                _allyConfirm->activate();
            } else {
                if (target && _session)
                    _session->executeAttackOnTarget(_attacker, target, true);
                finishAttack(true);
            }
            break;
        }
        case 'E':
        default:
            finishAttack(false);
            break;
        }
    } else if (_state == STATE_MANUAL_AIM) {
        // Results from CombatManualAimDialog.
        if (_manualAim->isActive()) {
            _manualAim->deactivate();
            detachDialog(_manualAim);
        }
        if (result._success)
            finishAttack(true);
        else
            finishAttack(false);
    } else if (_allyConfirm->isActive()) {
        // Results from HorizontalYesNo "Attack Ally?".
        _allyConfirm->deactivate();
        detachDialog(_allyConfirm);
        if (result._success) {
            // Player confirmed: mutate combat state then execute.
            _session->makePartyHostile();
            if (_pendingTarget)
                _session->executeAttackOnTarget(_attacker, _pendingTarget, true);
            _pendingTarget = nullptr;
            finishAttack(true);
        } else {
            _pendingTarget = nullptr;
            finishAttack(false);
        }
    }
}

// ---------------------------------------------------------------------------
// Private helpers

void CombatAttackDialog::cycleTarget(int direction) {
    if (_targetList.targetOrder.empty())
        return;
    const int count = (int)_targetList.targetOrder.size();
    _targetIndex += direction;
    if (_targetIndex < 1)
        _targetIndex = count;
    if (_targetIndex > count)
        _targetIndex = 1;
}

Goldbox::Data::PlayerCharacter *CombatAttackDialog::currentTarget() const {
    if (!_session || _targetIndex < 1)
        return nullptr;
    const int idx = _targetIndex - 1;
    if (idx >= (int)_targetList.targetOrder.size())
        return nullptr;
    const uint8 tableIdx = _targetList.targetOrder[idx];
    return _session->getTable().getCharacter(tableIdx);
}

void CombatAttackDialog::enterTargetCommand() {
    if (_manualAim->isActive()) {
        _manualAim->deactivate();
        detachDialog(_manualAim);
    }
    if (_targetCommand->isActive()) {
        _targetCommand->deactivate();
        detachDialog(_targetCommand);
    }

    _state = STATE_TARGET_COMMAND;

    Goldbox::Data::PlayerCharacter *target = currentTarget();
    if (!target) {
        finishAttack(false);
        return;
    }

    _targetCommand->beginTargetCommand(_attacker, target, 0xFF, false, _session);
    attachDialog(_targetCommand);
    _targetCommand->activate();
}

void CombatAttackDialog::enterManualAim() {
    if (_targetCommand->isActive()) {
        _targetCommand->deactivate();
        detachDialog(_targetCommand);
    }

    _state = STATE_MANUAL_AIM;

    _manualAim->beginManualAim(_attacker, currentTarget(), 0xFF, true, false, _session);
    attachDialog(_manualAim);
    _manualAim->activate();
}

void CombatAttackDialog::finishAttack(bool actionComplete) {
    if (g_events)
        g_events->postMenuResult("Combat", actionComplete,
                                 Common::KEYCODE_RETURN,
                                 (int)Combat::CombatSession::PA_ATTACK,
                                 Common::String(), true, false);
    deactivate();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
