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

#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combat_setup.h"
#include "goldbox/combat/combat_ai.h"
#include "goldbox/combat/combat_ground_info.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/core/direction.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"

namespace Goldbox {
namespace Combat {

CombatSession *g_combatSession = nullptr;

CombatSession::CombatSession()
    : _phase(PHASE_NONE), _currentActor(nullptr) {
}

CombatSession::~CombatSession() {
    if (g_combatSession == this)
        g_combatSession = nullptr;
}

void CombatSession::setup(const CombatParams &params) {
    _params = params;
    _globals.turnCounter = 0;
    _phase = PHASE_SETUP;

    if (_params.tilePropertyProvider)
        _battlefieldMap.setTilePropertyProvider(_params.tilePropertyProvider);

    setupCombat(_params, _globals, _battlefieldMap, _table,
                _placement, _viewport, nullptr);

    _table.setViewportOrigin(_viewport.getTopLeft());
    _phase = PHASE_PLAYER_TURN;
    _currentActor = nullptr;
    _context.reset(new CombatContext(makeContext()));
}

CombatSession::TickResult CombatSession::tick() {
    TickResult result;

    if (_phase == PHASE_NONE || _phase == PHASE_ENDED)
        return result;

    // --- Round start ---
    if (_phase == PHASE_PLAYER_TURN && _currentActor == nullptr) {
        _globals.updateSideCount(_params.roster);
        if (_globals.sideCount[0] == 0 || _globals.sideCount[1] == 0) {
            _globals.turnCounter++;
            _phase = PHASE_ENDED;
            result.event = TickResult::EV_COMBAT_END;
            return result;
        }

        CombatContext initCtx = makeContext();
        initCtx.initAllTurnStates();
        // Clear D_CombatIsAmbush after all initiatives are rolled.
        if (_params.eclMemory && _params.vmGlobalLayout) {
            const VmFieldLocation field =
                _params.vmGlobalLayout->field(kVmGlobalFieldCombatIsAmbush);
            if (VmLayout::isValid(field))
                _params.eclMemory->write8(field.vmAddr, 0);
        }
        _currentActor = initCtx.selectNextActor();
    }

    if (_currentActor == nullptr)
        return result;

    result.actor = _currentActor;

    // Per-actor setup (mirrors COMBAT_ExecuteTurn pre-dispatch block).
    // Returns false if an effect cancelled the turn.
    if (!prepareTurn(_currentActor)) {
        _currentActor = makeContext().selectNextActor();
        if (_currentActor == nullptr) {
            _globals.turnCounter++;
            updateHostileHealthPercent(_table, _globals);
            _phase = PHASE_PLAYER_TURN;
            result.event = TickResult::EV_ROUND_END;
        }
        return result;
    }

    result.event = TickResult::EV_ACTOR_FOCUSED;
    {
        const int idx = _table.findIndex(_currentActor);
        result.actorSize = (idx >= 0) ? _table.getSize(idx) : 1;
    }

    // --- AI turn ---
    if (_currentActor->combatSide == ::Goldbox::Data::CS_ENEMY) {
        _phase = PHASE_AI_TURN;
        CombatContext ctx = makeContext();
        AiTurnResult ai = executeAiTurn(_currentActor, ctx);

        if (ai.action == AiTurnResult::ACTION_ATTACK && ai.target && ai.damage > 0) {
            result.event          = TickResult::EV_AI_ATTACK;
            result.target         = ai.target;
            result.damage         = ai.damage;
            result.targetWentDown = ai.targetWentDown;
        }
    } else {
        // Party actor — pause and wait for player input.
        _phase = PHASE_AWAITING_PLAYER;
        return result;
    }

    // --- Advance to next actor ---
    _currentActor = makeContext().selectNextActor();

    if (_currentActor == nullptr) {
        _globals.turnCounter++;
        updateHostileHealthPercent(_table, _globals);
        _phase = PHASE_PLAYER_TURN;
        if (result.event == TickResult::EV_NONE)
            result.event = TickResult::EV_ROUND_END;
    } else if (_currentActor->combatSide == ::Goldbox::Data::CS_PARTY) {
        _phase = PHASE_PLAYER_TURN;
    }

    return result;
}

CombatSession::TickResult CombatSession::submitPlayerAction(PlayerAction action) {
    TickResult result;
    if (_phase != PHASE_AWAITING_PLAYER || !_currentActor)
        return result;

    result.actor = _currentActor;
    Data::CombatAction *cs = _currentActor->combatState;

    switch (action) {
    case PA_GUARD:
        if (cs) cs->guarding = true;
        break;
    case PA_FLEE:
        if (cs) cs->fleeing = true;
        break;
    case PA_ATTACK:
    case PA_CAST:
    case PA_USE:
    case PA_MOVE:
    default:
        // Full implementations (target selection, spell picker, etc.) are
        // added per-action in subsequent steps. For now mark as acted.
        break;
    }

    // Mark actor as having acted this round.
    if (cs)
        cs->initiative = 0xFF;

    // Advance to next actor.
    _currentActor = makeContext().selectNextActor();

    if (_currentActor == nullptr) {
        _globals.turnCounter++;
        updateHostileHealthPercent(_table, _globals);
        _phase = PHASE_PLAYER_TURN;
        result.event = TickResult::EV_ROUND_END;
    } else if (_currentActor->combatSide == ::Goldbox::Data::CS_PARTY) {
        _phase = PHASE_AWAITING_PLAYER;
    } else {
        _phase = PHASE_AI_TURN;
    }

    return result;
}

void CombatSession::scrollViewport(TilePos target, uint8 radius) {
    _viewport.adjustToInclude(target, radius);
    _table.setViewportOrigin(_viewport.getTopLeft());
}

bool CombatSession::prepareTurn(Data::PlayerCharacter *ch) {
    if (!ch || !ch->combatState)
        return false;

    Data::CombatAction &cs = *ch->combatState;

    // Mirrors COMBAT_ExecuteTurn: reset per-turn fields before dispatch.
    cs.attackCount     = 0;
    cs.directionChange = 0;
    cs.guarding        = false;

    // ES_POST_MOVEMENT_TILE (7) — may apply poison/regen/etc.
    if (_params.effectRuntime && ch->getEffects())
        _params.effectRuntime->checkEffectSet(
            Data::Effects::ES_POST_MOVEMENT_TILE,
            *ch->getEffects(), *ch, &_globals);

    // initiative==20 is a sentinel; clamp to 19 so normal ordering applies.
    if (cs.initiative == 20)
        cs.initiative = 19;

    // If initiative dropped to 0 (e.g. paralysis effect), skip this actor.
    if (cs.initiative == 0)
        return false;

    // Make this character the active combat character (mirrors PTR_SELECTED_CHAR).
    _globals.attacker = ch;

    // Recalculate stats that may have changed since turn initialization
    // (mirrors CHARACTER_RecalcCombatStats).
    if (Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(ch))
        adnd->recalcCombatStats();

    // ES_POISON_CYCLE (15) — may cancel the turn (sets initiative=0).
    if (_params.effectRuntime && ch->getEffects())
        _params.effectRuntime->checkEffectSet(
            Data::Effects::ES_POISON_CYCLE,
            *ch->getEffects(), *ch, &_globals);

    return cs.initiative > 0;
}

CombatContext CombatSession::makeContext() {
    return CombatContext(_globals, _params, _table, _battlefieldMap, _viewport);
}

void CombatSession::updateFacing(Data::PlayerCharacter *ch, uint8 direction) {
    if (ch && ch->combatState)
        ch->combatState->direction = direction;
}

void CombatSession::queryGround(Data::PlayerCharacter *ch, uint8 direction,
                                int *outOccupant, uint8 *outTile) const {
    getGroundInfo(ch, direction, outOccupant, outTile);
}

uint8 CombatSession::getTilePassability(uint8 tileId) const {
    if (tileId == 0)
        return 0;
    const TilePropertyProvider *props = _battlefieldMap.getTilePropertyProvider();
    if (!props)
        return 1;
    const TileProp *p = props->getTileProp(tileId - 1);
    return p ? (uint8)p->passable : 0;
}

CombatSession::MoveStepResult CombatSession::performMoveStep(
        Data::PlayerCharacter *ch, uint8 direction) {
    MoveStepResult result;
    if (!ch || !ch->combatState) {
        result.kind = MoveStepResult::MS_DISABLED;
        return result;
    }

    int occupant = 0;
    uint8 tileId = 0;
    getGroundInfo(ch, direction, &occupant, &tileId);

    if (occupant != 0) {
        result.kind = MoveStepResult::MS_OCCUPIED;
        result.occupantIndex = occupant;
        return result;
    }

    if (tileId == kTileIdNone) {
        result.kind = MoveStepResult::MS_OUT_OF_BOUNDS;
        return result;
    }

    if (ch->combatState->movePoints < getTilePassability(tileId)) {
        result.kind = MoveStepResult::MS_BLOCKED;
        return result;
    }

    // Advance-engage check (mirrors tryAdvanceEngage).
    // Deduct movement cost and update position via table.
    const uint8 cost = getTilePassability(tileId);
    ch->combatState->movePoints -= cost;

    // Compute destination tile.
    const int idx = _table.findIndex(ch);
    if (idx >= 0) {
        const int8 dx = ::Goldbox::kDirDeltaX[direction];
        const int8 dy = ::Goldbox::kDirDeltaY[direction];
        uint8 newCol = (uint8)(_table.getTileCol(idx) + dx);
        uint8 newRow = (uint8)(_table.getTileRow(idx) + dy);
        _table.setPosition(idx, TilePos(newCol, newRow));
        _table.rebuildOccupancy();
        scrollViewport(TilePos(newCol, newRow), 2);
    }

    if (!ch->enabled) {
        if (ch->combatState) ch->combatState->reset();
        result.kind = MoveStepResult::MS_DISABLED;
        result.actionComplete = true;
        return result;
    }

    // Status check (mirrors checkAndApplyStatus / hasNegativeEffect).
    if (_params.effectRuntime && ch->getEffects()) {
        _params.effectRuntime->checkEffectSet(
            Data::Effects::ES_POST_MOVEMENT_TILE,
            *ch->getEffects(), *ch, &_globals);
    }

    if (!ch->enabled) {
        if (ch->combatState) ch->combatState->reset();
        result.kind = MoveStepResult::MS_DISABLED;
        result.actionComplete = true;
    }

    return result;
}

void CombatSession::cancelMove(Data::PlayerCharacter *ch,
                               uint8 origMovePoints, uint8 origDirection,
                               uint8 origCol, uint8 origRow) {
    if (!ch || !ch->combatState)
        return;
    ch->combatState->movePoints = origMovePoints;
    ch->combatState->direction  = origDirection;
    const int idx = _table.findIndex(ch);
    if (idx >= 0) {
        _table.setPosition(idx, TilePos(origCol, origRow));
        _table.rebuildOccupancy();
        scrollViewport(TilePos(origCol, origRow), 2);
    }
}

bool CombatSession::trySetFleeing(Data::PlayerCharacter *ch) {
    if (!ch || !ch->combatState)
        return false;
    ch->combatState->fleeing = true;
    // Mirrors trySetRunning: action is complete when fleeing is set.
    return true;
}

CombatSession::TickResult CombatSession::finishMoveAction(Data::PlayerCharacter *ch) {
    if (ch && ch->combatState)
        ch->combatState->initiative = 0xFF;
    return submitPlayerAction(PA_NONE);
}

} // namespace Combat
} // namespace Goldbox
