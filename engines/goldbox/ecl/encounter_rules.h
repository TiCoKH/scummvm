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

#ifndef GOLDBOX_ECL_ENCOUNTER_RULES_H
#define GOLDBOX_ECL_ENCOUNTER_RULES_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Goldbox {
namespace ECL {

// GB13-002: pure, testable core of the encounter decision loop (EclEncounter,
// eclvm.pas:1057), the two surprise rolls (EclPartySurprise eclvm.pas:784 /
// EclSurprise eclvm.pas:814) and the party movement extremes (MoveExtremes,
// eclmenu.pas:992).
//
// None of these functions touch g_engine, the RNG, the party model or VM
// memory directly: every randomness or party input is injected. That keeps the
// response-table combinations, the exact two-d6 draw order and the ranger
// predicate unit-provable without a live engine. The live opcode handlers wire
// the core to getOpcodeRandom()/VmInterface::rollDice, VmInterface::getParty
// (ranger scan + movement) and the destination writes.

// ---------------------------------------------------------------------------
// Encounter decision transition (EclEncounter)
// ---------------------------------------------------------------------------

/**
 * Immutable per-encounter inputs, copied by value from the decoded operands.
 *
 * The five response-table values map menu selections onto a response code; the
 * two flee thresholds gate the party-flee / monster-flee outcomes against the
 * party movement extremes. The three message strings drive the fallback
 * rotation only (presentation); they are not required for the outcome.
 */
struct EncounterConfig {
	uint8 resp[5];   // operands 5..9: response code per menu selection 0..4
	uint8 fleePar;   // operand 13: party-flee movement threshold
	uint8 fleeMon;   // operand 14: monster-flee movement threshold
	uint8 hiMove;    // MoveExtremes max party movement
	uint8 loMove;    // MoveExtremes min party movement

	EncounterConfig() : fleePar(0), fleeMon(0), hiMove(0), loMove(0) {
		for (int i = 0; i < 5; ++i)
			resp[i] = 0;
	}
};

/**
 * Mutable encounter state that survives a reopen (WAIT/ADVANCE) across a yield.
 *
 * @c monsterDist is the live monster distance (decremented by ADVANCE/PARLAY).
 * @c view3dFlag mirrors the indoor-render / 3D-view byte (kVmFieldIndoorModeFlag):
 * zero means the outdoor view is active, which forces the PARLAY menu variant.
 */
struct EncounterState {
	uint8 monsterDist;
	uint8 view3dFlag;

	EncounterState() : monsterDist(0), view3dFlag(1) {}
};

/** Terminal outcome codes written to the destination (EclEncounter). */
enum EncounterOutcomeCode : uint8 {
	kEncounterMonstersFlee = 0,
	kEncounterCombat       = 1,
	kEncounterPartyFlees   = 2,
	kEncounterParley       = 3
};

/**
 * Result of one iteration of the decision loop for a reported menu selection.
 *
 * When @c terminal is true the loop ends: @c outcome is the code to write to
 * the destination address, and the host clears the encounter/message flags and
 * resumes exactly once (GB13-009 seam). When @c terminal is false the loop
 * reopens the prompt without any VM write (WAIT/ADVANCE): @c monsterDist and
 * @c redisplay reflect the state the host should re-present.
 */
struct EncounterStep {
	bool  terminal;    // true => write outcome and finish
	uint8 outcome;     // valid only when terminal
	uint8 monsterDist; // post-step monster distance (ADVANCE/PARLAY decrement)
	bool  redisplay;   // non-terminal: monster stage needs a redraw (dist changed)

	EncounterStep()
		: terminal(false), outcome(kEncounterCombat), monsterDist(0),
		  redisplay(false) {}
};

/**
 * True when the current state forces the PARLAY menu variant rather than
 * ADVANCE: adjacent monsters (distance 0) or the outdoor view (view3dFlag 0).
 * In that case a menu selection of 3 (fourth hotkey) is remapped to selection
 * 4 (PARLAY) before the response table is consulted.
 */
bool encounterUsesParleyMenu(const EncounterState &state);

/**
 * Resolve one decision-loop iteration for a raw menu selection.
 *
 * @p rawSel is the 0-based hotkey the dialog reported (0=COMBAT, 1=WAIT,
 * 2=FLEE, 3=ADVANCE-or-PARLAY). The PARLAY/ADVANCE substitution is applied
 * here, then @c resp[sel] selects the response branch. This is the pure
 * transition: it mutates nothing, reads no RNG and performs no VM write — the
 * host applies the returned outcome.
 */
EncounterStep encounterResolveSelection(const EncounterConfig &config,
		const EncounterState &state, uint8 rawSel);

/**
 * Pick the message string index (1..3) shown for @p monsterDist, following the
 * original fallback rotation: distance 0 scans forward from slot 1 and stops at
 * the first non-empty (or slot 3); distance 1 starts at slot 2 and wraps; dist
 * 2 starts at slot 3 and wraps. Returns the chosen 1-based slot even when every
 * slot is empty (the host then suppresses the window clear, matching clearW).
 * @p nonEmpty[i] is true when message slot i+1 has text.
 */
uint8 encounterMessageSlot(uint8 monsterDist, const bool nonEmpty[3]);

// ---------------------------------------------------------------------------
// Movement extremes (MoveExtremes)
// ---------------------------------------------------------------------------

/** One party member's movement input for MoveExtremes. */
struct MoveExtremeMember {
	uint8 movementCur; // movement.current
	bool  hasted;      // HASTED effect active (doubles)
	bool  slowed;      // SLOWED effect active (halves); HASTED wins if both

	MoveExtremeMember() : movementCur(0), hasted(false), slowed(false) {}
};

/** Max/min effective party movement (output of MoveExtremes). */
struct MoveExtremes {
	uint8 hiMove;
	uint8 loMove;

	MoveExtremes() : hiMove(0), loMove(0) {}
};

/**
 * Port of MoveExtremes (eclmenu.pas:992). Seeds hi/lo from the first member's
 * raw movement, then for every member applies HASTED (x2) or else SLOWED (/2)
 * and tracks the max and min effective movement. An empty party yields {0,0}.
 */
MoveExtremes computeMoveExtremes(const Common::Array<MoveExtremeMember> &members);

// ---------------------------------------------------------------------------
// Party surprise predicate (EclPartySurprise)
// ---------------------------------------------------------------------------

/**
 * Two-value output of PARTY SURPRISE. @c mod1 is 1 when the party contains a
 * ranger (or cleric-ranger), else 0; @c mod2 is always 0. Matches the original
 * 1/0 or 0/0 pair.
 */
struct PartySurpriseResult {
	uint8 mod1;
	uint8 mod2;

	PartySurpriseResult() : mod1(0), mod2(0) {}
};

/**
 * Port of EclPartySurprise (eclvm.pas:784). Scans @p classIds for a ranger
 * (C_RANGER) or cleric-ranger (C_CLERIC_RANGER). There is deliberately NO
 * status filter in the original: an incapacitated/dead ranger still counts, so
 * the caller passes every party member's class id regardless of status.
 */
PartySurpriseResult resolvePartySurprise(const Common::Array<uint8> &classIds);

// ---------------------------------------------------------------------------
// Surprise roll (EclSurprise)
// ---------------------------------------------------------------------------

/**
 * Injectable d6 source for the surprise roll, recording draw order so the exact
 * two-roll sequence and the both-success overwrite can be proven. The live host
 * forwards to getOpcodeRandom()/VmInterface::rollDice(1, 6).
 */
class ISurpriseRollSource {
public:
	virtual ~ISurpriseRollSource() {}

	/** Roll one six-sided die (1..6). Called exactly twice, in order. */
	virtual uint8 rollD6() = 0;
};

/** The four surprise operand values (GetEclVar 1..4). */
struct SurpriseOperands {
	uint8 v[4]; // v[0..3] = operand values 1..4

	SurpriseOperands() {
		for (int i = 0; i < 4; ++i)
			v[i] = 0;
	}
};

/**
 * Port of EclSurprise (eclvm.pas:814). Computes the two signed thresholds in
 * wide intermediates (no narrowing UB):
 *   t1 = v[4] + 2 - v[1]   (operand 4 + 2 - operand 1)
 *   t2 = v[2] + 2 - v[3]   (operand 2 + 2 - operand 3)
 * Then rolls r1 and r2 (in that order). The result starts at 0 and:
 *   if r1 <= t1: result = (r2 <= t2) ? 3 : 1;
 *   if r2 <= t2: result = 2;            // ORIGINAL QUIRK: this second test
 *                                        // OVERWRITES a both-success 3 with 2.
 * The both-success overwrite is preserved deliberately; an enhanced fix is a
 * separate opt-in task. Returns the final result byte written to the ambush
 * global.
 */
uint8 resolveSurprise(const SurpriseOperands &ops, ISurpriseRollSource &source);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ENCOUNTER_RULES_H
