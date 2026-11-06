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

#ifndef GOLDBOX_POOLRAD_PROGRAM_SERVICES_H
#define GOLDBOX_POOLRAD_PROGRAM_SERVICES_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Poolrad {

/**
 * GB13-007 — PROGRAM services, duels and victory flow (Requirement 31).
 *
 * This header holds the PURE, headless decision/lifecycle logic that the
 * host-owned PROGRAM dispatch and the CharFightsAlone duel path depend on.
 * None of it touches g_engine, the ECL VM, views or assets, so the service
 * routing, the selection-restore ordering, the duel settlement decision and
 * the quit / NEW-ECL / missing-resource lifecycle guards are all unit
 * assertable without a live engine.
 *
 * The live wiring (runRest / camp ONREST->ONRESTINTR, the WinGame ending
 * presentation, the CharFightsAlone temporary roster + mirror stat copy,
 * saveCallerLocation/restoreCallerLocation) stays in
 * PoolradEngineHostImpl / PoolradEngine; this module only decides WHAT the
 * live code should do at each seam, never HOW the UI renders it.
 */

/** The PROGRAM ids the DOS PROGRAM opcode (0x38) dispatches by evaluated id. */
enum ProgramId : uint8 {
	kProgramTraining = 0, // main-menu / roster / training workflow
	kProgramWinGame  = 8, // Ending.WinGame victory presentation
	kProgramCamp     = 9  // EnterCamp: ONREST -> camp -> optional ONRESTINTR
};

/** Which host service a PROGRAM id resolves to (unknown ids are an error). */
enum ProgramService : uint8 {
	kServiceUnknown = 0,
	kServiceTraining,
	kServiceWinGame,
	kServiceCamp
};

/** Resolve a PROGRAM id to its host service. Only 0/8/9 are defined. */
ProgramService resolveProgramService(uint8 programID);

/**
 * PROGRAM dispatch pre-step decision.
 *
 * Every PROGRAM, before dispatching the service, must restore any temporary
 * LOAD CHARACTER selection back to the real selected character (so a program
 * does not run against a transiently-selected disk character, and selection
 * is NOT silently reset to the party front). This struct is the pure plan:
 * the host snapshots the real selection, runs the program, and uses the plan
 * to decide whether a restore is required before dispatch.
 *
 * OPEN: the exact SOURCE of the temporary LOAD CHARACTER selection (which VM
 * field / host slot holds the pre-program "real" selection) is trace-gated
 * (GB13-006/GB13-003); the host passes the real pointer it captured, so this
 * decision is independent of that source.
 */
struct ProgramDispatchPlan {
	ProgramService service;
	// True when a temporary selection differs from the real selection and
	// must be restored before the service runs.
	bool restoreSelectionBeforeDispatch;

	ProgramDispatchPlan()
		: service(kServiceUnknown),
		  restoreSelectionBeforeDispatch(false) {}
};

/**
 * Build the dispatch plan for a PROGRAM.
 *
 * @param programID         evaluated PROGRAM id (already dereferenced by 0x38)
 * @param realSelection     the real selected-character pointer (opaque; only
 *                          compared for identity, never dereferenced here)
 * @param currentSelection  the selection active right now (possibly a
 *                          temporary LOAD CHARACTER selection)
 */
ProgramDispatchPlan planProgramDispatch(uint8 programID,
		const void *realSelection, const void *currentSelection);

/**
 * Duel settlement decision (CALL 0x8000 mirror / 0x8001 nonmirror).
 *
 * A duel is NOT ordinary combat: a lost mirror duel must never become
 * total-party-defeat. Win or lose, the temporary duel roster is discarded and
 * the REAL party survives. This decision is deliberately independent of
 * settleCombatLoot's defeat path (which drives GS_END_GAME) — the host routes
 * a duel outcome through here instead.
 */
struct DuelSettlementDecision {
	bool realPartySurvives;     // always true for a duel
	bool totalPartyDefeat;      // always false for a duel (even a lost mirror)
	bool discardTemporaryRoster; // always true: temp combat data is gone
	bool restoreRealSelection;  // real selection is restored after the duel

	DuelSettlementDecision()
		: realPartySurvives(true), totalPartyDefeat(false),
		  discardTemporaryRoster(true), restoreRealSelection(true) {}
};

/**
 * Decide the settlement of a finished duel.
 *
 * @param won     true when the fighting character won the duel
 * @param mirror  true for 0x8000 (mirror self-duel), false for 0x8001
 */
DuelSettlementDecision settleDuelOutcome(bool won, bool mirror);

/**
 * Lifecycle guard for a service wait (camp / ending / duel).
 *
 * At every wait/return seam the host must decide, from the live lifecycle
 * signals, whether it may resume the obsolete saved instruction position or
 * must abandon it. The three signals mirror the existing exploration/camp
 * yield-resume contract:
 *   - quitting      : application quit (shouldQuit) — exit cleanly, no resume.
 *   - eclReplaced   : a NEW ECL replaced the script during the wait — the
 *                     saved PC belongs to the obsolete script, do NOT resume.
 *   - resourceMissing: a required resource (ending asset / duel data) is
 *                     absent — fail truthfully, do NOT touch the real party.
 */
struct ServiceWaitSignals {
	bool quitting;
	bool eclReplaced;
	bool resourceMissing;

	ServiceWaitSignals()
		: quitting(false), eclReplaced(false), resourceMissing(false) {}
};

struct ServiceWaitDecision {
	bool resumeSavedPc;   // resume the saved instruction position after EXIT
	bool failTruthfully;  // report a missing-resource failure (no fabrication)
	bool exitCleanly;     // unwind the wait without resuming (quit)

	ServiceWaitDecision()
		: resumeSavedPc(false), failTruthfully(false), exitCleanly(false) {}
};

/**
 * Decide how a service wait concludes.
 *
 * Precedence: a quit short-circuits everything (clean exit, no resume); then a
 * missing resource fails truthfully; then a NEW ECL suppresses the obsolete
 * resume; otherwise the saved PC is resumed. The real party is never deleted
 * on any of these paths — that invariant lives in the caller, but this
 * decision never asks for a party deletion.
 */
ServiceWaitDecision decideServiceWait(const ServiceWaitSignals &signals);

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_PROGRAM_SERVICES_H
