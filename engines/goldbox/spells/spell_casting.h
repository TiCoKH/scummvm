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

#ifndef GOLDBOX_SPELLS_SPELL_CASTING_H
#define GOLDBOX_SPELLS_SPELL_CASTING_H

#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_registry.h"
#include "goldbox/spells/spell_targeter.h"

namespace Goldbox {
namespace Spells {

class SpellCastingService {
public:
    SpellCastingService();

    SpellCastResult castSpell(SpellContext &context,
                              Goldbox::Data::Spells::Spells spell);

    void setCombatTargeter(ISpellTargeter *targeter);
    void setNonCombatTargeter(ISpellTargeter *targeter);

    SpellRegistry &registry() { return _registry; }
    const SpellRegistry &registry() const { return _registry; }

private:
    SpellCastResult validate(const SpellContext &context,
                             const SpellDefinition &definition) const;
    ISpellTargeter *getTargeter(const SpellContext &context) const;

    SpellRegistry _registry;
    ISpellTargeter *_combatTargeter;
    ISpellTargeter *_nonCombatTargeter;
    UnimplementedSpellHandler _fallbackHandler;
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_CASTING_H
