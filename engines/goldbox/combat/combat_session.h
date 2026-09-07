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

#ifndef GOLDBOX_COMBAT_COMBAT_SESSION_H
#define GOLDBOX_COMBAT_COMBAT_SESSION_H

#include "common/scummsys.h"
#include "common/ptr.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_placement.h"
#include "goldbox/combat/combat_viewport.h"
#include "goldbox/combat/battlefield_map.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}
namespace Combat {

/**
 * Owns all combat state and drives the round loop.
 *
 * CombatView holds a CombatSession and calls tick() each frame.
 * The view only reads session state for rendering and reacts to
 * TickResult events for animation and message display.
 */
class CombatSession {
public:
    enum Phase {
        PHASE_NONE = 0,
        PHASE_SETUP,
        PHASE_PLAYER_TURN,      // waiting to select next actor
        PHASE_AWAITING_PLAYER,  // party actor selected, waiting for player input
        PHASE_AI_TURN,
        PHASE_ENDED
    };

    /** Player action choices submitted via submitPlayerAction(). */
    enum PlayerAction {
        PA_ATTACK = 0,
        PA_CAST,
        PA_USE,
        PA_MOVE,
        PA_GUARD,
        PA_FLEE
    };

    /** What happened during one tick — view reacts to these. */
    struct TickResult {
        enum Event {
            EV_NONE,
            EV_ACTOR_FOCUSED,  // Actor selected; view should scroll to actor
            EV_AI_ATTACK,      // AI actor attacked; damage/target valid
            EV_ROUND_END,      // All actors acted; round counter incremented
            EV_COMBAT_END      // One side eliminated; session is PHASE_ENDED
        };

        Event                    event  = EV_NONE;
        Data::PlayerCharacter   *actor  = nullptr; // who acted
        Data::PlayerCharacter   *target = nullptr; // who was hit (EV_AI_ATTACK)
        int                      damage = 0;       // damage dealt (EV_AI_ATTACK)
        bool                     targetWentDown = false;
    };

    CombatSession();
    ~CombatSession();

    void setup(const CombatParams &params);

    /**
     * Advance combat by one actor turn.
     * Returns a TickResult describing what happened so the view can
     * play animations and show messages without containing any logic.
     */
    TickResult tick();

    /**
     * Submit the player's chosen action for the current party actor.
     * Only valid when phase == PHASE_AWAITING_PLAYER.
     * Executes the action, marks the actor as done, advances to next actor.
     */
    TickResult submitPlayerAction(PlayerAction action);

    /** The party actor currently waiting for player input. nullptr if none. */
    Data::PlayerCharacter *getCurrentActor() const { return _currentActor; }

    Phase getPhase() const { return _phase; }
    bool isEnded() const { return _phase == PHASE_ENDED; }

    // --- Read-only accessors for rendering ---
    const CombatantTable  &getTable()          const { return _table; }
    CombatViewport        &getViewport()             { return _viewport; }
    const BattlefieldMap  &getBattlefieldMap() const { return _battlefieldMap; }
    BattlefieldMap        &getBattlefieldMap()       { return _battlefieldMap; }
    const CombatGlobals   &getGlobals()        const { return _globals; }
    const CombatParams    &getParams()         const { return _params; }

    /**
     * Non-owning reference bundle valid for the lifetime of the active
     * combat session (built in setup(), rebuilt on the next setup()).
     * nullptr before the first setup() call.
     */
    CombatContext *getContext() { return _context.get(); }
    const CombatContext *getContext() const { return _context.get(); }

    /** Scroll viewport to include target tile; rebuilds distance cache. */
    void scrollViewport(TilePos target);

private:
    CombatParams    _params;
    CombatGlobals   _globals;
    BattlefieldMap  _battlefieldMap;
    CombatantTable  _table;
    CombatPlacement _placement;
    CombatViewport  _viewport;
    Phase           _phase;

    Common::ScopedPtr<CombatContext> _context;

    Data::PlayerCharacter *_currentActor;

    CombatContext makeContext();
    uint8 readAndClearAmbushFlags() const;

    /**
     * Per-actor setup mirroring COMBAT_ExecuteTurn's pre-dispatch block:
     * resets moveBudget/directionChange/guarding, clamps initiative==20 to 19,
     * runs ES_POST_MOVEMENT_TILE (7) and ES_POISON_CYCLE (15).
     * Returns false if the actor's turn was cancelled by an effect.
     */
    bool prepareTurn(Data::PlayerCharacter *ch);
};

/**
 * Set by CombatView while a combat encounter is on-screen; nullptr
 * otherwise. Nullable - only valid during an active combat session.
 */
extern CombatSession *g_combatSession;

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_SESSION_H
