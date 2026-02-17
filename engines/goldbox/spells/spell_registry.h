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

#ifndef GOLDBOX_SPELLS_SPELL_REGISTRY_H
#define GOLDBOX_SPELLS_SPELL_REGISTRY_H

#include "common/hashmap.h"
#include "goldbox/spells/spell_definition.h"
#include "goldbox/spells/spell_handlers.h"

namespace Goldbox {
namespace Spells {

class SpellRegistry {
public:
    SpellRegistry();

    const SpellDefinition *getDefinition(Goldbox::Data::Spells::Spells id) const;
    const ISpellHandler *getHandler(HandlerId id) const;

    void setHandler(HandlerId id, ISpellHandler *handler);
    void setHandlerForSpell(Goldbox::Data::Spells::Spells id, HandlerId handler);

private:
    void buildDefinitions();

    Common::HashMap<int, SpellDefinition> _definitions;
    Common::HashMap<int, ISpellHandler *> _handlers;
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_REGISTRY_H
