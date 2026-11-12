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
 * Assisted-by: Claude:claude-opus-4-20250514
 */

#include "goldbox/poolrad/rest_operation.h"

#include "goldbox/data/items/character_inventory.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/poolrad_spell_mapping.h"
#include "goldbox/poolrad/spell_preparation.h"

namespace Goldbox {
namespace Poolrad {

namespace {

// Book level (1..3) for a 0-based knownSpells[] index, matching the fixed
// Pool of Radiance spell layout used by SpellPreparation. Zero for a
// non-player-learnable index. This is the spellTable[spellId].levelSp lookup
// the original CheckScribeDone / CheckMemorizeDone feed into prepTimers := lvl * 3.
uint8 bookLevelForLegacyIndex(int legacyIndex) {
	if (legacyIndex < 0 || legacyIndex >= Data::POOLRAD_PLAYER_KNOWN_SIZE)
		return 0;
	if (legacyIndex <= 7)
		return 1; // Cleric 1
	if (legacyIndex <= 20)
		return 1; // Mage 1
	if (legacyIndex <= 27)
		return 2; // Cleric 2
	if (legacyIndex <= 34)
		return 2; // Mage 2
	if (legacyIndex <= 43)
		return 3; // Cleric 3
	return 3;     // Mage 3 (44..54)
}

// Level of the next pending scribe payload on the member, or 0 if none.
// Scribe has priority, mirroring CheckScribeDone running before
// CheckMemorizeDone.
uint8 nextPendingScribeLevel(Data::PoolradCharacter *member) {
	Goldbox::Data::Items::CharacterInventory &inv = member->inventory;
	const int itemCount = inv.count();
	for (int i = 0; i < itemCount; ++i) {
		Goldbox::Data::Items::CharacterItem &item = inv[i];
		if (!item.isMissileOrScroll())
			continue;
		const uint8 effects[3] = { item.effect1, item.effect2, item.effect3 };
		for (int e = 0; e < 3; ++e) {
			if (effects[e] < 0x80)
				continue;
			const int spellIdx = (effects[e] & 0x7F) - 1;
			return bookLevelForLegacyIndex(spellIdx);
		}
	}
	return 0;
}

// Level of the next pending memorized slot on the member, or 0 if none.
uint8 nextPendingMemorizeLevel(Data::PoolradCharacter *member) {
	for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
		const uint8 v = member->spells.memorizedSpells[i];
		if (v >= 0x80)
			return bookLevelForLegacyIndex((v & 0x7F) - 1);
	}
	return 0;
}

} // namespace

RestOperation::RestOperation()
	: _remainingMinutes(0), _elapsedTicks(0), _healTicks(0), _baseTick(0),
	  _restTicks(0), _safeTime(0), _interruptPct(0), _reason(REASON_NONE) {
}

void RestOperation::start(
		const Common::Array<Data::PoolradCharacter *> &party,
		uint32 requestedMinutes, uint8 safeTime, uint8 interruptPct) {
	_party = party;

	// CampRest zeroes every per-character preparation timer up front
	// (gametime.pas:426-430).
	_prepTimers.clear();
	_prepTimers.resize(_party.size());
	for (uint i = 0; i < _prepTimers.size(); ++i)
		_prepTimers[i] = 0;

	_remainingMinutes = requestedMinutes;
	_elapsedTicks = 0;
	_healTicks = 0;
	_baseTick = 0;
	_restTicks = 0;
	_safeTime = safeTime;
	_interruptPct = interruptPct;
	_reason = REASON_NONE;
}

bool RestOperation::isResting() const {
	return _reason == REASON_NONE && _remainingMinutes > 0;
}

void RestOperation::requestStop() {
	if (_reason == REASON_NONE)
		_reason = REASON_STOP;
}

void RestOperation::requestScriptedInterruption() {
	if (_reason == REASON_NONE)
		_reason = REASON_INTERRUPTED;
}

void RestOperation::requestQuit() {
	if (_reason == REASON_NONE)
		_reason = REASON_QUIT;
}

bool RestOperation::advanceTick(Environment &env) {
	if (!isResting())
		return false;

	// Original per-tick order (gametime.pas:450-474):
	//   DecrementRestTime(5, MINUTES)
	//   AdvanceTime(5, MINUTES)
	//   CheckRestingHeal
	//   AdvanceSpellPrep
	//   AdvanceSpellPrepBase(tick)
	//   interruption check
	decrementRequestedTime();        // 1
	env.advanceFiveMinutes();        // 2
	++_elapsedTicks;
	checkRestingHeal(env);           // 3
	advanceSpellPrep();              // 4
	advanceSpellPrepBase();          // 5
	if (checkInterruption(env))      // 6
		_reason = REASON_INTERRUPTED;

	// Elapsed-duration completion: the loop guard is "any remaining time".
	if (_reason == REASON_NONE && _remainingMinutes == 0)
		_reason = REASON_ELAPSED;

	return _reason == REASON_NONE;
}

void RestOperation::decrementRequestedTime() {
	// DecrementRestTime(5, MINUTES): the requested duration is kept in minute
	// units; five minutes are removed per tick, clamped at zero.
	if (_remainingMinutes >= 5)
		_remainingMinutes -= 5;
	else
		_remainingMinutes = 0;
}

void RestOperation::checkRestingHeal(Environment &env) {
	// CheckRestingHeal (gametime.pas:269): one heal pass every 288 ticks,
	// restoring exactly 1 HP per member via the ordinary routine.
	++_healTicks;
	if (_healTicks < 288)
		return;

	for (uint i = 0; i < _party.size(); ++i) {
		if (_party[i])
			env.restoreOneHitPoint(_party[i]);
	}
	_healTicks = 0;
}

uint8 RestOperation::completeOnePreparation(Data::PoolradCharacter *member) {
	if (!member)
		return 0;

	// Capture the level BEFORE completion: the reload uses the level of the
	// work that just finished (lvl * 3). Scribe has priority over memorize.
	SpellPreparation prep(member);

	const uint8 scribeLevel = nextPendingScribeLevel(member);
	if (scribeLevel != 0 && prep.completeOnePendingScribe())
		return scribeLevel;

	const uint8 memorizeLevel = nextPendingMemorizeLevel(member);
	if (memorizeLevel != 0 && prep.completeOnePendingMemorize())
		return memorizeLevel;

	return 0;
}

void RestOperation::advanceSpellPrep() {
	// AdvanceSpellPrep (gametime.pas:355): every tick decrement each
	// per-character timer, then if it hit zero AND the base prep timer is also
	// zero, complete one unit of pending work and reload lvl * 3.
	//
	// Byte wrap is preserved: the timer is uint8 and a zero value decrements
	// to 0xFF (Dec of a Pascal Byte), exactly as the original. Do NOT widen.
	for (uint i = 0; i < _party.size(); ++i) {
		Data::PoolradCharacter *member = _party[i];

		_prepTimers[i] = (uint8)(_prepTimers[i] - 1); // byte wrap on zero

		if (_prepTimers[i] == 0 && member &&
				member->spellRegainRate == 0) {
			const uint8 lvl = completeOnePreparation(member);
			_prepTimers[i] = (uint8)(lvl * 3);
		}
	}
}

void RestOperation::advanceSpellPrepBase() {
	// AdvanceSpellPrepBase (gametime.pas:381): an hourly cadence. The base
	// tick counter wraps at 12 ticks (12 * 5 min = 60 min). Only on the hour
	// do the per-character base preparation timers (spellRegainRate, the
	// legacy spellPrepTime at char offset 0x2C) decrement, and completing one
	// reloads the per-character timer with lvl * 3.
	++_baseTick;
	if (_baseTick < 12)
		return;
	_baseTick = 0;

	for (uint i = 0; i < _party.size(); ++i) {
		Data::PoolradCharacter *member = _party[i];
		if (!member)
			continue;

		// ShortInt(spellPrepTime) > 0 guard: only a positive signed byte.
		if ((int8)member->spellRegainRate > 0) {
			--member->spellRegainRate;
			if (member->spellRegainRate == 0) {
				const uint8 lvl = completeOnePreparation(member);
				_prepTimers[i] = (uint8)(lvl * 3);
			}
		}
	}
}

bool RestOperation::checkInterruption(Environment &env) {
	// Interruption check (gametime.pas:461-474): only when the area defines a
	// positive safe-rest window. Each time the window expires, roll d100 and
	// compare against REST_INTERRUPT_PCT.
	if (_safeTime == 0)
		return false;

	++_restTicks;
	if (_restTicks < _safeTime)
		return false;

	_restTicks = 0;
	return env.rollInterruptionPercent() <= (int)_interruptPct;
}

} // namespace Poolrad
} // namespace Goldbox
