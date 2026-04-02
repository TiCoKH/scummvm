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

#ifndef GOLDBOX_SPELLS_SPELL_METADATA_H
#define GOLDBOX_SPELLS_SPELL_METADATA_H

#include "common/str.h"
#include "goldbox/data/spells/spell.h"

namespace Goldbox {
namespace Spells {

struct SpellMetadata {
    Common::String name;
    Common::String spellClass;
    Common::String level;

    SpellMetadata() {
    }

    SpellMetadata(const Common::String &spellName,
            const Common::String &spellClassName,
            const Common::String &spellLevel)
        : name(spellName), spellClass(spellClassName), level(spellLevel) {
    }
};

SpellMetadata getSpellMetadata(Goldbox::Data::Spells::Spells spell);
Common::String getSpellName(Goldbox::Data::Spells::Spells spell);
Common::String getSpellClassName(Goldbox::Data::Spells::Spells spell);
Common::String getSpellLevelText(Goldbox::Data::Spells::Spells spell);

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_METADATA_H