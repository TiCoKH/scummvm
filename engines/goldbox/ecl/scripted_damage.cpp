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

#include "goldbox/ecl/scripted_damage.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace ECL {

// Attack-byte bit fields (EclDamage, eclvm.pas:1341-1346).
static const uint8 kAttackSaveMode    = 0x80; // bit 7: saving-throw mode
static const uint8 kAttackWholeParty   = 0x40; // bit 6: whole party
static const uint8 kAttackBypassSaves  = 0x20; // bit 5: bypass whole-party saves
static const uint8 kAttackSaveModMask  = 0x1F; // low five bits: save modifier

// Value-byte bit fields (eclvm.pas:1347/1358).
static const uint8 kValueCurrentChar   = 0x80; // bit 7: current character
static const uint8 kValueSaveTypeMask   = 0x07; // low three bits: save category

// Walk to the 1-based party position `count`, clamped to the live party. The
// original advances p along nextCharAddr `count-1` times; an index past the
// tail would stop at the tail in the linked list. We clamp identically.
static Data::PlayerCharacter *memberAt(
		const Common::Array<Data::PlayerCharacter *> &party, uint8 count) {
	if (party.empty())
		return nullptr;
	uint idx = (count == 0) ? 0 : static_cast<uint>(count - 1);
	if (idx >= party.size())
		idx = party.size() - 1;
	return party[idx];
}

static bool anyEnabled(const Common::Array<Data::PlayerCharacter *> &party) {
	for (uint i = 0; i < party.size(); ++i) {
		const Data::PlayerCharacter *c = party[i];
		if (c && c->enabled)
			return true;
	}
	return false;
}

ScriptedDamageOutcome resolveScriptedDamage(
		const ScriptedDamageOperands &ops,
		const Common::Array<Data::PlayerCharacter *> &party,
		Data::PlayerCharacter *currentChar,
		IScriptedDamageSource &source,
		IScriptedDamageSink &sink) {
	ScriptedDamageOutcome outcome;

	// Empty party: no target exists. The original would walk a nil creature
	// list; we treat it as degenerate and apply nothing (Req 2.3 defensive).
	if (party.empty()) {
		outcome.degenerate = true;
		// partyDefeated mirrors the original scan over an empty list: it would
		// stay True (no enabled member clears it). Report defeat so the host
		// presentation matches a wiped party.
		outcome.partyDefeated = true;
		return outcome;
	}

	// Step 1: initial damage roll BEFORE branching (eclvm.pas:1340). bonus is a
	// byte and RollDice returns at most dice*sides; the sum is held in a wider
	// intermediate so the byte arithmetic never overflows.
	uint16 dmg = static_cast<uint16>(ops.bonus) + source.rollDice(ops.dice, ops.sides);

	const bool allParty = (ops.attack & kAttackWholeParty) != 0;

	// Step 2: when NOT whole party, draw the target count once up front
	// (eclvm.pas:1344). The whole-party branch skips this draw. In save mode
	// this `count` picks the random single member; the current-character branch
	// still performs (and ignores) the draw, matching the original draw order.
	// In attack mode (bit 7 clear) this up-front draw is likewise performed but
	// never consulted (the per-iteration loop rolls its own target), preserving
	// the exact RNG sequence.
	uint8 count = 0;
	if (!allParty)
		count = source.rollTargetIndex(static_cast<uint8>(party.size()));

	if (ops.attack & kAttackSaveMode) {
		// -------------------------------------------------------------------
		// Save mode (eclvm.pas:1345-1366).
		// -------------------------------------------------------------------
		const uint8 saveMod = ops.attack & kAttackSaveModMask;
		const Data::Spells::SaveVerseType saveType =
			static_cast<Data::Spells::SaveVerseType>(ops.value & kValueSaveTypeMask);

		if (allParty) {
			// Whole party: bit 5 bypasses saves, otherwise each member rolls.
			for (uint i = 0; i < party.size(); ++i) {
				Data::PlayerCharacter *p = party[i];
				if (!p)
					continue;
				if (ops.attack & kAttackBypassSaves) {
					sink.inflict(*p, dmg);
				} else if (!source.rollSave(*p, saveType, saveMod)) {
					sink.inflict(*p, dmg);
				}
			}
		} else if (ops.value & kValueCurrentChar) {
			// Current character with the special one-less save category
			// (eclvm.pas:1358-1360). value==0 already means saveType==0 => no
			// save. The selected character is passed explicitly (GDVars.curChar),
			// not inferred from the party head.
			Data::PlayerCharacter *cur = currentChar;
			if (cur) {
				bool saved = false;
				if ((ops.value & kValueSaveTypeMask) != 0) {
					const Data::Spells::SaveVerseType oneLess =
						static_cast<Data::Spells::SaveVerseType>(
							(ops.value & kValueSaveTypeMask) - 1);
					saved = source.rollSave(*cur, oneLess, saveMod);
				}
				if (!saved)
					sink.inflict(*cur, dmg);
			}
		} else {
			// Random single member: the count draw above picks the position.
			Data::PlayerCharacter *p = memberAt(party, count);
			if (p && !source.rollSave(*p, saveType, saveMod))
				sink.inflict(*p, dmg);
		}
	} else {
		// -------------------------------------------------------------------
		// Attack-roll mode (eclvm.pas:1367-1380): `attack` strikes against
		// random members. Damage is REROLLED after every iteration INCLUDING
		// the last; the final reroll is intentionally unused but preserved so
		// the RNG draw count/order matches the reference exactly.
		// -------------------------------------------------------------------
		for (uint8 i = 1; i <= ops.attack; ++i) {
			const uint8 targetPos =
				source.rollTargetIndex(static_cast<uint8>(party.size()));
			Data::PlayerCharacter *p = memberAt(party, targetPos);
			if (p && source.attackHits(*p, ops.value))
				sink.inflict(*p, dmg);
			// Reroll AFTER the hit, including the final iteration.
			dmg = static_cast<uint16>(ops.bonus)
				+ source.rollDice(ops.dice, ops.sides);
			// `attack` is a byte; the 1..attack loop cannot exceed 255, so the
			// counter never overflows.
			if (i == 0xFF)
				break;
		}
	}

	// Step 5a: total-party defeat from enabled members (eclvm.pas:1381-1387).
	outcome.partyDefeated = !anyEnabled(party);
	return outcome;
}

} // namespace ECL
} // namespace Goldbox
