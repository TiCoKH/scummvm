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
#include "goldbox/core/coords.h"
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

// Interactive aiming hook for player-controlled combat casts (mirrors the
// COMBAT_AttackTarget call in COMBAT_spellTargetSelection). Implemented by
// the combat view; may be null, in which case the combat targeter falls back
// to an automatic choice.
class ISpellAimer {
public:
    virtual ~ISpellAimer() {}

    // Aim at a combatant (tileMode == false) or at a tile for a burst
    // (tileMode == true). range is SPELL_ComputeRange in tiles. On success
    // returns true with tile set and target set when a combatant was chosen.
    // Returns false when the player cancelled.
    virtual bool aim(Goldbox::Data::PlayerCharacter *caster, uint8 spellId,
            uint8 range, bool tileMode,
            Goldbox::Data::PlayerCharacter *&target, TilePos &tile) = 0;
};

// Presentation hook fired once targeting succeeded, before the memorized slot
// is spent and the spell handler runs (the caster animation block of
// ACTION_UseSpell). May be null.
class ISpellCastObserver {
public:
    virtual ~ISpellCastObserver() {}

    virtual void onSpellTargeted(Goldbox::Data::PlayerCharacter *caster,
            uint8 spellId, TilePos targetTile) = 0;

    // Per-target projectile (effect quad, sound, path) used by multi-target
    // spells such as Hold Person.
    virtual void onSpellProjectile(Goldbox::Data::PlayerCharacter *caster,
            Goldbox::Data::PlayerCharacter *target) {
        (void)caster;
        (void)target;
    }

    // Message shown by a spell handler (e.g. "Creates a noxious cloud").
    virtual void onSpellMessage(Goldbox::Data::PlayerCharacter *caster,
            const Common::String &text) {
        (void)caster;
        (void)text;
    }
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

    // Optional combat aiming hook for player-controlled casts.
    ISpellAimer *aimer;

    // Optional presentation hook fired after successful targeting.
    ISpellCastObserver *observer;

    // True when the caster is AI controlled (selects targets automatically
    // and aborts instead of prompting).
    bool aiControl;

    // Optional — non-null only during combat. Passed to GenericSpellHandler
    // for attack-roll checks and effect-set evaluation.
    Goldbox::Combat::CombatGlobals *combat;

    // Optional — used by GenericSpellHandler to apply direct damage for
    // spells whose fixedRange == SP_ATTACK_ROLL (need_ar path).
    Goldbox::Data::DamageSystem *damageSystem;

    // Set when the spell originates from a magic item rather than a memorized
    // spell slot. Preserves BOOL_MAGIC_ITEM context for downstream handlers.
    bool fromMagicItem;

    SpellContext() : caster(nullptr), spellBook(nullptr),
        effectSystem(nullptr), inCombat(false), casterLevel(0),
        targetPicker(nullptr), aimer(nullptr), observer(nullptr),
        aiControl(false),
        combat(nullptr), damageSystem(nullptr), fromMagicItem(false) {}
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_CONTEXT_H
