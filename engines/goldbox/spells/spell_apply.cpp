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

#include "goldbox/spells/spell_apply.h"

#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/rules/saving_throw.h"
#include "goldbox/poolrad/effect_handler.h"
#include "goldbox/spells/spell_duration.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Spells {

SpellCastResult applyOnTargets(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets,
        const SpellApplyParams &params) {
    if (!definition.entry || !context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    const Goldbox::Data::Spells::SpellEntry &entry = *definition.entry;
    const uint8 spellId = static_cast<uint8>(definition.id);

    // ST_CASTER spells select no targets explicitly outside combat (see
    // spell_targeter.cpp); fall back to the caster so the effect still lands.
    // In combat the targeter always supplies the list (an empty burst hits
    // nobody, like the original).
    Common::Array<Goldbox::Data::PlayerCharacter *> singleCaster;
    const Common::Array<Goldbox::Data::PlayerCharacter *> *list =
        &targets.targetCharacters;
    if (list->empty() && context.caster && !context.inCombat) {
        singleCaster.push_back(context.caster);
        list = &singleCaster;
    }
    if (list->empty())
        return SpellCastResult(context.inCombat ? CAST_OK
                                                : CAST_INVALID_TARGET);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    const Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();
    Goldbox::Data::Effects::EffectRuntime *runtime =
        context.effectSystem->getRuntime();

    const uint8 effectPower = params.effectPowerOverride
        ? params.effectPowerOverride : context.casterLevel;
    const Common::String &message = params.message.empty()
        ? definition.effectMessage : params.message;

    // Like the original, a miss zeroes the damage for the rest of the list.
    uint8 baseDamage = params.baseDamage;

    for (uint i = 0; i < list->size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = (*list)[i];
        if (!target)
            continue;

        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(target);

        bool saved = false;
        if (entry.savingEffect != Goldbox::Data::Spells::DMG_NO_SAVE && adnd)
            saved = Goldbox::Data::Rules::checkSavingThrow(*adnd,
                context.combat, handler, bridge, entry.saveType, 0);

        // need_ar: the spell must beat the target's armour class.
        if (entry.fixedRange == 0xFF && context.caster && context.inCombat) {
            if (adnd)
                adnd->recalcCombatStats();
            if (runtime && target->getEffects())
                runtime->checkEffectSet(
                    Goldbox::Data::Effects::ES_ALIGN_PROTECTION,
                    *target->getEffects(), *target, context.combat);
            const bool hit = Goldbox::Combat::rollToHit(context.caster, target,
                static_cast<uint8>(target->armorClass.getCurrent()));
            if (!hit) {
                baseDamage = 0;
                saved = true;
            }
        }

        if (baseDamage != 0 && context.damageSystem) {
            context.damageSystem->applyLegacy(*target, baseDamage,
                static_cast<Goldbox::Data::DamageModifier>(entry.savingEffect),
                saved, params.behaviorFlags);
        }

        if (entry.effectId != 0) {
            const uint8 duration = computeSpellDuration(spellId,
                context.casterLevel, context.inCombat);
            Goldbox::Poolrad::checkUnaffectedEffect(*target, handler, bridge,
                context.combat, entry.effectId, duration, effectPower,
                params.effectImmediate,
                entry.savingEffect != Goldbox::Data::Spells::DMG_NO_SAVE,
                saved, message.c_str());
        }
    }

    return SpellCastResult(CAST_OK);
}

SpellCastResult applyToSide(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets,
        Goldbox::Data::CombatSide side,
        const Common::String &message) {
    TargetSelection filtered;
    filtered.tileX = targets.tileX;
    filtered.tileY = targets.tileY;

    Goldbox::Combat::CombatContext *cc = context.inCombat
        ? VmInterface::getCombatContext() : nullptr;

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target || target->combatSide != side)
            continue;

        // Bless (spell 1) skips targets that are engaged in melee.
        if (definition.id == Goldbox::Data::Spells::SP_CL1_BLESS && cc) {
            cc->buildTargetList(target, 1);
            if (!cc->targetList.targetOrder.empty())
                continue;
        }
        filtered.targetCharacters.push_back(target);
    }

    if (context.combat)
        context.combat->multiTarget = true;

    SpellApplyParams params;
    params.message = message;
    return applyOnTargets(context, definition, filtered, params);
}

} // namespace Spells
} // namespace Goldbox
