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

#include "goldbox/poolrad/exploration_operation.h"

namespace Goldbox {
namespace Poolrad {

ExplorationOperation::ExplorationOperation()
	: _command(CMD_NONE),
	  _phase(PHASE_IDLE),
	  _suspended(false),
	  _barrier(BARRIER_OPEN),
	  _destX(0),
	  _destY(0),
	  _clamped(false),
	  _committed(false),
	  _tookNewEcl(false),
	  _doorHeld(false),
	  _savedSelectedId(0),
	  _restoredSelection(false),
	  _lookBegun(false) {}

void ExplorationOperation::startMove(Environment &env) {
	_command = CMD_MOVE;
	_phase = PHASE_PREPARE_INTENT;
	_suspended = false;
	_barrier = BARRIER_OPEN;
	_destX = 0;
	_destY = 0;
	_clamped = false;
	_committed = false;
	_tookNewEcl = false;
	_doorHeld = false;
	_restoredSelection = false;
	_lookBegun = false;

	// Save the real selection at the loop boundary (Req 28.4): we restore THIS
	// character afterwards instead of resetting to the party front. Keyed by a
	// stable id so a character removed mid-yield is not stale-restored.
	_savedSelectedId = env.captureSelectedId();
}

void ExplorationOperation::startLook(Environment &env) {
	_command = CMD_LOOK;
	_phase = PHASE_EXEC_ONSEARCH; // Look skips prepare/move; it is a search.
	_suspended = false;
	_barrier = BARRIER_OPEN;
	_destX = 0;
	_destY = 0;
	_clamped = false;
	_committed = false;
	_tookNewEcl = false;
	_doorHeld = false;
	_restoredSelection = false;
	_lookBegun = false;

	_savedSelectedId = env.captureSelectedId();
}

bool ExplorationOperation::advance(Environment &env) {
	_suspended = false;

	if (_command == CMD_LOOK)
		return runLook(env);

	// MOVE: drive the phases in order, stopping on a yield or completion.
	bool keepGoing = true;
	while (keepGoing) {
		switch (_phase) {
		case PHASE_PREPARE_INTENT:
			keepGoing = runPrepareIntent(env);
			break;
		case PHASE_EXEC_ONMOVE:
			keepGoing = runExecOnMove(env);
			break;
		case PHASE_RESOLVE:
			keepGoing = runResolve(env);
			break;
		case PHASE_REDRAW:
			keepGoing = runRedraw(env);
			break;
		case PHASE_EXEC_ONSEARCH:
			keepGoing = runExecOnSearch(env);
			break;
		case PHASE_RESTORE:
			keepGoing = runRestore(env);
			break;
		case PHASE_AWAIT_INPUT:
		case PHASE_DONE:
		case PHASE_IDLE:
		default:
			keepGoing = false;
			break;
		}
	}

	return _suspended;
}

// --- PREPARE_INTENT ---------------------------------------------------------
//
// Compute the prospective destination and edge/leave flag exactly as
// MoveForward does, WITHOUT committing x/y or advancing time. The barrier
// predicate and the clamped-edge / TriedToLeaveMap writes ARE part of the
// original preparation, so they happen here; only the destination commit and
// the clock charge are deferred to RESOLVE.
bool ExplorationOperation::runPrepareIntent(Environment &env) {
	// Sync facing direction to VM memory (original did this before stepping).
	env.syncDirection();

	// Save the pre-move position into SavedMapX/SavedMapY.
	env.saveCurrentPosition();

	// Resolve the barrier in the facing direction (wall / door / open).
	_barrier = env.facingBarrier();

	if (_barrier == BARRIER_WALL) {
		// Solid wall: no move, no door, no time. Preserve the original
		// "return without commit" behaviour; selection is still restored.
		_phase = PHASE_RESTORE;
		return true;
	}

	if (_barrier == BARRIER_DOOR) {
		// Locked door: DoorDialog / bash / pick / Knock. Hold pending movement
		// until the interaction finishes. If it opens, fall through to the
		// prospective destination; if not, no commit happens.
		_doorHeld = true;
		const bool opened = env.tryOpenDoor();
		if (!opened) {
			// Door stayed shut: no move, no time. Restore selection and stop.
			_phase = PHASE_RESTORE;
			return true;
		}
		// Door opened: movement may proceed through the now-open cell.
	}

	// Compute the prospective destination + clamp exactly as MoveForward.
	int x = 0, y = 0, dx = 0, dy = 0;
	env.currentCell(x, y);
	env.facingStep(dx, dy);
	x += dx;
	y += dy;

	_clamped = false;
	if (x > 15) { x = 15; _clamped = true; }
	if (x < 0)  { x = 0;  _clamped = true; }
	if (y > 15) { y = 15; _clamped = true; }
	if (y < 0)  { y = 0;  _clamped = true; }

	_destX = x;
	_destY = y;

	// Write the edge/leave flag now (part of preparation in the original).
	env.setTriedToLeaveMap(_clamped);

	// Read geo at the (prospective) position so ONMOVE can decide the event.
	// The original read geo before ONMOVE; we keep that ordering but do NOT
	// commit coordinates or charge time yet.
	env.readGeoAtPosition();

	_phase = PHASE_EXEC_ONMOVE;
	return true;
}

// --- EXEC_ONMOVE ------------------------------------------------------------
//
// Run ONMOVE BEFORE resolving/committing (Req 28.1). A yield suspends with no
// commit. If ONMOVE cancels / opens a menu / starts combat / fires NEW ECL,
// take the NEW-ECL transition path and do NOT commit the normal move.
bool ExplorationOperation::runExecOnMove(Environment &env) {
	const ScriptResult r = env.runOnMove();

	if (r == SCRIPT_YIELD) {
		// Suspend at the ONMOVE boundary; no coordinate/time commit occurred.
		_suspended = true;
		return false;
	}

	if (r == SCRIPT_NEW_ECL) {
		// NEW ECL replaces the normal move path (Req 28.2). No premature
		// coordinate/time commit; drive the ENGINE_Run transition loop.
		_tookNewEcl = true;
		const ScriptResult t = env.runNewEclTransition();
		if (t == SCRIPT_YIELD) {
			_suspended = true;
			return false;
		}
		// Transition drained: restore selection/search context, then await.
		_phase = PHASE_RESTORE;
		return true;
	}

	// ONMOVE completed normally: proceed to commit.
	_phase = PHASE_RESOLVE;
	return true;
}

// --- RESOLVE ----------------------------------------------------------------
//
// Only now commit movement (write x/y) and advance time ONCE. One minute for
// an ordinary dungeon move, one turn for a search move. Time is charged on this
// single path, never twice across UI and host.
bool ExplorationOperation::runResolve(Environment &env) {
	// Commit the resolved destination computed in PREPARE_INTENT. Edge wrap
	// without a map change still writes the clamped destination (same cell at
	// the border) - the leave flag was already set in preparation and no
	// map-change transition is involved here.
	env.commitPosition(_destX, _destY);
	_committed = true;

	// Advance the clock exactly once based on search mode.
	if (env.isSearchMoveActive())
		env.chargeOneTurn();   // search move = one turn (field=2, amount=1)
	else
		env.chargeOneMinute(); // ordinary move = one minute (field=1, amount=1)

	_phase = PHASE_REDRAW;
	return true;
}

// --- REDRAW -----------------------------------------------------------------
//
// Preserve the original post-move redraw/sound/picture writes. The legacy path
// re-checked the door after ONMOVE and played the bump sound when the position
// changed; that host sub-flow is driven by the Environment (applyPostMoveRedraw
// owns the bool3dRedraw/boolPictureReady writes). The bump sound is played by
// the host's door re-check, kept verbatim in the host Environment.
bool ExplorationOperation::runRedraw(Environment &env) {
	env.applyPostMoveRedraw();
	_phase = PHASE_EXEC_ONSEARCH;
	return true;
}

// --- EXEC_ONSEARCH ----------------------------------------------------------
//
// Always run ONSEARCH after movement (unconditional in the original). Refresh
// geo first so the script can inspect the square. A NEW ECL fired during
// ONSEARCH drives the same ENGINE_Run transition loop.
bool ExplorationOperation::runExecOnSearch(Environment &env) {
	env.readGeoAtPosition();

	const ScriptResult r = env.runOnSearch();

	if (r == SCRIPT_YIELD) {
		_suspended = true;
		return false;
	}

	if (r == SCRIPT_NEW_ECL) {
		_tookNewEcl = true;
		const ScriptResult t = env.runNewEclTransition();
		if (t == SCRIPT_YIELD) {
			_suspended = true;
			return false;
		}
	}

	_phase = PHASE_RESTORE;
	return true;
}

// --- RESTORE ----------------------------------------------------------------
//
// Restore the saved selected character ONLY after yielded scripts/transitions
// complete, keyed by stable id so a removed character is not stale-restored
// (Req 28.4). Temporary search bits survive yields / NEW ECL because the
// restore is keyed by the stable id rather than a raw pointer captured earlier.
bool ExplorationOperation::runRestore(Environment &env) {
	if (_savedSelectedId != 0)
		_restoredSelection = env.restoreSelectedId(_savedSelectedId);
	else
		_restoredSelection = false;

	_phase = PHASE_AWAIT_INPUT;
	return true;
}

// --- LOOK -------------------------------------------------------------------
//
// One-shot Look search. Advances ONE TURN (field=2, amount=1) once - fixing
// the two-minute defect - sets the Look search flag, runs ONSEARCH once, then
// restores flags and the selection. Resumable on yield.
bool ExplorationOperation::runLook(Environment &env) {
	if (!_lookBegun) {
		// Enter Look search mode (flags |= 2 then = 1 for the pass).
		env.beginLookSearch();
		_lookBegun = true;

		// ONE TURN (field=2, amount=1), charged exactly once. The original
		// charged two minutes here (field=1, amount=2) - DEFECT B - corrected
		// to match the search-move one-turn form in the same dispatcher.
		env.chargeOneTurn();

		// Refresh MapSquareInfo so ONSEARCH can check the square.
		env.readGeoAtPosition();
	}

	const ScriptResult r = env.runOnSearch();

	if (r == SCRIPT_YIELD) {
		// Suspend; flags are restored only after the script resolves, so we do
		// NOT call endLookSearch() yet - resume re-enters here and, because
		// _lookBegun stays true, re-runs ONSEARCH from the resumed PC.
		_suspended = true;
		return true;
	}

	// Restore the search flags saved by beginLookSearch().
	env.endLookSearch();

	if (r == SCRIPT_NEW_ECL) {
		_tookNewEcl = true;
		const ScriptResult t = env.runNewEclTransition();
		if (t == SCRIPT_YIELD) {
			_suspended = true;
			return true;
		}
	}

	// Restore the saved selection (stable id), then finish.
	if (_savedSelectedId != 0)
		_restoredSelection = env.restoreSelectedId(_savedSelectedId);

	_phase = PHASE_DONE;
	return false;
}

} // namespace Poolrad
} // namespace Goldbox
