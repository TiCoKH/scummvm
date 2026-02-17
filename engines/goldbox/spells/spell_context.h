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

#ifndef GOLDBOX_SPELLS_SPELL_CONTEXT_H
#define GOLDBOX_SPELLS_SPELL_CONTEXT_H

#include "common/array.h"
#include "common/types.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Data {
namespace Spells {
class SpellBook;
}
}
namespace Data {
namespace Effects {
class EffectSystem;
}
}

namespace Spells {

struct TargetSelection {
    Common::Array<int> targetIndices;
    Common::Array<Goldbox::Data::PlayerCharacter *> targetCharacters;
    int16 tileX;
    int16 tileY;

    TargetSelection() : tileX(-1), tileY(-1) {}
};

struct SpellContext {
    Goldbox::Data::PlayerCharacter *caster;
    Goldbox::Data::Spells::SpellBook *spellBook;
    Goldbox::Data::Effects::EffectSystem *effectSystem;
    bool inCombat;
    uint8 casterLevel;

    SpellContext() : caster(nullptr), spellBook(nullptr),
        effectSystem(nullptr), inCombat(false), casterLevel(0) {}
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_CONTEXT_H
