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

#include "goldbox/data/items/random_item.h"

namespace Goldbox {
namespace Data {
namespace Items {

// Scroll base-item typeIndex (mirrors CharacterItem::isScroll /
// SCROLL_TYPE_INDEX in character_item.cpp).
static const uint8 kScrollTypeIndex = 63;

// ---------------------------------------------------------------------------
// Category roll (rollRandomMagicItemType)
//
// Ported verbatim from poolrad_engine_host_impl.cpp. Roll order and every
// threshold are preserved; only VmInterface::rollDice is replaced by the
// injected source so tests can walk the full decision tree deterministically.
// ---------------------------------------------------------------------------
uint8 rollRandomMagicItemType(IItemRollSource &source) {
	int roll = source.rollDice(1, 100);

	if (roll >= 1 && roll <= 60) {
		int roll2 = source.rollDice(1, 100);
		if (roll2 >= 1 && roll2 <= 47)
			return static_cast<uint8>(roll2);
		if (roll2 == 48 || roll2 == 49)
			return 59;
		if (roll2 >= 50 && roll2 <= 59)
			return static_cast<uint8>(roll2);
		if (roll2 >= 60 && roll2 <= 90) {
			int roll3 = source.rollDice(1, 10);
			if (roll3 >= 1 && roll3 <= 4) return 36;
			if (roll3 >= 5 && roll3 <= 7) return 35;
			if (roll3 == 8) return 34;
			if (roll3 == 9) return 37;
			return 38;
		}
		if (roll2 >= 91 && roll2 <= 94) return 73;
		if (roll2 >= 95 && roll2 <= 97) return 93;
		if (roll2 >= 98 && roll2 <= 100) return 77;
	} else if (roll >= 61 && roll <= 85) {
		return 61;
	} else if (roll >= 86 && roll <= 92) {
		return 62;
	} else if (roll >= 93 && roll <= 98) {
		int roll2 = source.rollDice(1, 16);
		if (roll2 >= 1 && roll2 <= 7)  return 71;
		if (roll2 == 8 || roll2 == 9)  return 70;
		if (roll2 == 10) return 84;
		if (roll2 == 11 || roll2 == 12) return 78;
		if (roll2 == 13 || roll2 == 14) return 79;
		if (roll2 == 15) return 92;
		return 67;
	} else {
		return 69;
	}

	return 36;
}

// ---------------------------------------------------------------------------
// RandomBonus
//
// REVIEW-OPEN: the exact DOS plus-distribution thresholds and per-item charge
// counts are reference data that could not be byte-confirmed from the sources
// available in-repo. The table below follows the standard Gold Box 1d100 plus
// distribution (common +1, rarer +2/+3, scarce +4/+5) and a 1d10+10 charge
// roll for charged items. It is applied deterministically so tests pin the
// boundaries; the specific numbers are flagged for review and are isolated
// here so a confirmed table drops in without touching callers.
// ---------------------------------------------------------------------------
BonusRoll rollRandomBonus(IItemRollSource &source) {
	BonusRoll out;

	int roll = source.rollDice(1, 100);
	// Plus distribution (boundaries REVIEW-OPEN):
	//   01-50 -> +1   51-75 -> +2   76-90 -> +3   91-98 -> +4   99-100 -> +5
	if (roll <= 50)
		out.bonus = 1;
	else if (roll <= 75)
		out.bonus = 2;
	else if (roll <= 90)
		out.bonus = 3;
	else if (roll <= 98)
		out.bonus = 4;
	else
		out.bonus = 5;

	// Wearables carry the same saving-throw plus as their enchantment plus.
	out.saveBonus = out.bonus;

	// Charge count for charged items: 1d10 + 10 (11..20). Boundary REVIEW-OPEN.
	out.charges = static_cast<uint8>(source.rollDice(1, 10) + 10);

	return out;
}

// ---------------------------------------------------------------------------
// Item classification by base-item slot. Confirmable from ItemProperty/Slot:
// weapons/armour/wearables take an enchantment plus; non-slot "device" items
// (slot S_NONE) are treated as charged magic items (wand/rod/staff/scroll).
// ---------------------------------------------------------------------------
static bool slotIsWearable(uint8 slotID) {
	return slotID < static_cast<uint8>(Slot::SLOT_COUNT);
}

static bool slotIsMissile(uint8 slotID) {
	return slotID >= static_cast<uint8>(Slot::S_ARROW) &&
	       slotID < static_cast<uint8>(Slot::SLOT_COUNT);
}

CharacterItem generateRandomItem(uint8 typeIndex, IItemRollSource &source,
		const Storage &props) {
	// Start from a fully zero-initialised record; every field is then set
	// explicitly so no generated item ships with an uninitialised field
	// (the original bug was a half-filled stub).
	CharacterItem item;
	item.name        = Common::String();
	item.nextAddress = 0;
	item.typeIndex   = typeIndex;
	item.nameCode1   = 0;
	item.nameCode2   = 0;
	item.nameCode3   = 0;
	item.bonus       = 0;
	item.saveBonus   = 0;
	item.readied     = 0;
	item.hidden      = 0;
	item.cursed      = 0;
	item.weight      = 0;
	item.stackSize   = 1;
	item.value       = 0;
	item.effect1     = 0;
	item.effect2     = 0;
	item.effect3     = 0;

	// Read the base template (never mutated). The ItemProperty default ctor
	// leaves slotID == 0 (S_MAIN_HAND), so an out-of-range typeIndex falls back
	// to the wearable branch; it still produces a fully-populated item.
	ItemProperty base;
	const Common::Array<ItemProperty> &all = props.all();
	if (typeIndex < all.size())
		base = all[typeIndex];

	const bool isScroll   = (typeIndex == kScrollTypeIndex);
	const bool isWearable = slotIsWearable(base.slotID) && !isScroll;
	const bool isMissile  = slotIsMissile(base.slotID);

	BonusRoll roll = rollRandomBonus(source);

	// Base weight/value. REVIEW-OPEN: the DOS per-type weight/value tables are
	// not byte-confirmed; we assign a non-zero, type-plausible baseline so the
	// instance is tradeable/saveable, scaled by the enchantment for wearables.
	if (isWearable) {
		item.weight = isMissile ? 1 : 30;        // light missile vs. worn item
		item.value  = static_cast<uint16>(50 + roll.bonus * 100);
	} else {
		item.weight = 5;                          // wand/rod/staff/scroll
		item.value  = static_cast<uint16>(200 + roll.charges * 25);
	}

	if (isWearable) {
		// Enchanted weapon/armour: plus + matching save plus. Missile stacks.
		item.bonus     = roll.bonus;
		item.saveBonus = roll.saveBonus;
		if (isMissile)
			item.stackSize = static_cast<uint8>(source.rollDice(1, 10) + 10);
		// Name component marks the enchantment type (identification payload).
		item.nameCode1 = typeIndex;
	} else {
		// Charged device (wand/rod/staff/scroll): effect1 = charges remaining.
		// bonus doubles as "max charges" for wands in the display path
		// (CharacterItem::getDisplayName).
		item.effect1 = roll.charges;
		if (isScroll) {
			// Scrolls: charges in effect1, no spell-id/special payload here;
			// the spell is chosen interactively via SpellsMenu at use time.
			item.effect2 = 0;
			item.effect3 = 0;
		} else {
			// Wand/rod/staff multi-payload: effect1 = charges remaining,
			// effect2 = spell id, effect3 < 128 so CharacterItem::getCharges()
			// recognises it as a charged device (effect3 >= 128 is reserved for
			// the separate "special effect" item kind and would DISABLE the
			// charge read, so it must stay clear here).
			// REVIEW-OPEN: the per-type spell-id mapping is reference data; we
			// derive a stable id from the category so the payload is non-empty
			// and getMagicItemSpellId() returns a usable id.
			item.effect2 = typeIndex;
			item.effect3 = 0;
			item.bonus   = roll.charges;         // max charges (display path)
		}
		item.nameCode1 = typeIndex;
	}

	// Identification state: a freshly dropped magic item starts UNIDENTIFIED.
	// Hide the magic name components and the bonus suffix (bits 0-2 + bit 3,
	// per CharacterItem::getDisplayName); Detect/Identify clears these later
	// WITHOUT this generator rerolling the item.
	if (item.bonus > 0 || item.saveBonus > 0 || item.effect1 > 0 || item.cursed)
		item.hidden = 0x0F;

	return item;
}

CharacterItem generateRandomItem(IItemRollSource &source, const Storage &props) {
	const uint8 typeIndex = rollRandomMagicItemType(source);
	return generateRandomItem(typeIndex, source, props);
}

} // namespace Items
} // namespace Data
} // namespace Goldbox
