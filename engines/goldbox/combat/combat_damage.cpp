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

#include "goldbox/combat/combat_damage.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Combat {

DamageResult applyDamage(CombatContext &ctx,
                         Data::PlayerCharacter *ch,
                         uint8 baseDamage,
                         DamageModifier modifier,
                         bool applyModifier,
                         Data::Effects::EffectRuntime *effectRuntime) {
    using namespace Data::Effects;

    DamageResult result;

    if (!ch)
        return result;

    // BYTE_DAMAGE is the mutable current-hit damage value in the original
    // runtime. Resistance/effect handlers modify this before DamageSystem
    // applies the character damage.
    ctx.globals.damage = baseDamage;

    // ETS_ON_DAMAGE_TAKEN (set 6): resistance/immunity checks that may
    // modify globals.behaviorFlags before damage is calculated.
    if (effectRuntime && ch->getEffects())
        effectRuntime->checkEffectSet(ES_ON_DAMAGE_TAKEN,
                *ch->getEffects(), *ch, &ctx.globals);

    result.behaviorFlags = ctx.globals.behaviorFlags;

    Data::DamageSystem damageSystem(nullptr);
    const Data::DamageResult dataResult = damageSystem.apply(
        *ch, Data::DamageRequest(ctx.globals.damage, false,
            static_cast<Data::DamageModifier>(modifier), applyModifier,
            ctx.globals.behaviorFlags));

    result.finalDamage = static_cast<uint8>(MIN<int>(dataResult.applied, 0xff));
    result.spellLost = dataResult.interruptedSpell;
    result.wentDown = dataResult.wentDown;
    result.wasKilled = dataResult.killed;
    result.isDying = (ch->healthStatus == Data::S_DYING);

    if (result.wentDown) {
        // Legacy logic decremented SIDE_MEMBERS immediately.
        // Here side counts are derived from roster, so refresh once the
        // character transitions to a disabled state.
        ctx.updateSideCount();

        if (effectRuntime && ch->getEffects()) {
            ch->getEffects()->clear();
            effectRuntime->checkEffectSet(ES_ON_DEATH,
                    *ch->getEffects(), *ch, &ctx.globals);
        }
    }

    return result;
}

} // namespace Combat
} // namespace Goldbox
