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

#include "goldbox/poolrad/training_rules.h"

#include "common/util.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/poolrad.h"

namespace Goldbox {
namespace Poolrad {

using Goldbox::Poolrad::Data::PoolradCharacter;
namespace Rules = Goldbox::Data::Rules;

TrainingRules::TrainingRules()
	: _member(nullptr), _phase(PHASE_IDLE), _trainBaseClass(0),
	  _hallBaseClass(0), _haveMageSpellOffered(false),
	  _noMageSpellOffered(false), _xpClamped(false), _charged(false),
	  _lastHpGain(0) {
}

bool TrainingRules::meetsXpThreshold(const PoolradCharacter *member,
		uint8 trainBaseClass) {
	if (!member || trainBaseClass >= BASE_CLASS_NUM)
		return false;
	const Common::Array<uint8> &lvls = member->levels.levels;
	if (trainBaseClass >= lvls.size() || lvls[trainBaseClass] == 0)
		return false; // class not held: not eligible (no invented rule)

	const uint8 curLevel = lvls[trainBaseClass];
	const int32 needed = Rules::xpForClassAtLevel(trainBaseClass, curLevel);
	if (needed < 0)
		return false; // end of progression / out of range
	return member->experiencePoints >= (uint32)needed;
}

TrainingRules::BeginResult TrainingRules::begin(PoolradCharacter *member,
		uint8 trainBaseClass, uint8 hallBaseClass, bool haveMageSpellOffered) {
	_member = member;
	_phase = PHASE_IDLE;
	_trainBaseClass = trainBaseClass;
	_hallBaseClass = hallBaseClass;
	_haveMageSpellOffered = haveMageSpellOffered;
	_noMageSpellOffered = false;
	_xpClamped = false;
	_charged = false;
	_lastHpGain = 0;

	if (!member)
		return RESULT_BLOCKED_NO_CLASS;

	// Original order/timing:
	// 1. consciousness check. S_OKAY is the only trainable status; an
	//    unconscious/dying/dead/stoned/gone member cannot train.
	if (member->healthStatus != Goldbox::Data::S_OKAY)
		return RESULT_BLOCKED_STATUS;

	// 2. funds check - needs 1000 gp but NOTHING is deducted yet.
	if (member->valuableItems.getGoldValue() < kTrainingCostGold)
		return RESULT_BLOCKED_FUNDS;

	// 3. class eligibility / XP threshold across the eight class-level entries.
	//    Race/stat caps are already folded into the rules tables
	//    (xpForClassAtLevel returns -1 at the class's capped end); we invent no
	//    additional tabletop racial restriction here.
	if (trainBaseClass >= BASE_CLASS_NUM)
		return RESULT_BLOCKED_NO_CLASS;
	const Common::Array<uint8> &lvls = member->levels.levels;
	if (trainBaseClass >= lvls.size() || lvls[trainBaseClass] == 0)
		return RESULT_BLOCKED_NO_CLASS;
	if (!meetsXpThreshold(member, trainBaseClass))
		return RESULT_BLOCKED_XP;

	// XP CLAMP TIMING: the source clamps XP here, BEFORE the hall check and
	// confirmation. A later abort/decline leaves this clamp in place - there is
	// NO general rollback. Apply it now and never undo it.
	applyEarlyXpClamp();

	// 4. wrong-hall check. OPEN: the exact way the original keys a training
	//    hall to a class could not be confirmed from the reconstruction. The
	//    bounded, faithful interpretation is "the hall must serve the base
	//    class being trained"; callers that cannot supply a real hall id pass
	//    hallBaseClass == trainBaseClass to opt out. Never guess beyond this.
	if (hallBaseClass != trainBaseClass)
		return RESULT_BLOCKED_WRONG_HALL;

	// Determine the no-offered-mage-spell anomaly up front (Req 27.4, OPEN).
	_noMageSpellOffered = !haveMageSpellOffered;

	_phase = PHASE_CONFIRM;
	return RESULT_OK;
}

void TrainingRules::applyEarlyXpClamp() {
	// Clamp the character's XP down to exactly the threshold for the level
	// just reached in the trained class. This is the early clamp the source
	// performs before confirmation; it persists even if the player declines.
	if (!_member || _xpClamped)
		return;
	if (_trainBaseClass >= _member->levels.levels.size())
		return;
	const uint8 curLevel = _member->levels.levels[_trainBaseClass];
	const int32 threshold = Rules::xpForClassAtLevel(_trainBaseClass, curLevel);
	if (threshold < 0)
		return;
	if (_member->experiencePoints > (uint32)threshold)
		_member->experiencePoints = (uint32)threshold;
	_xpClamped = true;
}

void TrainingRules::decline() {
	// Charge NO money. The early XP clamp (if begin() applied it) stays in
	// place - that is the documented quirk, not a bug to "fix" with a rollback.
	_phase = PHASE_IDLE;
	_charged = false;
}

void TrainingRules::confirm(RngSource &rng) {
	if (_phase != PHASE_CONFIRM || !_member)
		return;

	// Deduct 1000 gp ONCE, only after confirmation.
	if (!_charged) {
		const uint32 gold = _member->valuableItems.getGoldValue();
		const uint32 remaining =
			(gold >= kTrainingCostGold) ? (gold - kTrainingCostGold) : 0;
		_member->valuableItems.setFromGoldValue(remaining);
		_charged = true;
	}

	// Apply order after confirmation: level/drain -> recalc skills -> HP.
	applyLevelUp(rng);

	// THEN select the new mage spell in the original order. When a spell is on
	// offer the view drives PHASE_MAGE_SPELL; otherwise take the OPEN no-spell
	// no-op path and finish.
	if (_haveMageSpellOffered) {
		_phase = PHASE_MAGE_SPELL;
	} else {
		// OPEN (Req 27.4): no mage spell offered. The reconstruction notes
		// spellsKnown[0] may ALIAS hpMax; emulating that aliasing blindly
		// risks indexing outside the C++ spell table. Until the real case is
		// traced we take a bounded, safe no-op: no spell is learned and no
		// index is dereferenced. Left labelled OPEN on purpose.
		_phase = PHASE_DONE;
	}
}

void TrainingRules::applyLevelUp(TrainingRules::RngSource &rng) {
	if (!_member)
		return;

	// Level/drain changes. If the character has drained levels, the regained
	// level restores a drained level first (undoing the drain) before adding a
	// genuinely new class level - matching the source interaction.
	if (_member->drainedLevels > 0) {
		--_member->drainedLevels;
	} else if (_trainBaseClass < _member->levels.levels.size()) {
		uint8 &lvl = _member->levels.levels[_trainBaseClass];
		if (lvl < MAX_LEVEL)
			++lvl;
	}

	// HP gain for the new level: roll + minimum gain + Con contribution,
	// divided by the active class count (multiclass), preserving prior damage.
	const uint8 activeClasses = _member->countActiveBaseClasses();
	const uint8 gain = computeHpGain(rng, _trainBaseClass,
			_member->abilities.constitution.current,
			activeClasses == 0 ? 1 : activeClasses);
	_lastHpGain = gain;

	// Preserve existing damage: the amount of current-HP damage before the
	// gain is re-applied after the max increases, so a wounded character stays
	// wounded by the same amount (source behaviour).
	const uint8 prevMax = _member->hitPoints.max;
	const uint8 prevCur = _member->hitPoints.current;
	const uint8 priorDamage = (prevMax >= prevCur) ? (uint8)(prevMax - prevCur) : 0;

	uint16 newMax = (uint16)prevMax + (uint16)gain;
	if (newMax > 0xFF)
		newMax = 0xFF;
	_member->hitPoints.max = (uint8)newMax;
	const int newCur = (int)_member->hitPoints.max - (int)priorDamage;
	_member->hitPoints.current = (uint8)CLIP<int>(newCur, 0, 0xFF);

	// Recalculate skills AFTER level/drain and BEFORE spell selection:
	// recalcCombatStats refreshes THAC0, saving throws, thief skills and
	// spell slots from the updated level progression (see PoolradCharacter).
	_member->recalcCombatStats();
}

uint8 TrainingRules::computeHpGain(TrainingRules::RngSource &rng,
		uint8 trainBaseClass, uint8 constitution, uint8 activeClassCount) {
	if (trainBaseClass >= BASE_CLASS_NUM)
		return 0;
	if (activeClassCount == 0)
		activeClassCount = 1;

	const Goldbox::Data::DiceRoll &hpDice = Rules::getHPRoll(trainBaseClass);

	int rolled = 0;
	if (hpDice.diceNum > 0 && hpDice.diceSides > 0)
		rolled = rng.rollDice(hpDice.diceNum, hpDice.diceSides);

	// Minimum gain: never fewer than the number of dice (a floor of 1 per die),
	// matching the source min-gain rule for a single level.
	const int minGain = (hpDice.diceNum > 0) ? (int)hpDice.diceNum : 1;
	if (rolled < minGain)
		rolled = minGain;

	// Constitution contribution per the rules table.
	rolled += (int)Rules::conHPModifier(constitution);
	if (rolled < 1)
		rolled = 1;

	// Class-count division for multiclass characters (integer division).
	rolled /= (int)activeClassCount;
	if (rolled < 1)
		rolled = 1;

	if (rolled > 0xFF)
		rolled = 0xFF;
	return (uint8)rolled;
}

void TrainingRules::selectMageSpell(uint8 offerIndex) {
	if (_phase != PHASE_MAGE_SPELL)
		return;

	// Bounds safety (Req 27.4): the view supplies the real offered list and
	// drives the actual learn via the spell-book path; this operation only
	// records that selection happened and guards the index. We never index the
	// C++ spell table here, so an out-of-range offerIndex is a safe no-op.
	(void)offerIndex;
	_phase = PHASE_DONE;
}

// ---------------------------------------------------------------------------
// RosterActions

bool RosterActions::removeSelectedFromParty(
		Goldbox::Data::PlayerCharacter *member, Action action) {
	(void)action; // see OPEN note below.
	if (!member || !g_engine)
		return false;

	PoolradEngineHostImpl *host = g_engine->getEclHost();
	if (!host)
		return false;

	// Active-party removal via the shared ownership op (GB13-006): stable
	// capture-next-before-erase, frees the icon slot, updates selection + VM
	// window, invalidates pending refs - all atomic inside the host.
	//
	// OPEN (Drop-vs-Remove disk semantics): the reconstruction did not make it
	// confirmable whether DROP and/or REMOVE also delete/update the stored DISK
	// character versus only the active party. The confirmable, shared behaviour
	// is the active-party removal below, which both actions route through. The
	// disk-deletion distinction is left OPEN and is NOT guessed here; the legacy
	// file operations remain available for a later trace to wire precisely.
	return host->removeLoadedCharacter(member);
}

} // namespace Poolrad
} // namespace Goldbox
