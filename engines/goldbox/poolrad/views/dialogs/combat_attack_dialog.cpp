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
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

namespace {

Common::String rangeText(uint8 range) {
    return Common::String::format("Range = %u  ", (unsigned)range);
}

} // namespace

CombatAttackDialog::CombatAttackDialog()
    : Dialog("CombatAttack"), _host(nullptr), _session(nullptr), _range(1),
      _state(STATE_IDLE), _index(0), _skipStep(1), _manualCell(0, 0),
      _pendingTarget(nullptr) {
    _targetCommand = new CombatTargetCommandDialog();
    _manualAim = new CombatManualAimDialog();

    HorizontalYesNoConfig cfg;
    cfg.promptTxt       = "Attack Ally: ";
    cfg.promptColor     = 13;
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

void CombatAttackDialog::beginAttack(::Goldbox::Data::PlayerCharacter *attacker,
                                     Combat::CombatSession *session) {
    Combat::AimRequest request;
    request.attacker = attacker;
    request.range = 0xFF;
    request.weaponMode = true;
    request.groundOk = false;
    begin(request, session);
}

void CombatAttackDialog::beginSpellAim(::Goldbox::Data::PlayerCharacter *caster,
        Combat::CombatSession *session, uint8 range, bool groundOk) {
    Combat::AimRequest request;
    request.attacker = caster;
    request.range = range;
    request.weaponMode = false;
    request.groundOk = groundOk;
    begin(request, session);
}

void CombatAttackDialog::begin(const Combat::AimRequest &request,
                               Combat::CombatSession *session) {
    _request = request;
    _session = session;
    _range = Combat::resolveAimRange(request.attacker, request.range);
    _state = STATE_IDLE;
    _index = 0;
    _skipStep = 1;
    _pendingTarget = nullptr;
}

void CombatAttackDialog::activate() {
    Dialog::activate();
    if (!_session || !_request.attacker) {
        finish(false, false);
        return;
    }
    buildTargets();
    showTargetMenu();
}

void CombatAttackDialog::deactivate() {
    closeChildren();
    _pendingTarget = nullptr;
    Dialog::deactivate();
}

void CombatAttackDialog::closeChildren() {
    if (_targetCommand->isActive())
        _targetCommand->deactivate();
    detachDialog(_targetCommand);
    if (_manualAim->isActive())
        _manualAim->deactivate();
    detachDialog(_manualAim);
    if (_allyConfirm->isActive())
        _allyConfirm->deactivate();
    detachDialog(_allyConfirm);
}

void CombatAttackDialog::draw() {
    if (!_isVisible)
        return;
    switch (_state) {
    case STATE_TARGET_MENU:
        _targetCommand->draw();
        break;
    case STATE_MANUAL_AIM:
        _manualAim->draw();
        break;
    case STATE_ALLY_CONFIRM:
        _allyConfirm->draw();
        break;
    default:
        break;
    }
}

bool CombatAttackDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;
    switch (_state) {
    case STATE_TARGET_MENU:
        return _targetCommand->msgKeypress(msg);
    case STATE_MANUAL_AIM:
        return _manualAim->msgKeypress(msg);
    case STATE_ALLY_CONFIRM:
        return _allyConfirm->msgKeypress(msg);
    default:
        return false;
    }
}

// Only the Attack Ally yes/no reports through menu results (1 = yes).
void CombatAttackDialog::handleMenuResult(const MenuResultMessage &result) {
    if (_state != STATE_ALLY_CONFIRM || !result._hasIntValue)
        return;

    _allyConfirm->deactivate();
    detachDialog(_allyConfirm);
    ::Goldbox::Data::PlayerCharacter *target = _pendingTarget;
    _pendingTarget = nullptr;

    if (result._success && result._intValue == 1 && target) {
        _session->makePartyHostile();
        attack(target, true);
    } else {
        returnToMenu();
    }
}

// ---------------------------------------------------------------------------
// Target cycling

void CombatAttackDialog::buildTargets() {
    _targets.clear();
    _session->buildFacingTargetList(_request.attacker, _targets);
    const int count = (int)_targets.targetOrder.size();
    if (_index >= count)
        _index = count > 0 ? count - 1 : 0;
}

::Goldbox::Data::PlayerCharacter *CombatAttackDialog::currentTarget() const {
    if (!_session || _index < 0 || _index >= (int)_targets.targetOrder.size())
        return nullptr;
    return _session->getTable().getCharacter(_targets.targetOrder[_index]);
}

TilePos CombatAttackDialog::currentTile() const {
    ::Goldbox::Data::PlayerCharacter *target = currentTarget();
    return target ? _session->getTable().getCharacterPos(target) : TilePos(0, 0);
}

// COMBAT_cycleTarget: step through the list with wrap-around.
void CombatAttackDialog::advance(int step, bool animate) {
    const int count = (int)_targets.targetOrder.size();
    if (count == 0)
        return;
    const TilePos from = currentTile();
    _index = ((_index + step) % count + count) % count;
    if (animate && _host && step != 0)
        _host->aimFlyCursor(from, currentTile());
}

// Loop body of COMBAT_AttackTarget: skip what cannot be engaged, then offer
// the Aim menu on the current creature.
void CombatAttackDialog::showTargetMenu() {
    Combat::CombatContext *ctx = _session->getContext();
    const int count = (int)_targets.targetOrder.size();
    if (!ctx || count == 0) {
        finish(false, false);
        return;
    }

    int skipped = 0;
    while (!ctx->canEngageTarget(_request.attacker, currentTarget())) {
        if (++skipped > count) {
            finish(false, false);
            return;
        }
        advance(_skipStep, false);
    }

    ::Goldbox::Data::PlayerCharacter *target = currentTarget();
    if (_host) {
        if (_request.weaponMode)
            _host->aimShowRange(rangeText(
                ctx->getTargetRange(_request.attacker, target)));
        _host->aimShowTarget(target);
    }

    _state = STATE_TARGET_MENU;
    _targetCommand->beginTargetCommand(this, Combat::aimTargetOffered(
        *_session, _request.attacker, target, _range, _request.weaponMode));
    attachDialog(_targetCommand);
    _targetCommand->activate();
    redraw();
}

// After Manual or Target left the run open: rebuild the list and stay on
// the same entry.
void CombatAttackDialog::returnToMenu() {
    buildTargets();
    showTargetMenu();
}

void CombatAttackDialog::onAimCommand(AimCommand command) {
    if (_state == STATE_MANUAL_AIM) {
        if (command == AIM_CENTER && _host)
            _host->aimCenterOn(_manualCell);
        return;
    }
    if (_state != STATE_TARGET_MENU)
        return;
    detachDialog(_targetCommand);

    int step = 0;
    switch (command) {
    case AIM_NEXT:
        _skipStep = 1;
        step = 1;
        break;
    case AIM_PREV:
        _skipStep = -1;
        step = -1;
        break;
    case AIM_MANUAL:
        _state = STATE_MANUAL_AIM;
        _manualAim->beginManualAim(this, _session, _request, _range,
                                   currentTile());
        attachDialog(_manualAim);
        _manualAim->activate();
        redraw();
        return;
    case AIM_TARGET:
        confirmTarget(currentTarget());
        return;
    case AIM_CENTER:
        if (_host)
            _host->aimCenterOn(currentTile());
        break;
    case AIM_EXIT:
    default:
        finish(false, false);
        return;
    }

    advance(step, true);
    showTargetMenu();
}

// ---------------------------------------------------------------------------
// Free cursor

void CombatAttackDialog::onManualAimCursor(TilePos cell,
                                           const Combat::AimCell &info) {
    _manualCell = cell;
    if (!_host)
        return;
    if (_request.weaponMode)
        _host->aimShowRange(info.lineOfSight ? rangeText(info.range)
                                             : Common::String());
    _host->aimShowCell(cell, info.occupant);
}

void CombatAttackDialog::onManualAimFinished(bool picked,
                                             const Combat::AimPick &pick) {
    if (_state != STATE_MANUAL_AIM)
        return;
    detachDialog(_manualAim);

    if (!picked) {
        returnToMenu();
    } else if (_request.weaponMode) {
        confirmTarget(pick.target);
    } else {
        finish(true, false, pick);
    }
}

// ---------------------------------------------------------------------------
// Commit (COMBAT_ExecuteAttackOnTarget)

void CombatAttackDialog::confirmTarget(::Goldbox::Data::PlayerCharacter *target) {
    if (!target) {
        returnToMenu();
        return;
    }

    if (!_request.weaponMode) {
        Combat::AimPick pick;
        pick.target = target;
        pick.tile = _session->getTable().getCharacterPos(target);
        finish(true, false, pick);
        return;
    }

    if (!_session->confirmAttackNonHostile(_request.attacker, target)) {
        _pendingTarget = target;
        _state = STATE_ALLY_CONFIRM;
        attachDialog(_allyConfirm);
        _allyConfirm->activate();
        redraw();
        return;
    }

    attack(target, false);
}

void CombatAttackDialog::attack(::Goldbox::Data::PlayerCharacter *target,
                                bool allyConfirmed) {
    if (_host) {
        _host->aimShowRange(Common::String());
        _host->aimShowTarget(nullptr);
    }

    const bool done = _session->executeAttackOnTarget(_request.attacker,
        target, true, allyConfirmed);
    if (done) {
        Combat::AimPick pick;
        pick.target = target;
        finish(true, true, pick);
    } else {
        returnToMenu();
    }
}

void CombatAttackDialog::finish(bool committed, bool turnComplete,
                                const Combat::AimPick &pick) {
    if (_state == STATE_FINISHED)
        return;
    _state = STATE_FINISHED;
    closeChildren();

    AimOutcome outcome;
    outcome.committed = committed;
    outcome.turnComplete = turnComplete;
    outcome.pick = pick;
    if (_host) {
        _host->aimShowRange(Common::String());
        _host->aimFinished(outcome);
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
