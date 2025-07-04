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

#ifndef GOLDBOX_DATA_SPELLS_SPELLBOOK_H
#define GOLDBOX_DATA_SPELLS_SPELLBOOK_H

#include "common/hashmap.h"
#include "common/array.h"
#include "common/str.h"
#include "common/types.h"

namespace Goldbox {
namespace Data {
namespace Spells {

// Enum for spell class/type
enum SpellClass {
    SPELLCLASS_CLERIC,
    SPELLCLASS_MAGICUSER,
    SPELLCLASS_DRUID,
    SPELLCLASS_PALADIN,
    SPELLCLASS_RANGER,
    SPELLCLASS_OTHER
};

// Base spell definition
struct SpellDef {
    uint8 id;                  // Unique spell id
    SpellClass spellClass;
    uint8 level;

    SpellDef() : id(0), spellClass(SPELLCLASS_OTHER), level(0) {}
    SpellDef(uint8 i, SpellClass c, uint8 l)
        : id(i), spellClass(c), level(l) {}
};

// Known/memorized spell entry for a character
struct SpellEntry {
    uint8 spellId;
    bool known;
    uint8 memorized; // Number of times memorized

    SpellEntry() : spellId(0), known(false), memorized(0) {}
    SpellEntry(uint8 id, bool k, uint8 m) : spellId(id), known(k), memorized(m) {}
};

// Base spellbook class (for a character)
class SpellBook {
public:
    // spellId -> SpellEntry
    Common::HashMap<uint8, SpellEntry> spells;

    // spellLevel -> slots available
    Common::HashMap<uint8, uint8> spellSlots;

    virtual ~SpellBook() {}

    // Add or update a spell
    virtual void setSpell(uint8 spellId, bool known, uint8 memorized) {
        spells.setVal(spellId, SpellEntry(spellId, known, memorized));
    }

    // Set slots for a level
    virtual void setSlots(uint8 level, uint8 slots) {
        spellSlots.setVal(level, slots);
    }

    // Get spell entry (nullptr if not found)
    virtual const SpellEntry *getSpell(uint8 spellId) const {
        return spells.contains(spellId) ? &spells.getVal(spellId) : nullptr;
    }

    // Get slots for a level (0 if not found)
    virtual uint8 getSlots(uint8 level) const {
        return spellSlots.contains(level) ? spellSlots.getVal(level) : 0;
    }

    // Clear all
    virtual void clear() {
        spells.clear();
        spellSlots.clear();
    }
};

} // namespace Spells
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_SPELLS_SPELLBOOK_H
