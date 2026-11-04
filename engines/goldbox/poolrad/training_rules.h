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

#ifndef GOLDBOX_POOLRAD_TRAINING_RULES_H
#define GOLDBOX_POOLRAD_TRAINING_RULES_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}
namespace Poolrad {
namespace Data {
class PoolradCharacter;
}

/**
 * Shared training rule core (GB13-020, Requirement 27).
 *
 * Owns the Pool of Radiance TRAIN workflow outside any rendering dialog: the
 * eligibility checks in their original order, the early XP clamp (and its
 * quirk), the 1000 gp charge gated behind confirmation, the level/drain
 * application, skill recalculation and the final mage-spell selection, plus
 * the multiclass HP gain math. The mainmenu view drives it as two explicit
 * continuation phases (confirmation, then mage-spell choice), mirroring
 * RestOperation. The single engine-coupled side effect (the per-level HP dice
 * roll) is injected through RngSource so the whole operation is unit-testable
 * without g_engine; the host forwards to VmInterface::rollDice and tests
 * supply deterministic rolls.
 *
 * DROP / REMOVE are distinct roster actions, not part of this operation. Their
 * routing lives in RosterActions below; the active-party removal is the shared
 * ownership op (PoolradEngineHostImpl::removeLoadedCharacter, GB13-006), wired
 * from the view. See the OPEN notes in training_rules.cpp.
 *
 * Three items are deliberately left OPEN / trace-gated (documented at their
 * sites): the wrong-hall keying, the Drop-vs-Remove disk-character semantics,
 * and the no-offered-mage-spell anomaly (spellsKnown[0] possibly aliasing
 * hpMax). None of them is guessed; each takes the most faithful bounded path
 * and never indexes outside the C++ spell table.
 */
class TrainingRules {
public:
	/**
	 * Injected RNG for the per-level HP dice roll. Kept minimal so the
	 * operation stays pure and testable. The host forwards to
	 * VmInterface::rollDice(number, sides); tests return scripted values.
	 */
	class RngSource {
	public:
		virtual ~RngSource() {}
		// Roll `number` dice of `sides` sides and return their sum.
		virtual int rollDice(int number, int sides) = 0;
	};

	// Continuation phases the view drives, in original order.
	enum Phase {
		PHASE_IDLE = 0,          // nothing started / finished
		PHASE_CONFIRM = 1,       // waiting for player to confirm the 1000 gp charge
		PHASE_MAGE_SPELL = 2,    // confirmed; waiting for the new mage spell choice
		PHASE_DONE = 3           // training applied and complete
	};

	// Why begin() refused to start (no state was mutated except, when
	// RESULT_OK_* is returned, the documented early XP clamp).
	enum BeginResult {
		RESULT_OK = 0,           // eligible; now in PHASE_CONFIRM
		RESULT_BLOCKED_STATUS,   // unconscious / dying / dead
		RESULT_BLOCKED_FUNDS,    // fewer than 1000 gp available
		RESULT_BLOCKED_XP,       // XP below the next-level threshold for the trained class
		RESULT_BLOCKED_WRONG_HALL, // hall does not match the class being trained (OPEN keying)
		RESULT_BLOCKED_NO_CLASS  // trained base class not held by the character
	};

	// Cost of one training session, charged once on confirmation.
	static const uint32 kTrainingCostGold = 1000;

	TrainingRules();

	/**
	 * Run the eligibility checks in the original order and, if eligible,
	 * apply the early XP clamp and enter PHASE_CONFIRM. `hallBaseClass` is
	 * the base class (0..7) this training hall trains; `trainBaseClass` is
	 * the base class the character advances. On any blocked result no money
	 * is charged; the early XP clamp is applied only on an OK result and is
	 * never rolled back (the documented quirk).
	 *
	 * @param member         the character to train (not owned).
	 * @param trainBaseClass base class index (0..7) being advanced.
	 * @param hallBaseClass  base class index (0..7) the hall serves.
	 * @param haveMageSpellOffered whether a new mage spell is on offer this
	 *                       level-up (false triggers the OPEN no-spell path).
	 */
	BeginResult begin(Goldbox::Poolrad::Data::PoolradCharacter *member,
			uint8 trainBaseClass, uint8 hallBaseClass,
			bool haveMageSpellOffered);

	/**
	 * Player declined at the confirmation prompt. Charges no money. The early
	 * XP clamp, if begin() applied it, stays in place (no rollback). Returns
	 * the operation to PHASE_IDLE.
	 */
	void decline();

	/**
	 * Player confirmed. Deducts 1000 gp exactly once, then applies the
	 * level/drain changes, recalculates skills, and rolls HP. If a mage spell
	 * is on offer, advances to PHASE_MAGE_SPELL; otherwise takes the OPEN
	 * no-spell path and finishes in PHASE_DONE. `rng` supplies the HP roll.
	 */
	void confirm(RngSource &rng);

	/**
	 * Select the new mage spell by its 0-based offer index, applied in the
	 * original order (after the level/skill/HP changes). Bounds-checked
	 * against `offeredCount`; an out-of-range index is a safe no-op. Finishes
	 * in PHASE_DONE.
	 */
	void selectMageSpell(uint8 offerIndex);

	// Introspection for the wiring / tests.
	Phase phase() const { return _phase; }
	bool isFinished() const { return _phase == PHASE_DONE; }
	// HP gained by the last confirm() (max-HP delta, before damage preserved).
	uint8 lastHpGain() const { return _lastHpGain; }
	// True once the early XP clamp has been applied to the member this run.
	bool xpClamped() const { return _xpClamped; }
	// True when begin() found no mage spell on offer (the OPEN anomaly path).
	bool noMageSpellOffered() const { return _noMageSpellOffered; }

	// --------------------------------------------------------------------
	// Static helpers (pure; no member state) exposed for the view and tests.

	/**
	 * HP gained for a single level in the trained class, matching the source:
	 * roll getHPRoll(trainBaseClass), apply the minimum-gain floor, add the
	 * Constitution contribution (conHPModifier), then divide by the number of
	 * active base classes for multiclass characters (integer division, source
	 * behaviour). Never returns less than the multiclass-divided minimum.
	 */
	static uint8 computeHpGain(RngSource &rng, uint8 trainBaseClass,
			uint8 constitution, uint8 activeClassCount);

	/**
	 * XP threshold eligibility across all eight class-level entries: for the
	 * trained class, is experiencePoints >= xpForClassAtLevel(base, level)?
	 * Returns false when the threshold table ends (-1) or the class is not
	 * held. Pure wrapper over the rules table; invents no racial rule.
	 */
	static bool meetsXpThreshold(
			const Goldbox::Poolrad::Data::PoolradCharacter *member,
			uint8 trainBaseClass);

private:
	// Apply the early XP clamp exactly where the source performs it (before
	// the hall check and confirmation). Idempotent per run.
	void applyEarlyXpClamp();

	// Confirmed-path application: level/drain, skill recalc, HP roll.
	void applyLevelUp(RngSource &rng);

	Goldbox::Poolrad::Data::PoolradCharacter *_member; // not owned
	Phase _phase;
	uint8 _trainBaseClass;
	uint8 _hallBaseClass;
	bool _haveMageSpellOffered;
	bool _noMageSpellOffered;
	bool _xpClamped;
	bool _charged;
	uint8 _lastHpGain;
};

/**
 * Roster action routing for DROP / REMOVE (GB13-020). These are distinct from
 * TRAIN and from each other; both touch the ACTIVE PARTY through the shared
 * ownership op. The disk-character distinction is documented OPEN in the .cpp:
 * the confirmable behaviour is the active-party removal (removeLoadedCharacter);
 * the Drop-vs-Remove disk-deletion split could not be confirmed from the code
 * and is NOT guessed.
 */
class RosterActions {
public:
	enum Action {
		ACTION_DROP = 0,   // original DROP workflow
		ACTION_REMOVE = 1  // original REMOVE workflow
	};

	/**
	 * Perform the active-party removal for the selected member via the shared
	 * ownership op on g_engine's host. Selection and counts are updated
	 * atomically by that op (capture-next-before-erase). Returns true if a
	 * member was removed. Needs g_engine; this is the integration entry point.
	 * `action` records intent for the OPEN disk-semantics note; both actions
	 * currently route to the same confirmable active-party removal.
	 */
	static bool removeSelectedFromParty(
			Goldbox::Data::PlayerCharacter *member, Action action);
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_TRAINING_RULES_H
