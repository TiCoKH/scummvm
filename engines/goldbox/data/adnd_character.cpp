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

namespace Goldbox {
namespace Data {

void ADnDCharacter::clearEquippedItems() {
    equippedItems.clear();
}

void ADnDCharacter::resolveEquippedFromLegacyOffsets(const uint32 *offsets) {
    if (!offsets)
        return;
    clearEquippedItems();
    const Common::Array<Goldbox::Data::Items::CharacterItem> &itemsArr = inventory.items();
    for (int slot = 0; slot < EQUIPPED_SLOT_COUNT; ++slot) {
        uint32 wanted = offsets[slot];
        if (wanted == 0) {
            equippedItems.slots[slot] = nullptr;
            continue;
        }
        for (uint i = 0; i < itemsArr.size(); ++i) {
            if (itemsArr[i].nextAddress == wanted) {
                equippedItems.slots[slot] = const_cast<Goldbox::Data::Items::CharacterItem *>(&itemsArr[i]);
                break;
            }
        }
    }
    using Slot = Goldbox::Data::Items::Slot;
}

void ADnDCharacter::buildLegacyOffsetsFromEquipped(uint32 *offsetsOut) const {
    if (!offsetsOut)
        return;
    for (int i = 0; i < EQUIPPED_SLOT_COUNT; ++i)
        offsetsOut[i] = 0;

    const Common::Array<Goldbox::Data::Items::CharacterItem> &itemsArr = inventory.items();

    // Prefer explicit equippedItems mapping when available
    for (int slot = 0; slot < EQUIPPED_SLOT_COUNT; ++slot) {
        const auto *ci = equippedItems.slots[slot];
        if (ci)
            offsetsOut[slot] = ci->nextAddress;
    }

    // If a slot is still 0, infer from readied items
    for (uint i = 0; i < itemsArr.size(); ++i) {
        if (!itemsArr[i].isEquipped())
            continue;
        const auto &prop = itemsArr[i].prop();
        int slot = (int)prop.slotID;
        if (slot >= 0 && slot < EQUIPPED_SLOT_COUNT && offsetsOut[slot] == 0)
            offsetsOut[slot] = itemsArr[i].nextAddress;
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
    const Common::Array<Goldbox::Data::Items::CharacterItem> &itemsArr = inventory.items();
    for (int s = 0; s < EQUIPPED_SLOT_COUNT; ++s) {
        const auto *ci = equippedItems.slots[s];
        bool pointerMatches = false;
        if (ci) {
            // Ensure ci actually exists in inventory array
            for (uint i = 0; i < itemsArr.size(); ++i) {
                if (&itemsArr[i] == ci) {
                    pointerMatches = true;
                    break;
                }
            }
            if (!pointerMatches)
                debug("Equipped mismatch: slot %d has dangling pointer", s);
        }
        // Cross-check readied flags: find any item whose prop slotID == s and isEquipped
        bool anyReadiedForSlot = false;
        for (uint i = 0; i < itemsArr.size(); ++i) {
            if (!itemsArr[i].isEquipped())
                continue;
            const auto &prop = itemsArr[i].prop();
            if ((int)prop.slotID == s) {
                anyReadiedForSlot = true;
                break;
            }
        }
        if (anyReadiedForSlot && !ci)
            debug("Equipped mismatch: slot %d has readied item but equippedItems pointer is null", s);
        if (!anyReadiedForSlot && ci)
            debug("Equipped mismatch: slot %d has pointer set but no readied item", s);
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

void ADnDCharacter::armorMovementEffect(const Goldbox::Data::Items::CharacterItem *armorItem) {
    using Slot = Goldbox::Data::Items::Slot;
    if (!armorItem) return; // No armor equipped, no effect

    const auto &prop = armorItem->prop();
    // Verify this is actually body armor
    if (prop.slotID == (uint8)Slot::S_BODY_ARMOR) {
        // Apply movement effect based on weight and magical bonus
        const uint16 armorWeight = armorItem->weight;

        // Calculate movement penalty based on weight brackets
        if (armorWeight < 151) {
            // Light armor: no penalty
            movement.current = movement.base;
        } else if (armorWeight <= 399) {
            // Medium armor: fixed movement of 9
            movement.current = 9;
        } else {
            // Heavy armor: fixed movement of 6
            movement.current = 6;
        }
        // Magical bonus: if armor is enchanted and movement was reduced, grant +3
        if (armorItem->bonus != 0 && movement.current < 10)
            movement.current += 3;
    }
    return;
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
        int8 totalArmorAC = baseAC + item->bonus;
        // Update the character's armor value only if this piece is better.
        if (totalArmorAC > acComponents->armorBase) {
            acComponents->armorBase = totalArmorAC;

            // If this is magical body armor, set the 'is_magic_armor_worn' flag.
            // This flag might be used elsewhere to negate other bonuses (e.g., from a Ring of Protection).
            if (itemSlot == Slot::S_BODY_ARMOR && item->bonus > 0) {
                *magicArmorWorn = true;
            }
        }
        return;
    }
}

// ---- Combat rolls helpers ----
void ADnDCharacter::setBaseRolls(uint8 priAttacks, uint8 priNum, uint8 priSides, int8 priMod,
                                uint8 secAttacks, uint8 secNum, uint8 secSides, int8 secMod) {
    basePrimaryRoll.attacks  = priAttacks;
    basePrimaryRoll.rolls    = priNum;
    basePrimaryRoll.dice     = priSides;
    basePrimaryRoll.modifier = priMod;

    baseSecondaryRoll.attacks  = secAttacks;
    baseSecondaryRoll.rolls    = secNum;
    baseSecondaryRoll.dice     = secSides;
    baseSecondaryRoll.modifier = secMod;
}

void ADnDCharacter::setCurrentRolls(uint8 curPriAttacks, uint8 curPriNum, uint8 curPriSides, int8 curPriMod,
                                   uint8 curSecAttacks, uint8 curSecNum, uint8 curSecSides, int8 curSecMod) {
    curPrimaryRoll.attacks  = curPriAttacks;
    curPrimaryRoll.rolls    = curPriNum;
    curPrimaryRoll.dice     = curPriSides;
    curPrimaryRoll.modifier = curPriMod;

    curSecondaryRoll.attacks  = curSecAttacks;
    curSecondaryRoll.rolls    = curSecNum;
    curSecondaryRoll.dice     = curSecSides;
    curSecondaryRoll.modifier = curSecMod;
}

void ADnDCharacter::resetCurrentRollsFromBase() {
    curPrimaryRoll  = basePrimaryRoll;
    curSecondaryRoll = baseSecondaryRoll;
}

int ADnDCharacter::getCapacityModifier() const {
    int t = getStrengthTier();

    if (t == 0) {
        return 0; // not possible
    }
    if (t >= 1 && t <= 3) {
        return -350;
    }
    if (t >= 4 && t <= 5) {
        return -250;
    }
    if (t >= 6 && t <= 7) {
        return -150;
    }
    if (t >= 8 && t <= 11) {
        return 0;
    }
    if (t == 12 || t == 13) {
        return 100;
    }
    if (t == 14 || t == 15) {
        return 200;
    }
    if (t == 16) {
        return 350;
    }
    if (t >= 17 && t <= 21) {
        return 500 + (t - 17) * 250;
    }
    if (t >= 22 && t <= 26) {
        return 2000 + (t - 22) * 1000;
    }
    if (t == 27) {
        return 7500;
    }
    if (t >= 28 && t <= 30) {
        return 9000 + (t - 28) * 3000;
    }
    debug("ADnDCharacter::getCapacityModifier: invalid strength tier %d", t);
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
    } else if (effectiveEnc >= 769) {
        cap = 6;
    } else if (effectiveEnc >= 513) {
        cap = 9;
    }

    // Only reduce movement; never increase.
    if (cap < movement.current) {
        movement.current = cap;
    }
}

} // namespace Data
} // namespace Goldbox
