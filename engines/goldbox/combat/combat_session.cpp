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
#include "goldbox/combat/combat_turn.h"
#include "goldbox/combat/combat_ai.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/data/player_character.h"
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

        initAllTurnStates(_params.roster, nullptr, &_globals,
                           _params.eclMemory, _params.vmGlobalLayout);
        // Clear D_CombatIsAmbush after all initiatives are rolled.
        if (_params.eclMemory && _params.vmGlobalLayout) {
            const VmFieldLocation field =
                _params.vmGlobalLayout->field(kVmGlobalFieldCombatIsAmbush);
            if (VmLayout::isValid(field))
                _params.eclMemory->write8(field.vmAddr, 0);
        }
        _currentActor = selectNextActor(_params.roster, _globals);
    }

    if (_currentActor == nullptr)
        return result;

    result.actor = _currentActor;

    // Per-actor setup (mirrors COMBAT_ExecuteTurn pre-dispatch block).
    // Returns false if an effect cancelled the turn.
    if (!prepareTurn(_currentActor)) {
        _currentActor = selectNextActor(_params.roster, _globals);
        if (_currentActor == nullptr) {
            _globals.turnCounter++;
            updateHostileHealthPercent(_table, _globals);
            _phase = PHASE_PLAYER_TURN;
            result.event = TickResult::EV_ROUND_END;
        }
        return result;
    }

    result.event = TickResult::EV_ACTOR_FOCUSED;

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
    _currentActor = selectNextActor(_params.roster, _globals);

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
    _currentActor = selectNextActor(_params.roster, _globals);

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

void CombatSession::scrollViewport(TilePos target) {
    _viewport.adjustToInclude(target);
    _table.setViewportOrigin(_viewport.getTopLeft());
}

bool CombatSession::prepareTurn(Data::PlayerCharacter *ch) {
    if (!ch || !ch->combatState)
        return false;

    Data::CombatAction &cs = *ch->combatState;

    // Mirrors COMBAT_ExecuteTurn: reset per-turn fields before dispatch.
    cs.attackCount    = 0;
    cs.directionChange = 0;
    cs.guarding       = false;

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

} // namespace Combat
} // namespace Goldbox
