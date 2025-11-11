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
 */

#include "common/debug.h"
#include "goldbox/vm_interface.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

using namespace Goldbox::Data;

uint8 PoolradCharacter::countActiveBaseClasses() const {
	const Common::Array<uint8> &lvls = levels.levels;
	uint8 count = 0;
	for (uint i = 0; i < lvls.size() && i < BASE_CLASS_NUM; ++i) {
		if (lvls[i] > 0)
			++count;
	}
	return count;
}

void PoolradCharacter::computeSavingThrows() {
	using Goldbox::Data::Rules::savingThrowsAt;

	SavingThrows best;
	best.vsParalysis     = 20;
	best.vsPetrification = 20;
	best.vsRodStaffWand  = 20;
	best.vsBreathWeapon  = 20;
	best.vsSpell         = 20;

	const Common::Array<uint8> &lvls = levels.levels;
	const uint sz = MIN<uint>((uint)lvls.size(), BASE_CLASS_NUM);
	for (uint i = 0; i < sz; ++i) {
		uint8 lvl = lvls[i];
		if (lvl == 0)
			continue;

		const SavingThrows &st = savingThrowsAt((uint8)i, lvl);
		if (st.vsParalysis     < best.vsParalysis)     best.vsParalysis     = st.vsParalysis;
		if (st.vsPetrification < best.vsPetrification) best.vsPetrification = st.vsPetrification;
		if (st.vsRodStaffWand  < best.vsRodStaffWand)  best.vsRodStaffWand  = st.vsRodStaffWand;
		if (st.vsBreathWeapon  < best.vsBreathWeapon)  best.vsBreathWeapon  = st.vsBreathWeapon;
		if (st.vsSpell         < best.vsSpell)         best.vsSpell         = st.vsSpell;

		// Special dual-class quirk for class index 7 (Monk slot used in tables):
		if (i == 7) {
			uint8 oldLvl = highestLevel;
			if (oldLvl > 0 && lvl > oldLvl) {
				const SavingThrows &oldSt = savingThrowsAt(7, oldLvl);
				if (oldSt.vsParalysis     < best.vsParalysis)     best.vsParalysis     = oldSt.vsParalysis;
				if (oldSt.vsPetrification < best.vsPetrification) best.vsPetrification = oldSt.vsPetrification;
				if (oldSt.vsRodStaffWand  < best.vsRodStaffWand)  best.vsRodStaffWand  = oldSt.vsRodStaffWand;
				if (oldSt.vsBreathWeapon  < best.vsBreathWeapon)  best.vsBreathWeapon  = oldSt.vsBreathWeapon;
				if (oldSt.vsSpell         < best.vsSpell)         best.vsSpell         = oldSt.vsSpell;
			}
		}
	}

	savingThrows = best;
	debug("PoolradCharacter::computeSavingThrows -> P=%u Pe=%u R=%u B=%u S=%u",
		  (unsigned)savingThrows.vsParalysis,
		  (unsigned)savingThrows.vsPetrification,
		  (unsigned)savingThrows.vsRodStaffWand,
		  (unsigned)savingThrows.vsBreathWeapon,
		  (unsigned)savingThrows.vsSpell);
}

void PoolradCharacter::computeThac0() {
	uint8 bestThac0 = 0;
	const Common::Array<uint8> &lvls = levels.levels;
	for (uint i = 0; i < lvls.size() && i < BASE_CLASS_NUM; ++i) {
		uint8 lvl = lvls[i];
		if (lvl > 0) {
			int v = Goldbox::Data::Rules::thac0AtLevel((uint8)i, lvl);
			if (v > bestThac0)
				bestThac0 = (uint8)v;
		}
	}
	thac0.base = bestThac0;
	// Refresh item limit mask from active base classes
	itemsLimit = Goldbox::Data::Rules::computeItemLimitMask(levels.levels);

	debug("PoolradCharacter::computeThac0 -> base=%u actual=%d itemsLimit=%u",
		  (unsigned)thac0.base, 60 - (int)thac0.base, (unsigned)itemsLimit);
}

void PoolradCharacter::rollInitialAge() {
	const uint8 raceId = race;
	const uint8 forcedBase = Goldbox::Data::Rules::forcedBaseIndexForMulticlass(classType);

	if (forcedBase != 0xFF) {
		const Goldbox::Data::Rules::AgeDefEntry &adef = Goldbox::Data::Rules::getAgeDef(raceId, forcedBase);
		const uint16 maxExtra = (uint16)(adef.dices * adef.sides);
		age = adef.base + maxExtra;
		debug("PoolradCharacter::rollInitialAge forced base=%u -> age=%u", (unsigned)forcedBase, (unsigned)age);
		return;
	}

	if (classType < C_CLERIC_FIGHTER) {
		uint8 baseIdx = classType;
		const Goldbox::Data::Rules::AgeDefEntry &adef = Goldbox::Data::Rules::getAgeDef(raceId, baseIdx);
		uint16 extra = (uint16)VmInterface::rollDice(adef.dices, adef.sides);
		age = adef.base + extra;
		debug("PoolradCharacter::rollInitialAge single base=%u -> age=%u", (unsigned)baseIdx, (unsigned)age);
		return;
	}

	// Fallback: use fighter row
	{
		const uint8 baseIdx = (uint8)C_FIGHTER;
		const Goldbox::Data::Rules::AgeDefEntry &adef = Goldbox::Data::Rules::getAgeDef(raceId, baseIdx);
		uint16 extra = (uint16)VmInterface::rollDice(adef.dices, adef.sides);
		age = adef.base + extra;
		warning("PoolradCharacter::rollInitialAge fallback path used -> age=%u", (unsigned)age);
	}
}

void PoolradCharacter::applyAgeingEffects() {
	if (classType == C_MONSTER)
		return;

	const AgeCategories &cats = Goldbox::Data::Rules::getAgeCategoriesForRace(race);
	const Common::Array<AgeingEffects> &aeffects = Goldbox::Data::Rules::getStatAgeingEffects();

	int stageMax = -1;
	if (age > 0) {
		if (age < cats.young) stageMax = 0;
		else if (age < cats.adult) stageMax = 1;
		else if (age < cats.middle) stageMax = 2;
		else if (age < cats.old) stageMax = 3;
		else if (age < cats.venitiar) stageMax = 4;
		else stageMax = 4;
	}

	if (stageMax < 0)
		return;

	auto applyStage = [&](Stat &st, int statRow, int stageIdx) {
		if (statRow < 0 || statRow >= (int)aeffects.size()) return;
		const AgeingEffects &ae = aeffects[statRow];
		int delta = 0;
		switch (stageIdx) {
		case 0: delta = ae.young; break;
		case 1: delta = ae.adult; break;
		case 2: delta = ae.middle; break;
		case 3: delta = ae.old; break;
		case 4: delta = ae.venitiar; break;
		default: return;
		}
		if (delta == 0)
			return;
		int cur = (int)st.current + delta;
		const int maxVal = (statRow == 1) ? 100 : 25;
		if (cur < 0) cur = 0;
		if (cur > maxVal) cur = maxVal;
		st.current = (uint8)cur;
	};

	for (int s = 0; s <= stageMax; ++s) {
		applyStage(abilities.strength,      0, s);
		applyStage(abilities.strException,  1, s);
		applyStage(abilities.intelligence,  2, s);
		applyStage(abilities.wisdom,        3, s);
		applyStage(abilities.dexterity,     4, s);
		applyStage(abilities.constitution,  5, s);
		applyStage(abilities.charisma,      6, s);
	}
}

void PoolradCharacter::computeSpellSlots() {
	using Goldbox::Data::Spells::SpellEntry;
	using Goldbox::Data::Spells::SC_CLERIC;
	using Goldbox::Data::Spells::Spells;

	const uint8 wis = abilities.wisdom.current;

	// Cleric slots and known
	if (levels[Goldbox::Data::C_CLERIC] > 0) {
		spellSlots.cleric.level1 = 1;
		spellSlots.cleric.level2 = 0;
		spellSlots.cleric.level3 = 0;

		if (spellSlots.cleric.level1 > 0) {
			if (wis >= 13) spellSlots.cleric.level1 += 1;
			if (wis >= 14) spellSlots.cleric.level1 += 1;
		}
		if (spellSlots.cleric.level2 > 0) {
			if (wis >= 15) spellSlots.cleric.level2 += 1;
			if (wis >= 16) spellSlots.cleric.level2 += 1;
		}
		if (spellSlots.cleric.level3 > 0) {
			if (wis >= 17) spellSlots.cleric.level3 += 1;
		}

		const Common::Array<SpellEntry> &se = Goldbox::Data::Rules::getSpellEntries();
		for (uint i = 0; i < se.size(); ++i) {
			const SpellEntry &e = se[i];
			if (e.spellClass == SC_CLERIC && e.spellLevel >= 1 && e.spellLevel <= 3)
				setSpellKnown(static_cast<Spells>(i));
		}
	}

	// Magic-User slots and a few initial known spells
	if (levels[Goldbox::Data::C_MAGICUSER] > 0) {
		spellSlots.magicUser.level1 = 1;
		spellSlots.magicUser.level2 = 0;
		spellSlots.magicUser.level3 = 0;

		setSpellKnown(Goldbox::Data::Spells::SP_MUL1_DETECT_MAGIC);
		setSpellKnown(Goldbox::Data::Spells::SP_MUL1_READ_MAGIC);
		setSpellKnown(Goldbox::Data::Spells::SP_MUL1_SHIELD);
		setSpellKnown(Goldbox::Data::Spells::SP_MUL1_SLEEP);
	}
}

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

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

#include "common/stream.h"
#include "goldbox/vm_interface.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

// SpellBook abstraction removed; legacy arrays are used directly

	void PoolradCharacter::load(Common::SeekableReadStream &stream) {

		name = Goldbox::Data::PascalStringBuffer<15>::read(stream); // 0x000–0x00F
		abilities.strength.current     = stream.readByte(); // 0x010
		abilities.intelligence.current = stream.readByte(); // 0x011
		abilities.wisdom.current       = stream.readByte(); // 0x012
		abilities.dexterity.current    = stream.readByte(); // 0x013
		abilities.constitution.current = stream.readByte(); // 0x014
		abilities.charisma.current     = stream.readByte(); // 0x015
		abilities.strException.current = stream.readByte(); // 0x016

		// 0x017–0x02B: memorized spells
		stream.read(spells.memorizedSpells, 21);
		// 0x02C: unknown
		stream.seek(0x2D, SEEK_SET);
		combat.thac0Base = stream.readByte(); // 0x02D

		race       = stream.readByte(); // 0x02E
		classType  = stream.readByte(); // 0x02F
		age        = stream.readUint16LE(); // 0x030–0x031

		hitPoints.max = stream.readByte(); // 0x032

		// 0x033–0x06A: cleric/mage spell knowledge — 55 bytes
		stream.read(spells.knownSpells, 55);

		attackLevel = stream.readByte();   // 0x06B
		iconDimension = stream.readByte(); // 0x06C

		savingThrows.vsParalysis     = stream.readByte(); // 0x06D
		savingThrows.vsPetrification = stream.readByte(); // 0x06E
		savingThrows.vsRodStaffWand  = stream.readByte(); // 0x06F
		savingThrows.vsBreathWeapon  = stream.readByte(); // 0x070
		savingThrows.vsSpell         = stream.readByte(); // 0x071

		baseMovement = stream.readByte();      // 0x072
		highestLevel = stream.readByte();      // 0x073
		drainedLevels = stream.readByte();     // 0x074
		drainedHp = stream.readByte();         // 0x075
		undeadResistance = stream.readByte();  // 0x076

		thiefSkills.pickPockets      = stream.readByte(); // 0x077
		thiefSkills.openLocks        = stream.readByte(); // 0x078
		thiefSkills.findRemoveTraps  = stream.readByte(); // 0x079
		thiefSkills.moveSilently     = stream.readByte(); // 0x07A
		thiefSkills.hideInShadows    = stream.readByte(); // 0x07B
		thiefSkills.hearNoise        = stream.readByte(); // 0x07C
		thiefSkills.climbWalls       = stream.readByte(); // 0x07D
		thiefSkills.readLanguages    = stream.readByte(); // 0x07E

		// 0x07F–0x082: effects address (pointer)
		stream.seek(0x84, SEEK_SET);
		npc = stream.readSByte();      // 0x084
		modified = stream.readByte(); // 0x085
		// 0x086–0x087: unknown
		stream.seek(0x88, SEEK_SET);
		// 0x088–0x095: coin and item valuables
		for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i)
			valuableItems.values[i] = stream.readUint16LE();

		// 0x096–0x09D: class levels
		for (int i = 0; i < 8; ++i)
			levels.levels[i] = stream.readByte();

		// 0x09E–0x0A0: gender, type, alignment
		gender    = (Goldbox::Data::Gender)stream.readByte(); // 0x09E
		monsterType = stream.readByte();                     // 0x09F
		alignment = stream.readByte();                       // 0x0A0

		// 0x0A1–0x0A2: primary & secondary attacks ×2
		primaryAttacks   = stream.readByte();
		secondaryAttacks = stream.readByte();

		// 0x0A3–0x0A8: unarmed combat data
		priDmgDiceNum     = stream.readByte();
		secDmgDiceNum     = stream.readByte();
		priDmgDiceSides   = stream.readByte();
		secDmgDiceSides   = stream.readByte();
		priDmgModifier    = stream.readSByte();
		secDmgModifier    = stream.readSByte();

		// Initialize unified combat rolls model in base class
		setBaseRolls(primaryAttacks,
				    priDmgDiceNum,
				    priDmgDiceSides,
				    priDmgModifier,
				    secondaryAttacks,
				    secDmgDiceNum,
				    secDmgDiceSides,
				    secDmgModifier);

		armorClass.base = stream.readByte();          // 0x0A9
		strengthBonusAllowed = stream.readByte();     // 0x0AA
		combatIcon = stream.readByte();               // 0x0AB

		experiencePoints = stream.readUint32LE();     // 0x0AC–0x0AF

		itemsLimit = stream.readByte(); // 0x0B0:
		hitPointsRolled = stream.readByte(); // 0x0B1

	// 0x0B2–0x0B7: spell slot capacities (how many can be memorized)
	spellSlots.cleric.level1    = stream.readByte(); // cleric 1
	spellSlots.cleric.level2    = stream.readByte(); // cleric 2
	spellSlots.cleric.level3    = stream.readByte(); // cleric 3
	spellSlots.magicUser.level1 = stream.readByte(); // magic-user 1
	spellSlots.magicUser.level2 = stream.readByte(); // magic-user 2
	spellSlots.magicUser.level3 = stream.readByte(); // magic-user 3

		xpForDefeating = stream.readUint16LE(); // 0x0B8–0x0B9
		bonusXpPerHp = stream.readByte();       // 0x0BA

		portrait.head = stream.readByte(); // 0x0BB
		portrait.body = stream.readByte(); // 0x0BC

		iconData.iconHead = stream.readByte(); // 0x0BD
		iconData.iconBody = stream.readByte(); // 0x0BE
		orderNumber = stream.readByte();       // 0x0BF
		iconData.iconSize = stream.readByte(); // 0x0C0

		iconData.setBody(stream.readByte());     // 0x0C1
		iconData.setArm(stream.readByte());      // 0x0C2
		iconData.setLeg(stream.readByte());      // 0x0C3
		iconData.setHairFace(stream.readByte()); // 0x0C4
		iconData.setShield(stream.readByte());   // 0x0C5
		iconData.setWeapon(stream.readByte());   // 0x0C6

		numOfItems = stream.readByte();          // 0x0C7
		itemsAddress = stream.readUint32LE();    // 0x0C8–0x0CB — pointer

		// 0x0CC–0x0FF: equipped items (all pointers)
		for (int i = 0; i < EQUIPMENT_SLOT_COUNT; ++i)
			equippedOffsets[i] = stream.readUint32LE();

		handsUsed   = stream.readByte();       // 0x100
		saveBonus   = stream.readByte();       // 0x101
		encumbrance = stream.readUint16LE();   // 0x102–0x103

		stream.readUint32LE(); // 0x104–0x107: next character address (pointer)
		stream.readUint32LE(); // 0x108–0x10B: combat address (pointer)

		healthStatus = stream.readByte();        // 0x10C
		enabled      = (stream.readByte() != 0); // 0x10D
		hostile      = (stream.readByte() != 0); // 0x10E
		quickfight   = (stream.readByte() != 0); // 0x10F

		thac0.current        = stream.readByte(); // 0x110
		armorClass.current   = stream.readByte(); // 0x111
		acRear.current       = stream.readByte(); // 0x112

		curPriAttacks = stream.readByte(); // 0x113
		curSecAttacks = stream.readByte(); // 0x114

		curPriDiceNum   = stream.readByte(); // 0x115
		curSecDiceNum   = stream.readByte(); // 0x116
		curPriDiceSides = stream.readByte(); // 0x117
		curSecDiceSides = stream.readByte(); // 0x118
		curPriBonus     = stream.readSByte(); // 0x119
		curSecBonus     = stream.readSByte(); // 0x11A

		// Mirror current legacy values into unified model
		setCurrentRolls(primaryAttacks,
					   curPriDiceNum,
					   curPriDiceSides,
					   curPriBonus,
					   secondaryAttacks,
					   curSecDiceNum,
					   curSecDiceSides,
					   curSecBonus);

		hitPoints.current = stream.readByte(); // 0x11B
		movement.current  = stream.readByte(); // 0x11C
		// No SpellBook import; legacy arrays already populated above
	}

	PoolradCharacter::PoolradCharacter() {
		initialize();
	}

	void PoolradCharacter::initialize() {
		highestLevel = 0;
		creatureSize = 1;
		baseMovement = 0;
		hitDice = 0;
		drainedLevels = 0;
		drainedHp = 0;
		undeadResistance = 0;
		monsterType = 0;
		primaryAttacks = 0;
		secondaryAttacks = 0;
		priDmgDiceNum = 0;
		secDmgDiceNum = 0;
		priDmgDiceSides = 0;
		secDmgDiceSides = 0;
		priDmgModifier = 0;
		secDmgModifier = 0;
		strengthBonusAllowed = 0;
		combatIcon = 0;
		hitPointsRolled = 0;

		xpForDefeating = 0;
		bonusXpPerHp = 0;
		itemsLimit = 0;
		handsUsed = 0;
		encumbrance = 0;
		actions = 0;

		hostile = false;
		quickfight = false;
		npc = 0;
		modified = 0;
		curPriAttacks = 0;
		curSecAttacks = 0;
		curPriDiceNum = 0;
		curSecDiceNum = 0;
		curPriDiceSides = 0;
		curSecDiceSides = 0;
		curPriBonus = 0;
		curSecBonus = 0;
	}

	void PoolradCharacter::initializeNewCharacter() {
		// Base (likely DOS) defaults for a newly generated character.
		// Other platforms (Amiga / PC-98, etc.) can override this method to
		// customize icon color layout, starting combat stats, etc.
		iconData.setBody(getBaseIconColor(1));
		iconData.setArm(getBaseIconColor(2));
		iconData.setLeg(getBaseIconColor(3));
		iconData.setHairFace(getBaseIconColor(4));
		iconData.setShield(getBaseIconColor(5));
		iconData.setWeapon(getBaseIconColor(6));

		armorClass.base = 50; // TODO: verify platform-specific baseline
		thac0.base = 40;      // TODO: verify platform-specific baseline
		healthStatus = Goldbox::Data::S_OKAY;
		enabled = true;
		portrait.head = 1;
		portrait.body = 1;
		iconDimension = 1;
		// Random initial combat icon selection
		combatIcon = static_cast<uint8>(Goldbox::g_engine ? Goldbox::g_engine->getRandomNumber(256) : 0);
	}

	// Returns true if the character has any memorized spell
	bool PoolradCharacter::haveMemorizedSpell() const {
		for (int i = 0; i < 21; ++i)
			if (spells.memorizedSpells[i] > 0)
				return true;
		return false;
	}

	void PoolradCharacter::resolveEquippedItems() {
		for (int slot = 0; slot < EQUIPMENT_SLOT_COUNT; ++slot) {
			equipment.slots[slot] = -1;
			for (size_t i = 0; i < inventory.items().size(); ++i) {
				if (inventory.items()[i].nextAddress == equippedOffsets[slot]) {
					equipment.slots[slot] = static_cast<int>(i);
					break;
				}
			}
		}

		// Also populate the pointer-based equippedItems for runtime usage
		resolveEquippedFromLegacyOffsets(equippedOffsets);
	}

	void PoolradCharacter::recalcCombatStats() {
		using namespace Goldbox::Data::Items;

		handsUsed = 0;
		encumbrance = 0;
		numOfItems = static_cast<int8>(inventory.items().size());
		// Single pass: rebuild equippedItems and compute total/equipped weights and stats
		equippedItems.clear();
		const Common::Array<CharacterItem> &items = inventory.items();
		uint32 totalWeight = 0; // modern accumulation without 16-bit per-add clamping
		uint32 equippedOnlyWeight = 0; // track weight of equipped (readied) items (single final clamp)
		bool specialEncumbranceFlag = false; // bag-of-holding/cursed-like behavior (typeIndex == 0xBA)
		bool mainHandEquipped = false;
		uint8 totalProtect = 0; // aggregate item protection (CHARACTER_setItemProtection analogue)

		for (uint i = 0; i < items.size(); ++i) {
			const CharacterItem &ci = items[i];
			// Weight for this item (respect stack)
			uint32 w = ci.weight;
			if (ci.stackSize != 0)
				w *= ci.stackSize;
			totalWeight += w;

			if (!ci.isEquipped())
				continue;

			CharacterItem *ptr = const_cast<CharacterItem *>(&ci);
			const ItemProperty &p = ci.prop();
			bool placed = false;
			int sid = (int)p.slotID;

			// Place by slot when within range
			if (sid >= 0 && sid < 9) {
				if (!equippedItems.slots[sid]) { equippedItems.slots[sid] = ptr; placed = true; }
			} else if (sid == 9) {
				if (!equippedItems.slots[(int)Slot::S_RING1]) { equippedItems.slots[(int)Slot::S_RING1] = ptr; placed = true; }
				else if (!equippedItems.slots[(int)Slot::S_RING2]) { equippedItems.slots[(int)Slot::S_RING2] = ptr; placed = true; }
				else {
					// More than two rings with slot id 9 equipped — unexpected, log for diagnostics
					debug("PoolradCharacter::recalcCombatStats extra ring (slot id 9) cannot be placed: idx=%u type=%u", (unsigned)i, (unsigned)ci.typeIndex);
				}
			}
			// Arrow / Bolt by type index (legacy propID==73/28)
			if (!placed && ci.typeIndex == 73) {
				int a = (int)Slot::S_ARROW;
				if ( !equippedItems.slots[(int)Slot::S_ARROW] ) { equippedItems.slots[(int)Slot::S_ARROW] = ptr; placed = true; }
			}
			if (!placed && ci.typeIndex == 28) {
				if ( !equippedItems.slots[(int)Slot::S_BOLT]) { equippedItems.slots[(int)Slot::S_BOLT] = ptr; placed = true; }
			}

			// If successfully placed in any slot, contribute to equipped-only aggregates
			if (placed) {
				equippedOnlyWeight += w;
				// Count hands used (defer final clamp)
				handsUsed = static_cast<uint8>(handsUsed + p.hands);
				// Detect main-hand weapon presence
				if (p.slotID == (uint8)Slot::S_MAIN_HAND)
					mainHandEquipped = true;
				// Aggregate protection values (simple model; legacy code used CHARACTER_setItemProtection)
				if (p.protect > 0)
					totalProtect = static_cast<uint8>(CLIP<int>(totalProtect + p.protect, 0, 255));
				// Special container / encumbrance-modifying item by base type index (legacy 0xBA)
				if (ci.nameCode1 == 186) // 0xBA name component probably "Cursed"
					specialEncumbranceFlag = true;
			}
		}

		// Final clamp for handsUsed
		handsUsed = static_cast<uint8>(MIN<uint32>(0xFFu, handsUsed));

		// Add weight of valuables (coins, gems, jewelry): assume 1 unit weight each
		totalWeight += valuableItems.getTotalWeight();



		// Compute final encumbrance using modern 32-bit totals, then clamp once
		if (specialEncumbranceFlag) {
			uint32 enc = totalWeight;
			if (enc < 5000)
				enc = 0;
			else
				enc -= 5000;
			if (enc < equippedOnlyWeight)
				enc = equippedOnlyWeight;
			encumbrance = static_cast<uint16>(MIN<uint32>(0xFFFFu, enc));
		} else {
			encumbrance = static_cast<uint16>(MIN<uint32>(0xFFFFu, totalWeight));
		}

		// Base combat values
		thac0.current = thac0.base;

		// Dexterity defence bonus (reaction adjustment analogue) + item protection
		const int8 dexAdj = getDexDefenceBonus();
		int acWork = (int)armorClass.base + dexAdj - (int)totalProtect;
		armorClass.current = static_cast<uint8>(CLIP<int>(acWork, 0, 255));

		// Strength bonuses when unarmed (no main-hand item)
		if (!mainHandEquipped) {
			thac0.current = static_cast<uint8>(CLIP<int>(thac0.current + getStrengthBonus(), 0, 255));
			// Placeholder for melee damage bonus (legacy current_pri_mod); add to curPriBonus if desired
			// curPriBonus = static_cast<uint8>(CLIP<int>(curPriBonus + getStrengthBonus(), 0, 255));
		}

		// Rear/flanked AC approximation: -2 from front AC (legacy combined components logic)
		acRear.current = static_cast<uint8>(CLIP<int>(armorClass.current - 2, 0, 255));

		// Attack level heuristic (legacy used fighter level if race >0)
		attackLevel = (levels.levels[C_FIGHTER] > 0 && race > 0) ? levels.levels[C_FIGHTER] : 1;

		// Movement: default to base movement when available, else keep current
		if (baseMovement != 0)
			movement.current = baseMovement;

		debug("PoolradCharacter::recalcCombatStats -> handsUsed=%u enc=%u ac=%d thac0=%d rearAC=%d items=%u",
			  (unsigned)handsUsed,
			  (unsigned)encumbrance,
			  60 - (int)armorClass.current,
			  60 - (int)thac0.current,
			  60 - (int)acRear.current,
			  (unsigned)items.size());
	}

	void PoolradCharacter::rollAbilityScores() {
		abilities.strength.base = abilities.strength.current = VmInterface::rollDice(3, 6);
		abilities.intelligence.base = abilities.intelligence.current = VmInterface::rollDice(3, 6);
		abilities.wisdom.base       = abilities.wisdom.current       = VmInterface::rollDice(3, 6);
		abilities.dexterity.base    = abilities.dexterity.current    = VmInterface::rollDice(3, 6);
		abilities.constitution.base = abilities.constitution.current = VmInterface::rollDice(3, 6);
		abilities.charisma.base     = abilities.charisma.current     = VmInterface::rollDice(3, 6);
	}

	uint8 PoolradCharacter::getBaseIconColor(int index) {
		if (index < 0 || index >= 7)
			return 0;
		uint8 base = BASE_ICON_VALUES[index];
		return base + static_cast<uint8>((base + 8) * 0x10);
	}

	bool PoolradCharacter::meetsClassRequirements() const {
		using namespace Goldbox::Data;

		switch (classType) {
		case C_FIGHTER:
			return abilities.strength.current >= 9;
		case C_RANGER:
			return abilities.strength.current >= 13 &&
				   abilities.dexterity.current >= 13 &&
				   abilities.wisdom.current >= 14;
		case C_PALADIN:
			return abilities.strength.current >= 12 &&
				   abilities.charisma.current >= 17;
		// Add more cases
		default:
			return true;
		}
	}
// calculateHitPoints removed: character creation sets HP via CreateCharacterView::setInitHP()

	uint8 PoolradCharacter::getRolledHP(Goldbox::Data::ClassFlag flags) {
		// New behavior:
		// - Iterate base classes
		// - Include class if levels[base] > 0 and flag bit is set
		// - Roll per-class kHPRolls
		// - If level == 1, clamp roll to floor(2 * diceSides / 3)
		// - Sum all included rolls and set hitPointsRolled to that sum
		int sum = 0;
		const uint8 flagMask = static_cast<uint8>(flags);
		for (uint8 base = 0; base < BASE_CLASS_NUM; ++base) {
			const uint8 baseBit = (uint8)(1u << base);
			const uint8 lvl = levels.levels[base];
			if (lvl == 0)
				continue; // skip classes with no levels
			if ((flagMask & baseBit) == 0)
				continue; // skip classes not included by mask
			const Goldbox::Data::DiceRoll &dr = Goldbox::Data::Rules::getHPRoll(base);
			int roll = VmInterface::rollDice(dr.numDice, dr.diceSides);
			int used = roll;
			if (lvl == 1) {
				const int minFirstLevel = (2 * dr.diceSides) / 3;
				if (used < minFirstLevel)
					used = minFirstLevel;
				debug("getRolledHP: base=%u lvl=1 roll=%d min=%d used=%d", (unsigned)base, roll, minFirstLevel, used);
			} else {
				debug("getRolledHP: base=%u lvl=%u roll=%d used=%d", (unsigned)base, (unsigned)lvl, roll, used);
			}
			sum += used;
		}
		if (sum < 0) sum = 0;
		if (sum > 255) sum = 255;
		debug("getRolledHP: final sum=%d", sum);
		return (uint8)sum;
	}

	int8 PoolradCharacter::getConHPModifier() const {
		using namespace Goldbox::Data;
		// Base modifier from rules table (handles clamping)
		const uint8 con = abilities.constitution.current;
		int perSlot = (int)Rules::conHPModifier(con);

		// Fighter adjustment applies if the character has any Fighter level
		bool isFighter = false;
		if (levels.levels.size() > C_FIGHTER)
			isFighter = (levels.levels[C_FIGHTER] > 0);

		if (isFighter) {
			if (con >= 17) perSlot += 1;
			if (con >= 18) perSlot += 1; // total +2 on top of base at 18+
		}

		// Sum per active base-class slot (0..7)
		int total = 0;
		const Common::Array<uint8> &lvls = levels.levels;
		uint sz = (uint)lvls.size();
		if (sz > BASE_CLASS_NUM) sz = BASE_CLASS_NUM;
		for (uint8 base = 0; base < sz; ++base) {
			if (lvls[base] == 0)
				continue; // skip classes with no levels
			total += perSlot;
		}
		return total;
	}

	void PoolradCharacter::save(Common::WriteStream &stream) {

		// Write spells from legacy arrays directly
		Goldbox::Data::PascalStringBuffer<15>::write(stream, name);
		// Ability scores
		stream.writeByte(abilities.strength.current);
		stream.writeByte(abilities.intelligence.current);
		stream.writeByte(abilities.wisdom.current);
		stream.writeByte(abilities.dexterity.current);
		stream.writeByte(abilities.constitution.current);
		stream.writeByte(abilities.charisma.current);
		stream.writeByte(abilities.strException.current);

	// Spells memorized (21 bytes)
	stream.write(spells.memorizedSpells, 21);

		stream.writeByte(0); // Unknown at 0x02C

		// Combat
		stream.writeByte(combat.thac0Base);

		stream.writeByte(race);
		stream.writeByte(classType);
		stream.writeUint16LE(age);

		stream.writeByte(hitPoints.max);

	// Known spells (55 bytes in Poolrad layout)
	stream.write(spells.knownSpells, 55);

		stream.writeByte(attackLevel);
		stream.writeByte(iconDimension);

		// Saving throws
		stream.writeByte(savingThrows.vsParalysis);
		stream.writeByte(savingThrows.vsPetrification);
		stream.writeByte(savingThrows.vsRodStaffWand);
		stream.writeByte(savingThrows.vsBreathWeapon);
		stream.writeByte(savingThrows.vsSpell);

		stream.writeByte(baseMovement);
		stream.writeByte(highestLevel);
		stream.writeByte(drainedLevels);
		stream.writeByte(drainedHp);
		stream.writeByte(undeadResistance);

		// Thief skills
		stream.writeByte(thiefSkills.pickPockets);
		stream.writeByte(thiefSkills.openLocks);
		stream.writeByte(thiefSkills.findRemoveTraps);
		stream.writeByte(thiefSkills.moveSilently);
		stream.writeByte(thiefSkills.hideInShadows);
		stream.writeByte(thiefSkills.hearNoise);
		stream.writeByte(thiefSkills.climbWalls);
		stream.writeByte(thiefSkills.readLanguages);

		for (int i = 0; i < 4; ++i) stream.writeByte(0); // Skip effects address

		stream.writeByte(0); // Unknown at 0x083
		stream.writeSByte(npc);
		stream.writeByte(modified);

		for (int i = 0; i < 2; ++i) stream.writeByte(0);

		// Coins and valuables
		for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; ++i)
			stream.writeUint16LE(valuableItems.values[i]);

		// Class levels
		for (int i = 0; i < 8; ++i)
			stream.writeByte(levels.levels[i]);

		// Gender, Monster type, Alignment
		stream.writeByte(gender);
		stream.writeByte(monsterType);
		stream.writeByte(alignment);

		// Attacks
        stream.writeByte(primaryAttacks);
        stream.writeByte(secondaryAttacks);

        // Unarmed / base combat dice
        stream.writeByte(priDmgDiceNum);
        stream.writeByte(secDmgDiceNum);
        stream.writeByte(priDmgDiceSides);
        stream.writeByte(secDmgDiceSides);
		stream.writeSByte(priDmgModifier);
		stream.writeSByte(secDmgModifier);

		stream.writeByte(armorClass.base);
		stream.writeByte(strengthBonusAllowed);
		stream.writeByte(combatIcon);

		stream.writeUint32LE(experiencePoints);

		stream.writeByte(itemsLimit);
		stream.writeByte(hitPointsRolled);

	// Spell slots (cleric L1-3, then magic-user L1-3)
	stream.writeByte(spellSlots.cleric.level1);
	stream.writeByte(spellSlots.cleric.level2);
	stream.writeByte(spellSlots.cleric.level3);
	stream.writeByte(spellSlots.magicUser.level1);
	stream.writeByte(spellSlots.magicUser.level2);
	stream.writeByte(spellSlots.magicUser.level3);

		stream.writeUint16LE(xpForDefeating);
		stream.writeByte(bonusXpPerHp);

		stream.writeByte(portrait.head);
		stream.writeByte(portrait.body);

		stream.writeByte(iconData.iconHead);
		stream.writeByte(iconData.iconBody);
		stream.writeByte(orderNumber);
		stream.writeByte(iconData.iconSize);

		// Icon color data
		stream.writeByte((iconData.iconColorBody1 << 4) | iconData.iconColorBody2);
		stream.writeByte((iconData.iconColorArm1 << 4) | iconData.iconColorArm2);
		stream.writeByte((iconData.iconColorLeg1 << 4) | iconData.iconColorLeg2);
		stream.writeByte((iconData.iconColorHair << 4) | iconData.iconColorFace);
		stream.writeByte((iconData.iconColorShield1 << 4) | iconData.iconColorShield2);
		stream.writeByte((iconData.iconColorWeapon1 << 4) | iconData.iconColorWeapon2);

		stream.writeByte(numOfItems);
		stream.writeUint32LE(itemsAddress);

		// Equipped items legacy pointers: build from current equippedItems
		{
			uint32 legacy[EQUIPMENT_SLOT_COUNT];
			buildLegacyOffsetsFromEquipped(legacy);
			for (int i = 0; i < EQUIPMENT_SLOT_COUNT; ++i)
				stream.writeUint32LE(legacy[i]);
		}

		stream.writeByte(handsUsed);
		stream.writeByte(saveBonus);
		stream.writeUint16LE(encumbrance);

		stream.writeUint32LE(0); // next character address
		stream.writeUint32LE(0); // combat address

		stream.writeByte(healthStatus);
		stream.writeByte(enabled ? 1 : 0);
		stream.writeByte(hostile ? 1 : 0);
		stream.writeByte(quickfight ? 1 : 0);

		stream.writeByte(thac0.current);
		stream.writeByte(armorClass.current);
		stream.writeByte(acRear.current);

		stream.writeByte(curPriAttacks);
		stream.writeByte(curSecAttacks);

		// Current combat dice (mirror from unified model to preserve save format)
		stream.writeByte(curPriDiceNum);
		stream.writeByte(curSecDiceNum);
		stream.writeByte(curPriDiceSides);
		stream.writeByte(curSecDiceSides);
		stream.writeSByte(curPriBonus);
		stream.writeSByte(curSecBonus);

		stream.writeByte(hitPoints.current);
		stream.writeByte(movement.current);
	}

	bool PoolradCharacter::isSpellKnown(Goldbox::Data::Spells::Spells spell) const {
		uint8 id = static_cast<uint8>(spell);
		if (id == 0)
			return false;
		// Legacy known array covers ids 1..55
		if (id >= 1 && id <= 55)
			return spells.knownSpells[id - 1] != 0;
		// Extended ids not supported in legacy Poolrad format
		return false;
	}

	void PoolradCharacter::setSpellKnown(Goldbox::Data::Spells::Spells spell) {
		uint8 id = static_cast<uint8>(spell);
		if (id == 0)
			return;

		// Update legacy buffer when within saved range 1..55
		if (id >= 1 && id <= 55)
			spells.knownSpells[id - 1] = 1;

		// No SpellBook; legacy buffer already updated
	}

	byte PoolradCharacter::getNameColor() {
		if (name.size() == 0) return 0;
		int txtColor = 15;
		if (!enabled) {
			txtColor = 12;
		} else if (hostile) {
			 txtColor = 14;
		} else {
			 txtColor = 11;
		}
		return txtColor;
	}


} // namespace Data
} // namespace Poolrad
} // namespace Goldbox
