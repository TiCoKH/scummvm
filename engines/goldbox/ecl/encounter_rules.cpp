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

#include "goldbox/ecl/encounter_rules.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace ECL {

// ---------------------------------------------------------------------------
// Encounter decision transition
// ---------------------------------------------------------------------------

bool encounterUsesParleyMenu(const EncounterState &state) {
	// Original: (MONSTER_DIST = 0) or (view3dFlag = 0) chooses the PARLAY menu
	// variant; otherwise the ADVANCE variant is shown.
	return state.monsterDist == 0 || state.view3dFlag == 0;
}

// Build the terminal-write / reopen result for a response branch. All distance
// decrements clamp at 0 (byte movement; the original Dec() only runs when the
// distance is non-zero).
static EncounterStep makeWrite(uint8 outcome, uint8 dist) {
	EncounterStep step;
	step.terminal = true;
	step.outcome = outcome;
	step.monsterDist = dist;
	step.redisplay = false;
	return step;
}

static EncounterStep makeReopen(uint8 dist, bool redisplay) {
	EncounterStep step;
	step.terminal = false;
	step.monsterDist = dist;
	step.redisplay = redisplay;
	return step;
}

EncounterStep encounterResolveSelection(const EncounterConfig &config,
		const EncounterState &state, uint8 rawSel) {
	// PARLAY/ADVANCE substitution: when the PARLAY variant is shown, a raw
	// selection of 3 (fourth hotkey) is remapped to 4 before the response
	// table is consulted (eclvm.pas:1138).
	uint8 sel = rawSel;
	if (encounterUsesParleyMenu(state) && sel == 3)
		sel = 4;
	if (sel > 4)
		sel = 4;

	const uint8 choice = config.resp[sel];
	const uint8 dist = state.monsterDist;

	switch (choice) {
	case 0:
		// sel != 2: combat; sel == 2 (FLEE): party flees iff min movement
		// meets the party-flee threshold, else combat.
		if (sel != 2)
			return makeWrite(kEncounterCombat, dist);
		if (config.loMove >= config.fleePar)
			return makeWrite(kEncounterPartyFlees, dist);
		return makeWrite(kEncounterCombat, dist);

	case 1:
		switch (sel) {
		case 0:
			return makeWrite(kEncounterCombat, dist);
		case 1:
			// WAIT: both sides wait, reopen with no VM write.
			return makeReopen(dist, false);
		case 2:
			return makeWrite(kEncounterPartyFlees, dist);
		case 3:
			// ADVANCE: close distance when possible, then reopen.
			if (dist != 0)
				return makeReopen(static_cast<uint8>(dist - 1), true);
			return makeReopen(dist, false);
		case 4:
			// PARLAY: at adjacent range parley succeeds; otherwise close and
			// reopen.
			if (dist == 0)
				return makeWrite(kEncounterParley, dist);
			return makeReopen(static_cast<uint8>(dist - 1), true);
		default:
			break;
		}
		break;

	case 2:
		if (sel == 0) {
			// COMBAT vs a monster-flee response: the monsters flee when their
			// flee threshold exceeds the party's max movement, else combat.
			if (config.fleeMon > config.hiMove)
				return makeWrite(kEncounterMonstersFlee, dist);
			return makeWrite(kEncounterCombat, dist);
		}
		// Any non-COMBAT selection: the monsters flee.
		return makeWrite(kEncounterMonstersFlee, dist);

	case 3:
		switch (sel) {
		case 0:
			return makeWrite(kEncounterCombat, dist);
		case 1:
		case 3:
			if (dist != 0)
				return makeReopen(static_cast<uint8>(dist - 1), true);
			return makeReopen(dist, false);
		case 2:
			return makeWrite(kEncounterPartyFlees, dist);
		case 4:
			if (dist == 0)
				return makeWrite(kEncounterParley, dist);
			return makeReopen(static_cast<uint8>(dist - 1), true);
		default:
			break;
		}
		break;

	case 4:
		switch (sel) {
		case 0:
			return makeWrite(kEncounterCombat, dist);
		case 1:
		case 3:
		case 4:
			if (dist == 0)
				return makeWrite(kEncounterParley, dist);
			return makeReopen(static_cast<uint8>(dist - 1), true);
		case 2:
			return makeWrite(kEncounterPartyFlees, dist);
		default:
			break;
		}
		break;

	default:
		break;
	}

	// A response code outside 0..4 has no original handling; keep the prompt
	// open with no VM write rather than inventing an outcome.
	return makeReopen(dist, false);
}

uint8 encounterMessageSlot(uint8 monsterDist, const bool nonEmpty[3]) {
	// Original fallback rotation (eclvm.pas:1104-1128). Message slots are
	// 1-based; nonEmpty[] is 0-based (slot i uses nonEmpty[i-1]).
	if (monsterDist == 0) {
		// Scan forward from slot 1, stop at first non-empty or slot 3.
		uint8 k = 1;
		while (k <= 3 && !nonEmpty[k - 1])
			++k;
		return (k > 3) ? 3 : k;
	}

	// Distances 1 and 2 start at slot (dist + 1) and wrap, stopping at the
	// first non-empty. The original repeat/until reads a slot, advances (with
	// wrap), then tests the just-read slot or whether the index has returned to
	// the start; on full exhaustion the last-read slot is the one just before
	// the start, so an all-empty scan reports that slot (content stays empty,
	// so clearW is suppressed either way).
	const uint8 start = static_cast<uint8>(monsterDist + 1);
	uint8 k = start;
	uint8 lastRead = start;
	for (;;) {
		lastRead = k;
		if (nonEmpty[k - 1])
			return k;
		++k;
		if (k > 3)
			k = 1;
		if (k == start)
			return lastRead;
	}
}

// ---------------------------------------------------------------------------
// Movement extremes
// ---------------------------------------------------------------------------

MoveExtremes computeMoveExtremes(
		const Common::Array<MoveExtremeMember> &members) {
	MoveExtremes out;
	if (members.empty())
		return out;

	// Seed from the first member's RAW movement (eclmenu.pas:999-1000).
	out.hiMove = members[0].movementCur;
	out.loMove = members[0].movementCur;

	for (uint i = 0; i < members.size(); ++i) {
		uint16 m = members[i].movementCur;
		if (members[i].hasted)
			m = static_cast<uint16>(m * 2);
		else if (members[i].slowed)
			m = static_cast<uint16>(m / 2);
		const uint8 mb = static_cast<uint8>(m & 0xFF);
		if (mb > out.hiMove)
			out.hiMove = mb;
		if (mb < out.loMove)
			out.loMove = mb;
	}
	return out;
}

// ---------------------------------------------------------------------------
// Party surprise predicate
// ---------------------------------------------------------------------------

PartySurpriseResult resolvePartySurprise(
		const Common::Array<uint8> &classIds) {
	PartySurpriseResult out;
	// No status filter in the original: every class id counts (eclvm.pas:799).
	for (uint i = 0; i < classIds.size(); ++i) {
		if (classIds[i] == Data::C_RANGER
				|| classIds[i] == Data::C_CLERIC_RANGER) {
			out.mod1 = 1;
		}
	}
	// mod2 is always 0 in the original.
	return out;
}

// ---------------------------------------------------------------------------
// Surprise roll
// ---------------------------------------------------------------------------

uint8 resolveSurprise(const SurpriseOperands &ops,
		ISurpriseRollSource &source) {
	// Signed thresholds in wide intermediates, then truncated to the original
	// ShortInt (int8) width so the comparison matches the DOS sign-extended
	// test without C++ narrowing UB.
	const int t1Wide = static_cast<int>(ops.v[3]) + 2 - static_cast<int>(ops.v[0]);
	const int t2Wide = static_cast<int>(ops.v[1]) + 2 - static_cast<int>(ops.v[2]);
	const int t1 = static_cast<int>(static_cast<int8>(t1Wide & 0xFF));
	const int t2 = static_cast<int>(static_cast<int8>(t2Wide & 0xFF));

	// Exact draw order: r1 first, then r2 (eclvm.pas:830-831).
	const int r1 = static_cast<int>(source.rollD6());
	const int r2 = static_cast<int>(source.rollD6());

	uint8 result = 0;
	if (r1 <= t1)
		result = (r2 <= t2) ? 3 : 1;
	// ORIGINAL QUIRK (eclvm.pas:837-838): the second test overwrites a
	// both-success result of 3 with 2. Preserved deliberately.
	if (r2 <= t2)
		result = 2;
	return result;
}

} // namespace ECL
} // namespace Goldbox
