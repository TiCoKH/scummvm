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

#include "goldbox/spells/spell_duration.h"
#include "goldbox/combat/cloud_effect_manager.h"
#include "goldbox/poolrad/effect_handler.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/core/direction.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_inventory.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/poolrad/data/poolrad_effects.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/rules/saving_throw.h"
#include "goldbox/engine.h"
#include "goldbox/runtime/runtime_exchange.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/spells/spell_generic_handler.h"
#include "goldbox/spells/spell_apply.h"
#include "goldbox/spells/spell_aoe.h"

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
// Heals 1d8 HP on each target. FxEngine.RestoreHitPoints with bypassRot
// False, so mummy rot blocks it.
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

        if (target->healHp(amount, false) && bridge)
            bridge->showHealResult(target);
    }

    return SpellCastResult(CAST_OK);
}

// --- Bless (ID01) / Curse (ID02) / Cause Light Wounds (ID04) ---
SpellCastResult BlessHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.caster)
        return SpellCastResult(CAST_ERROR);
    return applyToSide(context, definition, targets,
        context.caster->combatSide, definition.effectMessage);
}

SpellCastResult CurseHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.caster)
        return SpellCastResult(CAST_ERROR);
    const Goldbox::Data::CombatSide side =
        (context.caster->combatSide == Goldbox::Data::CS_PARTY)
        ? Goldbox::Data::CS_ENEMY : Goldbox::Data::CS_PARTY;
    return applyToSide(context, definition, targets, side,
        definition.effectMessage);
}

SpellCastResult CauseLightWoundsHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    const uint8 damage = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(1, 8)) : 4;

    SpellApplyParams params;
    params.baseDamage = damage;
    params.behaviorFlags = 8;
    params.damageDice = 1;
    return applyOnTargets(context, definition, targets, params);
}

// --- Burning Hands (ID09) ---
// Deals casterLevel points of fire damage to all targets (no saving throw).
SpellCastResult BurningHandsHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    SpellApplyParams params;
    params.baseDamage = context.casterLevel;
    params.behaviorFlags = 9;
    return applyOnTargets(context, definition, targets, params);
}

// --- Charm Person (ID10) ---
// Magic.CharmPersonHandler: only charType 0..1 with icon size 1 can be
// charmed. ResolveSpell adds the effect (save negates) at strength
// hostile * 128 + caster level, then the Charm handler runs with
// removed = False.
SpellCastResult CharmPersonHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (targets.targetCharacters.empty() || !targets.targetCharacters[0])
        return SpellCastResult(CAST_INVALID_TARGET);

    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[0];
    const Goldbox::Poolrad::Data::PoolradCharacter *pc =
        dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(target);
    const uint8 charType = pc ? pc->monsterType : 0;
    if (charType > 1 || target->iconDimension > 1) {
        if (bridge)
            bridge->postEffectMessage(target, "is unaffected", true);
        return SpellCastResult(CAST_OK);
    }

    const uint8 hostile = (context.caster &&
            context.caster->combatSide == Goldbox::Data::CS_ENEMY) ? 1 : 0;
    SpellApplyParams params;
    params.effectPowerOverride =
        static_cast<uint8>(hostile * 128 + context.casterLevel);
    params.effectImmediate = true;
    params.message = "is charmed";
    applyOnTargets(context, definition, targets, params);

    Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
    Goldbox::Data::Effects::Effect *charmEffect = fx ?
        fx->findEffectById(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_CHARM))
        : nullptr;
    if (charmEffect && handler)
        handler->apply(Goldbox::Data::Effects::EFF_ADD, *charmEffect,
            *target, context.combat, bridge);

    return SpellCastResult(CAST_OK);
}

// --- Enlarge (ID12) ---
// Strength 18 with exceptional strength by caster level (1->0, 2->1, 3->51,
// 4->76, 5->91, 6->100), then a plain AddEffect (no save or message) whose
// power is the encoded previous strength. The original leaves levels above 6
// at a stale value; they are clamped to 6 here.
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

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        uint8 effectValue = 0;
        if (target->applyStrengthChange(18, extStr, effectValue)) {
            if (bridge)
                bridge->postEffectMessage(target, "is stronger", true);
        }

        target->addEffect(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ENLARGE),
            computeSpellDuration(static_cast<uint8>(definition.id),
                context.casterLevel, context.inCombat),
            effectValue, true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Reduce (ID13) ---
// Saving throw vs spell negates. If save fails, removes E_POOLRAD_ENLARGE
// (0x0C) from the target and posts "has been reduced".
SpellCastResult ReduceHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(target);
        if (adnd && Goldbox::Data::Rules::checkSavingThrow(
                *adnd, context.combat, handler, bridge,
                Goldbox::Data::Spells::SVS_SPELL, 0))
            continue;

        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (!fx)
            continue;

        if (context.effectSystem->removeEffectById(
                *target, *fx,
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ENLARGE))) {
            if (bridge)
                bridge->postEffectMessage(target, "has been reduced", true);
        }
    }

    return SpellCastResult(CAST_OK);
}

// --- Friends (ID14) ---
// Buffs caster's charisma by 2d4 (capped at 25), then applies the generic
// effect path with oldCharisma as the power so the effect handler can
// restore the original value on expiry.
SpellCastResult FriendsHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.caster)
        return SpellCastResult(CAST_INVALID_TARGET);

    const uint8 oldCharisma = context.caster->abilities.charisma.current;
    const uint8 roll = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(2, 4)) : 4;
    const uint8 newCharisma = static_cast<uint8>(
        MIN<uint16>(oldCharisma + roll, 25));
    context.caster->abilities.charisma.current = newCharisma;

    return GenericSpellHandler::applyToTargets(context, definition, targets, oldCharisma, true);
}

// --- Magic Missile (ID15) ---
// damageLevel = (casterLevel + 1) >> 1
// damage = damageLevel + rollDice(damageLevel, 4), element mask 8.
// The original uses casterLevel >> 1, so a 1st-level caster does no damage;
// rounding up keeps one missile at 1st level.
SpellCastResult MagicMissileHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    const uint8 damageLevel = static_cast<uint8>((context.casterLevel + 1) >> 1);
    const uint8 roll = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(damageLevel, 4)) : damageLevel;
    const uint8 baseDamage = static_cast<uint8>(damageLevel + roll);

    SpellApplyParams params;
    params.baseDamage = baseDamage;
    params.behaviorFlags = 8;
    params.damageDice = damageLevel;
    return applyOnTargets(context, definition, targets, params);
}

// --- Shield (ID19) ---
// Pure effect spell; no damage. Delegates entirely to GenericSpellHandler.
SpellCastResult ShieldHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Shocking Grasp (ID20) ---
// damage = casterLevel + 1d8; applies with behavior flag 12.
SpellCastResult ShockingGraspHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    const uint8 roll = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(1, 8)) : 4;
    const uint8 baseDamage = static_cast<uint8>(context.casterLevel + roll);

    SpellApplyParams params;
    params.baseDamage = baseDamage;
    params.behaviorFlags = 12;
    params.damageDice = 1;
    return applyOnTargets(context, definition, targets, params);
}

// --- Sleep (ID21) ---
// Rolls 4d4 as a shared budget. Each target consumes a level-based cost;
// targets already carrying E_POOLRAD_SLEEP or whose cost exceeds the
// remaining budget are removed from the list before the generic effect path.
SpellCastResult SleepHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    uint8 sleepPower = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(4, 4)) : 8;

    TargetSelection filtered;
    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        // PoolradCharacter shadows the base status and holds highestLevel.
        const Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
            dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(target);

        // Animated (undead) targets are immune.
        if (poolrad && poolrad->healthStatus ==
                static_cast<uint8>(Goldbox::Data::S_ANIMATED))
            continue;

        // Already sleeping — skip.
        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (fx && fx->hasEffect(
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SLEEP)))
            continue;
        const uint8 highestLevel = poolrad ? poolrad->highestLevel : 1;

        uint8 consume;
        if (highestLevel <= 1)       consume = 1;
        else if (highestLevel == 2)  consume = 2;
        else if (highestLevel == 3)  consume = 4;
        else if (highestLevel == 4)  consume = 6;
        else if (highestLevel == 5)  consume = (target->race != 0) ? 20 : 10;
        else                         consume = 20;

        if (sleepPower < consume)
            continue;

        sleepPower -= consume;
        filtered.targetCharacters.push_back(target);
    }

    if (filtered.targetCharacters.empty())
        return SpellCastResult(CAST_OK);

    return GenericSpellHandler::applyToTargets(context, definition, filtered, 0);
}

// --- Hold Person (ID23 / ID49) ---
// Save modifier depends on target count: 1 target -> -2 (cleric) or -3 (mage),
// 2 targets -> -1, 3-4 targets -> 0.
// Targets with monsterType > 1 or iconDimension > 1 automatically save.
// Processes targets in reverse order to match original iteration.
//
// TODO: presentation layer not yet implemented.
//   Original sequence per secondary target (targetIndex < targetCount):
//     SOUND_Setup(0)
//     SOUND_Play(ARRAY_SOUND_MAP[3])  // m68k: SOUND_Play(SOUND_ID_0x03)
//     GFX_LoadEffectTileQuad(0x12)
//     COMBAT_AnimateMissilePath(caster, target)
//     SOUND_Setup(10)
//   The first target receives no projectile animation.
//   Wire this up once the sound/GFX subsystems are available.
SpellCastResult HoldPersonHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!definition.entry || !context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    const uint count = targets.targetCharacters.size();
    if (count == 0)
        return SpellCastResult(CAST_INVALID_TARGET);

    // Save modifier: cleric (ID23) vs mage (ID49) differ only at count == 1.
    const bool isCleric =
        (definition.id == Goldbox::Data::Spells::SP_CL2_HOLD_PERSON);
    int8 saveAdj;
    if (count == 1)       saveAdj = isCleric ? -2 : -3;
    else if (count == 2)  saveAdj = -1;
    else                  saveAdj = 0;   // 3 or 4 targets

    const uint16 duration = computeSpellDuration(
        static_cast<uint8>(definition.id), context.casterLevel,
        context.inCombat);
    const bool usesSave = definition.entry->savingEffect !=
        Goldbox::Data::Spells::DMG_NO_SAVE;

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    const Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    // Iterate in reverse to match original target-list traversal order.
    for (uint i = count; i-- > 0;) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        // The highest-index target is processed first and gets no projectile;
        // every other target does.
        if (i + 1 < count && context.observer && context.caster)
            context.observer->onSpellProjectile(context.caster, target);

        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(target);
        bool saved = adnd && Goldbox::Data::Rules::checkSavingThrow(
            *adnd, context.combat, handler, bridge,
            definition.entry->saveType, saveAdj);

        // Non-standard size/type targets always save.
        const Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
            dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(target);
        const uint8 monsterType = poolrad ? poolrad->monsterType : 0;
        if (monsterType > 1 || target->iconDimension > 1)
            saved = true;

        // UTIL_CheckUnaffected: "is Unaffected" when saved, otherwise the
        // effect is added (not immediate) with the caster level as power.
        Goldbox::Poolrad::checkUnaffectedEffect(*target, handler, bridge,
            context.combat, definition.entry->effectId, duration,
            context.casterLevel, false, usesSave, saved, "is held");
    }

    return SpellCastResult(CAST_OK);
}

// --- Resist Fire (ID24) ---
// Pure effect spell; delegates entirely to GenericSpellHandler.
SpellCastResult ResistFireHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Silence 15' Radius (ID25) ---
// Pure effect spell; delegates entirely to GenericSpellHandler.
SpellCastResult Silence15RadiusHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Slow Poison (ID26) ---
// Single-target spell with precondition checks:
//   1. Target must not be STATUS_ANIMATED.
//   2. Target must carry raw effect 0x37 (active poison).
//   3. Target HP is floored to 1 before effect application.
// After the generic effect path (effectPowerOverride=0xFF), runs the
// HEAL_WAKE (0x4E) handler with no node, which revives the target, then adds
// E_POOLRAD_SLOW_POISON_HIT (0x0F) with duration=10 and power=0xFF.
SpellCastResult SlowPoisonHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    // Slow Poison operates on a single target (first in list).
    Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[0];
    if (!target)
        return SpellCastResult(CAST_INVALID_TARGET);

    // Animated targets cannot be affected.
    const Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
        dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(target);
    if (poolrad && poolrad->healthStatus == static_cast<uint8>(Goldbox::Data::S_ANIMATED))
        return SpellCastResult(CAST_NOT_ALLOWED);

    // Target must carry the active poison effect (raw 0x37).
    Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
    if (!fx || !fx->hasEffect(0x37))
        return SpellCastResult(CAST_NOT_ALLOWED);

    // Ensure target is not left at zero HP.
    if (target->hitPoints.current == 0)
        target->hitPoints.current = 1;

    // Apply the Slow Poison effect (effectPowerOverride=0xFF mirrors level override 0xFF).
    TargetSelection single;
    single.targetCharacters.push_back(target);
    GenericSpellHandler::applyToTargets(context, definition, single, 0xFF, true);

    // FxEngine.CallEffectHandler(HEAL_WAKE, tgt, nil, True): the target
    // need not carry the effect.
    if (handler) {
        Goldbox::Data::Effects::Effect wake;
        wake.id = static_cast<uint8>(
            Goldbox::Data::Effects::E_POOLRAD_HEAL_WAKE);
        wake.power = 0xFF;
        handler->apply(Goldbox::Data::Effects::EFF_REMOVE, wake,
            *target, context.combat, bridge);
    }

    // Add the resulting slow-poison marker: effect 0x0F, duration 10, power 0xFF.
    target->addEffect(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SLOW_POISON_HIT),
            10, 0xFF, true);

    return SpellCastResult(CAST_OK);
}

// --- Snake Charm (ID27) ---
// Ignores the pre-selected target list. Builds its own from every creature:
// only characters with monsterType == 0x0E (snake) whose hp_current fits
// within the remaining HP budget (starting at caster's hp_current) are kept.
// Each accepted snake consumes its hp_current from the budget.
// The filtered list is then passed to GenericSpellHandler for effect application.
SpellCastResult SnakeCharmHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)targets;
    if (!context.caster || !context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    uint8 remainingHp = context.caster->hitPoints.current;

    // GDVars.creatureList: every creature, whatever its side.
    Goldbox::Combat::CombatContext *ctx = context.inCombat && Goldbox::g_engine
        ? Goldbox::g_engine->getCombatContext() : nullptr;
    const Common::Array<Goldbox::Data::PlayerCharacter *> &creatures =
        ctx ? ctx->params.roster : context.allies;

    TargetSelection filtered;
    for (uint i = 0; i < creatures.size(); ++i) {
        Goldbox::Data::PlayerCharacter *candidate = creatures[i];
        if (!candidate)
            continue;

        const Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
            dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(candidate);
        if (!poolrad || poolrad->monsterType != 0x0E)
            continue;

        if (candidate->hitPoints.current > remainingHp)
            continue;

        remainingHp -= candidate->hitPoints.current;
        filtered.targetCharacters.push_back(candidate);
    }

    if (filtered.targetCharacters.empty())
        return SpellCastResult(CAST_OK);

    return GenericSpellHandler::applyToTargets(context, definition, filtered, 0);
}

// --- Spiritual Hammer (ID28) ---
// Applies the generic effect path on the caster, then fires EFF_ADD on
// E_POOLRAD_SPIRIT_HAMMER (0x17) to activate the weapon-creation side effect.
SpellCastResult SpiritualHammerHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    SpellCastResult result =
        GenericSpellHandler::applyToTargets(context, definition, targets, 0, true);
    if (result.status != CAST_OK)
        return result;

    // Fire EFF_ADD on the spiritual hammer effect to trigger weapon creation.
    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();
    if (handler && context.caster) {
        Goldbox::Data::Effects::CharacterEffects *fx = context.caster->getEffects();
        Goldbox::Data::Effects::Effect *hammerEffect = fx ?
            fx->findEffectById(
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SPIRIT_HAMMER))
            : nullptr;
        if (hammerEffect)
            handler->apply(Goldbox::Data::Effects::EFF_ADD, *hammerEffect,
                *context.caster, context.combat,
                context.effectSystem->getHostBridge());
    }

    return SpellCastResult(CAST_OK);
}

// --- Mirror Image (ID32) ---
// effectPowerOverride = 1d4 (number of duplicate images created).
SpellCastResult MirrorImageHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    const uint8 images = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(1, 4)) : 2;
    return GenericSpellHandler::applyToTargets(context, definition, targets, images);
}

// --- Stinking Cloud (ID34) ---
// Creates a persistent cloud object on the battlefield centered on targets.tileX/Y.
// power byte = castingLevel (low nibble) | cloudIndex (high nibble), matching
// CloudEffectManager's encoding: cloudIndex = countOwnedBy(caster) before insert.
// Adds E_STINKING_CLOUD_EXPAIR (raw 40) to the caster so the cloud expires when
// the effect ticks out. Then applies initial nausea to all combatants currently
// occupying the four cloud cells.
//
// After the cloud is created the caster's "Creates a noxious cloud" message
// is shown through the cast observer (which also redraws the viewport).
SpellCastResult StinkingCloudHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.caster || !context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Combat::CombatContext *ctx =
        Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr;
    if (!ctx)
        return SpellCastResult(CAST_ERROR);

    const uint8 centerX = static_cast<uint8>(targets.tileX);
    const uint8 centerY = static_cast<uint8>(targets.tileY);

    // Allocate the cloud record, paint tiles, and get the owner-relative index.
    const uint8 cloudIndex = ctx->clouds.create(context.caster, TilePos(centerX, centerY));

    // power = castingLevel in low nibble, cloudIndex in high nibble.
    const uint8 power = static_cast<uint8>(
        (context.casterLevel & 0x0F) | ((cloudIndex & 0x0F) << 4));

    // Add the persistent cloud effect to the caster.
    // Duration = castingLevel rounds (mirrors original CHARACTER_addEffect call).
    Goldbox::Data::Effects::CharacterEffects *casterFx = context.caster->getEffects();
    if (casterFx) {
        context.caster->addEffect(
            static_cast<uint8>(Goldbox::Data::Effects::E_STINKING_CLOUD_EXPAIR),
            context.casterLevel, power, true);
    }

    // Apply initial nausea to all combatants currently in the four cloud cells.
    // CloudEffectManager::kCloudDirections = {8, 2, 3, 4}.
    static const uint8 kCloudDirs[4] = { 8, 2, 3, 4 };

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    for (int dir = 0; dir < 4; ++dir) {
        const uint8 wireDir = kCloudDirs[dir];
        const uint8 cx = static_cast<uint8>(centerX + kDirDeltaX[wireDir]);
        const uint8 cy = static_cast<uint8>(centerY + kDirDeltaY[wireDir]);

        const uint8 occupantIdx = ctx->table.getOccupant(cx, cy);
        if (occupantIdx == 0)
            continue;

        Goldbox::Data::PlayerCharacter *occupant =
            ctx->table.getCharacter(occupantIdx - 1);
        if (!occupant)
            continue;

        Goldbox::Poolrad::checkCloudEffect(*occupant, handler, bridge);
    }

    if (context.observer)
        context.observer->onSpellMessage(context.caster,
            "Creates a noxious cloud");

    return SpellCastResult(CAST_OK);
}

// --- Strength (ID35) ---
// Bonus dice by class (highest priority wins): fighter=1d8, cleric/thief=1d6, mage=1d4.
// Excess over 18 converts to exceptional strength for fighters (capped at 100).
// Adds E_POOLRAD_STRENGTH (0x26) with duration from computeSpellDuration.
SpellCastResult StrengthHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(target);

        // Determine bonus dice by class (priority: fighter > cleric/thief > mage).
        uint8 bonus = 0;
        if (adnd) {
            if (adnd->levels[Goldbox::Data::C_MAGICUSER] > 0)
                bonus = static_cast<uint8>(
                    Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 4) : 2);
            if (adnd->levels[Goldbox::Data::C_CLERIC] > 0 ||
                    adnd->levels[Goldbox::Data::C_THIEF] > 0)
                bonus = static_cast<uint8>(
                    Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 6) : 3);
            if (adnd->levels[Goldbox::Data::C_FIGHTER] > 0)
                bonus = static_cast<uint8>(
                    Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 8) : 4);
        }

        uint8 newStr = static_cast<uint8>(target->abilities.strength.current + bonus);
        uint8 newExtStr = 0;

        if (newStr > 18) {
            if (adnd && adnd->levels[Goldbox::Data::C_FIGHTER] > 0) {
                newExtStr = static_cast<uint8>(
                    MIN<uint16>(target->abilities.strException.current +
                        (newStr - 18) * 10, 100));
            }
            newStr = 18;
        }

        uint8 effectPower = 0;
        const bool changed = target->applyStrengthChange(newStr, newExtStr, effectPower);
        if (changed && bridge)
            bridge->postEffectMessage(target, "is stronger", true);

        const uint16 duration = computeSpellDuration(
            static_cast<uint8>(Goldbox::Data::Spells::SP_MUL2_STRENGTH),
            context.casterLevel, context.inCombat);

        target->addEffect(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_STRENGTH),
            duration, effectPower, true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Animate Dead (ID36) ---
// Magic.AnimateDeadHandler: up to casterLevel dead humans (charType 0) from
// every creature in the combat, each only if it fits back on its own cell.
// It joins the caster's side as an undead (charType 4) under AI control and
// is revived at full HP with E_POOLRAD_ANIMATE_DEAD (0x20), power
// originalSide * 16 + casterLevel.
SpellCastResult AnimateDeadHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)targets;
    if (!context.caster || !context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Combat::CombatContext *ctx =
        Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr;
    if (!ctx)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    const Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    uint8 remaining = context.casterLevel;
    const Common::Array<Goldbox::Data::PlayerCharacter *> &roster =
        ctx->params.roster;

    for (uint i = 0; i < roster.size() && remaining > 0; ++i) {
        Goldbox::Data::PlayerCharacter *ch = roster[i];
        Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(ch);
        if (!poolrad || poolrad->healthStatus != Goldbox::Data::S_DEAD ||
                poolrad->monsterType != 0)
            continue;

        // The revive message already reads the new side, so switch it first
        // and undo it when the cell is taken.
        const Goldbox::Data::CombatSide originalSide = ch->combatSide;
        ch->combatSide = context.caster->combatSide;
        if (!Goldbox::Poolrad::reviveCreature(*ch, ch->hitPoints.max,
                bridge)) {
            ch->combatSide = originalSide;
            continue;
        }
        --remaining;

        const uint8 effectPower = static_cast<uint8>(
            static_cast<uint8>(originalSide) * 16 + context.casterLevel);
        ch->ai_control = true;
        poolrad->levelUndead = 2;
        poolrad->attackLevel = 0;
        poolrad->movement.base = 6;
        for (int j = 0; j <= 20; ++j)
            poolrad->spells.memorizedSpells[j] = 0;
        // C_Npc.ANIMATED_NPC / ANIMATED_PC by the NPC flag (bit 7).
        poolrad->npc = (poolrad->npc < 0) ?
            static_cast<int8>(0xB2) : static_cast<int8>(0xB3);
        poolrad->monsterType = 4;
        if (ch->combatState)
            ch->combatState->target = nullptr;

        Goldbox::Poolrad::checkUnaffectedEffect(*ch, handler, bridge,
            context.combat,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ANIMATE_DEAD),
            0, effectPower, true, false, false, "is animated");
        ch->healthStatus = Goldbox::Data::S_ANIMATED;
    }

    return SpellCastResult(CAST_OK);
}

// --- Cure Blindness (ID37) ---
// Removes E_POOLRAD_BLIND (0x21) from the target via removeEffectById,
// which fires EFF_REMOVE on the effect handler before erasing the record.
// Posts "can see" only when the effect was actually present and removed.
SpellCastResult CureBlindnessHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (!fx)
            continue;

        if (context.effectSystem->removeEffectById(
                *target, *fx,
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_BLIND)) && bridge)
            bridge->postEffectMessage(target, "can see", true);
    }

    return SpellCastResult(CAST_OK);
}

// --- removeAfflictions helper ---
// Removes disease/affliction effect groups from a single target.
// Mirrors SPELL_RemoveAfflictions: removes 0x22, 0x2B+{0x2C,0x1F}, 0x32+{0x39}.
// Returns true if any effect was removed.
static bool removeAfflictions(Goldbox::Data::Effects::EffectSystem *effectSystem,
        Goldbox::Data::PlayerCharacter *target) {
    if (!effectSystem || !target)
        return false;

    Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
    if (!fx)
        return false;

    bool removed = false;

    // Magic.RemoveDiseaseEffects holds the effect lock so the disease
    // handlers do not re-arm while being removed.
    if (Goldbox::g_engine)
        Goldbox::g_engine->setInSpellProcess(true);

    if (effectSystem->removeEffectById(*target, *fx, 0x22))
        removed = true;

    if (effectSystem->removeEffectById(*target, *fx, 0x2B)) {
        removed = true;
        fx->eraseEffectById(0x2C);
        fx->eraseEffectById(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_HELPLESS));
    }

    if (effectSystem->removeEffectById(*target, *fx,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ROT_NO_HEAL))) {
        removed = true;
        fx->eraseEffectById(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_MUMMY_ROT));
    }

    if (Goldbox::g_engine)
        Goldbox::g_engine->setInSpellProcess(false);

    return removed;
}

// --- Cure Disease (ID39) ---
SpellCastResult CureDiseaseHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    for (uint i = 0; i < targets.targetCharacters.size(); ++i)
        removeAfflictions(context.effectSystem, targets.targetCharacters[i]);

    return SpellCastResult(CAST_OK);
}

// --- Prayer (ID42) ---
// effectPowerOverride = casterLevel + caster's combatSide * 16.
SpellCastResult PrayerHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    const uint8 power = static_cast<uint8>(
        context.casterLevel +
        static_cast<uint8>(context.caster ? context.caster->combatSide : 0) * 16);
    return GenericSpellHandler::applyToTargets(context, definition, targets, power);
}

// --- Remove Curse (ID43) ---
// 1. Try to remove E_POOLRAD_BESTOW_CURSE (0x24) from the target; post "is un-cursed" if removed.
// 2. Otherwise scan inventory for the first cursed item, clear its flag, post "'s item is un-cursed".
SpellCastResult RemoveCurseHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[0];
    if (!target)
        return SpellCastResult(CAST_INVALID_TARGET);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();

    if (fx && context.effectSystem->removeEffectById(
            *target, *fx,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_BESTOW_CURSE))) {
        if (bridge)
            bridge->postEffectMessage(target, "is un-cursed", true);
        return SpellCastResult(CAST_OK);
    }

    Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<Goldbox::Data::ADnDCharacter *>(target);
    if (adnd) {
        for (Goldbox::Data::Items::CharacterItem &item : adnd->inventory.items()) {
            if (item.cursed) {
                item.cursed = 0;
                if (bridge)
                    bridge->postEffectMessage(target, "'s item is un-cursed", true);
                return SpellCastResult(CAST_OK);
            }
        }
    }

    return SpellCastResult(CAST_OK);
}

// --- Bestow Curse (ID44) ---
// Pure generic delegate.
SpellCastResult BestowCurseHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Haste (ID48) / Slow (ID55) shared helper ---
// Mirrors SPELL_ApplyEffectToLimitedTargets. Filters targets to casterLevel
// slots on the requested side. A target that carries the opposing effect is
// cured of it ("is Cured") and dropped; the rest get the spell via
// GenericSpellHandler::applyToTargets, followed by ES_COMBAT_RATE_MODIFIER
// (set 18) on each retained target.
SpellCastResult HasteSlowHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    if (!context.caster)
        return SpellCastResult(CAST_ERROR);

    // Haste: the caster's own side. Slow: CHARACTER_isInParty(caster) used as
    // the side id, i.e. the opposing side.
    const Goldbox::Data::CombatSide casterSide = context.caster->combatSide;
    const Goldbox::Data::CombatSide opposingSide =
        (casterSide == Goldbox::Data::CS_PARTY)
        ? Goldbox::Data::CS_ENEMY : Goldbox::Data::CS_PARTY;
    const Goldbox::Data::CombatSide wantedSide =
        _targetEnemies ? opposingSide : casterSide;

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    // SPELL_ApplyEffectToLimitedTargets: at most casterLevel targets of the
    // wanted side. A target that had the opposing effect is cured of it and
    // dropped; the rest receive the new effect.
    uint8 remaining = context.casterLevel;

    TargetSelection filtered;
    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target || target->combatSide != wantedSide || remaining == 0)
            continue;

        --remaining;

        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (fx && fx->hasEffect(_removeEffectId)) {
            if (bridge)
                bridge->postEffectMessage(target, "is Cured", true);
            context.effectSystem->removeEffectById(*target, *fx,
                _removeEffectId);
            continue;
        }
        filtered.targetCharacters.push_back(target);
    }

    if (context.combat)
        context.combat->multiTarget = true;

    if (filtered.targetCharacters.empty())
        return SpellCastResult(CAST_OK);

    SpellCastResult result =
        GenericSpellHandler::applyToTargets(context, definition, filtered, 0);

    // Post-application: effect set 18 (combat rate) on each retained target.
    Goldbox::Data::Effects::EffectRuntime *runtime =
        context.effectSystem->getRuntime();
    if (runtime) {
        for (uint i = 0; i < filtered.targetCharacters.size(); ++i) {
            Goldbox::Data::PlayerCharacter *target = filtered.targetCharacters[i];
            if (!target)
                continue;
            Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
            if (fx)
                runtime->checkEffectSet(
                    Goldbox::Data::Effects::ES_COMBAT_RATE_MODIFIER,
                    *fx, *target, context.combat);
        }
    }

    return result;
}

// --- Blink (ID45) ---
// Pure generic delegate.
SpellCastResult BlinkHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Fireball (ID47) ---
// Damage dice:
//   spell ID 0x40 (magic item): 1d3*2+1 d6
//   otherwise: casterLevel d6
// Outdoor combat: rebuild target list from all combatants within
// Chebyshev distance 2 of targets.tileX/Y (mirrors COMBAT_BuildTargetListCore
// with radius 2, side 0xFF = all sides).
// Stores damageDice in combat->damageDice (mirrors RollDamageDice).
// Applies damage with behaviorFlags = 9 (fire=0x01 | magic=0x08).
SpellCastResult FireballHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (!context.damageSystem)
        return SpellCastResult(CAST_ERROR);

    // Determine damage dice count.
    uint8 damageDice;
    if (context.combat &&
            context.combat->activeSpellId == 0x40) {
        const uint8 roll = Goldbox::g_engine ?
            static_cast<uint8>(Goldbox::g_engine->rollDice(1, 3)) : 2;
        damageDice = static_cast<uint8>(roll * 2 + 1);
    } else {
        damageDice = context.casterLevel > 0 ? context.casterLevel : 1;
    }

    // Roll damage dice.
    const uint8 baseDamage = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(damageDice, 6)) : damageDice;
    if (context.combat)
        context.combat->damageDice = damageDice;

    // Query indoor/outdoor mode from the ECL runtime state.
    bool isIndoor = true;
    if (Goldbox::g_engine) {
        RuntimeExchange *rx = Goldbox::g_engine->getRuntimeExchange();
        if (rx) {
            RuntimeMapSnapshot snap;
            if (rx->captureMapSnapshot(snap))
                isIndoor = snap.indoorMode;
        }
    }

    // Build AoE target list for outdoor combat.
    TargetSelection aoeTargets;
    if (!isIndoor && context.combat) {
        Goldbox::Combat::CombatContext *ctx =
            Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr;
        if (ctx) {
            ctx->buildTargetListCore(TilePos((uint8)targets.tileX,
                                             (uint8)targets.tileY),
                                     1, DIR_ANY, 2);
            for (uint i = 0; i < ctx->targetList.entries.size(); ++i) {
                Goldbox::Data::PlayerCharacter *ch =
                    ctx->table.getCharacter(ctx->targetList.entries[i].idx);
                if (ch)
                    aoeTargets.targetCharacters.push_back(ch);
            }
        }
    }

    const TargetSelection &effectiveTargets =
        (!isIndoor && !aoeTargets.targetCharacters.empty())
        ? aoeTargets : targets;

    if (effectiveTargets.targetCharacters.empty())
        return SpellCastResult(CAST_OK);

    // SPELL_MULTI_TARGET is set for the whole spell.
    if (context.combat)
        context.combat->multiTarget = true;

    SpellApplyParams params;
    params.baseDamage = baseDamage;
    params.behaviorFlags = 9;
    return applyOnTargets(context, definition, effectiveTargets, params);
}

// --- Dispel Magic (ID41 / ID46) ---
// Multi-target. For each effect on the target (except power==0xFF):
//   chance = 50 + (castingLevel - effectPowerNibble) * 5  if caster level > effect level
//   chance = 50 - (effectPowerNibble - castingLevel) * 2  if effect level > caster level
//   chance = 50                                           if equal
// Rolls 1d100; removes effect on success. Posts "is affected" if any effect was removed.
SpellCastResult DispelMagicHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (!fx)
            continue;

        // Collect effect IDs first to avoid iterator invalidation during removal.
        Common::Array<uint8> ids;
        Common::Array<uint8> powers;
        for (const Goldbox::Data::Effects::Effect &e : fx->effects()) {
            ids.push_back(e.id);
            powers.push_back(e.power);
        }

        bool effectRemoved = false;
        for (uint j = 0; j < ids.size(); ++j) {
            if (powers[j] == 0xFF)
                continue;

            const uint8 effectPowerNibble = powers[j] & 0x0F;
            int chance;
            if (context.casterLevel > effectPowerNibble)
                chance = 50 + (context.casterLevel - effectPowerNibble) * 5;
            else if (effectPowerNibble > context.casterLevel)
                chance = 50 - (effectPowerNibble - context.casterLevel) * 2;
            else
                chance = 50;

            const uint8 roll = Goldbox::g_engine ?
                static_cast<uint8>(Goldbox::g_engine->rollDice(1, 100)) : 50;

            if (roll <= static_cast<uint8>(chance)) {
                context.effectSystem->removeEffectById(*target, *fx, ids[j]);
                effectRemoved = true;
            }
        }

        if (effectRemoved && bridge)
            bridge->postEffectMessage(target, "is affected", true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Cause Disease (ID40) ---
// Delegates entirely to GenericSpellHandler (mirrors SPELL_ApplyOnTargets).
SpellCastResult CauseDiseaseHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0, true);
}

// --- Spell ID58 (SP_MI2) ---
// 1. If effect 0x37 is present: remove it, silently erase 0x16, return.
// 2. Else if afflictions present: remove them, return.
// 3. Else: heal 1d4+8 HP and post "is Healed".
SpellCastResult SpellID58Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (!fx)
            continue;

        // Step 1: cure primary condition (raw 0x37) and its associated 0x16.
        if (context.effectSystem->removeEffectById(*target, *fx, 0x37)) {
            fx->eraseEffectById(
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SLOW_POISON_END));
            continue;
        }

        // Step 2: cure disease/afflictions.
        if (removeAfflictions(context.effectSystem, target))
            continue;

        // Step 3: heal 1d4+8 HP.
        const uint8 amount = static_cast<uint8>(8 +
            (Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 4) : 2));

        if (target->healHp(amount, false) && bridge)
            bridge->postEffectMessage(target, "is Healed", true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Lightning Bolt (ID51) ---
// Mirrors Spell_ID51_LightningBolt:
//   damage = casterLevel d6
//   resolveAoEHitAtTile at target (save category SPELL)
//   traceSpellPath(save category SPELL, pathLength=8)
SpellCastResult LightningBoltHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (!context.combat)
        return SpellCastResult(CAST_NOT_ALLOWED);

    Goldbox::Combat::CombatContext *ctx =
        Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr;
    if (!ctx)
        return SpellCastResult(CAST_ERROR);

    const uint8 damageDice = context.casterLevel > 0 ? context.casterLevel : 1;
    const uint8 baseDamage = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(damageDice, 6)) : damageDice;
    context.combat->damageDice = damageDice;

    const TilePos attackerPos(
        ctx->table.getCharacterCol(context.caster),
        ctx->table.getCharacterRow(context.caster));
    const TilePos targetPos(
        static_cast<uint8>(targets.tileX),
        static_cast<uint8>(targets.tileY));

    (void)Spells::resolveAoEHitAtTile(*ctx, targetPos, baseDamage,
                                      Goldbox::Data::Spells::SVS_SPELL, 19,
                                      nullptr);

    ctx->globals.targetPos = targetPos;
    // TODO(spell_aoe): pass context.presenter once SpellContext carries it.
    Spells::traceSpellPath(*ctx, attackerPos,
                           8,           // pathLength (constant, not caster level)
                           baseDamage,
                           Goldbox::Data::Spells::SVS_SPELL, // save category
                           true,        // animatePath
                           19,          // effectTileId (lightning)
                           nullptr);

    return SpellCastResult(CAST_OK);
}

// --- Spell ID60 (SP_MI4) ---
// Mirrors Spell_ID60:
//   damage = 20 + 1d6
//   resolveAoEHitAtTile at target (save category SPELL)
//   traceSpellPath(save category SPELL, baseDamage=20, pathLength=3)
SpellCastResult SpellID60Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (!context.combat)
        return SpellCastResult(CAST_NOT_ALLOWED);

    Goldbox::Combat::CombatContext *ctx =
        Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr;
    if (!ctx)
        return SpellCastResult(CAST_ERROR);

    const uint8 roll = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(1, 6)) : 3;
    const uint8 baseDamage = static_cast<uint8>(20 + roll);

    const TilePos attackerPos(
        ctx->table.getCharacterCol(context.caster),
        ctx->table.getCharacterRow(context.caster));
    const TilePos targetPos(
        static_cast<uint8>(targets.tileX),
        static_cast<uint8>(targets.tileY));

    (void)Spells::resolveAoEHitAtTile(*ctx, targetPos, baseDamage,
                                      Goldbox::Data::Spells::SVS_SPELL, 19,
                                      nullptr);

    ctx->globals.targetPos = targetPos;
    // TODO(spell_aoe): pass context.presenter once SpellContext carries it.
    Spells::traceSpellPath(*ctx, attackerPos,
                           3,    // pathLength
                           20,   // baseDamage (fixed component, matches original)
                           Goldbox::Data::Spells::SVS_SPELL, // save category
                           true, // animatePath
                           19,   // effectTileId
                           nullptr);

    return SpellCastResult(CAST_OK);
}

// --- Spell ID57 (SP_MI1) ---
// Removes E_POOLRAD_SLOWED (0x2A) from the target; only when there was none
// to remove is the spell applied via the generic path ("is Speedy").
SpellCastResult SpellID57Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[0];
    if (!target)
        return SpellCastResult(CAST_INVALID_TARGET);

    Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
    if (fx && context.effectSystem->removeEffectById(
            *target, *fx,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SLOWED)))
        return SpellCastResult(CAST_OK);

    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Spell ID59 (SP_MI3) ---
// Applies applyStrengthChange(21, 0); posts "is stronger"; adds
// E_POOLRAD_STRENGTH with duration from computeSpellDuration.
SpellCastResult SpellID59Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        uint8 strengthChange = 0;
        if (target->applyStrengthChange(21, 0, strengthChange) && bridge)
            bridge->postEffectMessage(target, "is stronger", true);

        const uint16 duration = computeSpellDuration(
            static_cast<uint8>(Goldbox::Data::Spells::SP_MI3),
            context.casterLevel, context.inCombat);

        target->addEffect(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_STRENGTH),
            duration, strengthChange, true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Spell ID61 (SP_MI5) ---
// Pure generic delegate — "is paralyzed".
SpellCastResult SpellID61Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Spell ID62 (SP_MI6) ---
// Heals 2d4+2 HP; posts "is Healed".
SpellCastResult SpellID62Handler::execute(const SpellContext &context,
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

        const uint8 amount = static_cast<uint8>(2 +
            (Goldbox::g_engine ? Goldbox::g_engine->rollDice(2, 4) : 5));

        if (target->healHp(amount, false) && bridge)
            bridge->postEffectMessage(target, "is Healed", true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Spell ID63 (SP_MI7) ---
// Pure generic delegate — "is invisible".
SpellCastResult SpellID63Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Spell ID65 (SP_MI9) ---
// Deals rollDiceAttack(2,4)+2 damage with behavior 8.
SpellCastResult SpellID65Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    const uint8 damage = static_cast<uint8>(2 +
        (Goldbox::g_engine ? Goldbox::g_engine->rollDice(2, 4) : 5));
    SpellApplyParams params;
    params.baseDamage = damage;
    params.behaviorFlags = 8;
    params.damageDice = 2;
    return applyOnTargets(context, definition, targets, params);
}

// --- Spell ID66 (SP_MI10) ---
// Magic.UnusedSpellHandler: does nothing.
SpellCastResult UnusedSpellHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)context;
    (void)definition;
    (void)targets;
    return SpellCastResult(CAST_OK);
}

// --- Spell ID67 (SP_MI11) ---
// Manual of Bodily Health: the effect at power 255, fired when added.
SpellCastResult SpellID67Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    return GenericSpellHandler::applyToTargets(context, definition, targets,
        0xFF, true);
}

// --- Restore (ID56) ---
// Restores one drained level: recovers HP proportional to drained_hp/drained_level,
// then finds the best class slot to re-grant a level (lowest XP threshold that
// hasn't yet hit its racial/strength cap), increments that slot's level,
// floors experiencePoints to the XP needed for that level, and recalcs stats.
static bool restoreCheckLevelRequired(
        const Goldbox::Poolrad::Data::PoolradCharacter *ch, uint8 classSlot) {
    const uint8 lvl = ch->levels.levels[classSlot];
    if (lvl == 0)
        return false;

    const uint8 str = ch->abilities.strength.current;

    switch (ch->race) {
    case Goldbox::Data::R_DWARF:
        if (classSlot == Goldbox::Data::C_FIGHTER &&
                ((lvl == 8 && str == 17) || (lvl == 7 && str < 17)))
            return true;
        break;
    case Goldbox::Data::R_ELF:
        if (classSlot == Goldbox::Data::C_FIGHTER &&
                (lvl == 7 ||
                 (lvl == 6 && str == 17) ||
                 (lvl == 5 && str < 17)))
            return true;
        break;
    case Goldbox::Data::R_GNOME:
        if (classSlot == Goldbox::Data::C_FIGHTER &&
                (lvl == 6 || (lvl == 5 && str < 18)))
            return true;
        break;
    case Goldbox::Data::R_HALF_ELF:
        if (classSlot == Goldbox::Data::C_CLERIC && lvl == 5)
            return true;
        if (classSlot == Goldbox::Data::C_FIGHTER &&
                (lvl == 8 ||
                 (lvl == 7 && str == 17) ||
                 (lvl == 6 && str < 17)))
            return true;
        break;
    case Goldbox::Data::R_HALFLING:
        if (classSlot == Goldbox::Data::C_FIGHTER &&
                (lvl == 6 ||
                 (lvl == 5 && str == 17) ||
                 (lvl == 4 && str < 17)))
            return true;
        break;
    default:
        break;
    }
    return false;
}

SpellCastResult RestoreHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (targets.targetCharacters.empty())
        return SpellCastResult(CAST_INVALID_TARGET);

    Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[0];
    if (!target)
        return SpellCastResult(CAST_INVALID_TARGET);

    Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(target);
    if (!poolrad || poolrad->drainedLevels == 0)
        return SpellCastResult(CAST_NOT_ALLOWED);

    // Restore HP proportional to one drained level.
    const uint8 restoredHp = poolrad->drainedHPs / poolrad->drainedLevels;
    poolrad->hitPoints.max     = static_cast<uint8>(MIN<uint16>(poolrad->hitPoints.max     + restoredHp, 255));
    poolrad->hitPoints.current = static_cast<uint8>(MIN<uint16>(poolrad->hitPoints.current + restoredHp, 255));
    poolrad->hitPointsRolled   = static_cast<uint8>(MIN<uint16>(poolrad->hitPointsRolled   + restoredHp, 255));
    poolrad->drainedHPs    -= restoredHp;
    poolrad->drainedLevels -= 1;

    // Magic.RestorationHandler: scanning in class order, a class becomes the
    // pick when its level is not above the current pick's, its next-level XP
    // is nonzero and smaller, and it is not at its level cap.
    int bestSlot   = -1;
    int32 bestXp   = 10000000;
    uint8 bestLvl  = 13;

    for (uint8 slot = 0; slot < BASE_CLASS_NUM; ++slot) {
        const uint8 lvl = poolrad->levels.levels[slot];
        if (lvl == 0 || lvl > bestLvl)
            continue;
        if (restoreCheckLevelRequired(poolrad, slot))
            continue;

        const int32 xpNeeded =
            Goldbox::Data::Rules::xpForClassAtLevel(slot, lvl);
        if (xpNeeded <= 0 || xpNeeded >= bestXp)
            continue;

        bestSlot = slot;
        bestLvl  = lvl;
        bestXp   = xpNeeded;
    }

    // The original raises an uninitialized class when none qualifies.
    if (bestSlot >= 0) {
        poolrad->levels.levels[bestSlot] += 1;
        if (poolrad->experiencePoints < static_cast<uint32>(bestXp))
            poolrad->experiencePoints = static_cast<uint32>(bestXp);
    }

    poolrad->recalcCombatStats();

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem ? context.effectSystem->getHostBridge() : nullptr;
    if (bridge)
        bridge->postEffectMessage(target, "is restored", true);

    return SpellCastResult(CAST_OK);
}

} // namespace Spells
} // namespace Goldbox
