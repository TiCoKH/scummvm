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

// ID34 Stinking Cloud: allocates a CloudEffect record, paints the battlefield,
// adds E_STINKING_CLOUD_EXPAIR (raw 40) to the caster, then applies initial
// nausea status to all combatants occupying the four cloud cells.
// power byte = castingLevel | (cloudIndex << 4).
class StinkingCloudHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID35 Strength: bonus dice by class (mage=1d4, cleric/thief=1d6, fighter=1d8);
// excess over 18 converts to exceptional strength for fighters (capped at 100);
// adds E_POOLRAD_ENLARGE_STRENGTHEN (0x0C) with duration from computeSpellDuration.
class StrengthHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID36 Animate Dead: iterates allies filtered by S_DEAD + classType==0;
// re-places on combat map; converts to undead (combatSide, ai_control,
// levelUndead=2, npc=0xB2/0xB3, classType=4); revives at max HP;
// adds E_POOLRAD_ANIMATING_DEAD (0x20) with power = originalSide*16 + casterLevel.
class AnimateDeadHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID37 Cure Blindness: removes E_POOLRAD_BLINDED (0x21) from target;
// posts "can see" only when the effect was actually present and removed.
class CureBlindnessHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID39 Cure Disease: removes disease/affliction effects (0x22, 0x2B+0x2C+0x1F, 0x32+0x39).
class CureDiseaseHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID42 Prayer: effectPowerOverride = casterLevel + combatSide * 16.
class PrayerHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID43 Remove Curse: removes E_POOLRAD_ACCURSED (0x24) from target; if absent,
// clears the cursed flag on the first cursed inventory item found.
class RemoveCurseHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID44 Bestow Curse: pure generic delegate.
class BestowCurseHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID47 Fireball: damage = casterLevel d6 (or 1d3*2+1 d6 for magic item ID 0x40).
// Outdoor combat rebuilds the target list from all combatants within
// Chebyshev distance 2 of targets.tileX/Y before applying damage.
class FireballHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID48 Haste / ID55 Slow: shared helper that filters the target list to
// casterLevel targets on the given side, removes a prerequisite effect from
// each, applies the spell to those that succeeded, then fires
// ES_SAVING_THROW_MODS on each retained target.
// Haste (ID48): removes E_POOLRAD_SLOWED (0x2A) from allies.
// Slow  (ID55): removes raw haste effect (0x27) from enemies.
class HasteSlowHandler : public ISpellHandler {
public:
    explicit HasteSlowHandler(uint8 removeEffectId, bool targetEnemies)
        : _removeEffectId(removeEffectId), _targetEnemies(targetEnemies) {}

    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
private:
    uint8 _removeEffectId;
    bool  _targetEnemies;
};

// ID45 Blink: pure generic delegate.
class BlinkHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID41/ID46 Dispel Magic: iterates all effects on each target; removes those whose
// power nibble (& 0x0F) loses a level-based % roll. Power 0xFF is never dispelled.
class DispelMagicHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID40 Cause Disease: applies disease effect via generic SPELL_ApplyOnTargets path.
class CauseDiseaseHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID58 (SP_MI2): cures effect 0x37+0x16, or afflictions, or heals 1d4+8 HP.
class SpellID58Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID51 Lightning Bolt: casterLevel d6 damage; resolveAoEHitAtTile at target
// then traceSpellPath(initialAnimFrame=8, savingThrowMod=4, pathLength=damageDice).
class LightningBoltHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID60 (SP_MI4): damage = 20 + 1d6; resolveAoEHitAtTile at target
// then traceSpellPath(initialAnimFrame=3, baseDamage=20, savingThrowMod=4, pathLength=3).
class SpellID60Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// Lingering breath attack: same path-trace mechanic as Lightning Bolt but
// uses a different effect tile and damage parameters supplied by the caller.
class BreathWeaponHandler : public ISpellHandler {
public:
    explicit BreathWeaponHandler(uint8 effectTileId, uint8 baseDamage,
                                 uint8 pathLength, int8 savingThrowMod)
        : _effectTileId(effectTileId), _baseDamage(baseDamage),
          _pathLength(pathLength), _savingThrowMod(savingThrowMod) {}

    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
private:
    uint8 _effectTileId;
    uint8 _baseDamage;
    uint8 _pathLength;
    int8  _savingThrowMod;
};

// ID57 (SP_MI1): removes E_POOLRAD_SLOWED (0x2A); if removed, applies "is Speedy" via generic path.
class SpellID57Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID59 (SP_MI3): applyStrengthChange(21, 0); posts "is stronger"; adds E_POOLRAD_ENLARGE_STRENGTHEN
// with duration from computeSpellDuration.
class SpellID59Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID61 (SP_MI5): pure generic delegate — "is paralyzed".
class SpellID61Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID62 (SP_MI6): heals 2d4+2 HP; posts "is Healed".
class SpellID62Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID63 (SP_MI7): pure generic delegate — "is invisible".
class SpellID63Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID65 (SP_MI9): effectPower = rollDiceAttack(2,4)+2; applies via generic path with behavior 8.
class SpellID65Handler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

// ID56 Restore: restores one drained level, re-grants the best available
// class level, and adjusts XP to the minimum required for that level.
class RestoreHandler : public ISpellHandler {
public:
    SpellCastResult execute(const SpellContext &context,
                            const SpellDefinition &definition,
                            const TargetSelection &targets) const override;
};

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_HANDLERS_H
