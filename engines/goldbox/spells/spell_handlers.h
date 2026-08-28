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

// ID03 Cure Light Wounds: heals 1d8 HP on each target.
class CureLightWoundsHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID09 Burning Hands: deals casterLevel points of fire damage to all targets.
class BurningHandsHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID10 Charm Person: affects normal/small targets; adjusts power by combat side.
class CharmPersonHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID12 Enlarge: strength buff scaled by caster level; adds E_ENLARGE effect.
class EnlargeHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID13 Reduce: saving throw vs spell; removes E_POOLRAD_ENLARGE_STRENGTHEN.
class ReduceHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID14 Friends: buffs charisma by 2d4 (max 25); effect power stores old charisma.
class FriendsHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID15 Magic Missile: damage = damageLevel + rollDice(damageLevel, 4); behavior 8.
class MagicMissileHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID19 Shield: pure effect spell; delegates to GenericSpellHandler with no damage.
class ShieldHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID20 Shocking Grasp: damage = casterLevel + 1d8; behavior 12.
class ShockingGraspHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID21 Sleep: shared 4d4 budget consumed per target by level cost; pure effect.
class SleepHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID23/ID49 Hold Person: save modifier by target count (-2/-3 for 1, -1 for 2, 0 for 3-4);
// type/size check gates eligibility; shared handler for cleric and mage variants.
class HoldPersonHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID24 Resist Fire: pure effect; delegates entirely to GenericSpellHandler.
class ResistFireHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID25 Silence 15' Radius: pure effect; delegates entirely to GenericSpellHandler.
class Silence15RadiusHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID26 Slow Poison: status/effect gate; HP floor; applies effect then transitions
// raw effect 0x4E (EFF_REMOVE) and adds E_POOLRAD_POISON_DAMAGE (0x0F, dur=10, power=0xFF).
class SlowPoisonHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID27 Snake Charm: builds target list from context.enemies filtered by
// monsterType==0x0E and hp_current <= remaining HP budget (caster's current HP).
class SnakeCharmHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID28 Spiritual Hammer: generic effect path then EFF_ADD on E_POOLRAD_SPIRITUAL_HAMMER
// (0x17) fired on the caster.
class SpiritualHammerHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID32 Mirror Image: effectPowerOverride = 1d4 (number of images).
class MirrorImageHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_HANDLERS_H
