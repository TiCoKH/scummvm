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

#include "goldbox/spells/spell_handlers.h"

#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/player_character.h"
#include "goldbox/engine.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/spells/spell_casting.h"

namespace Goldbox {
namespace Spells {

SpellCastResult UnimplementedSpellHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)context;
    (void)definition;
    (void)targets;
    return SpellCastResult(CAST_NO_HANDLER);
}

// --- Cure Light Wounds (ID03) ---
// Heals 1d8 HP on each target. Mirrors CHARACTER_HealHp + CHARACTER_ShowHealResult.
SpellCastResult CureLightWoundsHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem ? context.effectSystem->getHostBridge() : nullptr;

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        const uint8 amount = static_cast<uint8>(
            Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 8) : 4);

        if (target->healHp(amount, true) && bridge)
            bridge->showHealResult(target);
    }

    return SpellCastResult(CAST_OK);
}

// --- Burning Hands (ID09) ---
// Deals casterLevel points of fire damage to all targets (no saving throw).
SpellCastResult BurningHandsHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    const uint8 damage = context.casterLevel > 0 ? context.casterLevel : 1;

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        if (context.damageSystem)
            context.damageSystem->applyLegacy(*target, damage,
                Goldbox::Data::DAMAGE_NORMAL, false);
    }

    return SpellCastResult(CAST_OK);
}

// --- Charm Person (ID10) ---
// Only affects normal/small targets (monsterType < 2, iconDimension < 2).
// Power encodes caster's combat side: combatSide * 0x80 + 12.
// After applying the effect, fires EFF_ADD on the charm effect record.
SpellCastResult CharmPersonHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    const uint16 duration = definition.entry ?
        static_cast<uint16>(definition.entry->fixedDuration +
            definition.entry->perLvlDuration * context.casterLevel) : 0;

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        // Charm Person only affects normal/small characters.
        // monsterType and iconDimension are PoolradCharacter fields;
        // access via ADnDCharacter cast is safe since all runtime chars derive from it.
        const Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<const Goldbox::Data::ADnDCharacter *>(target);
        // iconDimension is on PlayerCharacter base.
        if (target->iconDimension >= 2) {
            if (bridge)
                bridge->postEffectMessage(target, "is unaffected", true);
            continue;
        }
        // monsterType check: only available on PoolradCharacter; skip for non-poolrad.
        // We use a duck-type check via the adnd pointer's npc field as a proxy:
        // npc >= 0x80 means NPC/monster with potentially large type — skip if so.
        // The decompiled logic checks target->type < 2; we map that to npc < 2.
        if (adnd && static_cast<uint8>(adnd->npc) >= 2) {
            if (bridge)
                bridge->postEffectMessage(target, "is unaffected", true);
            continue;
        }

        // Power: caster's combat side * 0x80 + 12 (mirrors handleCharmingGaze).
        const uint8 power = static_cast<uint8>(
            static_cast<uint8>(context.caster ? context.caster->combatSide : 0)
            * 0x80 + 12);

        context.effectSystem->addOrRefreshEffect(
            *target->getEffects(), *target,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_CHARM_PERSON),
            duration, power, true);

        if (bridge)
            bridge->postEffectMessage(target, "is charmed", true);

        // Fire EFF_ADD on the charm effect to trigger combatSide swap / ai_control.
        if (handler) {
            Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
            Goldbox::Data::Effects::Effect *charmEffect = fx ?
                fx->findEffectById(
                    static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_CHARM_PERSON))
                : nullptr;
            if (charmEffect)
                handler->apply(Goldbox::Data::Effects::EFF_ADD, *charmEffect,
                    *target, context.combat, bridge);
        }
    }

    return SpellCastResult(CAST_OK);
}

// --- Enlarge (ID12) ---
// Strength buff scaled by caster level (table: 1->0, 2->1, 3->51, 4->76, 5->91, 6->100).
// Calls applyStrengthChange; on success posts "is stronger" and adds E_ENLARGE
// with duration from computeSpellDuration and the encoded previous-strength power.
SpellCastResult EnlargeHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    // Exceptional strength value by caster level (matches decompile table).
    static const uint8 kExtStrByLevel[] = { 0, 0, 1, 51, 76, 91, 100 };
    const uint8 level = (context.casterLevel <= 6) ? context.casterLevel : 6;
    const uint8 extStr = kExtStrByLevel[level];

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem ? context.effectSystem->getHostBridge() : nullptr;

    const uint8 spellId = definition.id;
    const uint8 duration = computeSpellDuration(
        spellId, context.casterLevel, context.inCombat);

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        uint8 effectValue = 0;
        if (target->applyStrengthChange(18, extStr, effectValue)) {
            if (bridge)
                bridge->postEffectMessage(target, "is stronger", true);
        }

        // Always add the E_ENLARGE effect (raw 0x0C) with the encoded
        // previous-strength power, regardless of whether the buff applied.
        target->addEffect(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ENLARGE_STRENGTHEN),
            duration, effectValue, true);
    }

    return SpellCastResult(CAST_OK);
}

} // namespace Spells
} // namespace Goldbox
