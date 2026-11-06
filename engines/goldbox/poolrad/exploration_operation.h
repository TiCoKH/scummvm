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
 * Assisted-by: Claude:claude-opus-4.6
 */

#ifndef GOLDBOX_POOLRAD_EXPLORATION_OPERATION_H
#define GOLDBOX_POOLRAD_EXPLORATION_OPERATION_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Poolrad {

/**
 * Resumable exploration command operation (GB13-022, Requirement 28).
 *
 * Replaces the monolithic movement mutation inside dispatchPlayerCommand
 * (poolrad.cpp :1380/:1434/:1488) with an EXPLICIT, resumable phase machine
 * that owns the intent and the saved context outside the dispatcher. The
 * dispatcher stays a thin driver; the phase ORDER, TIMING and
 * selection-preservation live here so they can be unit tested without
 * g_engine.
 *
 * The original committed coordinates AND time BEFORE running ONMOVE, reset the
 * selected character to the party front on every command, and charged Look as
 * two minutes. This operation fixes all three while preserving the original
 * observable event order (CONFIRMED-MISMATCH P0 refactor):
 *
 *   PREPARE_INTENT -> EXEC_ONMOVE -> RESOLVE -> REDRAW -> EXEC_ONSEARCH
 *                  -> RESTORE -> AWAIT_INPUT
 *
 *   PREPARE_INTENT  compute the prospective destination + edge/leave flag
 *                   exactly as MoveForward does (barrier predicate, clamp,
 *                   TriedToLeaveMap write) WITHOUT committing x/y or time. The
 *                   clamped-edge and leave-flag writes ARE part of preparation
 *                   in the original, so this is not a blanket side-effect-free
 *                   prepare.
 *   EXEC_ONMOVE     run ONMOVE BEFORE resolving/committing. A VM yield
 *                   suspends (resumable, no commit). If ONMOVE cancels, opens a
 *                   menu, starts combat or fires NEW ECL (eclReady), take the
 *                   NEW-ECL transition path (ENGINE_Run ONINIT->ONMOVE->
 *                   ONSEARCH, forced indoor) and do NOT commit the normal
 *                   move's coordinates/time.
 *   RESOLVE         only now commit movement (write x/y) and advance time
 *                   ONCE - one minute (field=1,amount=1) for an ordinary
 *                   dungeon move, one turn (field=2,amount=1) for a search
 *                   move. Door/bash/pick/Knock reuse tryOpenDoor/DoorDialog,
 *                   holding pending movement until it finishes. Time is never
 *                   charged twice across UI and host paths.
 *   REDRAW/ONSEARCH preserve the original redraw/sound/picture writes and the
 *                   unconditional post-move ONSEARCH (including its NEW-ECL
 *                   loop).
 *   RESTORE         restore the SAVED selected character and temporary search
 *                   mode only AFTER yielded scripts/transitions complete, keyed
 *                   by STABLE ID so a character removed mid-yield is not
 *                   stale-restored.
 *
 * Look (kCmdLook) is modelled as a one-shot search that advances ONE TURN
 * (field=2, amount=1) once - fixing the two-minute defect - runs ONSEARCH once
 * with the Look flag and restores flags; resumable on yield.
 *
 * All engine-coupled side effects (VM clock, ECL entry points, geo read, door
 * interaction, sound, position write, wall query, selection resolution) are
 * injected through Environment. The host wiring supplies the real
 * implementation; tests supply recording fakes.
 */
class ExplorationOperation {
public:
	/** Which exploration command the operation is driving. */
	enum Command {
		CMD_NONE = 0,
		CMD_MOVE = 1, // ordinary / search dungeon step
		CMD_LOOK = 2  // one-shot Look search
	};

	/** The resumable phases, in the design order. */
	enum Phase {
		PHASE_IDLE = 0,
		PHASE_PREPARE_INTENT,
		PHASE_EXEC_ONMOVE,
		PHASE_RESOLVE,
		PHASE_REDRAW,
		PHASE_EXEC_ONSEARCH,
		PHASE_RESTORE,
		PHASE_AWAIT_INPUT,
		PHASE_DONE
	};

	/**
	 * Result of running ONMOVE (or an ECL sub-step). Mirrors the engine's
	 * VmResult meaning without pulling the ECL enum into the pure operation:
	 *  - CONTINUE: script completed normally, carry on to the next phase.
	 *  - YIELD:    script suspended (VM_YIELD); operation stays resumable.
	 *  - NEW_ECL:  NEW ECL fired (eclReady); take the NEW-ECL transition path
	 *              instead of the normal move commit.
	 */
	enum ScriptResult {
		SCRIPT_CONTINUE = 0,
		SCRIPT_YIELD = 1,
		SCRIPT_NEW_ECL = 2
	};

	/** The resolved cell barrier in the facing direction. */
	enum Barrier {
		BARRIER_OPEN = 0,  // passable (original wall flag 1)
		BARRIER_WALL = 1,  // solid wall (original wall flag 0)
		BARRIER_DOOR = 2   // locked door (original wall flag 2/3)
	};

	/**
	 * Injected engine side effects. Minimal so the operation stays pure.
	 * Every method mirrors one host call the original made, in the original
	 * order; the host implementation forwards to the real engine, tests record.
	 */
	class Environment {
	public:
		virtual ~Environment() {}

		// --- PREPARE_INTENT ------------------------------------------------

		// Barrier in the facing direction at the current cell
		// (RuntimeGeoBlock::getWallFlag mapped to Barrier).
		virtual Barrier facingBarrier() = 0;

		// Open/bash/pick/Knock the facing door (tryOpenDoor / DoorDialog).
		// Returns true if the door opened (movement may then proceed).
		virtual bool tryOpenDoor() = 0;

		// Save the current position into SavedMapX/SavedMapY (pre-move).
		virtual void saveCurrentPosition() = 0;

		// Write the prospective destination's TriedToLeaveMap edge/leave flag
		// (0 = did not clamp, 1 = clamped at a border). Part of preparation in
		// the original - the leave flag is written whether or not we commit.
		virtual void setTriedToLeaveMap(bool clamped) = 0;

		// Sync the facing direction to VM memory before ONMOVE.
		virtual void syncDirection() = 0;

		// The current cell (x,y) and the per-axis step for the facing
		// direction, so the operation can compute the clamped destination
		// exactly as MoveForward does WITHOUT touching VM types.
		virtual void currentCell(int &x, int &y) = 0;
		virtual void facingStep(int &dx, int &dy) = 0;

		// --- EXEC_ONMOVE ---------------------------------------------------

		// Run the ONMOVE ECL entry point (BEFORE any commit).
		virtual ScriptResult runOnMove() = 0;

		// --- RESOLVE -------------------------------------------------------

		// Commit the resolved destination to VM memory (write DungeonX/Y) and
		// sync it back to the in-game view. Called ONLY on the normal path.
		virtual void commitPosition(int x, int y) = 0;

		// Refresh MapSquareInfo at the current cell (readGeoAtPosition /
		// MAP_getGEOData) so the ONSEARCH/ONMOVE script can inspect it.
		virtual void readGeoAtPosition() = 0;

		// True while a search mode is active (search flag bit 0). Decides the
		// one-minute vs one-turn charge for an ordinary move.
		virtual bool isSearchMoveActive() = 0;

		// Advance the clock by one minute (field=1, amount=1) for an ordinary
		// dungeon move.
		virtual void chargeOneMinute() = 0;

		// Advance the clock by one turn (field=2, amount=1) for a search move
		// and for Look.
		virtual void chargeOneTurn() = 0;

		// Play the "blocked"/bump sound (PlaySound 0x0B).
		virtual void playBumpSound() = 0;

		// --- REDRAW / EXEC_ONSEARCH ----------------------------------------

		// Preserve the original 3D-redraw / picture-ready writes
		// (bool3dRedraw = false, boolPictureReady = true).
		virtual void applyPostMoveRedraw() = 0;

		// Run the ONSEARCH ECL entry point.
		virtual ScriptResult runOnSearch() = 0;

		// Drive the NEW-ECL transition loop (ENGINE_Run: force indoor, then
		// ONINIT -> ONMOVE -> ONSEARCH until no more NEW ECL fires). Returns
		// the final script result (CONTINUE when the loop drains, YIELD if a
		// sub-step suspended).
		virtual ScriptResult runNewEclTransition() = 0;

		// --- Look-specific search flag handling ----------------------------

		// Enter the Look search mode (search flags |= 2, then = 1 for the
		// ONSEARCH pass) and remember the prior persistent-search bit.
		virtual void beginLookSearch() = 0;
		// Restore the search flags saved by beginLookSearch().
		virtual void endLookSearch() = 0;

		// --- RESTORE (selection / search context, stable id) ----------------

		// Capture the currently selected character's STABLE ID at the loop
		// boundary. 0 means "no selection to preserve".
		virtual uint32 captureSelectedId() = 0;

		// Restore the selection to the character with this stable id, but ONLY
		// if that character still exists. Returns true if restored; false if
		// the id is stale (character removed mid-yield) - in which case no
		// stale restore happens.
		virtual bool restoreSelectedId(uint32 stableId) = 0;
	};

	ExplorationOperation();

	/**
	 * Begin a MOVE command. Captures the selected character's stable id at the
	 * loop boundary (for RESTORE) and arms PREPARE_INTENT. Does NOT run any
	 * phase yet; call advance() to drive it.
	 */
	void startMove(Environment &env);

	/**
	 * Begin a LOOK command. Captures the selected character's stable id and
	 * arms the Look search path.
	 */
	void startLook(Environment &env);

	/**
	 * Drive the phase machine forward until it either completes or suspends on
	 * a yield. Returns true if the operation suspended (resumable); false if it
	 * finished. Re-call after a yield resolves to resume from the saved phase.
	 */
	bool advance(Environment &env);

	// Introspection for the driver and tests.
	Command command() const { return _command; }
	Phase phase() const { return _phase; }
	bool isSuspended() const { return _suspended; }
	bool isFinished() const { return _phase == PHASE_DONE; }

	// Whether the normal move committed coordinates/time (false when ONMOVE
	// cancelled or a NEW-ECL transition replaced the normal path).
	bool didCommit() const { return _committed; }
	// Whether the NEW-ECL transition path ran instead of the normal commit.
	bool tookNewEclPath() const { return _tookNewEcl; }
	// Resolved barrier from PREPARE_INTENT.
	Barrier barrier() const { return _barrier; }
	// The prospective destination computed in PREPARE_INTENT.
	int destX() const { return _destX; }
	int destY() const { return _destY; }
	// Whether the destination clamped at a border (edge wrap / leave flag).
	bool clampedAtEdge() const { return _clamped; }
	// The stable id captured at the loop boundary.
	uint32 savedSelectedId() const { return _savedSelectedId; }
	// Whether RESTORE actually restored the selection (false if stale id).
	bool restoredSelection() const { return _restoredSelection; }

private:
	// Phase handlers. Each returns true to keep advancing in this call, false
	// to stop (either suspended on a yield, or finished).
	bool runPrepareIntent(Environment &env);
	bool runExecOnMove(Environment &env);
	bool runResolve(Environment &env);
	bool runRedraw(Environment &env);
	bool runExecOnSearch(Environment &env);
	bool runRestore(Environment &env);

	// Look variant uses a shortened path driven from advance().
	bool runLook(Environment &env);

	Command _command;
	Phase _phase;
	bool _suspended;

	// PREPARE_INTENT results.
	Barrier _barrier;
	int _destX;
	int _destY;
	bool _clamped;

	// Flow bookkeeping.
	bool _committed;    // normal move committed coordinates + time
	bool _tookNewEcl;   // NEW-ECL transition replaced the normal path
	bool _doorHeld;     // a door interaction is holding pending movement

	// Selection / search context.
	uint32 _savedSelectedId;
	bool _restoredSelection;

	// Look sub-state.
	bool _lookBegun;
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_EXPLORATION_OPERATION_H
