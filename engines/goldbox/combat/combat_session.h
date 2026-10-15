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
#include "goldbox/combat/combat_spell_targeter.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_placement.h"
#include "goldbox/combat/combat_viewport.h"
#include "goldbox/combat/battlefield_map.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}
namespace Combat {

struct AiMoveViewDelegate;
struct CombatViewDelegate;

/**
 * Owns all combat state and drives the round loop.
 *
 * CombatView holds a CombatSession and calls executeTurn() to advance
 * one actor's turn. The view only reads session state for rendering
 * and reacts to TurnResult events for animation and message display.
 */
class CombatSession {
public:
    enum Phase {
        PHASE_NONE = 0,
        PHASE_SETUP,
        PHASE_PLAYER_TURN,      // waiting to select next actor
        PHASE_AWAITING_PLAYER,  // party actor selected, waiting for player input
        PHASE_AI_TURN,
        PHASE_ROUND_END,        // all actors acted; waiting for DIALOG_CombatEnd
        PHASE_COMBAT_END,       // one side eliminated; waiting for DIALOG_CombatEnd
        PHASE_ENDED
    };

    /** Player action choices submitted via submitPlayerAction(). */
    enum PlayerAction {
        PA_ATTACK = 0,  // Aim: select target and attack
        PA_CAST,        // Cast spell
        PA_USE,         // Use item
        PA_MOVE,        // Move on battlefield
        PA_DONE,        // Done: end turn without acting
        PA_FLEE,        // Flee combat
        PA_TURN,        // Turn Undead (cleric)
        PA_VIEW,        // View: inspect battlefield (loops back to menu)
        PA_QUICK,       // Quick: auto-attack without targeting (current actor only)
        PA_QUICK_ALL,   // Party quickfight: initiative=20, quickfight all party members
        PA_SUBMENU,     // Other: open combat submenu (Guard/Delay/Quit/Bandage/Speed)
        PA_AUTOSPELL,   // Toggle autospell on/off (';' key)
        PA_PARTY_MANUAL,// Space: clear ai_control for all party members with npc < 0x80
        PA_NONE         // skip turn without menu (disabled / pre-cast spell)
    };

    /** What happened during one actor turn — view reacts to these. */
    struct TurnResult {
        enum Event {
            EV_NONE,
            EV_ACTOR_FOCUSED,   // Actor selected; view should scroll to actor and refresh status panel
            EV_AI_ATTACK,       // AI actor attacked; damage/target valid
            EV_ROUND_END,       // All actors acted; round counter incremented
            EV_COMBAT_END       // One side eliminated; session is PHASE_ENDED
        };

        Event                    event     = EV_NONE;
        Data::PlayerCharacter   *actor     = nullptr; // who acted
        Data::PlayerCharacter   *target    = nullptr; // who was hit (EV_AI_ATTACK)
        int                      damage    = 0;       // damage dealt (EV_AI_ATTACK)
        bool                     targetWentDown = false;
        uint8                    actorSize = 1;  // icon_dim of actor (EV_ACTOR_FOCUSED)
    };

    CombatSession();
    ~CombatSession();

    void setup(const CombatParams &params);

    /**
     * Set view delegates for AI and attack presentation callbacks.
     * Both may be nullptr to suppress all presentation (e.g. unit tests).
     * Must be called after setup() and before the first tick().
     */
    void setViewDelegates(AiMoveViewDelegate *aiDelegate,
                          CombatViewDelegate *attackDelegate) {
        _aiDelegate     = aiDelegate;
        _attackDelegate = attackDelegate;
    }

    /**
     * Mirrors COMBAT_ExecuteTurn.
     * Drives one actor's turn: applies turn-start effects, sets the acting
     * character, recalculates stats, runs ES15, then either dispatches to
     * moveByAI() or pauses at PHASE_AWAITING_PLAYER for player input.
     * Returns a TurnResult describing what happened.
     */
    TurnResult executeTurn();

    /**
     * Submit the player's chosen action for the current party actor.
     * Only valid when phase == PHASE_AWAITING_PLAYER.
     * Applies the action's own state change and advances to the next actor.
     * The actor's initiative is not forced: actions that consume the turn
     * call endTurn() themselves, and an actor with initiative left (Delay,
     * quickfight-all) is reselected like in COMBAT_MainLoop.
     */
    TurnResult submitPlayerAction(PlayerAction action);

    // -----------------------------------------------------------------------
    // Movement step API — called by CombatMoveDialog each step.
    // All data mutations live here; the dialog owns only interaction state.

    /** Outcome of one movement step. */
    struct MoveStepResult {
        enum Kind {
            MS_OK,
            MS_OCCUPIED,
            MS_OUT_OF_BOUNDS,
            MS_BLOCKED,
            MS_DISABLED
        };
        Kind kind = MS_OK;
        int  occupantIndex = 0;
        bool actionComplete = false;
        TilePos fromPos;   // position before the step (valid for MS_OK)
        TilePos toPos;     // position after the step  (valid for MS_OK)
    };

    /** Update character facing without moving. */
    void updateFacing(Data::PlayerCharacter *ch, uint8 direction);

    /** Query ground/occupant one step in direction from ch. */
    void queryGround(Data::PlayerCharacter *ch, uint8 direction,
                     int *outOccupant, uint8 *outTile) const;

    /** Get terrain passability cost for a tile id (0 = impassable/OOB). */
    uint8 getTilePassability(uint8 tileId) const;

    /**
     * Execute one movement step: advance-engage check, move, status check.
     * Returns MoveStepResult describing what happened.
     */
    MoveStepResult performMoveStep(Data::PlayerCharacter *ch, uint8 direction);

    /**
     * Mirrors COMBAT_ApplyMoveStep.
     * Attempts to move ch one tile in direction (0-7, odd = diagonal).
     * Handles movement cost, placement update, occupancy rebuild, viewport
     * scroll, action-state reset, step sound, guard reactions, and the
     * conditional move-budget preservation.
     *
     * viewCallback is invoked for viewport redraws (AI radius=3 path);
     * pass nullptr to skip presentation (e.g. unit tests).
     *
     * Returns false if ch had insufficient movement (no move occurred).
     */
    bool applyMoveStep(Data::PlayerCharacter *ch, uint8 direction,
                       void (*viewCallback)(Data::PlayerCharacter *) = nullptr);

    /** Restore character to saved position/facing (cancel path). */
    void cancelMove(Data::PlayerCharacter *ch,
                    uint8 origMovePoints, uint8 origDirection,
                    TilePos origPos);

    /**
     * Mirrors COMBAT_SetCharacterStatus (data layer).
     *
     * Removes ch from active combat:
     *   - Sets enabled=false, healthStatus=status.
     *   - Zeroes hp_current unless status==S_RUNNING.
     *   - Calls endTurn(); if the character was the current actor,
     *     also calls clearStatusEffects().
     *   - Zeroes icon_dim in the combatant table and rebuilds occupancy.
     *
     * Returns the combatant table index (for view redraw) and whether
     * effects were cleared (view may need to refresh status panel).
     *
     * Presentation (focus, message, tile redraw, gfx refresh) is the
     * caller's responsibility via SetStatusViewDelegate.
     */
    struct SetStatusResult {
        int  combatIndex;       // table index of ch (-1 if not found)
        bool effectsCleared;    // true if clearStatusEffects() was called
    };
    SetStatusResult setCharacterStatus(Data::PlayerCharacter *ch,
                                       uint8 status);

    /**
     * Attempt to flee combat for a party character.
     * Returns true if the character escaped (healthStatus set to S_RUNNING).
     * Always ends the character's turn.
     */
    bool flee(Data::PlayerCharacter *ch);

    /** Mark actor's move action finished (ends its turn) and advance. */
    TurnResult finishMoveAction(Data::PlayerCharacter *ch);

    /**
     * True when the actor's activation is over and the session should
     * advance (disabled, initiative <= 0, or a timed spell is pending).
     * When false, the view returns the actor to the combat menu, mirroring
     * DIALOG_CombatMain looping while its result is false.
     */
    bool isActorTurnComplete(const Data::PlayerCharacter *ch) const;

    /** True when the VM no-magic flag (G_NoMagicFlag) is set. */
    bool isMagicSuppressed() const;

    /** The party actor currently waiting for player input. nullptr if none. */
    Data::PlayerCharacter *getCurrentActor() const { return _currentActor; }

    Phase getPhase() const { return _phase; }
    bool isEnded() const { return _phase == PHASE_ENDED; }

    /**
     * Summary of the end-of-round processing (mirrors the out-parameters of
     * DIALOG_CombatEnd). Valid while phase == PHASE_ROUND_END.
     */
    struct RoundEndInfo {
        bool exitCombat     = false; // one side has no active members
        bool askContinue    = false; // party alive, enemies gone: "Continue Battle?"
        bool teammateDying  = false; // show "Your Teammate is Dying"
    };
    const RoundEndInfo &getRoundEndInfo() const { return _roundEnd; }

    /**
     * Acknowledge the end-of-round or end-of-combat interaction and
     * resume the encounter loop.  Only valid when phase is
     * PHASE_ROUND_END or PHASE_COMBAT_END.
     *
     * continueEncounter: true  → start the next round (PHASE_ROUND_END only)
     *                    false → end the encounter (transitions to PHASE_ENDED)
     */
    void acknowledgeRoundEnd(bool continueEncounter);

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

    /**
     * Scroll viewport to include target tile; rebuilds distance cache.
     * radius mirrors the guard zone in COMBAT_FocusCharacter/redrawViewport:
     * only scrolls if target is more than radius tiles from the viewport center.
     * Default 0xFF scrolls unconditionally.
     */
    void scrollViewport(TilePos target, uint8 radius = 0xFF);

    /**
     * Mirrors COMBAT_BuildFacingTargetList.
     * Builds the full reachable target list for attacker using maxRange=0x7f
     * and DIR_ANY (no facing restriction). Populates result with the entries.
     * Called by DIALOG_CombatAttack before entering the target-cycle loop.
     */
    void buildFacingTargetList(Data::PlayerCharacter *attacker, TargetList &result);

    /**
     * Mirrors COMBAT_ExecuteAttackOnTarget.
     *
     * Commits the selected target and, when immediateAttack=true, executes
     * the full attack sequence:
     *   1. COMBAT_ConfirmAttackNonHostile (hostile-target gate)
     *   2. COMBAT_ExecuteMultiAttack (sweep path)
     *   3. COMBAT_ApplyAttackFacingChange
     *   4. Ranged-item determination (range==1 clears item)
     *   5. COMBAT_ResolveAttack
     *
     * Returns true when the actor's turn is complete (attack resolved or
     * multi-attack ended the turn). Returns false when the target was
     * rejected (non-hostile gate failed).
     *
     * nonHostileConfirmed may only be set after an affirmative ally-attack
     * confirmation; it bypasses the repeated gate for that attack.
     *
     * Called by DIALOG_CombatAttack after the player confirms a target.
     */
    bool executeAttackOnTarget(Data::PlayerCharacter *attacker,
                               Data::PlayerCharacter *target,
                               bool immediateAttack,
                               bool nonHostileConfirmed = false);

    /**
     * Mirrors COMBAT_ConfirmAttackNonHostile (rule layer only — no UI).
     * Returns true when the attack may proceed without a player confirmation:
     *   - target is on the opposing combat side, OR
     *   - attacker is AI-controlled.
     * Returns false when a player confirmation is required (same-side attack
     * by a player-controlled character). The caller (DIALOG_CombatAttack) is
     * responsible for showing the "Attack Ally?" prompt and calling
     * makePartyHostile() if the player confirms, then passing
     * nonHostileConfirmed=true to executeAttackOnTarget().
     */
    bool confirmAttackNonHostile(Data::PlayerCharacter *attacker,
                                 Data::PlayerCharacter *target);

    /**
     * Mirrors the state-mutation half of COMBAT_ConfirmAttackNonHostile
     * (the YES branch). Called by DIALOG_CombatAttack after the player
     * confirms attacking a non-hostile target.
     *
     * Sets BYTE_COMBAT_ALLY_ATTACK / D_unknownCombatFlag1, converts all
     * qualifying party members (status==OKAY, npc>0x7f) to CS_ENEMY,
     * clears their combat targets, and recalculates side counts.
     */
    void makePartyHostile();

private:
    friend void processAiMove(Data::PlayerCharacter *,
                              const Common::Array<Data::PlayerCharacter *> &,
                              char, CombatContext &, AiMoveViewDelegate *,
                              CombatViewDelegate *);

    struct TryFleeResult {
        bool actionComplete;
        bool escaped;
    };

    /** Mirrors COMBAT_TrySetRunning. See TryFleeResult for semantics. */
    TryFleeResult trySetFleeing(Data::PlayerCharacter *ch);

    CombatParams    _params;
    CombatGlobals   _globals;
    BattlefieldMap  _battlefieldMap;
    CombatantTable  _table;
    CombatPlacement _placement;
    CombatViewport  _viewport;
    Phase           _phase;
    TargetList      _targetList;  // TARGET_LIST / TARGET_COUNT / ARRAY_TARGET_ORDER

    Common::ScopedPtr<CombatContext> _context;

    // Combat spell target selection; registered with the spell service for
    // the lifetime of this session.
    CombatSpellTargeter _spellTargeter;

    Data::PlayerCharacter *_currentActor;
    // True when setup() found a side with no members (original exit_flag
    // check before the first round).
    bool _exitAtRoundStart = false;
    RoundEndInfo _roundEnd;
    // Saved PTR_SELECTED_CHAR value across PHASE_AWAITING_PLAYER pause.
    // Mirrors the previousSelected local in COMBAT_ExecuteTurn.
    Data::PlayerCharacter *_previousAttacker = nullptr;

    AiMoveViewDelegate  *_aiDelegate     = nullptr;
    CombatViewDelegate  *_attackDelegate = nullptr;

    CombatContext makeContext();
    CombatContext makeContext() const;
    uint8 readAndClearAmbushFlags() const;

    /**
     * Mirrors DIALOG_CombatEnd (data part): advances the clock, bumps the
     * turn counter, refreshes hostile health, runs ES19 + cloud effects,
     * progresses dying characters, and recomputes side counts. Fills
     * _roundEnd for the view.
     */
    void processRoundEnd();

    /**
     * Turn-start effect processing (first half of COMBAT_ExecuteTurn,
     * before PTR_SELECTED_CHAR is set):
     *   - resets attackCount/directionChange/guarding
     *   - runs ES_POST_MOVEMENT_TILE (7)
     *   - first initiative gate (initiative > 0)
     *   - normalizes initiative 20 → 19
     * Returns false if the actor's turn is cancelled.
     * recalcCombatStats, ES_POISON_CYCLE (15), and the second initiative
     * gate run in executeTurn() after PTR_SELECTED_CHAR is set.
     */
    bool applyTurnStartEffects(Data::PlayerCharacter *ch);
};

/**
 * Set by CombatView while a combat encounter is on-screen; nullptr
 * otherwise. Nullable - only valid during an active combat session.
 */
extern CombatSession *g_combatSession;

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_SESSION_H
