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

#include "goldbox/poolrad/post_combat.h"

namespace Goldbox {
namespace Poolrad {

// Monetary XP multipliers, ported verbatim from DetermineLootExp
// (postcbt.pas:72-78): copper/200, silver/20, electrum/2, gold x1,
// platinum x5, gems x250, jewelry x2200. The copper/silver/electrum slots
// divide; the rest multiply.
static const uint8 kNpcFlag = 0x80;

// --- Prime-stat / multiclass XP share (AwardExperience, postcbt.pas:92) ---
//
// The Pascal class enum differs from Goldbox::Data::ClassADnD, so the split is
// classified by class semantics rather than by raw value:
//   single prime classes earn a 10% bonus when their prime stat is > 15,
//   two-class multis get share / 2, three-class multis share / 3.
static uint32 classAdjustedShare(const PostCombatCombatant &c, uint32 base) {
	switch (c.classType) {
	case Goldbox::Data::C_CLERIC:
		return (c.wisCur > kPrimeStatThreshold) ? (base / 10 + base) : base;
	case Goldbox::Data::C_FIGHTER:
		return (c.strCur > kPrimeStatThreshold) ? (base / 10 + base) : base;
	case Goldbox::Data::C_MAGICUSER:
		return (c.intCur > kPrimeStatThreshold) ? (base / 10 + base) : base;
	case Goldbox::Data::C_THIEF:
		return (c.dexCur > kPrimeStatThreshold) ? (base / 10 + base) : base;

	// Two-class multis (Pascal CLERIC_FIGHTER, CLERIC_RANGER..FIGHTER_THIEF,
	// MAGE_THIEF). Integer division, matching `experience div 2`.
	case Goldbox::Data::C_CLERIC_FIGHTER:
	case Goldbox::Data::C_CLERIC_RANGER:
	case Goldbox::Data::C_CLERIC_MAGICUSER:
	case Goldbox::Data::C_CLERIC_THIEF:
	case Goldbox::Data::C_FIGHTER_MAGICUSER:
	case Goldbox::Data::C_FIGHTER_THIEF:
	case Goldbox::Data::C_MAGICUSER_THIEF:
		return base / 2;

	// Three-class multis (Pascal CLERIC_FIGHTER_MAGE, FIGHTER_MAGE_THIEF).
	case Goldbox::Data::C_CLERIC_FIGHTER_MAGICUSER:
	case Goldbox::Data::C_FIGHTER_MAGICUSER_THIEF:
		return base / 3;

	default:
		return base;
	}
}

// --- DetermineLootExp (postcbt.pas:26) ---
//
// Pools currency + items from hostile, non-running combatants, computes the
// XP total (monster base/per-HP + currency conversions + positive item plus
// bonuses) and divides by the number of eligible recipients. The divisor is
// (PARTY_SIZE - nonAnimCount); nonAnimCount is produced by
// DetermineCombatResults just before this call. Returns the per-recipient
// share in @p outShare and leaves the pooled money in result.treasureMoney.
static void determineLootExp(const PostCombatInput &input,
		PostCombatRng &rng, uint8 nonAnimCount, PostCombatResult &result) {

	// Mirror-duel short-circuit (postcbt.pas:35-36): fixed 100 XP per level.
	if (input.duelFlag && input.mirrorDuel) {
		result.expShare = static_cast<uint32>(input.duelHighestLevel) * 100u;
		return;
	}

	uint32 expTotal = 0;
	uint8 cnt = 0; // capped count of random low-value inclusions (Pascal cnt)

	for (uint i = 0; i < input.combatants.size(); ++i) {
		const PostCombatCombatant &p = input.combatants[i];
		if (p.hostile == 1 && p.status != Goldbox::Data::S_RUNNING) {
			for (int d = 0; d < Goldbox::Data::VALUABLE_COUNT; ++d)
				result.treasureMoney[d] += p.coins[d];

			expTotal += p.xpAward +
				static_cast<uint32>(p.xpBonusPerHp) *
				static_cast<uint32>(p.hpRolled);

			if (!input.noCombatItems) {
				// Select this combatant's items (postcbt.pas:48-68). Valuable
				// items are always taken; otherwise a capped (cnt < 8) random
				// d10 <= 3 roll may include a low-value item. The RNG is only
				// consulted when value == 0, preserving the original call
				// order and count.
				for (uint k = 0; k < input.items.size(); ++k) {
					const PostCombatItem &item = input.items[k];
					if (item.ownerIndex != i)
						continue;
					bool include = false;
					if (item.value > 0) {
						include = true;
					} else if (cnt < 8 && rng.rollDice(1, 10) <= 3) {
						include = true;
					}
					if (include) {
						++cnt;
						result.lootItems.push_back(k);
					}
				}
			}
		}
	}

	// Currency XP conversions (postcbt.pas:72-78), computed in wide uint32.
	expTotal += result.treasureMoney[Goldbox::Data::VAL_COPPER]   / 200u;
	expTotal += result.treasureMoney[Goldbox::Data::VAL_SILVER]   / 20u;
	expTotal += result.treasureMoney[Goldbox::Data::VAL_ELECTRUM] / 2u;
	expTotal += result.treasureMoney[Goldbox::Data::VAL_GOLD];
	expTotal += result.treasureMoney[Goldbox::Data::VAL_PLATINUM] * 5u;
	expTotal += result.treasureMoney[Goldbox::Data::VAL_GEMS]     * 250u;
	expTotal += result.treasureMoney[Goldbox::Data::VAL_JEWELRY]  * 2200u;

	// Positive item-plus bonuses (postcbt.pas:79-84). The original stops at
	// lastMissileItem; selected items are in insertion order, so a missile
	// boundary item terminates the bonus scan.
	for (uint k = 0; k < result.lootItems.size(); ++k) {
		const PostCombatItem &item = input.items[result.lootItems[k]];
		if (item.isMissile)
			break;
		if (item.plus > 0)
			expTotal += static_cast<uint32>(item.plus) * 400u;
	}

	// share := expTotal div (PARTY_SIZE - nonAnimCount). Guard the invalid
	// zero denominator explicitly (the original would divide-by-zero); an
	// empty recipient set yields no share rather than trapping.
	const int denom = static_cast<int>(input.partySize) -
		static_cast<int>(nonAnimCount);
	result.expShare = (denom > 0) ? (expTotal / static_cast<uint32>(denom)) : 0u;
}

// --- AwardExperience (postcbt.pas:92) ---
//
// Awards the per-recipient share to every enabled, non-Animated combatant,
// applying the prime-stat bonus / multiclass split. Emits one XP award per
// recipient; the host adds it to experiencePoints.
static void awardExperience(const PostCombatInput &input, uint32 experience,
		PostCombatResult &result) {
	for (uint i = 0; i < input.combatants.size(); ++i) {
		const PostCombatCombatant &p = input.combatants[i];
		if (p.enabled && p.status != Goldbox::Data::S_ANIMATED) {
			PostCombatXpAward award;
			award.index = i;
			award.share = classAdjustedShare(p, experience);
			result.xpAwards.push_back(award);
		}
	}
}

// --- DetermineCombatResults (postcbt.pas:127) ---
//
// Classifies the outcome from the real party's status/hostility/NPC flags,
// handles status transitions (Dying->Unconscious, positive-HP recovery,
// running recovery), sets result 0/$81, and drives loot/XP when combat is won.
// Party entries precede monster entries in roster order, matching the
// `notPcNpc <> 1` while-guards.
static void determineCombatResults(const PostCombatInput &input,
		PostCombatRng &rng, PostCombatResult &result) {

	uint8 nonAnimCount = 0;
	bool partyDefeated = true;
	bool partyFled = false;
	bool combatWon = false;

	// First pass: any running party member flags a flight (postcbt.pas:135).
	for (uint i = 0; i < input.combatants.size(); ++i) {
		const PostCombatCombatant &p = input.combatants[i];
		if (p.isMonster)
			break; // past the party block
		if (p.status == Goldbox::Data::S_RUNNING)
			partyFled = true;
	}

	if (input.duelFlag)
		partyDefeated = false;

	if (!input.duelFlag) {
		// Classification pass over the party block (postcbt.pas:146-159).
		for (uint i = 0; i < input.combatants.size(); ++i) {
			const PostCombatCombatant &p = input.combatants[i];
			if (p.isMonster)
				break;

			const bool aliveish =
				(p.status == Goldbox::Data::S_RUNNING || p.status == Goldbox::Data::S_ANIMATED ||
				 p.status == Goldbox::Data::S_OKAY || input.mirrorDuel);
			if (aliveish && p.hostile == 0 &&
					(static_cast<uint8>(p.npc) < kNpcFlag))
				partyDefeated = false;

			if (p.status == Goldbox::Data::S_ANIMATED || p.status == Goldbox::Data::S_OKAY) {
				combatWon = true;
				partyFled = false;
			}

			if (!p.enabled || p.status == Goldbox::Data::S_ANIMATED)
				++nonAnimCount;
		}

		if (combatWon) {
			determineLootExp(input, rng, nonAnimCount, result);
			awardExperience(input, result.expShare, result);
		}

		result.combatWon = combatWon;
		result.partyFled = partyFled;
		result.partyDefeated = partyDefeated;

		if (!partyDefeated) {
			// Status recovery / result code (postcbt.pas:165-190).
			for (uint i = 0; i < input.combatants.size(); ++i) {
				const PostCombatCombatant &p = input.combatants[i];
				if (p.isMonster)
					break;

				if (!partyFled) {
					switch (p.status) {
					case Goldbox::Data::S_RUNNING: {
						PostCombatStatusChange ch;
						ch.index = i;
						ch.newStatus = Goldbox::Data::S_OKAY;
						ch.setEnabled = true;
						result.statusChanges.push_back(ch);
						break;
					}
					case Goldbox::Data::S_DYING: {
						PostCombatStatusChange ch;
						ch.index = i;
						ch.newStatus = Goldbox::Data::S_UNCONSCIOUS;
						result.statusChanges.push_back(ch);
						break;
					}
					case Goldbox::Data::S_UNCONSCIOUS:
						if (p.hpCur > 0) {
							PostCombatStatusChange ch;
							ch.index = i;
							ch.newStatus = Goldbox::Data::S_OKAY;
							ch.setEnabled = true;
							result.statusChanges.push_back(ch);
						}
						break;
					default:
						break;
					}
				} else {
					// Flight branch (postcbt.pas:179-187): result 0x81.
					result.result = 0x81;
					if (p.status == Goldbox::Data::S_RUNNING) {
						PostCombatStatusChange ch;
						ch.index = i;
						ch.newStatus = Goldbox::Data::S_OKAY;
						ch.setEnabled = true;
						result.statusChanges.push_back(ch);
					} else {
						// Non-running member on a flight: removed.
						PostCombatStatusChange ch;
						ch.index = i;
						ch.remove = true;
						result.statusChanges.push_back(ch);
					}
				}
			}
		} else {
			// Defeat branch (postcbt.pas:191-203): remove the party block and
			// wipe PARTY_SIZE, stopping at the first monster record.
			for (uint i = 0; i < input.combatants.size(); ++i) {
				const PostCombatCombatant &p = input.combatants[i];
				if (p.isMonster)
					break;
				PostCombatStatusChange ch;
				ch.index = i;
				ch.remove = true;
				result.statusChanges.push_back(ch);
			}
			result.partyWiped = true;
		}
	} else {
		// Duel branch (postcbt.pas:204-222): a single surviving friendly OKAY
		// member wins and loots; all OKAY/Animated members re-enable and Dying
		// becomes Unconscious across the full list.
		for (uint i = 0; i < input.combatants.size(); ++i) {
			const PostCombatCombatant &p = input.combatants[i];
			if (p.enabled && p.status == Goldbox::Data::S_OKAY && p.hostile != 1) {
				combatWon = true;
				determineLootExp(input, rng, nonAnimCount, result);
				awardExperience(input, result.expShare, result);
			}
		}
		for (uint i = 0; i < input.combatants.size(); ++i) {
			const PostCombatCombatant &p = input.combatants[i];
			if (p.status == Goldbox::Data::S_OKAY || p.status == Goldbox::Data::S_ANIMATED) {
				PostCombatStatusChange ch;
				ch.index = i;
				ch.newStatus = p.status;
				ch.setEnabled = true;
				result.statusChanges.push_back(ch);
			}
			if (p.status == Goldbox::Data::S_DYING) {
				PostCombatStatusChange ch;
				ch.index = i;
				ch.newStatus = Goldbox::Data::S_UNCONSCIOUS;
				result.statusChanges.push_back(ch);
			}
		}
		result.combatWon = combatWon;
		result.partyFled = false;
		result.partyDefeated = false;
	}
}

// --- PostCombatCreatureCleanup (postcbt.pas:453) ---
//
// Counts monster/hostile records that are not enabled as MONSTERS_DEFEATED.
// The actual record removal (RemoveCreature / FreeMem) is the host's job
// (clearMonsters), so the core only produces the defeated count; the host
// runs it in the original order relative to NPC shares.
static void postCombatCreatureCleanup(const PostCombatInput &input,
		PostCombatResult &result) {
	uint16 defeated = 0;
	for (uint i = 0; i < input.combatants.size(); ++i) {
		const PostCombatCombatant &p = input.combatants[i];
		if (p.isMonster || p.hostile == 1) {
			if (!p.enabled)
				++defeated;
		}
	}
	result.monstersDefeated = defeated;
}

// --- NpcTreasureShare (postcbt.pas:482) ---
//
// Removes the NPC cut from the pooled money AFTER cleanup. `npcShares` and
// `total` are BYTE accumulators in the original; the per-denomination cut is a
// BYTE computed as treasureMoney[i] div total. This byte narrowing is kept
// explicit (uint8) even though the pool itself is widened (GB13-027), because
// the original truncates the cut to a byte before multiplying by npcShares.
static void npcTreasureShare(const PostCombatInput &input,
		PostCombatResult &result) {
	uint8 npcShares = 0; // Pascal Byte
	uint8 total = 0;     // Pascal Byte

	for (uint i = 0; i < input.combatants.size(); ++i) {
		const PostCombatCombatant &p = input.combatants[i];
		// NpcTreasureShare runs AFTER PostCombatCreatureCleanup, which already
		// removed every monster / hostile record from the creatureList. Skip
		// those here so `total` counts only the surviving party/NPC members,
		// matching the shortened post-cleanup list.
		if (p.isMonster || p.hostile == 1)
			continue;

		// C_Npc.FLAG set: npc > $7F and status OKAY.
		if ((static_cast<uint8>(p.npc) > 0x7F) && p.status == Goldbox::Data::S_OKAY) {
			const uint8 share = static_cast<uint8>(p.npcTreasureShare & 7);
			npcShares = static_cast<uint8>(npcShares + share); // byte wrap
			total = static_cast<uint8>(total + share);         // byte wrap
		} else {
			total = static_cast<uint8>(total + 1);             // byte wrap
		}
	}

	result.npcShareCount = npcShares;
	if (npcShares == 0 || total == 0)
		return;

	for (int d = Goldbox::Data::VAL_COPPER; d <= Goldbox::Data::VAL_JEWELRY; ++d) {
		if (result.treasureMoney[d] > 0) {
			// cut is a Byte in the original: treasureMoney[i] div total then
			// truncated to 8 bits before Dec(treasureMoney[i], npcShares*cut).
			const uint8 cut = static_cast<uint8>(
				result.treasureMoney[d] / total);
			const uint32 taken = static_cast<uint32>(npcShares) *
				static_cast<uint32>(cut);
			const uint32 clamped =
				(taken > result.treasureMoney[d]) ? result.treasureMoney[d]
												  : taken;
			result.treasureMoney[d] -= clamped;
			result.npcShareTaken[d] = clamped;
		}
	}
}

// --- PostCombat driver (postcbt.pas:536) ---
PostCombatResult settlePostCombat(const PostCombatInput &input,
		PostCombatRng &rng) {
	PostCombatResult result;
	result.result = 0; // COMBAT_RESULT := 0

	// No-combat scripted treasure: stage the scripted money for the loot UI,
	// select the scripted items, and run the loot pipeline once without a
	// combat outcome or combat XP. The result still flows through the loot /
	// summary stage in the host exactly once.
	if (input.noCombat) {
		for (int d = 0; d < Goldbox::Data::VALUABLE_COUNT; ++d)
			result.treasureMoney[d] = input.scriptedMoney[d];
		for (uint k = 0; k < input.items.size(); ++k)
			result.lootItems.push_back(k);
		result.combatWon = false;
		result.partyDefeated = false;
		return result;
	}

	determineCombatResults(input, rng, result);
	postCombatCreatureCleanup(input, result);

	// NPC shares run only when the party was not defeated, or in a duel
	// (postcbt.pas:546-547). On a true defeat the result code becomes $80.
	if (!result.partyDefeated || input.duelFlag) {
		npcTreasureShare(input, result);
	} else {
		result.result = 0x80;
	}

	return result;
}

PostCombatInput buildNoCombatInput(const uint32 scriptedMoney[Goldbox::Data::VALUABLE_COUNT],
		const Common::Array<PostCombatItem> &items, uint8 partySize) {
	PostCombatInput input;
	input.noCombat = true;
	input.partySize = partySize;
	for (int d = 0; d < Goldbox::Data::VALUABLE_COUNT; ++d)
		input.scriptedMoney[d] = scriptedMoney[d];
	input.items = items;
	return input;
}

} // namespace Poolrad
} // namespace Goldbox
