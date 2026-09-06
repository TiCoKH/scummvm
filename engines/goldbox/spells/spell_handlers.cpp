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
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/core/direction.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_inventory.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect.h"
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
// Mutates strength first, then delegates effect application to applyToTargets
// with the encoded previous-strength as the power override.
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

        // Delegate effect record creation to applyToTargets with the encoded
        // previous-strength as the power override, mirroring Friends pattern.
        TargetSelection single;
        single.targetCharacters.push_back(target);
        GenericSpellHandler::applyToTargets(context, definition, single, effectValue);
    }

    return SpellCastResult(CAST_OK);
}

// --- Reduce (ID13) ---
// Saving throw vs spell negates. If save fails, removes E_POOLRAD_ENLARGE_STRENGTHEN
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
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ENLARGE_STRENGTHEN))) {
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

    return GenericSpellHandler::applyToTargets(context, definition, targets, oldCharisma);
}

// --- Magic Missile (ID15) ---
// damageLevel = (casterLevel + 1) >> 1
// damage = damageLevel + rollDice(damageLevel, 4)
// Applies damage with behavior flag 8 via DamageSystem::applyLegacy.
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

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;
        if (context.damageSystem)
            context.damageSystem->applyLegacy(*target, baseDamage,
                Goldbox::Data::DAMAGE_NORMAL, false, 8);
    }

    return SpellCastResult(CAST_OK);
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

    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;
        if (context.damageSystem)
            context.damageSystem->applyLegacy(*target, baseDamage,
                Goldbox::Data::DAMAGE_NORMAL, false, 12);
    }

    return SpellCastResult(CAST_OK);
}

// --- Sleep (ID21) ---
// Rolls 4d4 as a shared budget. Each target consumes a level-based cost;
// targets already carrying E_POOLRAD_HELPLESS_35 or whose cost exceeds the
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

        // Already sleeping — skip.
        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (fx && fx->hasEffect(
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_HELPLESS_35)))
            continue;

        // Derive level cost from highestLevel (PoolradCharacter field).
        const Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
            dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(target);
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

    const uint16 duration = static_cast<uint16>(
        definition.entry->fixedDuration +
        definition.entry->perLvlDuration * context.casterLevel);

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();
    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();

    // Iterate in reverse to match original target-list traversal order.
    for (uint i = count; i-- > 0;) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target)
            continue;

        // Non-standard size/type targets are immune.
        const Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
            dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(target);
        const uint8 monsterType = poolrad ? poolrad->monsterType : 0;
        if (monsterType > 1 || target->iconDimension > 1) {
            if (bridge)
                bridge->postEffectMessage(target, "is unaffected", true);
            continue;
        }

        // Saving throw.
        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(target);
        const bool saved = adnd && Goldbox::Data::Rules::checkSavingThrow(
            *adnd, context.combat, handler, bridge,
            definition.entry->saveType, saveAdj);

        if (saved)
            continue;

        context.effectSystem->addOrRefreshEffect(
            *target->getEffects(), *target,
            definition.entry->effectId, duration,
            context.casterLevel, true);

        if (bridge)
            bridge->postEffectMessage(target, "is held", true);
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
// After the generic effect path (effectPowerOverride=0xFF), fires EFF_REMOVE
// on raw effect 0x4E, then adds E_POOLRAD_POISON_DAMAGE (0x0F) with
// duration=10 and power=0xFF.
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
    GenericSpellHandler::applyToTargets(context, definition, single, 0xFF);

    // Fire EFF_REMOVE on raw effect 0x4E to perform the poison-state transition.
    if (handler) {
        Goldbox::Data::Effects::Effect *rawEffect = fx->findEffectById(0x4E);
        if (rawEffect)
            handler->apply(Goldbox::Data::Effects::EFF_REMOVE, *rawEffect,
                *target, context.combat, bridge);
    }

    // Add the resulting slow-poison marker: effect 0x0F, duration 10, power 0xFF.
    context.effectSystem->addOrRefreshEffect(
        *fx, *target,
        static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_POISON_DAMAGE),
        10, 0xFF, true);

    return SpellCastResult(CAST_OK);
}

// --- Snake Charm (ID27) ---
// Ignores the pre-selected target list. Builds its own from context.enemies:
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

    TargetSelection filtered;
    for (uint i = 0; i < context.enemies.size(); ++i) {
        Goldbox::Data::PlayerCharacter *candidate = context.enemies[i];
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
// E_POOLRAD_SPIRITUAL_HAMMER (0x17) to activate the weapon-creation side effect.
SpellCastResult SpiritualHammerHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    SpellCastResult result =
        GenericSpellHandler::applyToTargets(context, definition, targets, 0);
    if (result.status != CAST_OK)
        return result;

    // Fire EFF_ADD on the spiritual hammer effect to trigger weapon creation.
    Goldbox::Data::Effects::EffectHandlerBase *handler =
        context.effectSystem->getHandler();
    if (handler && context.caster) {
        Goldbox::Data::Effects::CharacterEffects *fx = context.caster->getEffects();
        Goldbox::Data::Effects::Effect *hammerEffect = fx ?
            fx->findEffectById(
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SPIRITUAL_HAMMER))
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
// TODO: presentation layer not yet implemented.
//   Original sequence after cloud creation:
//     TEXT_drawIntoMsgBox(caster, "Creates a noxious cloud", 10, false)
//     COMBAT_RedrawViewport(centerX, centerY, 0xFF, 8)
//     GAME_waitDelayTime()
//     SCREEN_clearMsgArea()
//   Wire this up once the viewport/message subsystems are available.
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
        context.effectSystem->addOrRefreshEffect(
            *casterFx, *context.caster,
            static_cast<uint8>(Goldbox::Data::Effects::E_STINKING_CLOUD_EXPAIR),
            context.casterLevel, power, true);
    }

    // Apply initial nausea to all combatants currently in the four cloud cells.
    // CloudEffectManager::kCloudDirections = {8, 2, 3, 4}.
    static const uint8 kCloudDirs[4] = { 8, 2, 3, 4 };

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem->getHostBridge();

    for (int dir = 0; dir < 4; ++dir) {
        const uint8 wireDir = kCloudDirs[dir];
        const uint8 cx = static_cast<uint8>(centerX + kDirDeltaX[wireDir]);
        const uint8 cy = static_cast<uint8>(centerY + kDirDeltaY[wireDir]);

        // Look up the combatant at this cell (1-based index, 0 = empty).
        const uint8 occupantIdx = ctx->table.getOccupant(cx, cy);
        if (occupantIdx == 0)
            continue;

        Goldbox::Data::PlayerCharacter *occupant =
            ctx->table.getCharacter(occupantIdx - 1);
        if (!occupant)
            continue;

        Goldbox::Data::Effects::CharacterEffects *fx = occupant->getEffects();
        if (!fx)
            continue;

        // Apply the in-cloud nausea marker (E_STINKING_CLOUD_EXPAIR = 40).
        // The effect handler decides whether the character is nauseated.
        context.effectSystem->addOrRefreshEffect(
            *fx, *occupant,
            static_cast<uint8>(Goldbox::Data::Effects::E_STINKING_CLOUD_EXPAIR),
            1, power, true);

        if (bridge)
            bridge->postEffectMessage(occupant, "is nauseated", false);
    }

    return SpellCastResult(CAST_OK);
}

// --- Strength (ID35) ---
// Bonus dice by class (highest priority wins): fighter=1d8, cleric/thief=1d6, mage=1d4.
// Excess over 18 converts to exceptional strength for fighters (capped at 100).
// Adds E_POOLRAD_ENLARGE_STRENGTHEN (0x0C) with duration from computeSpellDuration.
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

        const uint8 duration = computeSpellDuration(
            static_cast<uint8>(Goldbox::Data::Spells::SP_MUL2_STRENGTH),
            context.casterLevel, context.inCombat);

        context.effectSystem->addOrRefreshEffect(
            *target->getEffects(), *target,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ENLARGE_STRENGTHEN),
            duration, effectPower, true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Animate Dead (ID36) ---
// Iterates context.allies filtered by S_DEAD + classType==0 (normal party member).
// Re-places each on the combat map at their stored position; converts to undead
// (combatSide from caster, ai_control=true, levelUndead=2, npc=0xB2/0xB3, classType=4).
// Clears memorized spells and combat target. Revives at max HP.
// Adds E_POOLRAD_ANIMATING_DEAD (0x20) with power = originalSide*16 + casterLevel.
// Budget = casterLevel animations.
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

    uint8 remaining = context.casterLevel;

    for (uint i = 0; i < context.allies.size() && remaining > 0; ++i) {
        Goldbox::Data::PlayerCharacter *ch = context.allies[i];
        if (!ch)
            continue;

        // Only dead normal party members (classType==0 mirrors original type!=0 skip).
        const Goldbox::Poolrad::Data::PoolradCharacter *poolrad =
            dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(ch);
        if (!poolrad ||
                poolrad->healthStatus != static_cast<uint8>(Goldbox::Data::S_DEAD) ||
                ch->classType != 0)
            continue;

        // Re-place on combat map at stored position.
        const uint8 col = ctx->table.getCharacterCol(ch);
        const uint8 row = ctx->table.getCharacterRow(ch);
        const int slot = ctx->table.addCombatant(ch, ch->iconDimension);
        if (slot < 0)
            continue;
        ctx->table.setPosition(slot, TilePos(col, row));

        // Capture original side before mutation.
        const uint8 originalSide = static_cast<uint8>(ch->combatSide);
        const uint8 effectPower = static_cast<uint8>(originalSide * 16 + context.casterLevel);

        // Convert to undead combatant.
        ch->combatSide = context.caster->combatSide;
        ch->ai_control = true;

        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(ch);
        if (adnd) {
            adnd->levelUndead = 2;
            adnd->attackLevel = 0;
            for (int j = 0; j <= 20; ++j)
                adnd->spells.memorizedSpells[j] = 0;
            adnd->npc = (adnd->npc < static_cast<int8>(0x80)) ?
                static_cast<int8>(0xB3) : static_cast<int8>(0xB2);
        }

        ch->movement.current = 6;
        ch->classType = 4;

        // Clear combat target.
        if (ch->combatState)
            ch->combatState->target = nullptr;

        --remaining;

        // Revive at max HP.
        ch->hitPoints.current = ch->hitPoints.max;

        // Apply animated effect and set status.
        Goldbox::Data::Effects::CharacterEffects *fx = ch->getEffects();
        if (fx) {
            context.effectSystem->addOrRefreshEffect(
                *fx, *ch,
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ANIMATING_DEAD),
                0, effectPower, true);
        }

        Goldbox::Poolrad::Data::PoolradCharacter *poolradMut =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(ch);
        if (poolradMut)
            poolradMut->healthStatus = static_cast<uint8>(Goldbox::Data::S_ANIMATED);

        if (bridge)
            bridge->postEffectMessage(ch, "is animated", true);
    }

    return SpellCastResult(CAST_OK);
}

// --- Cure Blindness (ID37) ---
// Removes E_POOLRAD_BLINDED (0x21) from the target via removeEffectById,
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
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_BLINDED)) && bridge)
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

    if (effectSystem->removeEffectById(*target, *fx, 0x22))
        removed = true;

    if (effectSystem->removeEffectById(*target, *fx, 0x2B)) {
        removed = true;
        fx->eraseEffectById(0x2C);
        fx->eraseEffectById(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_HELPLESS));
    }

    if (effectSystem->removeEffectById(*target, *fx,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ENDLESS_REGEN))) {
        removed = true;
        fx->eraseEffectById(
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ROT));
    }

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
// 1. Try to remove E_POOLRAD_ACCURSED (0x24) from the target; post "is un-cursed" if removed.
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
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ACCURSED))) {
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
// Filters targets to casterLevel slots on the requested side.
// A target is retained only when removeEffectById succeeds (prerequisite
// removal). The spell is then applied to retained targets via
// GenericSpellHandler::applyToTargets, followed by ES_SAVING_THROW_MODS
// on each retained target.
SpellCastResult HasteSlowHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    if (!context.effectSystem)
        return SpellCastResult(CAST_ERROR);

    const Goldbox::Data::CombatSide wantedSide =
        _targetEnemies ? Goldbox::Data::CS_ENEMY : Goldbox::Data::CS_PARTY;

    uint8 remaining = context.casterLevel > 0 ? context.casterLevel : 1;

    TargetSelection filtered;
    for (uint i = 0; i < targets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = targets.targetCharacters[i];
        if (!target || target->combatSide != wantedSide || remaining == 0)
            continue;

        --remaining;

        Goldbox::Data::Effects::CharacterEffects *fx = target->getEffects();
        if (fx && context.effectSystem->removeEffectById(
                *target, *fx, _removeEffectId))
            filtered.targetCharacters.push_back(target);
    }

    if (filtered.targetCharacters.empty())
        return SpellCastResult(CAST_OK);

    SpellCastResult result =
        GenericSpellHandler::applyToTargets(context, definition, filtered, 0);

    // Post-application: fire ES_SAVING_THROW_MODS on each retained target.
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
                    Goldbox::Data::Effects::ES_SAVING_THROW_MODS,
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
// Stores damageDice in combat->attackCount (mirrors UTIL_RollDiceAttack).
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
            const int cx = targets.tileX;
            const int cy = targets.tileY;
            for (int i = 0; i < Goldbox::Combat::CombatantTable::MAX_COMBATANTS; ++i) {
                Goldbox::Data::PlayerCharacter *ch = ctx->table.getCharacter(i);
                if (!ch || !ch->isAlive())
                    continue;
                const int dx = ctx->table.getTileCol(i) - cx;
                const int dy = ctx->table.getTileRow(i) - cy;
                // Chebyshev distance <= 2 (radius 2 matches original).
                if (ABS(dx) <= 2 && ABS(dy) <= 2)
                    aoeTargets.targetCharacters.push_back(ch);
            }
        }
    }

    const TargetSelection &effectiveTargets =
        (!isIndoor && !aoeTargets.targetCharacters.empty())
        ? aoeTargets : targets;

    if (effectiveTargets.targetCharacters.empty())
        return SpellCastResult(CAST_OK);

    for (uint i = 0; i < effectiveTargets.targetCharacters.size(); ++i) {
        Goldbox::Data::PlayerCharacter *target = effectiveTargets.targetCharacters[i];
        if (!target)
            continue;

        // Saving throw vs spell: success halves damage.
        bool saved = false;
        Goldbox::Data::ADnDCharacter *adnd =
            dynamic_cast<Goldbox::Data::ADnDCharacter *>(target);
        if (adnd && context.effectSystem)
            saved = Goldbox::Data::Rules::checkSavingThrow(
                *adnd, context.combat,
                context.effectSystem->getHandler(),
                context.effectSystem->getHostBridge(),
                Goldbox::Data::Spells::SVS_SPELL, 0);

        const Goldbox::Data::DamageModifier mod =
            saved ? Goldbox::Data::DAMAGE_HALF : Goldbox::Data::DAMAGE_NORMAL;
        context.damageSystem->applyLegacy(*target, baseDamage, mod, saved, 9);
    }

    return SpellCastResult(CAST_OK);
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
    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
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
                static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SLOW_POISON));
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
//   resolveAoEHitAtTile at target (savingThrowMod=4)
//   traceSpellPath(initialAnimFrame=8, savingThrowMod=4, pathLength=damageDice)
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

    const TilePos attackerPos(
        ctx->table.getCharacterCol(context.caster),
        ctx->table.getCharacterRow(context.caster));
    const TilePos targetPos(
        static_cast<uint8>(targets.tileX),
        static_cast<uint8>(targets.tileY));

    bool hitObstacle = false;
    Spells::resolveAoEHitAtTile(*ctx, targetPos, baseDamage, 4, 19,
                                nullptr, hitObstacle);

    // TODO(spell_aoe): pass context.presenter once SpellContext carries it.
    Spells::traceSpellPath(*ctx, attackerPos, targetPos,
                           8,           // initialAnimFrame
                           4,           // savingThrowMod
                           baseDamage,
                           damageDice,  // pathLength
                           19,          // effectTileId (lightning)
                           nullptr);

    return SpellCastResult(CAST_OK);
}

// --- Spell ID60 (SP_MI4) ---
// Mirrors Spell_ID60:
//   damage = 20 + 1d6
//   resolveAoEHitAtTile at target (savingThrowMod=4)
//   traceSpellPath(initialAnimFrame=3, baseDamage=20, savingThrowMod=4, pathLength=3)
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

    bool hitObstacle = false;
    Spells::resolveAoEHitAtTile(*ctx, targetPos, baseDamage, 4, 19,
                                nullptr, hitObstacle);

    // TODO(spell_aoe): pass context.presenter once SpellContext carries it.
    Spells::traceSpellPath(*ctx, attackerPos, targetPos,
                           3,    // initialAnimFrame
                           4,    // savingThrowMod
                           20,   // baseDamage (fixed component, matches original)
                           3,    // pathLength
                           19,   // effectTileId
                           nullptr);

    return SpellCastResult(CAST_OK);
}

// --- Breath Weapon (lingering) ---
// Driven by effect 88 (handleLingeringBreath in effect_handler.cpp).
// This handler is the spell-cast entry point for a one-shot breath attack
// using the same path-trace mechanic; parameters are supplied at construction.
//
// TODO(spell_aoe): pass context.presenter once SpellContext carries it.
SpellCastResult BreathWeaponHandler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    (void)definition;
    if (!context.combat)
        return SpellCastResult(CAST_NOT_ALLOWED);

    Goldbox::Combat::CombatContext *ctx =
        Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr;
    if (!ctx)
        return SpellCastResult(CAST_ERROR);

    const TilePos attackerPos(
        ctx->table.getCharacterCol(context.caster),
        ctx->table.getCharacterRow(context.caster));
    const TilePos targetPos(
        static_cast<uint8>(targets.tileX),
        static_cast<uint8>(targets.tileY));

    bool hitObstacle = false;
    Spells::resolveAoEHitAtTile(*ctx, targetPos, _baseDamage, _savingThrowMod,
                                _effectTileId, nullptr, hitObstacle);

    Spells::traceSpellPath(*ctx, attackerPos, targetPos,
                           1,               // initialAnimFrame (breath uses frame offset)
                           _savingThrowMod,
                           _baseDamage,
                           _pathLength,
                           _effectTileId,
                           nullptr);

    return SpellCastResult(CAST_OK);
}

// --- Spell ID57 (SP_MI1) ---
// Removes E_POOLRAD_SLOWED (0x2A) from the target. If removed, applies
// the spell via the generic path ("is Speedy").
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
    if (!fx || !context.effectSystem->removeEffectById(
            *target, *fx,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_SLOWED)))
        return SpellCastResult(CAST_OK);

    return GenericSpellHandler::applyToTargets(context, definition, targets, 0);
}

// --- Spell ID59 (SP_MI3) ---
// Applies applyStrengthChange(21, 0); posts "is stronger"; adds
// E_POOLRAD_ENLARGE_STRENGTHEN with duration from computeSpellDuration.
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

        const uint8 duration = computeSpellDuration(
            static_cast<uint8>(Goldbox::Data::Spells::SP_MI3),
            context.casterLevel, context.inCombat);

        context.effectSystem->addOrRefreshEffect(
            *target->getEffects(), *target,
            static_cast<uint8>(Goldbox::Data::Effects::E_POOLRAD_ENLARGE_STRENGTHEN),
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
// effectPower = rollDiceAttack(2,4)+2; applies via generic path with behavior 8.
SpellCastResult SpellID65Handler::execute(const SpellContext &context,
        const SpellDefinition &definition,
        const TargetSelection &targets) const {
    const uint8 roll = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(2, 4)) : 5;
    const uint8 effectPower = static_cast<uint8>(roll + 2);
    return GenericSpellHandler::applyToTargets(context, definition, targets, effectPower);
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

    // Find the best class slot to restore: lowest XP threshold among slots
    // that have a level and haven't hit their racial/strength cap.
    uint8 bestSlot = 0;
    int32 bestXp   = 0x7FFFFFFF;
    uint8 bestLvl  = 13;

    for (uint8 slot = 0; slot < BASE_CLASS_NUM; ++slot) {
        const uint8 lvl = poolrad->levels.levels[slot];
        if (lvl == 0 || lvl > 13)
            continue;
        if (restoreCheckLevelRequired(poolrad, slot))
            continue;

        const int32 xpNeeded =
            Goldbox::Data::Rules::xpForClassAtLevel(slot, lvl);
        if (xpNeeded < 0)
            continue;

        if (xpNeeded < bestXp || (xpNeeded == bestXp && lvl < bestLvl)) {
            bestSlot = slot;
            bestLvl  = lvl;
            bestXp   = xpNeeded;
        }
    }

    poolrad->levels.levels[bestSlot] += 1;

    if (poolrad->experiencePoints < static_cast<uint32>(bestXp))
        poolrad->experiencePoints = static_cast<uint32>(bestXp);

    poolrad->recalcCombatStats();

    Goldbox::Data::Effects::EffectHostBridge *bridge =
        context.effectSystem ? context.effectSystem->getHostBridge() : nullptr;
    if (bridge)
        bridge->postEffectMessage(target, "is restored", true);

    return SpellCastResult(CAST_OK);
}

} // namespace Spells
} // namespace Goldbox
