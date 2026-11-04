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

#ifndef GOLDBOX_ECL_SCRIPTED_DAMAGE_H
#define GOLDBOX_ECL_SCRIPTED_DAMAGE_H

#include "common/array.h"
#include "common/scummsys.h"
#include "goldbox/data/spells/spell.h"

namespace Goldbox {

namespace Data {
class PlayerCharacter;
} // namespace Data

namespace ECL {

// GB13-004: testable core of the DAMAGE (0x2E) opcode.
//
// Ports EclDamage (eclvm.pas:1311), EclInflictDamage (eclmenu.pas:1015) and
// AttackRollNoAttacker (fxengine.pas:404) at the rule level. The deterministic
// target-selection + roll-sequence logic lives here so the exact random draw
// order and count (including the final, seemingly-unused reroll in attack mode)
// can be proven with a recorded roll source, and the "apply damage exactly
// once through the real DamageSystem path" contract can be unit-asserted with a
// fake damage sink. The live opcode handler wires this to the engine RNG, the
// real DamageSystem and the selection accessors, and runs the acknowledgment /
// total-party-defeat presentation through the GB13-009 continuation seam.

/**
 * Injectable randomness + saving-throw source for the scripted-damage core.
 *
 * Every random draw the original performs goes through one of these hooks so a
 * recorded implementation can assert the call count and order. The live host
 * forwards rollDice/rollTargetIndex to the engine RNG and rollSave/attackHits
 * to the shared saving-throw / attack-roll rules.
 */
class IScriptedDamageSource {
public:
	virtual ~IScriptedDamageSource() {}

	/** Roll @p count dice of @p sides each (bonus added by the caller). */
	virtual uint16 rollDice(uint8 count, uint8 sides) = 0;

	/**
	 * Original RollDice(1, partySize): pick a 1-based party position. The
	 * return value is clamped by the caller to the live party size.
	 */
	virtual uint8 rollTargetIndex(uint8 partySize) = 0;

	/**
	 * Saving throw for @p target against @p saveType with @p saveMod. True on
	 * a successful save (damage avoided). Mirrors FxEngine.RollSavingThrow.
	 */
	virtual bool rollSave(Data::PlayerCharacter &target,
			Data::Spells::SaveVerseType saveType, uint8 saveMod) = 0;

	/**
	 * Attack roll with no attacker (fxengine.pas:404) against @p target using
	 * @p sixtyMinusThac0. True when the attack connects. Rolls 1d20 internally.
	 */
	virtual bool attackHits(Data::PlayerCharacter &target,
			uint8 sixtyMinusThac0) = 0;
};

/**
 * Damage sink. The core never subtracts HP directly; it hands each resolved
 * hit here. The live host routes this through DamageSystem (EclInflictDamage
 * behavior: enabled/status/effect changes, down/death message), exactly once
 * per connecting hit. A test can supply a recording fake.
 */
class IScriptedDamageSink {
public:
	virtual ~IScriptedDamageSink() {}

	/** Apply @p dmg to @p target through the real damage/status path. */
	virtual void inflict(Data::PlayerCharacter &target, uint16 dmg) = 0;
};

/** Decoded DAMAGE operands (already read via readVar). */
struct ScriptedDamageOperands {
	uint8 attack;
	uint8 dice;
	uint8 sides;
	uint8 bonus;
	uint8 value;

	ScriptedDamageOperands()
		: attack(0), dice(0), sides(0), bonus(0), value(0) {}
};

/** Outcome of the deterministic core (drives the presentation tail). */
struct ScriptedDamageOutcome {
	// True when no enabled member remains after all damage was applied. The
	// host runs the "entire party killed" presentation when set (Req 16.7).
	bool partyDefeated;
	// True when the operand set was malformed or the party was empty, so the
	// core applied no damage. The host emits a diagnostic (Req 2.3 / 16).
	bool degenerate;

	ScriptedDamageOutcome() : partyDefeated(false), degenerate(false) {}
};

/**
 * Run the deterministic EclDamage core over @p party (list order).
 *
 * @p party is the live party in iteration order; members are never reordered.
 * @p currentChar is the selected character (GDVars.curChar) used by the
 * value-bit-7 current-character branch; it may be null (then that branch
 * applies nothing). Damage is applied through @p sink exactly once per
 * connecting hit; this
 * function performs NO selection restore and NO presentation — those are the
 * host's continuation tail. The returned outcome carries total-party defeat
 * and the degenerate (empty/malformed) flag.
 *
 * The draw order is fixed to the original:
 *   1. initial dmg = bonus + rollDice(dice, sides)  (before any branch)
 *   2. save mode (attack bit 7 set): target-count draw once when not whole
 *      party, then per-target save (or bypass) with no reroll;
 *   3. attack mode (attack bit 7 clear): for each of `attack` iterations,
 *      a target-index draw, an attack roll, then a dmg reroll AFTER the hit
 *      INCLUDING the final iteration (the last reroll is intentionally unused).
 */
ScriptedDamageOutcome resolveScriptedDamage(
		const ScriptedDamageOperands &ops,
		const Common::Array<Data::PlayerCharacter *> &party,
		Data::PlayerCharacter *currentChar,
		IScriptedDamageSource &source,
		IScriptedDamageSink &sink);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_SCRIPTED_DAMAGE_H
