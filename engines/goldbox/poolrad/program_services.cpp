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

#include "goldbox/poolrad/program_services.h"

namespace Goldbox {
namespace Poolrad {

ProgramService resolveProgramService(uint8 programID) {
	switch (programID) {
	case kProgramTraining:
		return kServiceTraining;
	case kProgramWinGame:
		return kServiceWinGame;
	case kProgramCamp:
		return kServiceCamp;
	default:
		return kServiceUnknown;
	}
}

ProgramDispatchPlan planProgramDispatch(uint8 programID,
		const void *realSelection, const void *currentSelection) {
	ProgramDispatchPlan plan;
	plan.service = resolveProgramService(programID);
	// Restore the real selection before dispatch whenever a temporary
	// selection differs from it. Identity-only comparison: the real pointer is
	// authoritative, so a transient LOAD CHARACTER selection is rolled back
	// rather than left active (and NOT reset to the party front).
	plan.restoreSelectionBeforeDispatch = (currentSelection != realSelection);
	return plan;
}

DuelSettlementDecision settleDuelOutcome(bool won, bool mirror) {
	(void)won;
	(void)mirror;
	// A duel never routes through ordinary combat defeat. Regardless of win,
	// loss, mirror or nonmirror, the temporary roster is discarded and the
	// real party survives. A lost mirror duel is explicitly NOT a
	// total-party-defeat (no GS_END_GAME / settleCombatLoot-defeat path).
	DuelSettlementDecision d;
	d.realPartySurvives = true;
	d.totalPartyDefeat = false;
	d.discardTemporaryRoster = true;
	d.restoreRealSelection = true;
	return d;
}

ServiceWaitDecision decideServiceWait(const ServiceWaitSignals &signals) {
	ServiceWaitDecision d;
	if (signals.quitting) {
		// Quit short-circuits every wait: unwind cleanly, resume nothing.
		d.exitCleanly = true;
		return d;
	}
	if (signals.resourceMissing) {
		// Missing ending/duel resource: report truthfully. The caller must not
		// fabricate a view or touch the real party on this path.
		d.failTruthfully = true;
		return d;
	}
	if (signals.eclReplaced) {
		// A NEW ECL replaced the script during the wait: the saved PC belongs
		// to the obsolete script, so it is not resumed.
		return d;
	}
	// Normal completion: resume the saved instruction position (service EXIT).
	d.resumeSavedPc = true;
	return d;
}

} // namespace Poolrad
} // namespace Goldbox
