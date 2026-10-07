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

// Forward declarations for combat-internal functions defined in combat_turn.cpp.
namespace Goldbox { namespace Data { class PlayerCharacter; } }
namespace Goldbox { namespace Combat {
uint8 calcMoveBudget(Data::PlayerCharacter *ch);
uint8 getOpposingSideMaxReach(const Data::PlayerCharacter *ch,
                              const Common::Array<Data::PlayerCharacter *> &roster);
} }
#include "goldbox/combat/combat_ai.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/core/direction.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/poolrad/effect_handler.h"
#include "goldbox/vm_interface.h"
#include "common/debug.h"

namespace Goldbox {
namespace Combat {

static const char *phaseStr(CombatSession::Phase p) {
    switch (p) {
    case CombatSession::PHASE_NONE:             return "NONE";
    case CombatSession::PHASE_SETUP:            return "SETUP";
    case CombatSession::PHASE_PLAYER_TURN:      return "PLAYER_TURN";
    case CombatSession::PHASE_AWAITING_PLAYER:  return "AWAITING_PLAYER";
    case CombatSession::PHASE_AI_TURN:          return "AI_TURN";
    case CombatSession::PHASE_ROUND_END:        return "ROUND_END";
    case CombatSession::PHASE_COMBAT_END:       return "COMBAT_END";
    case CombatSession::PHASE_ENDED:            return "ENDED";
    default:                                    return "?";
    }
}

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

    _phase = PHASE_PLAYER_TURN;
    _currentActor = nullptr;
    _context.reset(new CombatContext(makeContext()));

    debug(0, "[COMBAT] setup: roster=%u partyCount=%d",
        (unsigned)_params.roster.size(), _params.partyCount);
    for (uint i = 0; i < _params.roster.size(); i++) {
        Data::PlayerCharacter *ch = _params.roster[i];
        if (!ch) { debug(0, "  roster[%u] = NULL", i); continue; }
        debug(0, "  roster[%u] name='%s' side=%d ai=%d enabled=%d hp=%d/%d",
            i, ch->name.c_str(), (int)ch->combatSide,
            (int)ch->ai_control, (int)ch->enabled,
            ch->hitPoints.current, ch->hitPoints.max);
    }
}

CombatSession::TurnResult CombatSession::executeTurn() {
    TurnResult result;

    if (_phase == PHASE_NONE || _phase == PHASE_ENDED ||
            _phase == PHASE_ROUND_END || _phase == PHASE_COMBAT_END)
        return result;

    // --- Round start ---
    if (_phase == PHASE_PLAYER_TURN && _currentActor == nullptr) {
        _globals.updateSideCount(_params.roster);
        debug(0, "[COMBAT] round start: sideCount[PARTY]=%d sideCount[ENEMY]=%d",
            (int)_globals.sideCount[0], (int)_globals.sideCount[1]);
        if (_globals.sideCount[0] == 0 || _globals.sideCount[1] == 0) {
            _globals.turnCounter++;
            _phase = PHASE_COMBAT_END;
            result.event = TurnResult::EV_COMBAT_END;
            debug(0, "[COMBAT] -> EV_COMBAT_END (one side empty)");
            return result;
        }

        CombatContext initCtx = makeContext();
        initCtx.initAllTurnStates();
        debug(0, "[COMBAT] initiatives rolled:");
        for (uint i = 0; i < _params.roster.size(); i++) {
            Data::PlayerCharacter *ch = _params.roster[i];
            if (!ch) continue;
            uint8 init = ch->combatState ? ch->combatState->initiative : 0;
            debug(0, "  roster[%u] '%s' side=%d ai=%d enabled=%d initiative=%d",
                i, ch->name.c_str(), (int)ch->combatSide,
                (int)ch->ai_control, (int)ch->enabled, (int)init);
        }
        // Clear D_CombatIsAmbush after all initiatives are rolled.
        if (_params.eclMemory && _params.vmGlobalLayout) {
            const VmFieldLocation field =
                _params.vmGlobalLayout->field(kVmGlobalFieldCombatIsAmbush);
            if (VmLayout::isValid(field))
                _params.eclMemory->write8(field.vmAddr, 0);
        }
        _currentActor = initCtx.selectNextActor();
        debug(0, "[COMBAT] first actor selected: %s",
            _currentActor ? _currentActor->name.c_str() : "NULL");
    }

    if (_currentActor == nullptr) {
        debug(0, "[COMBAT] executeTurn: _currentActor=NULL phase=%s -> EV_NONE",
            phaseStr(_phase));
        return result;
    }

    debug(0, "[COMBAT] executeTurn: actor='%s' side=%d ai=%d phase=%s",
        _currentActor->name.c_str(), (int)_currentActor->combatSide,
        (int)_currentActor->ai_control, phaseStr(_phase));

    result.actor = _currentActor;

    // Mirrors COMBAT_ExecuteTurn pre-gate block.
    // Returns false if an effect cancelled the turn.
    if (!applyTurnStartEffects(_currentActor)) {
        debug(0, "[COMBAT] applyTurnStartEffects cancelled turn for '%s' (initiative=%d)",
            _currentActor->name.c_str(),
            _currentActor->combatState ? (int)_currentActor->combatState->initiative : -1);
        _currentActor = makeContext().selectNextActor();
        if (_currentActor == nullptr) {
            _globals.turnCounter++;
            updateHostileHealthPercent(_table, _globals);
            _phase = PHASE_ROUND_END;
            result.event = TurnResult::EV_ROUND_END;
        }
        return result;
    }

    // Temporarily make this actor the selected character (mirrors PTR_SELECTED_CHAR).
    _previousAttacker = _globals.attacker;
    _globals.attacker = _currentActor;

    // Recalculate stats now that PTR_SELECTED_CHAR is set.
    if (Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(_currentActor))
        adnd->recalcCombatStats();

    // Signal view to focus and draw combat info (mirrors COMBAT_FocusCharacter +
    // COMBATVIEW_drawCombatInfo). Must happen after recalc, before ES15.
    result.event = TurnResult::EV_ACTOR_FOCUSED;
    {
        const int idx = _table.findIndex(_currentActor);
        result.actorSize = (idx >= 0) ? _table.getSize(idx) : 1;
    }

    // ES15 runs after recalc+draw; may zero initiative and cancel the turn.
    if (_params.effectRuntime && _currentActor->getEffects())
        _params.effectRuntime->checkEffectSet(
            Data::Effects::ES_POISON_CYCLE,
            *_currentActor->getEffects(), *_currentActor, &_globals);

    // Second initiative gate: ES15 may have cancelled the turn.
    if (_currentActor->combatState->initiative == 0) {
        debug(0, "[COMBAT] ES15 zeroed initiative for '%s', skipping",
            _currentActor->name.c_str());
        _globals.attacker = _previousAttacker;
        _currentActor = makeContext().selectNextActor();
        result.event = TurnResult::EV_NONE;
        if (_currentActor == nullptr) {
            _globals.turnCounter++;
            updateHostileHealthPercent(_table, _globals);
            _phase = PHASE_ROUND_END;
            result.event = TurnResult::EV_ROUND_END;
        }
        return result;
    }

    // Dispatch: ai_control==0 means player-controlled.
    if (_currentActor->ai_control == 0) {
        debug(0, "[COMBAT] -> PHASE_AWAITING_PLAYER for '%s' initiative=%d",
            _currentActor->name.c_str(),
            _currentActor->combatState ? (int)_currentActor->combatState->initiative : -1);
        _phase = PHASE_AWAITING_PLAYER;
        return result;
    }

    // AI-controlled actor: first call returns EV_ACTOR_FOCUSED so the view
    // can redraw the viewport (mirrors GFX_ViewPortUpdate before COMBAT_MoveByAI).
    // The actual AI execution runs on the next executeTurn() call.
    if (_phase != PHASE_AI_TURN) {
        debug(0, "[COMBAT] -> PHASE_AI_TURN (focus) for '%s' initiative=%d",
            _currentActor->name.c_str(),
            _currentActor->combatState ? (int)_currentActor->combatState->initiative : -1);
        _phase = PHASE_AI_TURN;
        return result;
    }

    // Second call: run the AI turn.
    debug(0, "[COMBAT] -> moveByAI for '%s'", _currentActor->name.c_str());
    {
        CombatContext ctx = makeContext();
        moveByAI(_currentActor, ctx, _aiDelegate);
    }

    debug(0, "[COMBAT] moveByAI done for '%s'", _currentActor->name.c_str());

    // Restore selected-character context (mirrors PTR_SELECTED_CHAR = previousSelected).
    _globals.attacker = _previousAttacker;

    // --- Advance to next actor ---
    _currentActor = makeContext().selectNextActor();
    debug(0, "[COMBAT] next actor after AI: %s",
        _currentActor ? _currentActor->name.c_str() : "NULL");

    // Reset to PHASE_PLAYER_TURN so the next actor (AI or player) goes
    // through the full executeTurn() focus path on the next call.
    _phase = PHASE_PLAYER_TURN;

    if (_currentActor == nullptr) {
        _globals.turnCounter++;
        updateHostileHealthPercent(_table, _globals);
        _phase = PHASE_ROUND_END;
        result.event = TurnResult::EV_ROUND_END;
        debug(0, "[COMBAT] -> EV_ROUND_END (no more actors)");
    } else {
        // Signal the view to focus the next actor before the next executeTurn() call.
        result.event = TurnResult::EV_ACTOR_FOCUSED;
        result.actor = _currentActor;
        const int idx = _table.findIndex(_currentActor);
        result.actorSize = (idx >= 0) ? _table.getSize(idx) : 1;
        debug(0, "[COMBAT] -> EV_ACTOR_FOCUSED (post-AI) for '%s' side=%d ai=%d",
            _currentActor->name.c_str(),
            (int)_currentActor->combatSide, (int)_currentActor->ai_control);
    }

    return result;
}

CombatSession::TurnResult CombatSession::submitPlayerAction(PlayerAction action) {
    TurnResult result;
    if (_phase != PHASE_AWAITING_PLAYER || !_currentActor) {
        debug(0, "[COMBAT] submitPlayerAction: ignored (phase=%s actor=%s)",
            phaseStr(_phase), _currentActor ? _currentActor->name.c_str() : "NULL");
        return result;
    }
    debug(0, "[COMBAT] submitPlayerAction: action=%d actor='%s'",
        (int)action, _currentActor->name.c_str());

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
        break;
    }

    // Mark actor as having acted this round.
    if (cs)
        cs->initiative = 0xFF;

    // Restore selected-character context set in executeTurn() (mirrors PTR_SELECTED_CHAR = previousSelected).
    _globals.attacker = _previousAttacker;
    _previousAttacker = nullptr;

    // Advance to next actor.
    _currentActor = makeContext().selectNextActor();

    if (_currentActor == nullptr) {
        _globals.turnCounter++;
        updateHostileHealthPercent(_table, _globals);
        _phase = PHASE_ROUND_END;
        result.event = TurnResult::EV_ROUND_END;
        debug(0, "[COMBAT] submitPlayerAction -> EV_ROUND_END");
    } else {
        // Always go through PHASE_PLAYER_TURN so the next actor gets
        // EV_ACTOR_FOCUSED focus/redraw before acting (AI or player).
        _phase = PHASE_PLAYER_TURN;
        debug(0, "[COMBAT] submitPlayerAction -> next actor='%s' side=%d ai=%d phase=PLAYER_TURN",
            _currentActor->name.c_str(),
            (int)_currentActor->combatSide, (int)_currentActor->ai_control);
    }

    return result;
}

void CombatSession::scrollViewport(TilePos target, uint8 radius) {
    _viewport.adjustToInclude(target, radius);
}

void CombatSession::acknowledgeRoundEnd(bool continueEncounter) {
    if (_phase != PHASE_ROUND_END && _phase != PHASE_COMBAT_END)
        return;
    if (!continueEncounter || _phase == PHASE_COMBAT_END) {
        _phase = PHASE_ENDED;
        return;
    }
    // Resume: next executeTurn() call will start a fresh round.
    _phase = PHASE_PLAYER_TURN;
    _currentActor = nullptr;
}

bool CombatSession::applyTurnStartEffects(Data::PlayerCharacter *ch) {
    if (!ch || !ch->combatState)
        return false;

    Data::CombatAction &cs = *ch->combatState;

    // Reset per-turn transient fields.
    cs.attackCount     = 0;
    cs.directionChange = 0;
    cs.guarding        = false;

    // ES7 runs unconditionally before the first initiative gate.
    if (_params.effectRuntime && ch->getEffects())
        _params.effectRuntime->checkEffectSet(
            Data::Effects::ES_POST_MOVEMENT_TILE,
            *ch->getEffects(), *ch, &_globals);

    // First initiative gate.
    if (cs.initiative == 0)
        return false;

    return true;
}

CombatContext CombatSession::makeContext() {
    return CombatContext(_globals, _params, _table, _battlefieldMap, _viewport, _targetList);
}

CombatContext CombatSession::makeContext() const {
    return CombatContext(
        const_cast<CombatGlobals &>(_globals),
        const_cast<CombatParams &>(_params),
        const_cast<CombatantTable &>(_table),
        const_cast<BattlefieldMap &>(_battlefieldMap),
        const_cast<CombatViewport &>(_viewport),
        const_cast<TargetList &>(_targetList));
}

void CombatSession::updateFacing(Data::PlayerCharacter *ch, uint8 direction) {
    if (ch && ch->combatState)
        ch->combatState->direction = direction;
}

void CombatSession::queryGround(Data::PlayerCharacter *ch, uint8 direction,
                                int *outOccupant, uint8 *outTile) const {
    makeContext().getGroundInfo(ch, direction, outOccupant, outTile);
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
    makeContext().getGroundInfo(ch, direction, &occupant, &tileId);

    if (occupant != 0) {
        result.kind = MoveStepResult::MS_OCCUPIED;
        result.occupantIndex = occupant;
        return result;
    }

    if (tileId == 0x00) {
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
        TilePos cur = _table.getTilePos(idx);
        TilePos newPos((uint8)(cur.col + dx), (uint8)(cur.row + dy));
        result.fromPos = cur;
        result.toPos   = newPos;
        _table.setPosition(idx, newPos);
        _table.rebuildOccupancy();
        scrollViewport(newPos, 2);
    }

    if (!ch->enabled) {
        if (ch->combatState) ch->combatState->clear();
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
        if (ch->combatState) ch->combatState->clear();
        result.kind = MoveStepResult::MS_DISABLED;
        result.actionComplete = true;
    }

    return result;
}

void CombatSession::cancelMove(Data::PlayerCharacter *ch,
                               uint8 origMovePoints, uint8 origDirection,
                               TilePos origPos) {
    if (!ch || !ch->combatState)
        return;
    ch->combatState->movePoints = origMovePoints;
    ch->combatState->direction  = origDirection;
    const int idx = _table.findIndex(ch);
    if (idx >= 0) {
        _table.setPosition(idx, origPos);
        _table.rebuildOccupancy();
        scrollViewport(origPos, 2);
    }
}

CombatSession::TryFleeResult CombatSession::trySetFleeing(Data::PlayerCharacter *ch) {
    TryFleeResult result = { true, false };
    if (!ch || !ch->combatState)
        return result;

    CombatContext ctx = makeContext();

    // Build target list against all opposing combatants.
    ctx.buildTargetList(ch, 0xFF);
    const bool hasTargets = !ctx.targetList.targetOrder.empty();

    bool escaped = false;

    if (!hasTargets) {
        escaped = true;
    } else {
        const uint8 requiredMove = calcMoveBudget(ch) >> 1;
        const uint8 opposingReach = getOpposingSideMaxReach(ch, _params.roster);

        if (opposingReach < requiredMove) {
            escaped = true;
        } else if (opposingReach == requiredMove) {
            escaped = (VmInterface::rollDice(1, 2) == 1);
        }
        // opposingReach > requiredMove: escaped stays false
    }

    if (escaped)
        setCharacterStatus(ch, Data::S_RUNNING);
    else if (ch->combatState)
        ch->combatState->endTurn();
    result.escaped = escaped;
    return result;
}

CombatSession::SetStatusResult CombatSession::setCharacterStatus(
        Data::PlayerCharacter *ch, uint8 status) {
    SetStatusResult result = { -1, false };
    if (!ch || !ch->enabled)
        return result;

    result.combatIndex = _table.findIndex(ch);

    ch->enabled      = false;
    ch->healthStatus = status;
    if (status != Data::S_RUNNING)
        ch->hitPoints.current = 0;

    if (result.combatIndex >= 0) {
        _table.setSize(result.combatIndex, 0);
        _table.rebuildOccupancy();
    }

    bool wasCurrent = (ch == _currentActor);
    if (ch->combatState) {
        ch->combatState->endTurn();
        if (wasCurrent) {
            ch->clearStatusEffects();
            result.effectsCleared = true;
        }
    }

    return result;
}

bool CombatSession::flee(Data::PlayerCharacter *ch) {
    return trySetFleeing(ch).escaped;
}

CombatSession::TurnResult CombatSession::finishMoveAction(Data::PlayerCharacter *ch) {
    if (ch && ch->combatState)
        ch->combatState->initiative = 0xFF;
    return submitPlayerAction(PA_NONE);
}

bool CombatSession::applyMoveStep(Data::PlayerCharacter *ch, uint8 direction,
                                   void (*viewCallback)(Data::PlayerCharacter *)) {
    if (!ch || !ch->combatState)
        return false;

    Data::CombatAction &cs = *ch->combatState;

    // Diagonal (odd direction) costs 3; orthogonal costs 2.
    const uint8 cost = (direction & 1) ? 3 : 2;
    if (cs.movePoints < cost) {
        cs.movePoints = 0;
        return false;
    }
    cs.movePoints -= cost;

    const int idx = _table.findIndex(ch);
    if (idx < 0)
        return false;

    const TilePos src = _table.getTilePos(idx);
    const TilePos dst((uint8)(src.col + kDirDeltaX[direction]),
                      (uint8)(src.row + kDirDeltaY[direction]));

    // AI-controlled characters use a larger viewport scroll radius and
    // may need a viewport reposition before the move.
    uint8 scrollRadius = 1;
    if (ch->ai_control) {
        scrollRadius = 3;
        // If destination is outside the viewport, scroll to source first.
        if (!_context->isCharacterInBounds(ch, false))
            scrollViewport(src, 2);
    }

    // Erase old entity (presentation — caller's viewCallback handles this).
    // Update authoritative position and rebuild occupancy.
    _table.setPosition(idx, dst);
    _table.rebuildOccupancy();

    // Scroll viewport to destination.
    scrollViewport(dst, scrollRadius);

    // Movement invalidates these action states.
    cs.attackCount     = 0;
    cs.directionChange = 0;

    VmInterface::soundPlay(0x0B);

    // Cloud tile nausea check.
    {
        int dummyOccupant = 0;
        uint8 dstTile = 0;
        _context->getGroundInfo(ch, 0xFF, &dummyOccupant, &dstTile);
        if (dstTile == Combat::CloudEffectManager::kTileCloud)
            Goldbox::Poolrad::checkCloudEffect(*ch, nullptr, nullptr);
    }

    // Guard reactions (opportunity/counter attacks).
    _context->handleGuardReactions(ch, viewCallback);

    // Preserve remaining movement unless the character became disabled
    // or acquired a negative effect.
    if (!ch->enabled || ch->hasNegativeEffect())
        cs.movePoints = 0;

    return true;
}

} // namespace Combat
} // namespace Goldbox
