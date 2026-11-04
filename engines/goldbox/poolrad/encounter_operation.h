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

#ifndef GOLDBOX_POOLRAD_ENCOUNTER_OPERATION_H
#define GOLDBOX_POOLRAD_ENCOUNTER_OPERATION_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "goldbox/ecl/encounter_rules.h"

namespace Goldbox {
namespace Poolrad {

/**
 * GB13-002 — engine-owned encounter request for ENCOUNTER MENU (0x29).
 *
 * Holds everything the decision loop needs, copied BY VALUE from the decoder
 * scratch at decode time: evaluated monster id / picture id, requested
 * distance, the destination VM address, the five response values, the three
 * message strings and the two flee thresholds. It never retains pointers into
 * the mutable decoder operand/string scratch arrays, so it is safe to keep
 * across a WAIT/ADVANCE yield (same scratch-aliasing rule as GB13-009/004).
 *
 * The loop itself is the pure EncounterConfig transition in ecl/encounter_rules
 * (which has no engine/RNG dependency); this operation wires the owned request
 * to that transition and tracks the mutable state (monster distance, view
 * flag) that survives a reopen. The live host drives presentation
 * (encounterMenu) and performs the single destination write + flag clear when
 * the loop terminates.
 */
class EncounterOperation {
public:
	EncounterOperation();

	/**
	 * Populate the owned request from decoded operand values.
	 *
	 * @param monsterId      operand 1 (evaluated)
	 * @param requestedDist  operand 2 (requested starting distance)
	 * @param monsterPicId   operand 3 (evaluated)
	 * @param destAddr       operand 4 destination VM address (literal word)
	 * @param resp           operands 5..9 response-table values (exactly 5)
	 * @param messages       operands 10..12 decoded message strings (exactly 3)
	 * @param fleePar        operand 13 party-flee threshold
	 * @param fleeMon        operand 14 monster-flee threshold
	 */
	void build(uint8 monsterId, uint8 requestedDist, uint8 monsterPicId,
			uint16 destAddr, const Common::Array<uint8> &resp,
			const Common::Array<Common::String> &messages,
			uint8 fleePar, uint8 fleeMon);

	/**
	 * Seed the live monster distance and render flag.
	 *
	 * The original clamps the computed max-monster distance down to the
	 * requested distance: @p maxMonsterDist is the computed stage distance and
	 * the live distance becomes min(maxMonsterDist, requestedDist).
	 */
	void begin(uint8 maxMonsterDist, uint8 view3dFlag,
			const ECL::MoveExtremes &extremes);

	/**
	 * Resolve one reported menu selection through the pure transition.
	 *
	 * Updates the live monster distance when the step reopens the prompt
	 * (WAIT/ADVANCE/PARLAY-close). When the step is terminal the outcome is
	 * recorded and @c isComplete() becomes true; the host then writes the
	 * outcome once to the destination and clears the encounter/message flags.
	 */
	ECL::EncounterStep applySelection(uint8 rawSel);

	/** True once a terminal selection has been resolved. */
	bool isComplete() const { return _complete; }

	/** Terminal outcome code (valid only when isComplete()). */
	uint8 outcome() const { return _outcome; }

	/** Destination VM address for the outcome write. */
	uint16 destAddr() const { return _destAddr; }

	/** Live monster distance (post last applySelection). */
	uint8 monsterDist() const { return _state.monsterDist; }

	uint8 monsterId() const { return _monsterId; }
	uint8 monsterPicId() const { return _monsterPicId; }

	/** True when the PARLAY menu variant should be shown (vs. ADVANCE). */
	bool usesParleyMenu() const;

	/**
	 * Chosen message slot (1..3) for the current distance and whether it is
	 * empty (empty suppresses the window clear, matching clearW).
	 */
	uint8 currentMessageSlot() const;
	const Common::String &message(uint8 oneBasedSlot) const;

private:
	ECL::EncounterConfig _config;
	ECL::EncounterState  _state;
	Common::String       _messages[3];
	uint16               _destAddr;
	uint8                _monsterId;
	uint8                _monsterPicId;
	uint8                _requestedDist;
	bool                 _complete;
	uint8                _outcome;
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_ENCOUNTER_OPERATION_H
