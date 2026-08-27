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

#include "goldbox/data/adnd_character.h"
#include "common/debug.h"

namespace Goldbox {
namespace Data {

uint8 ADnDCharacter::getClassMaskForClassType(uint8 classType) {
    using namespace Goldbox::Data;

    switch (classType) {
    case C_CLERIC:
        return CF_CLERIC;
    case C_DRUID:
        return CF_DRUID;
    case C_FIGHTER:
        return CF_FIGHTER;
    case C_PALADIN:
        return CF_PALADIN;
    case C_RANGER:
        return CF_RANGER;
    case C_MAGICUSER:
        return CF_MAGICUSER;
    case C_THIEF:
        return CF_THIEF;
    case C_MONK:
        return CF_MONK;
    case C_CLERIC_FIGHTER:
        return CF_CLERIC | CF_FIGHTER;
    case C_CLERIC_FIGHTER_MAGICUSER:
        return CF_CLERIC | CF_FIGHTER | CF_MAGICUSER;
    case C_CLERIC_RANGER:
        return CF_CLERIC | CF_RANGER;
    case C_CLERIC_MAGICUSER:
        return CF_CLERIC | CF_MAGICUSER;
    case C_CLERIC_THIEF:
        return CF_CLERIC | CF_THIEF;
    case C_FIGHTER_MAGICUSER:
        return CF_FIGHTER | CF_MAGICUSER;
    case C_FIGHTER_THIEF:
        return CF_FIGHTER | CF_THIEF;
    case C_FIGHTER_MAGICUSER_THIEF:
        return CF_FIGHTER | CF_MAGICUSER | CF_THIEF;
    case C_MAGICUSER_THIEF:
        return CF_MAGICUSER | CF_THIEF;
    case C_MONSTER:
        return CF_ALL;
    default:
        break;
    }

    return CF_ALL;
}

void ADnDCharacter::clearEquippedItems() {
    equippedItems.clear();
}

void ADnDCharacter::resolveEquippedItems() {
	clearEquippedItems();
    const Common::List<Goldbox::Data::Items::CharacterItem> &itemsArr =
        inventory.items();

    int readiedCount = 0;
    int placedCount = 0;
    debug(4, "ADnDCharacter::resolveEquippedItems: totalItems=%u",
          (unsigned)itemsArr.size());

	// Resolve equipped items using readied flags and slotID from item properties.
	// This is the only reliable method for platform-independent save/load.
	// Legacy DOS segment:offset pointers cannot be restored across platforms.
    uint i = 0;
    for (const Goldbox::Data::Items::CharacterItem &item : itemsArr) {
		const uint itemIdx = i;
		++i;
		if (!item.isEquipped())
			continue;
        ++readiedCount;

		const auto &prop = item.prop();
		int slot = (int)prop.slotID;
        Common::String displayName = item.getDisplayName();
        debug(4, "ADnDCharacter::resolveEquippedItems: readied item[%u] displayName='%s' type=%u slot=%d prop.slotID=%u readied=%u nameCode=[%u,%u,%u]",
    		      (unsigned)itemIdx, displayName.c_str(), (unsigned)item.typeIndex, slot, (unsigned)prop.slotID, (unsigned)item.readied,
		      (unsigned)item.nameCode1, (unsigned)item.nameCode2, (unsigned)item.nameCode3);
		if (slot < 0 || slot >= EQUIPPED_SLOT_COUNT) {
			debug(4, "WARNING: Equipped item '%s' has invalid slotID %d (corrupted save data?)",
				  item.name.c_str(), slot);
			continue;
		}

		// For multiple-slot items (rings), place in first available slot
		if (slot == (int)Goldbox::Data::Items::Slot::S_RING1 || 
			slot == (int)Goldbox::Data::Items::Slot::S_RING2) {
			if (!equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING1]) {
				equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING1] = 
					const_cast<Goldbox::Data::Items::CharacterItem *>(&item);
                ++placedCount;
			} else if (!equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING2]) {
				equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING2] = 
					const_cast<Goldbox::Data::Items::CharacterItem *>(&item);
                ++placedCount;
            } else {
                debug(4, "ADnDCharacter::resolveEquippedItems: extra ring equipped, cannot place item[%u] name='%s'",
			      (unsigned)itemIdx, item.name.c_str());
			}
		} else if (!equippedItems.slots[slot]) {
			// For other slots, direct assignment
			equippedItems.slots[slot] = const_cast<Goldbox::Data::Items::CharacterItem *>(&item);
            ++placedCount;
        } else {
            debug(4, "ADnDCharacter::resolveEquippedItems: slot %d already occupied, item[%u] name='%s' skipped",
		          slot, (unsigned)itemIdx, item.name.c_str());
		}
	}

    debug(4, "ADnDCharacter::resolveEquippedItems: readied=%d placed=%d",
          readiedCount, placedCount);
}

void ADnDCharacter::buildLegacyOffsetsFromEquipped(uint32 *offsetsOut) const {
    if (!offsetsOut)
        return;
    for (int i = 0; i < EQUIPPED_SLOT_COUNT; ++i)
        offsetsOut[i] = 0;

    const Common::List<Goldbox::Data::Items::CharacterItem> &itemsArr =
        inventory.items();

    // Prefer explicit equippedItems mapping when available
    for (int slot = 0; slot < EQUIPPED_SLOT_COUNT; ++slot) {
        const auto *ci = equippedItems.slots[slot];
        if (ci)
            offsetsOut[slot] = ci->nextAddress;
    }

    // If a slot is still 0, infer from readied items
    for (const Goldbox::Data::Items::CharacterItem &item : itemsArr) {
        if (!item.isEquipped())
            continue;
        const Goldbox::Data::Items::ItemProperty &prop = item.prop();
        int slot = (int)prop.slotID;
        if (slot >= 0 && slot < EQUIPPED_SLOT_COUNT && offsetsOut[slot] == 0)
            offsetsOut[slot] = item.nextAddress;
    }
}

Goldbox::Data::Items::CharacterItem *ADnDCharacter::getEquippedItem(Goldbox::Data::Items::Slot slot) {
    int idx = (int)slot;
    if (idx < 0 || idx >= EQUIPPED_SLOT_COUNT)
        return nullptr;
    return equippedItems.slots[idx];
}

const Goldbox::Data::Items::CharacterItem *ADnDCharacter::getEquippedItem(Goldbox::Data::Items::Slot slot) const {
    int idx = (int)slot;
    if (idx < 0 || idx >= EQUIPPED_SLOT_COUNT)
        return nullptr;
    return equippedItems.slots[idx];
}

const Goldbox::Data::Items::ItemProperty *ADnDCharacter::getEquippedProp(Goldbox::Data::Items::Slot slot) const {
    const Goldbox::Data::Items::CharacterItem *ci = getEquippedItem(slot);
    if (!ci)
        return nullptr;
    return &ci->prop();
}

void ADnDCharacter::debugValidateEquipped() const {
    const Common::List<Goldbox::Data::Items::CharacterItem> &itemsArr =
        inventory.items();
    for (int s = 0; s < EQUIPPED_SLOT_COUNT; ++s) {
        const auto *ci = equippedItems.slots[s];
        bool pointerMatches = false;
        if (ci) {
            // Ensure ci actually exists in inventory array
            for (const Goldbox::Data::Items::CharacterItem &item : itemsArr) {
                if (&item == ci) {
                    pointerMatches = true;
                    break;
                }
            }
            if (!pointerMatches)
                debug(4, "Equipped mismatch: slot %d has dangling pointer", s);
        }
        // Cross-check readied flags: find any item whose prop slotID == s and isEquipped
        bool anyReadiedForSlot = false;
        for (const Goldbox::Data::Items::CharacterItem &item : itemsArr) {
            if (!item.isEquipped())
                continue;
            const Goldbox::Data::Items::ItemProperty &prop = item.prop();
            if ((int)prop.slotID == s) {
                anyReadiedForSlot = true;
                break;
            }
        }
        if (anyReadiedForSlot && !ci)
            debug(4, "Equipped mismatch: slot %d has readied item but equippedItems pointer is null", s);
        if (!anyReadiedForSlot && ci)
            debug(4, "Equipped mismatch: slot %d has pointer set but no readied item", s);
    }
}

bool ADnDCharacter::hasShieldEquipped() const {
    using Slot = Goldbox::Data::Items::Slot;
    const Goldbox::Data::Items::CharacterItem *off = getEquippedItem(Slot::S_OFF_HAND);
    if (!off) return false;
    const auto &prop = off->prop();
    // Heuristic: treat any OFF_HAND item with protect > 0 and hands == 1 as a shield
    return prop.slotID == (uint8)Slot::S_OFF_HAND && prop.protect > 0 && prop.hands == 1;
}

bool ADnDCharacter::isDualWielding() const {
    using Slot = Goldbox::Data::Items::Slot;
    const Goldbox::Data::Items::CharacterItem *mainIt = getEquippedItem(Slot::S_MAIN_HAND);
    const Goldbox::Data::Items::CharacterItem *offIt  = getEquippedItem(Slot::S_OFF_HAND);
    if (!mainIt || !offIt) return false;
    // Not counting shield as dual-wield
    if (hasShieldEquipped()) return false;
    // Basic heuristic: both items have wpnType != 0 (weapon) or damage dice > 0
    const auto &mp = mainIt->prop();
    const auto &op = offIt->prop();
    bool mainIsWeapon = (mp.dmgSmallMed.dices > 0 || mp.dmgLarge.dices > 0);
    bool offIsWeapon  = (op.dmgSmallMed.dices > 0 || op.dmgLarge.dices > 0);
    return mainIsWeapon && offIsWeapon;
}

const Goldbox::Data::Items::ItemProperty *ADnDCharacter::mainWeaponProp() const {
    using Slot = Goldbox::Data::Items::Slot;
    return getEquippedProp(Slot::S_MAIN_HAND);
}

bool ADnDCharacter::getRangedAttackItem(
        Goldbox::Data::Items::CharacterItem **attackItem) {
    using namespace Goldbox::Data::Items;

    if (!attackItem)
        return false;

    *attackItem = nullptr;
    CharacterItem *weapon = getEquippedItem(Slot::S_MAIN_HAND);
    uint8 missileType = 0;

    if (weapon) {
        missileType = weapon->prop().missileType;

        // The weapon itself is the projectile (for example, a thrown dart).
        if (missileType & static_cast<uint8>(MissileFlag::MF_DART))
            *attackItem = weapon;

        // Bow/crossbow weapons use a separate arrow/bolt item.
        if (missileType & static_cast<uint8>(MissileFlag::MF_BOW)) {
            if (missileType & static_cast<uint8>(MissileFlag::MF_RANGED_MELEE))
                *attackItem = getEquippedItem(Slot::S_ARROW);

            if (missileType & static_cast<uint8>(MissileFlag::MF_CROSSBOW))
                *attackItem = getEquippedItem(Slot::S_BOLT);
        }
    }

    // Legacy missile type 10 is valid even without a separate projectile.
    return *attackItem != nullptr || missileType == 10;
}

Goldbox::Data::Items::CharacterItem *ADnDCharacter::getWeaponOrAmmo() {
    using namespace Goldbox::Data::Items;

    CharacterItem *weapon = getEquippedItem(Slot::S_MAIN_HAND);
    if (!weapon)
        return nullptr;

    CharacterItem *attackItem = nullptr;
    if (!getRangedAttackItem(&attackItem) || !attackItem)
        return weapon;

    return attackItem;
}

void ADnDCharacter::setItemProtection(const Goldbox::Data::Items::CharacterItem *item,
                                      AcComponents *acComponents,
                                      bool *magicArmorWorn) {
    using Slot = Goldbox::Data::Items::Slot;

    if (!item)
        return;

    const auto &prop = item->prop();
    uint8 baseProtectionValue = prop.protect;

    // Check if the 8th bit is set (value >= 0x80).
    // This is a flag to see if the item offers protection at all.
    if (!(baseProtectionValue & 0x80))
        return;

    // Mask the value to get the lower 7 bits (the actual base AC).
    uint8 baseAC = baseProtectionValue & 0x7F;
    Slot itemSlot = static_cast<Slot>(prop.slotID);

    // Case 1: Item is a Shield (or in the off-hand slot)
    if (itemSlot == Slot::S_OFF_HAND) {
        // Shield AC = Base Shield AC + Magic Bonus
        acComponents->shield = baseAC + item->bonus;
        return;
    }

    // Case 2: Item is protection without base AC (e.g., Ring of Protection, Cloak)
    if (baseAC == 0) {
        if (itemSlot == Slot::S_RING1 || itemSlot == Slot::S_RING2) {
            // For rings, only the best bonus applies, it doesn't stack.
            if (item->bonus > acComponents->ring)
                acComponents->ring = item->bonus;
        } else {
            // For cloaks, belts, boots, etc., the bonus stacks with other items.
            int combined = (int)acComponents->misc + (int)item->bonus;
            acComponents->misc = static_cast<uint8>(CLIP<int>(combined, 0, 255));
        }

        // Any save bonus from the item is added to the character's total.
        saveBonus += item->saveBonus;
        return;
    }

    // Case 3: Item is Body Armor with a base AC value
    {
        int armorProtection = (int)baseAC + (int)item->bonus;
        if (armorProtection > (int)acComponents->armorBase) {
            acComponents->armorBase = static_cast<uint8>(CLIP<int>(armorProtection, 0, 255));
            if (item->bonus > 0 && itemSlot == Slot::S_BODY_ARMOR)
                *magicArmorWorn = true;
        }
        return;
    }
}

void ADnDCharacter::recalcCombatStats() {
	const EquipScanResult scan = rebuildEquipmentSlots();
	recalculateEncumbrance(scan);
	resetCombatModifiers();
	AcComponents ac;
	ac.dexAdj = getDexDefenceBonus();
	applyWeaponAndAbilityModifiers();
	applyEquippedItemModifiers(ac);
	applyEffectStateModifiers(ac);
	finalizeArmorClass(ac);
	attackLevel = (levels.levels[C_FIGHTER] > 0 && race > 0) ? levels.levels[C_FIGHTER] : 1;
}

ADnDCharacter::EquipScanResult ADnDCharacter::rebuildEquipmentSlots() {
	using namespace Goldbox::Data::Items;

	handsEquipped = 0;
	numOfItems = static_cast<int8>(inventory.items().size());
	equippedItems.clear();

	EquipScanResult result = {0, 0, false};
	uint i = 0;
	for (const CharacterItem &ci : inventory.items()) {
		const uint itemIdx = i++;
		uint32 w = ci.weight;
		if (ci.stackSize != 0)
			w *= ci.stackSize;
		result.totalWeight += w;

		if (!ci.isEquipped())
			continue;

		CharacterItem *ptr = const_cast<CharacterItem *>(&ci);
		const ItemProperty &p = ci.prop();
		bool placed = false;
		const int sid = (int)p.slotID;

		if (sid >= 0 && sid < 9) {
			if (!equippedItems.slots[sid]) {
				equippedItems.slots[sid] = ptr;
				placed = true;
			}
		} else if (sid == 9) {
			if (!equippedItems.slots[(int)Slot::S_RING1]) {
				equippedItems.slots[(int)Slot::S_RING1] = ptr;
				placed = true;
			} else if (!equippedItems.slots[(int)Slot::S_RING2]) {
				equippedItems.slots[(int)Slot::S_RING2] = ptr;
				placed = true;
			} else {
				debug(4, "ADnDCharacter::rebuildEquipmentSlots: extra ring cannot be placed idx=%u type=%u",
					  (unsigned)itemIdx, (unsigned)ci.typeIndex);
			}
		}
		if (!placed && ci.typeIndex == 73 && !equippedItems.slots[(int)Slot::S_ARROW]) {
			equippedItems.slots[(int)Slot::S_ARROW] = ptr;
			placed = true;
		}
		if (!placed && ci.typeIndex == 28 && !equippedItems.slots[(int)Slot::S_BOLT]) {
			equippedItems.slots[(int)Slot::S_BOLT] = ptr;
			placed = true;
		}

		if (placed) {
			result.equippedWeight += w;
			handsEquipped = static_cast<uint8>(handsEquipped + p.hands);
			if (ci.nameCode1 == 186)
				result.bagOfHolding = true;
		}
	}

	handsEquipped = static_cast<uint8>(MIN<uint32>(0xFFu, handsEquipped));
	result.totalWeight += valuableItems.getTotalWeight();
	return result;
}

void ADnDCharacter::recalculateEncumbrance(const EquipScanResult &scan) {
	uint32 total = static_cast<uint32>(CLIP<int32>(
		(int32)scan.totalWeight + (int32)effectState.mods.encumbrance, 0, 0x7FFFFFFF));

	if (scan.bagOfHolding) {
		if (total < 5000)
			total = 0;
		else
			total -= 5000;
		if (total < scan.equippedWeight)
			total = scan.equippedWeight;
	}
	encumbrance = static_cast<uint16>(MIN<uint32>(0xFFFFu, total));
}

void ADnDCharacter::resetCurrentRollsFromBase() {
	curPrimaryRoll   = basePrimaryRoll;
	curSecondaryRoll = baseSecondaryRoll;
}

void ADnDCharacter::resetCombatModifiers() {
	resetCurrentRollsFromBase();
	saveBonus = 0;
	armorClass.resetToBase();
	movement.resetToBase();
	thac0.resetToBase();
}

void ADnDCharacter::applyWeaponAndAbilityModifiers() {
	// Default: no weapon modifiers. Game-specific subclasses override.
}

void ADnDCharacter::applyEquippedItemModifiers(AcComponents &ac) {
	bool hasMagicArmor = false;
	for (int slot = 0; slot < EQUIPPED_SLOT_COUNT; ++slot) {
		if (auto *item = equippedItems.slots[slot]) {
			armorMovementEffect(item);
			setItemProtection(item, &ac, &hasMagicArmor);
		}
	}
	if (hasMagicArmor)
		ac.ring = 0;
}

void ADnDCharacter::applyEffectStateModifiers(AcComponents & /*ac*/) {
	// Default: no effect state modifiers. Game-specific subclasses override.
}

void ADnDCharacter::armorMovementEffect(const Goldbox::Data::Items::CharacterItem *item) {
	using Slot = Goldbox::Data::Items::Slot;
	if (!item || item->prop().slotID != (uint8)Slot::S_BODY_ARMOR)
		return;

	const uint16 w = item->weight;
	if (w < 151)
		movement.current = movement.base;
	else if (w <= 399)
		movement.current = 6;
	else
		movement.current = 9;

	if (item->bonus != 0 && movement.current < 10)
		movement.current += 3;
}

void ADnDCharacter::finalizeArmorClass(AcComponents &ac) {
	setMovement();
	if (ac.armorBase < armorClass.current)
		ac.armorBase = armorClass.current;
	armorClass.current = static_cast<uint8>(ac.getTotalAC());
}

int ADnDCharacter::getCapacityModifier() const {
	int t = getStrengthTier();
	if (t <= 3)  return -350;
	if (t <= 5)  return -250;
	if (t <= 7)  return -150;
	if (t <= 11) return 0;
	if (t <= 13) return 100;
	if (t <= 15) return 200;
	if (t == 16) return 350;
	if (t <= 21) return 500 + (t - 17) * 250;
	if (t <= 26) return 2000 + (t - 22) * 1000;
	if (t == 27) return 7500;
	if (t <= 30) return 9000 + (t - 28) * 3000;
	return 0;
}

void ADnDCharacter::setMovement() {
    int capacityModShort = getCapacityModifier();

    // Effective encumbrance after capacity modifier, clamped to 0.
    int effectiveEnc = static_cast<int>(encumbrance) - capacityModShort;
    if (effectiveEnc < 0)
        effectiveEnc = 0;

    // Determine movement cap based on thresholds, default is no change.
    uint8 cap = movement.current; // no cap if within the lowest tier
    if (effectiveEnc > 1024) {
        cap = 3;
    } else if (effectiveEnc > 768) {
        cap = 6;
    } else if (effectiveEnc > 512) {
        cap = 9;
    }

    // Only reduce movement; never increase.
    if (cap < movement.current) {
        movement.current = cap;
    }
}

Goldbox::Data::Items::CharacterInventory::Rules
ADnDCharacter::buildInventoryRules(uint16 maxEncumbrance, uint8 maxItems) const {
    Goldbox::Data::Items::CharacterInventory::Rules rules;
    rules.allowedClassMask = getClassMaskForClassType(classType);
    rules.maxEncumbrance = maxEncumbrance;
    rules.capacityModifier = getCapacityModifier();
    rules.maxItems = maxItems;
    return rules;
}

bool ADnDCharacter::canCarryItem(const Goldbox::Data::Items::CharacterItem &item,
                                 uint16 maxEncumbrance,
                                 uint8 maxItems) const {
    const Goldbox::Data::Items::CharacterInventory::Rules rules =
        buildInventoryRules(maxEncumbrance, maxItems);
    return inventory.canCarryItem(item, rules);
}

bool ADnDCharacter::addItem(const Goldbox::Data::Items::CharacterItem &item,
                            uint16 maxEncumbrance,
                            uint8 maxItems) {
    const Goldbox::Data::Items::CharacterInventory::Rules rules =
        buildInventoryRules(maxEncumbrance, maxItems);
    return inventory.addItem(item, rules, &equippedItems.slots);
}

bool ADnDCharacter::removeItem(Goldbox::Data::Items::CharacterItem *item) {
    if (!inventory.removeItem(item, &equippedItems.slots))
        return false;
    inventory.recomputeEquippedTotals(equippedItems.slots,
                                      &handsEquipped, &saveBonus);
    return true;
}

bool ADnDCharacter::equipItem(Goldbox::Data::Items::CharacterItem *item,
                              Goldbox::Data::Items::Slot slot) {
    return inventory.equipItem(item, slot, equippedItems.slots,
                               &handsEquipped, &saveBonus);
}

bool ADnDCharacter::unequipItem(Goldbox::Data::Items::Slot slot) {
    return inventory.unequipItem(slot, equippedItems.slots,
                                 &handsEquipped, &saveBonus);
}

ADnDCharacter::ReadyItemResult ADnDCharacter::canReadyItem(
    const Goldbox::Data::Items::CharacterItem *item,
    const Goldbox::Data::Items::CharacterItem **conflictingItem) const {
    using namespace Goldbox::Data::Items;

    if (conflictingItem)
        *conflictingItem = nullptr;

    if (!item)
        return RIR_INVALID;

    const ItemProperty &prop = item->prop();
    const Slot itemSlot = static_cast<Slot>(prop.slotID);

    // Match original precedence: hands -> slot conflict -> extra conflict -> class.
    uint8 result = RIR_SUCCESS;

    if (!ignoreHandsLimitForReady() &&
        ((uint)handsEquipped + (uint)prop.hands > 2)) {
        result = RIR_HANDS_FULL;
    }

    const CharacterItem *conflict = nullptr;
    if (itemSlot < Slot::S_RING1) {
        conflict = equippedItems.slots[(int)itemSlot];
        if (conflict)
            result = RIR_SLOT_IN_USE;
    } else if (itemSlot == Slot::S_RING1) {
        conflict = equippedItems.slots[(int)Slot::S_RING2];
        if (conflict)
            result = RIR_SLOT_IN_USE;
    }

    const CharacterItem *extraConflict = getExtraReadyConflictItem(item);
    if (extraConflict) {
        conflict = extraConflict;
        result = RIR_SLOT_IN_USE;
    }

    const uint8 allowedMask = getReadyAllowedClassMask();
    if ((allowedMask & prop.classMask) == 0)
        result = RIR_WRONG_CLASS;

    if (result == RIR_SLOT_IN_USE && conflictingItem)
        *conflictingItem = conflict;

    return static_cast<ReadyItemResult>(result);
}

ADnDCharacter::ReadyItemResult ADnDCharacter::toggleReadyItem(
    Goldbox::Data::Items::CharacterItem *item,
    const Goldbox::Data::Items::CharacterItem **conflictingItem) {
    using namespace Goldbox::Data::Items;

    if (conflictingItem)
        *conflictingItem = nullptr;

    if (!item)
        return RIR_INVALID;

    if (item->readied == 0) {
        const ReadyItemResult canReady = canReadyItem(item, conflictingItem);
        if (canReady != RIR_SUCCESS)
            return canReady;

        const Slot slot = static_cast<Slot>(item->prop().slotID);
        if (!equipItem(item, slot))
            return RIR_SLOT_IN_USE;

        onReadyItemEffect(item, true);
        return RIR_SUCCESS;
    }

    if (item->cursed)
        return RIR_CURSED;

    Slot slot = Slot::S_NONE;
    for (int i = 0; i < EQUIPPED_SLOT_COUNT; ++i) {
        if (equippedItems.slots[i] == item) {
            slot = static_cast<Slot>(i);
            break;
        }
    }

    if (slot != Slot::S_NONE) {
        if (!unequipItem(slot))
            return RIR_CURSED;
    } else {
        item->readied = 0;
        resolveEquippedItems();
    }

    onReadyItemEffect(item, false);
    return RIR_SUCCESS;
}

uint8 ADnDCharacter::getReadyAllowedClassMask() const {
    return getClassMaskForClassType(classType);
}

bool ADnDCharacter::ignoreHandsLimitForReady() const {
    return false;
}

const Goldbox::Data::Items::CharacterItem *ADnDCharacter::getExtraReadyConflictItem(
    const Goldbox::Data::Items::CharacterItem * /*item*/) const {
    return nullptr;
}

void ADnDCharacter::onReadyItemEffect(
    Goldbox::Data::Items::CharacterItem * /*item*/, bool /*equipping*/) {
    // Default: no special equip/unequip side effects.
}

bool ADnDCharacter::isNpc() const {
    return npc >= 0x80;
}

} // namespace Data
} // namespace Goldbox
