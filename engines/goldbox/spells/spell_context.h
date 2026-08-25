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
#include "common/str.h"
#include "common/types.h"
#include "goldbox/data/damage_system.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
class DamageSystem;
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
namespace Combat {
struct CombatGlobals;
}

namespace Spells {

struct TargetSelection {
    Common::Array<int> targetIndices;
    Common::Array<Goldbox::Data::PlayerCharacter *> targetCharacters;
    int16 tileX;
    int16 tileY;

    TargetSelection() : tileX(-1), tileY(-1) {}
};

class ISpellTargetPicker {
public:
    virtual ~ISpellTargetPicker() {}

    // Ask the host to pick one target from candidates (mirrors
    // DIALOG_CharacterSelect). Returns false if the player cancelled.
    virtual bool pickSingleTarget(
            const Common::Array<Goldbox::Data::PlayerCharacter *> &candidates,
            Goldbox::Data::PlayerCharacter *&picked) = 0;
};

struct SpellContext {
    Goldbox::Data::PlayerCharacter *caster;
    Goldbox::Data::Spells::SpellBook *spellBook;
    Goldbox::Data::Effects::EffectSystem *effectSystem;
    bool inCombat;
    uint8 casterLevel;

    // Candidate pools for target selection, populated by the caller before
    // castSpell(): caster's side (or whole party outside combat) and the
    // opposing side (empty outside combat). Kept decoupled from the combat/
    // module so spells/ has no dependency on it.
    Common::Array<Goldbox::Data::PlayerCharacter *> allies;
    Common::Array<Goldbox::Data::PlayerCharacter *> enemies;

    // Optional UI hook for ST_PARTY_MEMBER-style manual single-target spells.
    // May be null; selection then fails with CAST_INVALID_TARGET.
    ISpellTargetPicker *targetPicker;

    // Optional — non-null only during combat. Passed to GenericSpellHandler
    // for attack-roll checks and effect-set evaluation.
    Goldbox::Combat::CombatGlobals *combat;

    // Optional — used by GenericSpellHandler to apply direct damage for
    // spells whose fixedRange == SP_ATTACK_ROLL (need_ar path).
    Goldbox::Data::DamageSystem *damageSystem;

    SpellContext() : caster(nullptr), spellBook(nullptr),
        effectSystem(nullptr), inCombat(false), casterLevel(0),
        targetPicker(nullptr), combat(nullptr), damageSystem(nullptr) {}
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_CONTEXT_H
