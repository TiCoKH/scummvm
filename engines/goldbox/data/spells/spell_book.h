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

#ifndef GOLDBOX_DATA_SPELLS_SPELL_BOOK_H
#define GOLDBOX_DATA_SPELLS_SPELL_BOOK_H

#include "common/hashmap.h"
#include "common/stream.h"
#include "goldbox/data/spells/spell.h"

namespace Common {
    template<> struct Hash<Goldbox::Data::Spells::Spells> {
        uint operator()(Goldbox::Data::Spells::Spells val) const {
            return (uint)val;
        }
    };
}

namespace Goldbox {
namespace Data {
namespace Spells {

/**
 * Modern spell book storage for known and memorized spells.
 * Supports variable spell counts across different Gold Box games.
 * 
 * Each spell is identified by the Spells enum and stored with:
 * - known flag (can this character learn/use this spell?)
 * - memorized count (how many instances are currently prepared?)
 */
class SpellBook {
public:
    struct SpellEntry {
        bool known;
        uint8 memorized;

        SpellEntry() : known(false), memorized(0) {}
        SpellEntry(bool k, uint8 m) : known(k), memorized(m) {}
    };

    SpellBook();

    // Query methods
    bool isKnown(Spells spell) const;
    uint8 getMemorized(Spells spell) const;
    bool hasAnyMemorized() const;

    // Modification methods
    void setKnown(Spells spell, bool known = true);
    void setMemorized(Spells spell, uint8 count);
    void clearMemorized(Spells spell);
    void clearAllMemorized();

    // Legacy conversion for save/load
    // Each game defines its own spell ID mapping and array sizes
    void loadFromLegacyArrays(const uint8 *memorizedArray, int memorizedSize,
                             const uint8 *knownArray, int knownSize,
                             const Spells *spellMapping, int mappingSize);

    void saveToLegacyArrays(uint8 *memorizedArray, int memorizedSize,
                           uint8 *knownArray, int knownSize,
                           const Spells *spellMapping, int mappingSize) const;

    // Iteration support
    const Common::HashMap<Spells, SpellEntry> &getSpells() const { return _spells; }

private:
    Common::HashMap<Spells, SpellEntry> _spells;
};

} // namespace Spells
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_SPELLS_SPELL_BOOK_H
