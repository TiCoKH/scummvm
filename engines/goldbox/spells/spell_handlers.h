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

#ifndef GOLDBOX_SPELLS_SPELL_HANDLERS_H
#define GOLDBOX_SPELLS_SPELL_HANDLERS_H

#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_definition.h"

namespace Goldbox {
namespace Spells {

enum SpellCastStatus {
    CAST_OK = 0,
    CAST_NOT_KNOWN,
    CAST_NOT_MEMORIZED,
    CAST_INVALID_TARGET,
    CAST_NOT_ALLOWED,
    CAST_NO_HANDLER,
    CAST_ERROR
};

struct SpellCastResult {
    SpellCastStatus status;

    SpellCastResult() : status(CAST_ERROR) {}
    explicit SpellCastResult(SpellCastStatus s) : status(s) {}
};

class ISpellHandler {
public:
    virtual ~ISpellHandler() {}
    virtual SpellCastResult execute(const SpellContext &context,
                                    const SpellDefinition &definition,
                                    const TargetSelection &targets) const = 0;
};

class UnimplementedSpellHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_HANDLERS_H
