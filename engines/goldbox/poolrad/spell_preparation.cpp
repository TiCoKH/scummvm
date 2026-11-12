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

#include "goldbox/poolrad/spell_preparation.h"

#include "goldbox/data/items/character_inventory.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/poolrad_spell_mapping.h"

namespace Goldbox {
namespace Poolrad {

using Goldbox::Data::Spells::Spells;
using Goldbox::Data::Spells::SpellClass;
using Goldbox::Data::Spells::SC_CLERIC;
using Goldbox::Data::Spells::SC_MAGICUSER;

namespace {

// Resolve a spell's class and (1..3) book level from its knownSpells[] index.
// The Pool of Radiance character layout fixes these ranges (see
// poolrad_spell_mapping.h), so preparation eligibility needs no loaded rules
// table and stays unit-testable from a constructed character.
struct SpellClassLevel {
    SpellClass spellClass;
    uint8 level; // 0 when not a player-learnable spell
};

SpellClassLevel classLevelForLegacyIndex(int legacyIndex) {
    SpellClassLevel out;
    out.spellClass = Goldbox::Data::Spells::SC_UNKNOWN;
    out.level = 0;

    if (legacyIndex < 0 || legacyIndex >= Data::POOLRAD_PLAYER_KNOWN_SIZE)
        return out;

    if (legacyIndex <= 7) {
        out.spellClass = SC_CLERIC;
        out.level = 1;
    } else if (legacyIndex <= 20) {
        out.spellClass = SC_MAGICUSER;
        out.level = 1;
    } else if (legacyIndex <= 27) {
        out.spellClass = SC_CLERIC;
        out.level = 2;
    } else if (legacyIndex <= 34) {
        out.spellClass = SC_MAGICUSER;
        out.level = 2;
    } else if (legacyIndex <= 43) {
        out.spellClass = SC_CLERIC;
        out.level = 3;
    } else { // 44..54
        out.spellClass = SC_MAGICUSER;
        out.level = 3;
    }
    return out;
}

// knownSpells[] index for a Spells enum value (inverse of kPoolradSpellMapping).
int legacyIndexForSpell(Spells spell) {
    for (int i = 0; i < Data::POOLRAD_PLAYER_KNOWN_SIZE; ++i) {
        if (Data::kPoolradSpellMapping[i] == spell)
            return i;
    }
    return -1;
}

} // namespace

SpellPreparation::SpellPreparation(Data::PoolradCharacter *character)
    : _character(character) {
}

uint8 SpellPreparation::slotCapacityForSpell(Spells spell) const {
    if (!_character)
        return 0;

    const SpellClassLevel cl = classLevelForLegacyIndex(legacyIndexForSpell(spell));
    if (cl.level == 0 || cl.level > 3)
        return 0;

    const Goldbox::Data::SpellSlots &slots =
        (cl.spellClass == SC_CLERIC) ? _character->spellSlots.cleric
                                     : _character->spellSlots.magicUser;

    switch (cl.level) {
    case 1: return slots.level1;
    case 2: return slots.level2;
    case 3: return slots.level3;
    default: return 0;
    }
}

bool SpellPreparation::canDoActivity(Activity activity) const {
    if (!_character)
        return false;

    const Goldbox::Data::ClassSpellSlots &s = _character->spellSlots;
    const bool hasCleric = s.cleric.level1 || s.cleric.level2 || s.cleric.level3;
    const bool hasMage = s.magicUser.level1 || s.magicUser.level2 ||
        s.magicUser.level3;

    switch (activity) {
    case ACT_CAST:
    case ACT_MEMORIZE:
        // CanDoSpellActivity: any caster class with available slots qualifies.
        return hasCleric || hasMage;
    case ACT_SCRIBE:
        // Scribing writes into the magic-user spell book.
        return hasMage;
    default:
        return false;
    }
}

const char *SpellPreparation::activityRefusal(Activity activity,
        bool antimagicArea) const {
    if (!_character)
        return nullptr;
    if (activity == ACT_CAST && antimagicArea)
        return "cannot cast spells in this area";
    if (_character->healthStatus == Goldbox::Data::S_ANIMATED ||
            !_character->enabled) {
        switch (activity) {
        case ACT_CAST:
            return "is in no condition to cast any spells";
        case ACT_MEMORIZE:
            return "is in no condition to memorize spells";
        case ACT_SCRIBE:
            return "is in no condition to scribe any scrolls";
        default:
            break;
        }
    }
    return nullptr;
}

uint16 SpellPreparation::campPrepTime() {
    if (!_character)
        return 0;

    uint8 maxMem = 0;
    uint8 maxScr = 0;
    uint8 sumMem = 0;
    uint8 sumScr = 0;

    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        const uint8 v = _character->spells.memorizedSpells[i];
        if (v < 0x80)
            continue;
        const uint8 lvl = classLevelForLegacyIndex((v & 0x7F) - 1).level;
        maxMem = MAX(maxMem, lvl);
        sumMem += lvl;
    }

    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    for (int i = 0; i < inv.count(); ++i) {
        const Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;
        const uint8 effects[3] = { item.effect1, item.effect2, item.effect3 };
        for (int e = 0; e < 3; ++e) {
            if (effects[e] < 0x80)
                continue;
            const uint8 lvl = classLevelForLegacyIndex(
                (effects[e] & 0x7F) - 1).level;
            maxScr = MAX(maxScr, lvl);
            sumScr += lvl;
        }
    }

    uint8 base = 0;
    if (sumMem > 0 || sumScr > 0)
        base = 4;
    if (maxMem > 2 || maxScr > 2)
        base = 6;
    _character->spellRegainRate = base;
    return (uint16)(base * 60 + sumMem * 15 + sumScr * 15);
}

uint8 SpellPreparation::slotAllowance(SpellClass spellClass,
        uint8 level) const {
    if (!_character || level < 1 || level > 3)
        return 0;
    const Goldbox::Data::SpellSlots *slots = nullptr;
    if (spellClass == SC_CLERIC)
        slots = &_character->spellSlots.cleric;
    else if (spellClass == SC_MAGICUSER)
        slots = &_character->spellSlots.magicUser;
    if (!slots)
        return 0;
    switch (level) {
    case 1: return slots->level1;
    case 2: return slots->level2;
    default: return slots->level3;
    }
}

uint8 SpellPreparation::slotsRemaining(SpellClass spellClass,
        uint8 level) const {
    if (!_character)
        return 0;
    uint8 count = 0;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        const uint8 v = _character->spells.memorizedSpells[i];
        if (v == 0)
            continue;
        const SpellClassLevel have = classLevelForLegacyIndex((v & 0x7F) - 1);
        if (have.spellClass == spellClass && have.level == level)
            ++count;
    }
    return (uint8)(slotAllowance(spellClass, level) - count);
}

bool SpellPreparation::hasScrollSpells() const {
    if (!_character)
        return false;
    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    for (int i = 0; i < inv.count(); ++i) {
        const Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (item.isMissileOrScroll() &&
                (item.effect1 || item.effect2 || item.effect3))
            return true;
    }
    return false;
}

bool SpellPreparation::hasPendingScribe() const {
    if (!_character)
        return false;
    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    for (int i = 0; i < inv.count(); ++i) {
        const Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (item.isMissileOrScroll() && (item.effect1 > 0x7F ||
                item.effect2 > 0x7F || item.effect3 > 0x7F))
            return true;
    }
    return false;
}

bool SpellPreparation::isScribing(Spells spell) const {
    if (!_character)
        return false;
    const int legacyIndex = legacyIndexForSpell(spell);
    if (legacyIndex < 0)
        return false;
    const uint8 tag = (uint8)(legacyIndex + 1);
    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    for (int i = 0; i < inv.count(); ++i) {
        const Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;
        const uint8 effects[3] = { item.effect1, item.effect2, item.effect3 };
        for (int e = 0; e < 3; ++e)
            if (effects[e] > 0x7F && (effects[e] & 0x7F) == tag)
                return true;
    }
    return false;
}

bool SpellPreparation::markScribe(Spells spell) {
    if (!_character)
        return false;
    const int legacyIndex = legacyIndexForSpell(spell);
    if (legacyIndex < 0)
        return false;
    const uint8 tag = (uint8)(legacyIndex + 1);
    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    for (int i = 0; i < inv.count(); ++i) {
        Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;
        uint8 *effects[3] = { &item.effect1, &item.effect2, &item.effect3 };
        for (int e = 0; e < 3; ++e) {
            if (*effects[e] == tag) {
                *effects[e] = (uint8)(tag | 0x80);
                return true;
            }
        }
    }
    return false;
}

SpellPreparation::NoncombatCastCheck SpellPreparation::checkNoncombatCast(
        bool dungeonMap, bool antimagicArea) const {
    if (!_character)
        return NCC_NOT_OKAY;
    if (dungeonMap && _character->healthStatus != Goldbox::Data::S_OKAY)
        return NCC_NOT_OKAY;
    if (antimagicArea)
        return NCC_ANTIMAGIC_AREA;
    if (_character->healthStatus == Goldbox::Data::S_ANIMATED ||
            !_character->enabled)
        return NCC_NO_CONDITION;
    if (countUsableMemorized() == 0)
        return NCC_NOTHING_MEMORIZED;
    return NCC_ALLOWED;
}

bool SpellPreparation::canLearnSpell(Spells spell) const {
    if (!_character)
        return false;
    // A spell is learnable when its class has slots and it is not already
    // known. slotCapacityForSpell is zero for the wrong class / unknown level.
    return slotCapacityForSpell(spell) != 0 && !_character->isSpellKnown(spell);
}

Common::Array<SpellPreparation::ListEntry>
SpellPreparation::generateMemorizeList() const {
    Common::Array<ListEntry> list;
    if (!_character)
        return list;

    // GenerateSpellList: known spells whose class/level still has capacity.
    for (int i = 0; i < Data::POOLRAD_PLAYER_KNOWN_SIZE; ++i) {
        if (_character->spells.knownSpells[i] == 0)
            continue;

        const Spells spell = Data::kPoolradSpellMapping[i];
        if (slotCapacityForSpell(spell) == 0)
            continue;

        list.push_back(ListEntry(i, spell));
    }
    return list;
}

uint8 SpellPreparation::countUsableMemorized() const {
    if (!_character)
        return 0;
    uint8 count = 0;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        const uint8 v = _character->spells.memorizedSpells[i];
        if (v != 0 && v < 0x80)
            ++count;
    }
    return count;
}

uint8 SpellPreparation::countPendingMemorized() const {
    if (!_character)
        return 0;
    uint8 count = 0;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        if (_character->spells.memorizedSpells[i] >= 0x80)
            ++count;
    }
    return count;
}

uint8 SpellPreparation::countOccupiedSlots() const {
    if (!_character)
        return 0;
    uint8 count = 0;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        if (_character->spells.memorizedSpells[i] != 0)
            ++count;
    }
    return count;
}

uint8 SpellPreparation::remainingSlotsForSpell(Spells spell) const {
    if (!_character)
        return 0;

    const uint8 capacity = slotCapacityForSpell(spell);
    if (capacity == 0)
        return 0;

    const SpellClassLevel want =
        classLevelForLegacyIndex(legacyIndexForSpell(spell));

    // Count occupied slots (usable + pending) of the same class/level.
    uint8 used = 0;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        const uint8 v = _character->spells.memorizedSpells[i];
        if (v == 0)
            continue;
        const int idx = (v & 0x7F) - 1;
        const SpellClassLevel have = classLevelForLegacyIndex(idx);
        if (have.spellClass == want.spellClass && have.level == want.level)
            ++used;
    }

    return (used >= capacity) ? 0 : (uint8)(capacity - used);
}

bool SpellPreparation::queueMemorize(Spells spell) {
    if (!_character)
        return false;

    const int legacyIndex = legacyIndexForSpell(spell);
    if (legacyIndex < 0 || legacyIndex >= Data::POOLRAD_PLAYER_KNOWN_SIZE)
        return false;

    // Must be a known spell of a class/level that still has capacity.
    if (_character->spells.knownSpells[legacyIndex] == 0)
        return false;
    if (remainingSlotsForSpell(spell) == 0)
        return false;

    // Find the first free slot inside the 21-slot bound.
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        if (_character->spells.memorizedSpells[i] != 0)
            continue;

        const uint8 spellId = (uint8)(legacyIndex + 1);
        _character->spells.memorizedSpells[i] = (uint8)(spellId | 0x80);
        sortMemorizedSlots();
        return true;
    }

    return false; // 21 slots full.
}

void SpellPreparation::cancelPendingMemorize() {
    if (!_character)
        return;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        if (_character->spells.memorizedSpells[i] >= 0x80)
            _character->spells.memorizedSpells[i] = 0;
    }
    sortMemorizedSlots();
}

bool SpellPreparation::completeOnePendingMemorize() {
    if (!_character)
        return false;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        uint8 &v = _character->spells.memorizedSpells[i];
        if (v >= 0x80) {
            v = (uint8)(v & 0x7F); // Clear pending bit: now usable.
            // Keep the SpellBook count in step with the usable slot array.
            const Spells spell = static_cast<Spells>(v);
            _character->spellBook.setMemorized(spell,
                (uint8)(_character->spellBook.getMemorized(spell) + 1));
            sortMemorizedSlots();
            return true;
        }
    }
    return false;
}

void SpellPreparation::sortMemorizedSlots() {
    if (!_character)
        return;

    // Collect occupied slot values, pack toward slot 0, sort ascending by
    // spell id (ignoring the pending bit so pending/usable copies group
    // together), preserving the pending bit per entry.
    Common::Array<uint8> occupied;
    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        if (_character->spells.memorizedSpells[i] != 0)
            occupied.push_back(_character->spells.memorizedSpells[i]);
    }

    for (uint a = 0; a + 1 < occupied.size(); ++a) {
        for (uint b = 0; b + 1 < occupied.size() - a; ++b) {
            if ((occupied[b] & 0x7F) > (occupied[b + 1] & 0x7F)) {
                const uint8 t = occupied[b];
                occupied[b] = occupied[b + 1];
                occupied[b + 1] = t;
            }
        }
    }

    for (int i = 0; i < Data::POOLRAD_MEMORIZED_SIZE; ++i) {
        _character->spells.memorizedSpells[i] =
            ((uint)i < occupied.size()) ? occupied[i] : 0;
    }
}

Common::Array<SpellPreparation::ScribeEntry>
SpellPreparation::generateScribeList() const {
    Common::Array<ScribeEntry> list;
    if (!_character)
        return list;

    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    const int itemCount = inv.count();
    for (int i = 0; i < itemCount; ++i) {
        Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;

        const uint8 effects[3] = { item.effect1, item.effect2, item.effect3 };
        for (int e = 0; e < 3; ++e) {
            if (effects[e] == 0)
                continue;
            // Pending payloads already carry bit 7; skip those.
            if (effects[e] >= 0x80)
                continue;
            const uint8 tag = effects[e] & 0x7F;
            const int spellIdx = tag - 1;
            if (spellIdx < 0 || spellIdx >= Data::POOLRAD_PLAYER_KNOWN_SIZE)
                continue;

            const Spells spell = Data::kPoolradSpellMapping[spellIdx];
            if (!canLearnSpell(spell))
                continue;

            ScribeEntry entry;
            entry.scroll = &item;
            entry.payloadSlot = e;
            entry.spellId = spell;
            list.push_back(entry);
        }
    }
    return list;
}

bool SpellPreparation::queueScribe(Goldbox::Data::Items::CharacterItem *scroll,
        int payloadSlot) {
    if (!_character || !scroll || payloadSlot < 0 || payloadSlot > 2)
        return false;

    uint8 *effects[3] = { &scroll->effect1, &scroll->effect2, &scroll->effect3 };
    uint8 &slot = *effects[payloadSlot];
    if (slot == 0 || slot >= 0x80)
        return false; // Empty or already pending.

    const uint8 tag = slot & 0x7F;
    const int spellIdx = tag - 1;
    if (spellIdx < 0 || spellIdx >= Data::POOLRAD_PLAYER_KNOWN_SIZE)
        return false;

    const Spells spell = Data::kPoolradSpellMapping[spellIdx];
    if (!canLearnSpell(spell))
        return false;

    slot = (uint8)(tag | 0x80); // Mark pending.
    return true;
}

void SpellPreparation::cancelPendingScribe() {
    if (!_character)
        return;
    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    const int itemCount = inv.count();
    for (int i = 0; i < itemCount; ++i) {
        Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;
        uint8 *effects[3] = { &item.effect1, &item.effect2, &item.effect3 };
        for (int e = 0; e < 3; ++e) {
            if (*effects[e] >= 0x80)
                *effects[e] = (uint8)(*effects[e] & 0x7F); // Clear pending.
        }
    }
}

bool SpellPreparation::completeOnePendingScribe() {
    if (!_character)
        return false;

    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    const int itemCount = inv.count();
    for (int i = 0; i < itemCount; ++i) {
        Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;

        const uint8 effects[3] = { item.effect1, item.effect2, item.effect3 };
        for (int e = 0; e < 3; ++e) {
            if (effects[e] < 0x80)
                continue; // Not a pending payload.

            const uint8 tag = effects[e] & 0x7F;
            const int spellIdx = tag - 1;
            if (spellIdx < 0 || spellIdx >= Data::POOLRAD_PLAYER_KNOWN_SIZE)
                continue;

            // Capture the spell BEFORE touching the inventory. removeEffectTag
            // may free the scroll (its last payload), so nothing below may
            // dereference `item` afterwards — this is the freed-scroll bug the
            // Pascal flags, avoided by capture-before-remove.
            const Spells learned = Data::kPoolradSpellMapping[spellIdx];

            // removeEffectTag matches on the low 7 bits, clears the slot,
            // decrements the payload counter and removes the scroll when its
            // last payload is gone.
            inv.removeEffectTag(item, tag, nullptr);

            // Teach exactly one payload. setSpellKnown is idempotent, so a
            // repeat (interrupted/resumed) rest never double-learns.
            _character->setSpellKnown(learned);
            return true;
        }
    }
    return false;
}

bool SpellPreparation::hasPendingWork() const {
    if (!_character)
        return false;

    if (countPendingMemorized() != 0)
        return true;

    Goldbox::Data::Items::CharacterInventory &inv = _character->inventory;
    const int itemCount = inv.count();
    for (int i = 0; i < itemCount; ++i) {
        Goldbox::Data::Items::CharacterItem &item = inv[i];
        if (!item.isMissileOrScroll())
            continue;
        if (item.effect1 >= 0x80 || item.effect2 >= 0x80 ||
                item.effect3 >= 0x80)
            return true;
    }
    return false;
}

} // namespace Poolrad
} // namespace Goldbox
