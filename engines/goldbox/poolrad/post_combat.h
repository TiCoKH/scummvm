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

#ifndef GOLDBOX_POOLRAD_POST_COMBAT_H
#define GOLDBOX_POOLRAD_POST_COMBAT_H

#include "common/array.h"
#include "common/scummsys.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Poolrad {

/**
 * GB13-010 — post-combat settlement core (pure).
 *
 * This module is a deterministic reconstruction of the DOS post-combat
 * pipeline (postcbt.pas): DetermineCombatResults (postcbt.pas:127) →
 * DetermineLootExp (postcbt.pas:26) → AwardExperience (postcbt.pas:92) →
 * PostCombatCreatureCleanup (postcbt.pas:453) → NpcTreasureShare
 * (postcbt.pas:482), driven from PostCombat (postcbt.pas:536).
 *
 * The core operates ONLY over an immutable PostCombatInput snapshot plus an
 * injectable RNG (for the capped random low-value-item inclusion). It never
 * touches g_engine, the live party, the TreasurePool or any UI, so every coin
 * conversion, class/prime-stat split, multiclass integer division, random
 * loot cap, NO_COMBAT_ITEMS flag, last-missile boundary and byte-sized NPC
 * share is unit-assertable without a live engine.
 *
 * The host (poolrad_engine_host_impl) builds the input from the captured
 * BattleOutcomeSnapshot while the monster roster is still live, runs the core,
 * then applies the PostCombatResult to the TreasurePool, the party XP fields
 * and the TreasureDialog / GS_AFTER_COMBAT loot UI. The no-monster scripted
 * treasure branch builds its input through the separate no-combat adapter
 * (buildNoCombatInput) because ordinary combat records may be absent.
 */

/** Injectable dice source so the random loot inclusion is testable. */
struct PostCombatRng {
	virtual ~PostCombatRng() {}
	/** Mirrors FxEngine.RollDice(count, sides): sum of @p count d@p sides. */
	virtual int rollDice(int count, int sides) = 0;
};

/**
 * One item carried by a defeated combatant, copied by value from the live
 * inventory at capture time. `value` and `plus` mirror the Pascal item
 * record's value / plus (enchantment bonus) fields used by DetermineLootExp.
 * `sourceIndex` lets the host map a selected loot entry back to the live item.
 *
 * The original builds a prepended linked list and scans it until the
 * lastMissileItem pointer; that linked-list lifetime pattern is replaced here
 * by an ordered array plus an `isMissile` boundary flag (safe ID-based
 * iteration), so the plus-bonus scan stops at the first missile-flagged
 * selected item rather than at a raw pointer.
 */
struct PostCombatItem {
	uint16 value;        // item value (Pascal item^.value)
	uint8  plus;         // enchantment bonus (Pascal item^.plus)
	uint8  isMissile;    // 1 if this is the last-missile boundary item
	uint32 ownerIndex;   // index of the owning combatant in PostCombatInput
	uint32 sourceIndex;  // index of this item within the owner's inventory

	PostCombatItem()
		: value(0), plus(0), isMissile(0), ownerIndex(0), sourceIndex(0) {}
};

/**
 * One combatant row in roster order. Classification mirrors the Pascal record:
 * `isMonster` is combatAddr^.notPcNpc (temporary monster / summoned creature),
 * `hostile` is p^.hostile, `npc` is the raw npc byte (bit 7 = NPC flag), and
 * `npcTreasureShare` is the per-share cut byte.
 */
struct PostCombatCombatant {
	bool   isMonster;          // notPcNpc == 1 (host-owned monster record)
	bool   enabled;            // p^.enabled
	uint8  status;             // StatusADnD (S_OKAY/S_RUNNING/S_DYING/...)
	uint8  hostile;            // p^.hostile (0 = friendly, 1 = hostile)
	int8   npc;                // raw npc byte (bit 7 = NPC flag)
	uint8  npcTreasureShare;   // per-share cut (only &7 is used)

	uint8  classType;          // ClassADnD (for the XP class split)
	uint8  wisCur;             // current Wisdom  (cleric prime stat)
	uint8  strCur;             // current Strength (fighter prime stat)
	uint8  intCur;             // current Intelligence (mage prime stat)
	uint8  dexCur;             // current Dexterity (thief prime stat)

	uint32 xpAward;            // p^.xpAward (monster base XP)
	uint8  xpBonusPerHp;       // p^.xpBonusPerHp
	uint8  hpRolled;           // p^.hpRolled
	int16  hpCur;              // p^.hpCur (positive-HP recovery check)

	uint32 coins[Goldbox::Data::VALUABLE_COUNT]; // p^.coins[COPPER..JEWELRY]

	PostCombatCombatant()
		: isMonster(false), enabled(false), status(Goldbox::Data::S_OKAY),
		  hostile(0), npc(0), npcTreasureShare(0), classType(Goldbox::Data::C_FIGHTER),
		  wisCur(0), strCur(0), intCur(0), dexCur(0),
		  xpAward(0), xpBonusPerHp(0), hpRolled(0), hpCur(0) {
		for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i)
			coins[i] = 0;
	}
};

/**
 * Immutable settlement input. Roster order matches the live creatureList: the
 * party/NPC entries come first (isMonster == false), the host monster records
 * (isMonster == true) follow, which is what the original `notPcNpc <> 1`
 * while-guards rely on.
 */
struct PostCombatInput {
	Common::Array<PostCombatCombatant> combatants;
	Common::Array<PostCombatItem>      items; // all lootable items, in order

	bool   duelFlag;     // GDVars.duelFlag
	bool   mirrorDuel;   // zone1[MIRROR_DUEL] != 0
	bool   noCombatItems; // zone1[NO_COMBAT_ITEMS] == 1
	uint8  partySize;    // zone1[PARTY_SIZE]
	uint8  duelHighestLevel; // curChar^.levelHighest (duel XP = *100)

	// No-combat scripted treasure: when true, combatants is empty and the
	// scripted currency sits in scriptedMoney. The driver stages the money
	// into the pool and runs the loot pipeline once WITHOUT awarding combat
	// XP (there is no defeated monster to award from).
	bool   noCombat;
	uint32 scriptedMoney[Goldbox::Data::VALUABLE_COUNT];

	PostCombatInput()
		: duelFlag(false), mirrorDuel(false), noCombatItems(false),
		  partySize(0), duelHighestLevel(0), noCombat(false) {
		for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i)
			scriptedMoney[i] = 0;
	}
};

/** Status transition the host must apply back to a live combatant. */
struct PostCombatStatusChange {
	uint32 index;     // combatant index in PostCombatInput
	uint8  newStatus; // StatusADnD value to set
	bool   setEnabled; // true -> enabled = 1 (never cleared here)
	bool   remove;    // true -> RemoveCreature (defeat / duel teardown)

	PostCombatStatusChange()
		: index(0), newStatus(Goldbox::Data::S_OKAY), setEnabled(false),
		  remove(false) {}
};

/** XP gained by one eligible recipient (applied to experiencePoints). */
struct PostCombatXpAward {
	uint32 index; // combatant index in PostCombatInput
	uint32 share; // XP to add

	PostCombatXpAward() : index(0), share(0) {}
};

/**
 * Deterministic output of the settlement core. The host applies it to the
 * TreasurePool (treasureMoney / lootItems), the party XP fields (xpAwards),
 * and the roster (statusChanges). Result/monstersDefeated mirror the VM
 * globals COMBAT_RESULT and MONSTERS_DEFEATED.
 */
struct PostCombatResult {
	uint8 result;           // 0 / 0x81 (flight) / 0x80 (defeat)
	uint16 monstersDefeated;
	bool  combatWon;
	bool  partyDefeated;
	bool  partyFled;
	bool  partyWiped;       // defeat path zeroed PARTY_SIZE

	uint32 expShare;        // per-recipient base share (DetermineLootExp)
	uint32 treasureMoney[Goldbox::Data::VALUABLE_COUNT]; // pooled currency before shares

	// Loot items selected for the pool (indices into PostCombatInput.items).
	Common::Array<uint32> lootItems;

	// NPC treasure cut actually removed from treasureMoney per denomination.
	uint32 npcShareTaken[Goldbox::Data::VALUABLE_COUNT];
	uint8  npcShareCount;   // sum of (npcTreasureShare & 7) over eligible NPCs

	Common::Array<PostCombatXpAward>      xpAwards;
	Common::Array<PostCombatStatusChange> statusChanges;

	PostCombatResult()
		: result(0), monstersDefeated(0), combatWon(false),
		  partyDefeated(false), partyFled(false), partyWiped(false),
		  expShare(0), npcShareCount(0) {
		for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i) {
			treasureMoney[i] = 0;
			npcShareTaken[i] = 0;
		}
	}
};

/** 10% prime-stat bonus threshold: a stat STRICTLY above 15 earns it. */
static const uint8 kPrimeStatThreshold = 15;

/**
 * Run the full settlement over @p input, rolling the capped random low-value
 * item inclusion through @p rng. Pure: does not read or mutate engine state.
 * The returned result is complete and ready for the host to apply.
 */
PostCombatResult settlePostCombat(const PostCombatInput &input,
		PostCombatRng &rng);

/**
 * No-combat adapter: scripted treasure with no monster roster. The ordinary
 * combat records are absent, so this builds an input that carries ONLY the
 * scripted currency (and, optionally, scripted item plus-bonuses via @p items)
 * with no combatants, so settlePostCombat yields the currency XP total without
 * touching a (non-existent) roster. partySize gives the XP divisor. This keeps
 * the no-monster branch on its own input rather than borrowing combat records.
 */
PostCombatInput buildNoCombatInput(const uint32 scriptedMoney[Goldbox::Data::VALUABLE_COUNT],
		const Common::Array<PostCombatItem> &items, uint8 partySize);

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_POST_COMBAT_H
