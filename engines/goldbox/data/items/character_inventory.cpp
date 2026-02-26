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

#include "common/debug.h"
#include "common/file.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/data/items/character_inventory.h"

namespace Goldbox {
namespace Data {
namespace Items {

using Goldbox::Data::PascalStringBuffer;

bool CharacterInventory::load(const Common::String &filename) {
    clear();

    Common::File file;
    if (!file.open(filename.c_str())) {
        debug("CharacterInventory::load: missing .ITM file '%s'", filename.c_str());
        return false;  // no .ITM → empty inventory
    }

    auto &s = file;  // File implements SeekableReadStream

    const int recSize = 63;
    int    total   = s.size();

    if ((total % recSize) != 0) {
        debug("CharacterInventory::load: file '%s' size=%d not multiple of recSize=%d",
              filename.c_str(), total, recSize);
    }

    debug("CharacterInventory::load: file '%s' size=%d recSize=%d",
          filename.c_str(), total, recSize);

    int index = 0;
    while (s.pos() + recSize <= total) {
        CharacterItem it;

        it.name = PascalStringBuffer<41>::read(s);
        // --- dynamic fields at offsets 0x2A–0x3E ---
        it.nextAddress = s.readUint32LE();
        it.typeIndex   = s.readByte();
        it.nameCode1   = s.readByte();
        it.nameCode2   = s.readByte();
        it.nameCode3   = s.readByte();
        it.bonus       = s.readByte();
        it.saveBonus   = s.readByte();
        it.readied     = s.readByte();
        it.hidden      = s.readByte();
        it.cursed      = s.readByte();

        it.weight      = s.readUint16LE();
        it.stackSize   = s.readByte();
        it.value       = s.readUint16LE();

        it.effect1     = s.readByte();
        it.effect2     = s.readByte();
        it.effect3     = s.readByte();

        _items.push_back(it);

        if (it.readied != 0) {
            Common::String displayName = it.getDisplayName();
            debug("CharacterInventory::load: item[%d] equipped displayName='%s' type=%u readied=%u next=0x%08x nameCode=[%u,%u,%u] cachedBuffer='%s'",
                  index, displayName.c_str(), (unsigned)it.typeIndex, (unsigned)it.readied,
                  (unsigned)it.nextAddress, (unsigned)it.nameCode1, (unsigned)it.nameCode2, (unsigned)it.nameCode3,
                  it.name.c_str());
        }
        ++index;
    }

    file.close();
    debug("CharacterInventory::load: loaded %u items from '%s'",
          (unsigned)_items.size(), filename.c_str());
    return true;
}

bool CharacterInventory::save(const Common::String &filename) const {
    Common::DumpFile file;
    if (!file.open(filename.c_str()))
        return false;

    auto &s = file;  // DumpFile supports WriteStream

    for (const auto &it : _items) {
        PascalStringBuffer<41>::write(s, it.name);

        s.writeUint32LE(it.nextAddress);
        s.writeByte(it.typeIndex);
        s.writeByte(it.nameCode1);
        s.writeByte(it.nameCode2);
        s.writeByte(it.nameCode3);
        s.writeByte(it.bonus);
        s.writeByte(it.saveBonus);
        s.writeByte(it.readied);
        s.writeByte(it.hidden);
        s.writeByte(it.cursed);

        s.writeUint16LE(it.weight);
        s.writeByte(it.stackSize);
        s.writeUint16LE(it.value);

        s.writeByte(it.effect1);
        s.writeByte(it.effect2);
        s.writeByte(it.effect3);
    }

    file.flush();
    file.close();
    return true;
}

void CharacterInventory::debugPrint() const {
    for (size_t i = 0; i < _items.size(); ++i)
        _items[i].debugPrint(i);
}

uint16 CharacterInventory::calcItemEncumbrance(const CharacterItem &item) {
    uint8 stack = item.stackSize == 0 ? 1 : item.stackSize;
    uint32 total = (uint32)item.weight * (uint32)stack;
    if (total > 0xFFFF)
        total = 0xFFFF;
    return (uint16)total;
}

bool CharacterInventory::meetsClassRestriction(const CharacterItem &item,
                                               const Rules &rules) {
    const uint8 mask = item.prop().classMask;
    if (mask == 0)
        return true;
    return (mask & rules.allowedClassMask) != 0;
}

uint16 CharacterInventory::totalEncumbrance() const {
    uint32 total = 0;
    for (uint i = 0; i < _items.size(); ++i) {
        total += calcItemEncumbrance(_items[i]);
        if (total > 0xFFFF)
            return 0xFFFF;
    }
    return (uint16)total;
}

bool CharacterInventory::canCarryItem(const CharacterItem &item,
                                      const Rules &rules) const {
    if (_items.size() >= rules.maxItems)
        return false;
    if (!meetsClassRestriction(item, rules))
        return false;

    if (rules.maxEncumbrance == 0)
        return true;

    int capacity = (int)rules.maxEncumbrance + rules.capacityModifier;
    if (capacity < 0)
        capacity = 0;

    uint32 total = (uint32)totalEncumbrance() + calcItemEncumbrance(item);
    return total <= (uint32)capacity;
}

void CharacterInventory::captureEquippedLegacyOffsets(
    const Common::Array<CharacterItem *> &equippedSlots,
    Common::Array<uint32> &offsetsOut) {
    offsetsOut.clear();
    offsetsOut.resize(equippedSlots.size());
    for (uint i = 0; i < equippedSlots.size(); ++i) {
        offsetsOut[i] = equippedSlots[i] ? equippedSlots[i]->nextAddress : 0;
    }
}

void CharacterInventory::remapEquippedByLegacyOffsets(
    Common::Array<CharacterItem *> *equippedSlots,
    const Common::Array<uint32> &offsets) {
    if (!equippedSlots)
        return;
    if (equippedSlots->size() != offsets.size())
        equippedSlots->resize(offsets.size());

    for (uint i = 0; i < offsets.size(); ++i) {
        if (offsets[i] == 0) {
            (*equippedSlots)[i] = nullptr;
            continue;
        }
        (*equippedSlots)[i] = findByLegacyAddress(offsets[i]);
    }
}

bool CharacterInventory::addItem(
    const CharacterItem &item, const Rules &rules,
    Common::Array<CharacterItem *> *equippedSlots) {
    if (!canCarryItem(item, rules))
        return false;

    Common::Array<uint32> offsets;
    if (equippedSlots)
        captureEquippedLegacyOffsets(*equippedSlots, offsets);

    _items.push_back(item);

    if (equippedSlots)
        remapEquippedByLegacyOffsets(equippedSlots, offsets);
    return true;
}

bool CharacterInventory::removeItem(
    CharacterItem *item,
    Common::Array<CharacterItem *> *equippedSlots) {
    if (!item)
        return false;

    Common::Array<uint32> offsets;
    if (equippedSlots)
        captureEquippedLegacyOffsets(*equippedSlots, offsets);

    if (equippedSlots) {
        for (uint i = 0; i < equippedSlots->size(); ++i) {
            if ((*equippedSlots)[i] == item) {
                if (item->cursed)
                    return false;
                break;
            }
        }
    }

    for (uint i = 0; i < _items.size(); ++i) {
        if (&_items[i] == item) {
            _items.remove_at(i);
            if (equippedSlots)
                remapEquippedByLegacyOffsets(equippedSlots, offsets);
            return true;
        }
    }

    return false;
}

CharacterItem *CharacterInventory::findByLegacyAddress(uint32 legacyAddr) {
    for (uint i = 0; i < _items.size(); ++i) {
        if (_items[i].nextAddress == legacyAddr)
            return &_items[i];
    }
    return nullptr;
}

const CharacterItem *CharacterInventory::findByLegacyAddress(
    uint32 legacyAddr) const {
    for (uint i = 0; i < _items.size(); ++i) {
        if (_items[i].nextAddress == legacyAddr)
            return &_items[i];
    }
    return nullptr;
}

void CharacterInventory::recomputeEquippedTotals(
    const Common::Array<CharacterItem *> &equippedSlots,
    uint8 *handsUsed, uint8 *saveBonus) const {
    int hands = 0;
    int saves = 0;

    for (uint i = 0; i < equippedSlots.size(); ++i) {
        const CharacterItem *item = equippedSlots[i];
        if (!item)
            continue;
        const ItemProperty &prop = item->prop();
        hands += prop.hands;
        saves += item->saveBonus;
        if (hands > 255)
            hands = 255;
        if (saves > 255)
            saves = 255;
    }

    if (handsUsed)
        *handsUsed = (uint8)hands;
    if (saveBonus)
        *saveBonus = (uint8)saves;
}

bool CharacterInventory::equipItem(
    CharacterItem *item, Slot slot,
    Common::Array<CharacterItem *> &equippedSlots,
    uint8 *handsUsed, uint8 *saveBonus) {
    if (!item)
        return false;

    if (equippedSlots.size() != (uint)Slot::SLOT_COUNT)
        equippedSlots.resize((uint)Slot::SLOT_COUNT);

    const ItemProperty &prop = item->prop();
    const Slot itemSlot = static_cast<Slot>(prop.slotID);

    if (itemSlot == Slot::S_NONE)
        return false;
    if (itemSlot != slot)
        return false;

    const uint slotIndex = (uint)slot;

    // If item is already equipped elsewhere, unequip it first.
    for (uint i = 0; i < equippedSlots.size(); ++i) {
        if (equippedSlots[i] == item && i != slotIndex) {
            if (!unequipItem((Slot)i, equippedSlots, handsUsed, saveBonus))
                return false;
        }
    }

    // Unequip current item in slot, unless cursed
    if (equippedSlots[slotIndex]) {
        if (!unequipItem(slot, equippedSlots, handsUsed, saveBonus))
            return false;
    }

    // Off-hand cannot be equipped if main hand is two-handed
    if (slot == Slot::S_OFF_HAND) {
        CharacterItem *main = equippedSlots[(uint)Slot::S_MAIN_HAND];
        if (main && main->prop().hands == 2)
            return false;
    }

    // If two-handed, ensure off-hand is empty
    if (prop.hands == 2) {
        if (equippedSlots[(uint)Slot::S_OFF_HAND]) {
            if (!unequipItem(Slot::S_OFF_HAND, equippedSlots,
                             handsUsed, saveBonus))
                return false;
        }
    }

    equippedSlots[slotIndex] = item;
    item->readied = 1;

    recomputeEquippedTotals(equippedSlots, handsUsed, saveBonus);
    return true;
}

bool CharacterInventory::unequipItem(
    Slot slot, Common::Array<CharacterItem *> &equippedSlots,
    uint8 *handsUsed, uint8 *saveBonus) {
    if (equippedSlots.size() != (uint)Slot::SLOT_COUNT)
        equippedSlots.resize((uint)Slot::SLOT_COUNT);

    const uint slotIndex = (uint)slot;
    CharacterItem *item = equippedSlots[slotIndex];
    if (!item)
        return true;

    if (item->cursed)
        return false;

    equippedSlots[slotIndex] = nullptr;
    item->readied = 0;

    recomputeEquippedTotals(equippedSlots, handsUsed, saveBonus);
    return true;
}

} // namespace Items
} // namespace Data
} // namespace Goldbox
