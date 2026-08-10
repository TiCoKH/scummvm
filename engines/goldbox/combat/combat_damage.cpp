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

    // ETS_ON_DAMAGE_TAKEN (set 6): resistance/immunity checks that may
    // modify globals.behaviorFlags before damage is calculated.
    if (effectRuntime && ch->getEffects())
        effectRuntime->applyTriggerSet(ETS_ON_DAMAGE_TAKEN,
                *ch->getEffects(), *ch, &ctx.globals);

    result.behaviorFlags = ctx.globals.behaviorFlags;

    uint8 finalDamage = baseDamage;
    if (applyModifier) {
        if (modifier == DAMAGE_NULLIFY)
            finalDamage = 0;
        else if (modifier == DAMAGE_HALF)
            finalDamage >>= 1;
    }

    result.finalDamage = finalDamage;

    if (finalDamage == 0)
        return result;

    ch->damage(finalDamage);

    // Interrupt spell casting.
    if (ch->combatState) {
        ch->combatState->canCast = false;
        if (ch->combatState->spellId != 0) {
            result.spellLost = true;
            CharacterEffects *fx = ch->getEffects();
            if (fx) {
                int idx = fx->findEffectIndexByType(ch->combatState->spellId);
                if (idx >= 0)
                    fx->removeEffectAt(static_cast<uint>(idx));
            }
            ch->combatState->spellId = 0;
        }
    }

    if (!ch->enabled) {
        result.wentDown = true;
        result.wasKilled = (ch->healthStatus == Data::S_DEAD ||
                            ch->healthStatus == Data::S_GONE ||
                            ch->healthStatus == Data::S_STONED);
        result.isDying   = (ch->healthStatus == Data::S_DYING);

        if (effectRuntime && ch->getEffects()) {
            ch->getEffects()->clear();
            effectRuntime->applyTriggerSet(ETS_ON_DEATH,
                    *ch->getEffects(), *ch, &ctx.globals);
        }
    }

    return result;
}

} // namespace Combat
} // namespace Goldbox
