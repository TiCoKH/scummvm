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

#include "goldbox/spells/spell_metadata.h"

#include "goldbox/data/rules/rules.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Spells {

namespace {

const Goldbox::Data::Spells::SpellEntry *findSpellEntry(
        Goldbox::Data::Spells::Spells spell) {
    const Common::Array<Goldbox::Data::Spells::SpellEntry> &entries =
        Goldbox::Data::Rules::getSpellEntries();
    const uint spellIndex = (uint)spell;

    if (spellIndex >= entries.size()) {
        return nullptr;
    }

    return &entries[spellIndex];
}

Common::String getFallbackClassName(
        Goldbox::Data::Spells::SpellClass spellClass) {
    switch (spellClass) {
    case Goldbox::Data::Spells::SC_CLERIC:
        return "cleric";
    case Goldbox::Data::Spells::SC_MAGICUSER:
        return "magic-user";
    case Goldbox::Data::Spells::SC_ITEM:
        return "item";
    default:
        return "unknown";
    }
}

Common::String getStringOrFallback(const Common::String &key,
        const Common::String &fallback) {
    Common::String value = Goldbox::VmInterface::getString(key);
    return value.empty() ? fallback : value;
}

} // namespace

SpellMetadata getSpellMetadata(Goldbox::Data::Spells::Spells spell) {
    const uint spellIndex = (uint)spell;
    const Goldbox::Data::Spells::SpellEntry *entry = findSpellEntry(spell);

    if (spellIndex == 0 || !entry) {
        return SpellMetadata(Common::String(), Common::String(),
            Common::String());
    }

    const Common::String baseKey = Common::String::format("spells.%u",
        (unsigned)spellIndex);
    const Common::String fallbackName = Common::String::format("Spell %u",
        (unsigned)spellIndex);
    const Common::String fallbackClass = getFallbackClassName(entry->spellClass);
    const Common::String fallbackLevel = Common::String::format("%u",
        (unsigned)entry->spellLevel);

    return SpellMetadata(
        getStringOrFallback(baseKey + ".name", fallbackName),
        getStringOrFallback(baseKey + ".class", fallbackClass),
        getStringOrFallback(baseKey + ".level", fallbackLevel)
    );
}

Common::String getSpellName(Goldbox::Data::Spells::Spells spell) {
    return getSpellMetadata(spell).name;
}

Common::String getSpellClassName(Goldbox::Data::Spells::Spells spell) {
    return getSpellMetadata(spell).spellClass;
}

Common::String getSpellLevelText(Goldbox::Data::Spells::Spells spell) {
    return getSpellMetadata(spell).level;
}

} // namespace Spells
} // namespace Goldbox