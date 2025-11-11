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

} // namespace Data
} // namespace Goldbox
