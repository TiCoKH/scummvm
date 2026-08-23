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

#include "goldbox/poolrad/effect_handler.h"
#include "goldbox/combat/cloud_effect_manager.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_common_handler.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/engine.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {

namespace {

// --- Poolrad-specific handler helpers ---

using namespace Goldbox::Data::Effects;

static Data::PoolradCharacter &asPoolrad(Goldbox::Data::PlayerCharacter &ch) {
    return static_cast<Data::PoolradCharacter &>(ch);
}

static void applyFlag(EffectOp op, Data::PoolradCharacter &ch, uint32 flag) {
    if (op == EFF_ADD)
        ch.effectState.flags |= flag;
    else if (op == EFF_REMOVE)
        ch.effectState.flags &= ~flag;
}

// --- Individual poolrad effect handlers ---

static void handleNotImplemented(const EffectCall &) {
    // The original Poolrad handler exists but intentionally does nothing.
}

static void handleSilence(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_SILENCED);

    if (c.op != EFF_ADD || !c.character.combatState)
        return;

    // canUse covers magic items and scrolls; canCast covers memorized spells.
    // The legacy routine only displayed the message when item/spell use was
    // still available, then disabled both action categories.
    if (c.character.combatState->canUse && c.bridge)
        c.bridge->postEffectMessage(&c.character, "is silenced", true);

    c.character.combatState->canUse = false;
    c.character.combatState->canCast = false;
}

static void handleInvisibility(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_INVISIBLE);
}

static void handleItemInvisibility(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_ITEM_INVISIBLE);
}

static void handleCamouflage(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_CAMOUFLAGE);
}

static void handleImmuneElec(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_IMMUNE_ELEC);
}

static void handleResistFire(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_RESIST_FIRE);
}

static void handleResistFireAndCold(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_RESIST_FIRE_COLD);
}

static void handleFireResist(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_FIRE_RESIST);

    // EFFECT_20 operates on the current hit, not by applying damage itself.
    // DamageSystem consumes these resolved combat values afterward.
    if (c.op != EFF_ADD || !c.combat || !(c.combat->behaviorFlags &
            Goldbox::Combat::CombatGlobals::DMG_FIRE))
        return;

    c.combat->damage >>= 1;
    c.combat->savingThrow += 3;
}

static void handleProtNormalMissiles(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_PROT_NORMAL_MISSILES);
}

static void handleProtDragBreath(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_PROT_DRAG_BREATH);
}

static void handleMinorGlobe(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_MINOR_GLOBE);
}

static void handleRakshasaResist(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_RAKSHASA_RESIST);
}

static void handleDisplace(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_DISPLACE);
}

static void handleBlur(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.combat)
        return;

    const CharacterEffects *effects = c.character.getEffects();
    if (!effects || !effects->hasEffect(E_POOLRAD_TRUE_SEEING))
        c.combat->targetUnavailable = true;

    // Blur always imposes a -4 attack-roll penalty, including when True
    // Seeing suppresses its targeting penalty.
    c.combat->attackRoll -= 4;
}

static void handleDwarfTargetBonus(const EffectCall &c) {
    if (!c.combat || !c.character.combatState ||
            !c.character.combatState->target)
        return;

    const Data::PoolradCharacter *target =
        static_cast<const Data::PoolradCharacter *>(
            c.character.combatState->target);
    if (target->monsterType != 1 || target->iconDimension != 1)
        return;

    static const char *const kTargetNames[] = {
        "ORC",
        "ORC LEADER",
        "GOBLIN",
        "GOBLIN LEADER",
        "HOBGOBLIN",
        "HOBGOBLIN CHIEF",
        "GAGOOL",
        "MACE"
    };

    for (uint i = 0; i < ARRAYSIZE(kTargetNames); ++i) {
        if (target->name == kTargetNames[i]) {
            ++c.combat->attackRoll;
            return;
        }
    }
}

static void handleDuplicated(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.combat || c.effect.power == 0)
        return;

    // The legacy roll is 1..power+1; only rolls above 1 consume an image.
    const uint8 roll = static_cast<uint8>(Goldbox::g_engine ?
        Goldbox::g_engine->rollDice(1, c.effect.power + 1) : 1);
    const bool spellInProgress = c.combat->activeSpellId != 0;
    const bool spellMultiTarget = c.character.combatState &&
        c.character.combatState->maxTargets > 1;
    if (roll <= 1 || !spellInProgress || spellMultiTarget)
        return;

    // EFFECT_protectionIf(0): the consumed image protects the target from
    // the current spell effect.
    c.combat->damage = 0;

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "lost an image", true);

    --c.effect.power;
    if (c.effect.power == 0) {
        CharacterEffects *effects = c.character.getEffects();
        if (effects)
            effects->eraseEffectById(c.effect.id);
    }
}

static void handleEnfeebled(const EffectCall &c) {
    if (!c.combat)
        return;

    // BYTE_DAMAGE -= BYTE_DAMAGE / 4.
    c.combat->damage -= c.combat->damage / 4;
}

static void handleInStinkingCloudExpire(const EffectCall &c) {
    if (c.op != EFF_REMOVE)
        return;
    // TODO: investigate UTIL_addEffect / EffectSystem tick integration
    Combat::CombatContext *ctx = Goldbox::g_engine->getCombatContext();
    if (!ctx)
        return;
    const uint8 cloudIndex = c.effect.power >> 4;
    ctx->clouds.expire(&c.character, cloudIndex, c.bridge);
}

static void handleNauseated(const EffectCall &c) {
    if (c.op == EFF_REMOVE) {
        c.character.effectState.mods.armorClass = 0;
        return;
    }

    if (c.op != EFF_ADD)
        return;

    // Armor class is encoded as 60 - AC. Keep the adjustment in the effect
    // state so the normal stat rebuild applies it without knowing effect 30.
    const uint8 currentAC = c.character.armorClass.current;
    const uint8 targetAC = currentAC < 53 ? 50 : currentAC - 2;
    c.character.effectState.mods.armorClass =
        static_cast<int8>(targetAC - currentAC);

    if (!c.character.combatState)
        return;

    // The original only displayed this when item use was still available.
    if (c.character.combatState->canUse && c.bridge)
        c.bridge->postEffectMessage(&c.character, "is coughing", true);

    c.character.combatState->canUse = false;
    c.character.combatState->canCast = false;
}

static void callChildEffectHandler(const EffectCall &parent, uint8 effectId) {
    if (!parent.handler)
        return;

    // UTIL_CallEffectHandler dispatches the same operation and effect data
    // to another handler. Use a copy so a child cannot replace the composite
    // effect's id or mutate the effect-list node being iterated by EffectSystem.
    Effect child = parent.effect;
    child.id = effectId;
    parent.handler->apply(EffectCall(parent.op, child, parent.character,
        parent.combat, parent.bridge, parent.damage, parent.handler));
}

static void handleDiseased(const EffectCall &c) {
    // EFFECT_34_Diseased is a composite legacy effect. The original invokes
    // handlers 0x2B and 0x2C for every operation (add/remove/tick/eval).
    callChildEffectHandler(c, 0x2B);
    callChildEffectHandler(c, 0x2C);
}

static void handleAnimatingDead(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    // The legacy handler clears the immediate flag before doing anything
    // else. This also prevents a later removal from re-entering this logic.
    c.effect.immediate = 0;
    const uint8 combatSide = c.effect.power >> 4;

    // There is no separate IN_SPELL_PROCESS global in the modern runtime.
    // An active combat spell is the corresponding deferred-status case.
    const bool spellProcessing = c.combat && c.combat->activeSpellId != 0;
    if (!spellProcessing) {
        Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, c.bridge);
        effectSystem.setStatus(c.character, Goldbox::Data::S_DEAD, "collapses");
    }

    Data::PoolradCharacter &character = asPoolrad(c.character);
    character.combatSide = static_cast<Goldbox::Data::CombatSide>(combatSide);
    character.ai_control = true;
    character.levelUndead = 0;
    character.attackLevel = character.levels[Goldbox::Data::C_FIGHTER];
    character.movement.base = 12;

    if (character.npc == (int8)0xB3)
        character.npc = 0;

    // Poolrad uses monsterType as its character/monster type discriminator;
    // zero is the normal character type restored by the original handler.
    character.monsterType = 0;
}

static void handleHalfDamage(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_HALF_DAMAGE);
}

static void handleHalfFireDamage(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_HALF_FIRE_DAMAGE);
}

static void handleDamageReduction(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_DAMAGE_REDUCTION);
}

static void handleFearImmunity(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_FEAR_IMMUNE);
}

static void handleSlowPoison(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_SLOW_POISON);

    // On wear-off: if the character is still poisoned, they die from poison.
    // Then remove E_POISON_DAMAGE without triggering its EFF_REMOVE handler
    // (original clears IN_SPELL_PROCESS guard around UTIL_removeEffect).
    // We replicate that by clearing the immediate flag before removal so
    // removeEffectImpl skips the handler call.
    if (c.op != EFF_REMOVE)
        return;

    CharacterEffects *fx = c.character.getEffects();
    if (!fx)
        return;

    if (fx->hasEffect(static_cast<uint8>(E_POISONED))) {
        // Delegate to the host bridge for the full death sequence.
        // EffectSystem::setStatus is not reachable from a handler directly,
        // so we use the bridge message + status fields, matching what
        // setStatus does before calling onCharacterDied.
        if (c.bridge)
            c.bridge->postEffectMessage(&c.character, "dies from poison", true);
        c.character.healthStatus = Goldbox::Data::S_DEAD;
        c.character.enabled = false;
        c.character.hitPoints.current = 0;
        if (c.bridge)
            c.bridge->onCharacterDied(&c.character);
    }

    Effect *poisonDmg = fx->findEffectById(static_cast<uint8>(E_POISON_DAMAGE));
    if (poisonDmg) {
        poisonDmg->immediate = 0;
        fx->eraseEffectById(static_cast<uint8>(E_POISON_DAMAGE));
    }
}

static void handleEntangle(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_ENTANGLED);
}

static void handleAttackBonus2(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 2;
}

static void handleAttackDamageBonus(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
    c.combat->damage += 3;
}

static void handleSaveBonus1(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
}

static void handleSaveBonus2(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 2;
}

static void handleSaveBonus3(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 3;
}

static void handleSaveBonus5(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 5;
}

static void handleImmunitySleepCharm(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 6;
    asPoolrad(c.character).effectState.flags |= Data::PoolradCharacter::EF_FEAR_IMMUNE;
}

static void handleImmunityCold(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_HALF_DAMAGE);
}

static void handleImmunityFire(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character),
        Data::PoolradCharacter::EF_RESIST_FIRE |
        Data::PoolradCharacter::EF_FIRE_RESIST |
        Data::PoolradCharacter::EF_HALF_FIRE_DAMAGE);
}

static void handleImmunityParalysisPoisonFear(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character),
        Data::PoolradCharacter::EF_SLOW_POISON |
        Data::PoolradCharacter::EF_FEAR_IMMUNE);
    if (!c.combat)
        return;
    c.combat->attackRoll += 6;
}

static void handleImmunityNonmagical(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character),
        Data::PoolradCharacter::EF_RAKSHASA_RESIST |
        Data::PoolradCharacter::EF_DAMAGE_REDUCTION);
}

static void handleSavePenalty2(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 2;
}

static void handleExtraStrength(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
    c.combat->damage += 2;
}

static void handleBonusVsSmall(const EffectCall &c) {
    if (!c.combat || !c.character.combatState
            || !c.character.combatState->target)
        return;

    const Goldbox::Poolrad::Data::PoolradCharacter *target =
        static_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(
            c.character.combatState->target);
    if (target->monsterType != 1 || target->iconDimension != 1)
        return;

    static const char *const kSmallCreatures[] = {
        "KOBOLD",
        "KOBOLD LEADER",
        "GOBLIN",
        "GOBLIN LEADER"
    };

    for (uint i = 0; i < ARRAYSIZE(kSmallCreatures); ++i) {
        if (target->name == kSmallCreatures[i]) {
            ++c.combat->attackRoll;
            return;
        }
    }
}

static void handleFlameTongue(const EffectCall &c) {
    if (!c.combat || !c.character.combatState || !c.character.combatState->target)
        return;
    const Goldbox::Poolrad::Data::PoolradCharacter *target =
        static_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(c.character.combatState->target);
    int8 bonus = 0;
    switch (target->monsterType) {
    case 10: bonus = 1; break;
    case  9:
    case 12: bonus = 2; break;
    case  4: bonus = 3; break;
    default: break;
    }
    c.combat->attackRoll += bonus;
    c.combat->damage     += bonus;
    c.combat->behaviorFlags = 9; // DMG_FIRE | DMG_MAGIC
}

static void handleSwordVsUndead(const EffectCall &c) {
    if (!c.combat || !c.character.combatState || !c.character.combatState->target)
        return;
    const Goldbox::Poolrad::Data::PoolradCharacter *target =
        static_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(c.character.combatState->target);
    if (target->monsterType == 4) {
        c.combat->attackRoll += 2;
        c.combat->damage += 2;
    }
}

static void handleDwarfVsGiant(const EffectCall &c) {
    if (!c.combat || !c.character.combatState
            || !c.character.combatState->target)
        return;

    const Data::PoolradCharacter *target =
        static_cast<const Data::PoolradCharacter *>(
            c.character.combatState->target);
    if (target->monsterType != 1 || target->iconDimension != 2)
        return;

    static const char *const kGiantNames[] = {
        "HILL GIANT",
        "STONE GIANT",
        "FROST GIANT",
        "FIRE GIANT",
        "CLOUD GIANT",
        "STORM GIANT",
        "OGRE",
        "OGRE MAGE"
    };

    for (uint i = 0; i < ARRAYSIZE(kGiantNames); ++i) {
        if (target->name == kGiantNames[i]) {
            ++c.combat->attackRoll;
            return;
        }
    }
}

static void handleDwarfVsLargeMonster(const EffectCall &c) {
    if (!c.combat || !c.character.combatState
            || !c.character.combatState->target)
        return;

    const Data::PoolradCharacter *target =
        static_cast<const Data::PoolradCharacter *>(
            c.character.combatState->target);
    if (target->monsterType != 1 || (target->iconDimension & 0x7f) != 2)
        return;

    static const char *const kLargeMonsters[] = {
        "OGRE",
        "OGRE LEADER",
        "TROLL",
        "FIRE GIANT",
        "HILL GIANT"
    };

    for (uint i = 0; i < ARRAYSIZE(kLargeMonsters); ++i) {
        if (target->name == kLargeMonsters[i]) {
            c.combat->attackRoll -= 4;
            return;
        }
    }
}

static void handleGnomeVsBugbearGnoll(const EffectCall &c) {
    if (!c.combat || !c.character.combatState
            || !c.character.combatState->target)
        return;

    const Data::PoolradCharacter *target =
        static_cast<const Data::PoolradCharacter *>(
            c.character.combatState->target);
    if (target->monsterType != 1)
        return;

    if (target->name == "BUGBEAR" || target->name == "GNOLL")
        c.combat->attackRoll -= 4;
}

static void handleGnomeVsLarge(const EffectCall &c) {
    if (!c.combat || !c.character.combatState
            || !c.character.combatState->target)
        return;

    const Data::PoolradCharacter *target =
        static_cast<const Data::PoolradCharacter *>(
            c.character.combatState->target);
    if (target->monsterType != 1 || target->iconDimension != 2)
        return;

    ++c.combat->attackRoll;
}

static void handleRegeneration(const EffectCall &c) {
    if (c.op != EFF_TICK)
        return;
    tryAddEffect(c.character, E_POOLRAD_REGEN_3_HP, c.effect.power, 60);
    if (c.character.healHp(1, true) && c.bridge)
        c.bridge->showHealResult(&c.character);
}

static void handleEfreetiFireResistance(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & Combat::CombatGlobals::DMG_FIRE))
        return;
    const uint8 count = c.combat->attackCount;
    if (count == 0)
        return;
    uint8 dmg = c.combat->damage;
    for (uint8 i = 0; i < count; ++i) {
        dmg = (dmg < 1) ? 0 : dmg - 1;
        if (dmg < count)
            dmg = count;
    }
    c.combat->damage = dmg;
}

static void handleFireResistance(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & Combat::CombatGlobals::DMG_FIRE))
        return;

    const uint8 count = c.combat->attackCount;
    uint8 dmg = c.combat->damage;
    for (uint8 i = 0; i < count; ++i) {
        dmg = (dmg < 2) ? 0 : dmg - 2;
        if (dmg < count)
            dmg = count;
    }
    c.combat->damage = dmg;
    c.combat->savingThrow += 4;

    if (!(c.combat->behaviorFlags & Combat::CombatGlobals::DMG_MAGIC))
        c.combat->damage = 0;
}

static void handleRegenerating(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    c.character.addEffect(static_cast<uint8>(E_DAMAGE_REDUCTION), 0, 0xff, false);
}

static void handleImmobilized(const EffectCall &c) {
    handleParalyze(c);
    if (c.op != EFF_EVAL || !c.character.combatState)
        return;
    c.character.combatState->movePoints = 0;
    if (c.combat && c.combat->attackCountAdjusting)
        c.combat->attackCount = 0;
}

static void handleRot(const EffectCall &c) {
    if (c.op != EFF_TICK)
        return;

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "Rots...", true);

    c.character.abilities.charisma.current =
        MAX<uint8>(3, c.character.abilities.charisma.current - 2);

    if (c.effect.power / 16 < 2) {
        Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, c.bridge);
        effectSystem.setStatus(c.character, Goldbox::Data::S_GONE,
            "Dies rots away");
    } else {
        c.effect.power -= 16;
        tryAddEffect(c.character, E_POOLRAD_ROT, c.effect.power, 43200);
    }
}

static void handleInvisibleRing(const EffectCall &c) {
    if (c.op == EFF_ADD)
        c.character.addEffect(E_POOLRAD_BLUR, 12, 1, false);
}

static void handleHelplessPoolrad(const EffectCall &c) {
    handleHelpless(c);
    if (c.op == EFF_ADD)
        c.character.resetCombatAction();
}

static void handleEndlessRegen(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_REGEN_3);
    if (c.op != EFF_TICK)
        return;
    if (c.character.healHp(1, true) && c.bridge)
        c.bridge->showHealResult(&c.character);
}

// Mirrors UTIL_CheckUnaffected: runs the ES_APPLY_WITH_MESSAGE_GUARDS immunity
// set (set 9) against effectId. If the effect is blocked by an immunity, or if
// checkSave is true and the save succeeded, posts "is Unaffected" and returns
// false. Otherwise refreshes or adds the effect and posts message.
// duration == 0 has refresh-always semantics (matches original).
static bool checkUnaffected(const EffectCall &c, uint8 effectId,
        uint16 duration, uint8 power, bool immediate,
        bool checkSave, bool saved,
        const char *message) {
    // Run immunity/protection set. The original stores the current effect id
    // in a global (BYTE_CURRENT_EFFECT) so immunity handlers can zero it.
    // We replicate by running the set and checking whether the character
    // already has a blocking immunity effect for this id.
    // Simplified: check ES_APPLY_WITH_MESSAGE_GUARDS via EffectRuntime.
    CharacterEffects *fx = c.character.getEffects();

    // Immunity check: run set 9 handlers to let them cancel the effect.
    // We detect cancellation by checking if any set-9 effect for this id
    // is present and would block it. The original used a mutable global;
    // here we use the EffectRuntime path which calls handlers in-place.
    bool blocked = false;
    if (c.handler && fx) {
        // Build a temporary combat context to capture set-9 modifier output.
        Combat::CombatGlobals tempCombat;
        if (c.combat)
            tempCombat = *c.combat;
        EffectRuntime runtime(const_cast<EffectHandlerBase *>(c.handler), c.bridge);
        runtime.checkEffectSet(ES_APPLY_WITH_MESSAGE_GUARDS, *fx, c.character, &tempCombat);
        // If the set zeroed the damage (canonical immunity signal), treat as blocked.
        blocked = (tempCombat.damage == 0 && c.combat && c.combat->damage != 0);
    }

    if (blocked || (checkSave && saved)) {
        if (c.bridge)
            c.bridge->postEffectMessage(&c.character, "is Unaffected", true);
        return false;
    }

    // Refresh or add the effect, mirroring CHARACTER_findEffect /
    // CHARACTER_removeEffect / CHARACTER_addEffect sequence.
    if (fx) {
        Effect *existing = fx->findEffectById(effectId);
        if (existing) {
            const bool shouldRefresh = (duration == 0) ||
                (existing->durationMin != 0 && existing->durationMin < duration);
            if (shouldRefresh) {
                // Remove with handler notification, then re-add below.
                existing->immediate = 0; // suppress EFF_REMOVE on refresh
                fx->eraseEffectById(effectId);
            } else {
                // Existing effect is at least as long; nothing to do.
                return false;
            }
        }
    }

    c.character.addEffect(effectId, duration, power, immediate);

    if (message && message[0] != '\0' && c.bridge)
        c.bridge->postEffectMessage(&c.character, message, true);

    return true;
}

// Mirrors UTIL_CheckSavingThrow: rolls d20, auto-fail on 1, auto-pass on 20,
// otherwise accumulates saveBonus + modifier + ES_SAVING_THROW_MODS effect
// set mods, then compares against savingThrows[savingThrowType].
// savingThrowType: 0=vsParalysis, 1=vsPetrification, 2=vsRodStaffWand,
//                 3=vsBreathWeapon, 4=vsSpell
static bool checkSavingThrow(const EffectCall &c, uint8 savingThrowType,
        int8 saveModifier) {
    if (!Goldbox::g_engine)
        return false;

    const int roll = Goldbox::g_engine->rollDice(1, 20);
    if (roll == 1)
        return false;
    if (roll == 20)
        return true;

    const Goldbox::Data::ADnDCharacter &adnd =
        static_cast<const Goldbox::Data::ADnDCharacter &>(c.character);

    // Accumulate saving throw modifiers from ES_SAVING_THROW_MODS (set 12)
    // via a temporary combat globals (mirrors UTIL_checkEffectSet(12, char)).
    Combat::CombatGlobals tempCombat;
    if (c.combat)
        tempCombat = *c.combat;

    CharacterEffects *fx = c.character.getEffects();
    if (fx && c.handler) {
        EffectRuntime runtime(const_cast<EffectHandlerBase *>(c.handler), c.bridge);
        runtime.checkEffectSet(ES_SAVING_THROW_MODS, *fx, c.character, &tempCombat);
    }

    // SavingThrows fields are laid out in order matching savingThrowType 0-4.
    const uint8 target = (&adnd.savingThrows.vsParalysis)[savingThrowType];
    const int total = roll + adnd.saveBonus + saveModifier + tempCombat.savingThrow;
    return total >= target;
}

static void applyPoisonAttack(const EffectCall &c, int8 saveModifier) {
    if (!c.bridge)
        return;
    c.bridge->postEffectMessage(&c.character, "is Poisoned", true);

    // Remove the poisoned status effect (E_POISONED = 55).
    CharacterEffects *fx = c.character.getEffects();
    if (fx)
        fx->eraseEffectById(static_cast<uint8>(E_POISONED));
    c.character.effectState.flags &= ~CEF_POISONED;

    if (checkSavingThrow(c, 0, saveModifier))
        return;

    Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, c.bridge);
    effectSystem.setStatus(c.character, Goldbox::Data::S_DEAD, "is killed");
}

static void handlePoisonAttack(const EffectCall &c) {
    if (c.op == EFF_ADD)
        applyPoisonAttack(c, 0);
}

static void handlePoisonAttackSaveBonus4(const EffectCall &c) {
    if (c.op == EFF_ADD)
        applyPoisonAttack(c, +4);
}

static void handlePoisonAttackSaveBonus2(const EffectCall &c) {
    if (c.op == EFF_ADD)
        applyPoisonAttack(c, +2);
}

static void handlePoisonMeleeSavePenalty2(const EffectCall &c) {
    if (c.op == EFF_ADD)
        applyPoisonAttack(c, -2);
}

// Mirrors EFFECT_ApplyParalysis: on a failed save vs paralysis, shows
// combat feedback and applies the paralyzed status (effect param 12).
static void applyParalysisAttack(const EffectCall &c, uint8 duration) {
    if (checkSavingThrow(c, 0, 0))
        return;

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "is Paralyzed", true);

    c.character.addEffect(E_IMMOBILIZED, duration, 12, false);
}

static void handleParalysisMeleeAttack(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    const uint8 duration = Goldbox::g_engine ?
        Goldbox::g_engine->rollDice(2, 8) : 2;   // 2d8
    applyParalysisAttack(c, duration);
}

static void handleParalysisMeleeAttackNoElves(const EffectCall &c) {
    // Elves are immune to this particular paralysis attack.
    if (c.op != EFF_ADD || c.character.race == Goldbox::Data::R_ELF)
        return;
    applyParalysisAttack(c, 63);
}

static void handleParalysisMeleeAttackStrong(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    const uint8 duration = (Goldbox::g_engine ?
        Goldbox::g_engine->rollDice(1, 9) : 1) + 10;   // 1d9 + 10
    applyParalysisAttack(c, duration);
}

static void handleInvisibleAttack(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.combat)
        return;
    c.combat->targetUnavailable = true;
    c.combat->attackRoll -= 4;
}

static void handleCamouflagedAttack(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    // 95% chance to gain camouflage; fails (and stays uncamouflaged) on a
    // roll of 96-100. Raw effect id 25 per the decompiled table (not
    // E_CAMOUFLAGE, which is a different id in this port's Effects enum).
    const uint8 roll = Goldbox::g_engine ?
        Goldbox::g_engine->rollDice(1, 100) : 100;
    if (roll < 96)
        c.character.addEffect(25, 1, 12, false);
}

// Rear-claw rake: bonus 2d4 attack that fires mid-attack-sequence, only on
// the resolution where exactly two attacks remain this round (e.g. a
// claw/claw/bite monster raking with a rear claw after its first attack).
// TODO: applies damage directly since there is no ported to-hit resolver
// equivalent to COMBAT_ResolveAttack; the original re-rolls to-hit here too.
static void handleRearClawRake(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.combat || c.combat->attacksLeft != 2)
        return;

    Goldbox::Data::PlayerCharacter *target =
        c.character.combatState ? c.character.combatState->target : nullptr;
    if (!target || !target->enabled)
        return;

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "Rakes...", true);

    const uint8 damage = Goldbox::g_engine ?
        Goldbox::g_engine->rollDice(2, 4) : 2;
    if (c.damage)
        c.damage->applyLegacy(*target, damage, Goldbox::Data::DAMAGE_NORMAL, false);

    c.character.resetCombatAction();
}

// Grapple/rear attack: one-shot 3d4 hit against a roster-indexed target
// (effect.power holds the target's roster index, not the current combat
// target). Ends itself once the target goes down; otherwise, on interrupt
// or removal, releases the target's immobilized status
// (E_IMMOBILIZED, effect 58).
// TODO: applies damage directly, see handleRearClawRake's to-hit note above.
static void handleGrappleAttack(const EffectCall &c) {
    Combat::CombatContext *ctx = Goldbox::g_engine ?
        Goldbox::g_engine->getCombatContext() : nullptr;
    Goldbox::Data::PlayerCharacter *target =
        (ctx && c.effect.power < ctx->params.roster.size()) ?
        ctx->params.roster[c.effect.power] : nullptr;

    if (c.op == EFF_ADD && c.character.enabled && target && target->enabled) {
        const uint8 damage = Goldbox::g_engine ?
            Goldbox::g_engine->rollDice(3, 4) : 3;
        if (c.damage)
            c.damage->applyLegacy(*target, damage, Goldbox::Data::DAMAGE_NORMAL, false);

        c.character.resetCombatAction();

        if (!target->enabled) {
            CharacterEffects *fx = c.character.getEffects();
            if (fx)
                fx->eraseEffectById(c.effect.id);
        }
        return;
    }

    if (target) {
        CharacterEffects *targetFx = target->getEffects();
        if (targetFx)
            targetFx->eraseEffectById(static_cast<uint8>(E_IMMOBILIZED));
    }

    if (c.op == EFF_ADD) {
        c.effect.immediate = 0;
        CharacterEffects *fx = c.character.getEffects();
        if (fx)
            fx->eraseEffectById(c.effect.id);
    }
}

// Vampiric attack: effect.power is the target's roster index.
// Duration is measured in 16-unit ticks; each drain consumes damage*16 ticks.
static void handleSucksBlood(const EffectCall &c) {
    Combat::CombatContext *ctx = Goldbox::g_engine ?
        Goldbox::g_engine->getCombatContext() : nullptr;
    Goldbox::Data::PlayerCharacter *target =
        (ctx && c.effect.power < ctx->params.roster.size()) ?
        ctx->params.roster[c.effect.power] : nullptr;

    if (c.op == EFF_ADD && c.character.enabled && target && target->enabled) {
        if (c.bridge)
            c.bridge->postEffectMessage(&c.character, "Sucks some Blood", true);

        const uint8 damage = Goldbox::g_engine ?
            Goldbox::g_engine->rollDice(1, 4) : 1;

        if (c.combat)
            c.combat->behaviorFlags = 0;
        if (c.damage)
            c.damage->applyLegacy(*target, damage, Goldbox::Data::DAMAGE_NORMAL, false);

        c.character.resetCombatAction();

        if (target->enabled && (c.effect.durationMin / 16) > damage) {
            c.effect.durationMin -= damage * 16;
        } else {
            CharacterEffects *fx = c.character.getEffects();
            if (fx)
                fx->eraseEffectById(c.effect.id);
        }
        return;
    }

    // Drain interrupted (or naturally removed): release the target's immobilization.
    if (target) {
        CharacterEffects *targetFx = target->getEffects();
        if (targetFx)
            targetFx->eraseEffectById(static_cast<uint8>(E_IMMOBILIZED));
    }

    if (c.op == EFF_ADD) {
        c.effect.immediate = 0;
        CharacterEffects *fx = c.character.getEffects();
        if (fx)
            fx->eraseEffectById(c.effect.id);
    }

    if (c.character.enabled && target && target->enabled && c.character.combatState)
        c.character.combatState->fleeing = true;
}

// Revive: re-places the character on the combat map at their last known tile
// position. On success, restores status/enabled and posts a revival message,
// then calls updateSideCount. On failure (tile blocked), reschedules effect 78
// itself using effect.power as the retry duration.
static void handleRevive(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    Combat::CombatContext *ctx = Goldbox::g_engine ?
        Goldbox::g_engine->getCombatContext() : nullptr;
    if (!ctx)
        return;

    // Read last known position from the downed-member records, falling back
    // to the live table entry if the character was never recorded as downed.
    uint8 col = 0, row = 0;
    bool found = false;
    for (const auto &rec : ctx->table.getDownedMembers()) {
        if (rec.character == &c.character) {
            col = rec.tileCol;
            row = rec.tileRow;
            found = true;
            break;
        }
    }
    if (!found) {
        col = ctx->table.getCharacterCol(&c.character);
        row = ctx->table.getCharacterRow(&c.character);
    }

    const uint8 size = MAX<uint8>(1, ctx->table.getCharacterSize(&c.character));
    const uint8 hp = c.character.hitPoints.current;

    const int idx = ctx->table.addCombatant(&c.character, size);
    if (idx < 0) {
        // Tile blocked: retry next turn.
        tryAddEffect(c.character, 0x4e, c.effect.power, 1);
        return;
    }
    ctx->table.setPosition(idx, col, row);

    c.character.healthStatus = Goldbox::Data::S_OKAY;
    c.character.enabled = true;
    c.character.hitPoints.current = hp;

    if (c.bridge) {
        const char *msg = (c.character.combatSide == Goldbox::Data::CS_ENEMY) ?
            "stands up and grins" : "gets back up";
        c.bridge->postEffectMessage(&c.character, msg, true);
    }

    ctx->updateSideCount();
}

// Bite-and-hold attack: only works against targets with strength < 19.
// Starts effect 74 (grapple) on the attacker with the target's roster index,
// then sets and immediately fires effect 58 (E_IMMOBILIZED) on the target.
static void handleBiteAndHold(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    Goldbox::Data::PlayerCharacter *target =
        c.character.combatState ? c.character.combatState->target : nullptr;
    if (!target || target->abilities.strength.current >= 19)
        return;

    Combat::CombatContext *ctx = Goldbox::g_engine ?
        Goldbox::g_engine->getCombatContext() : nullptr;
    uint8 targetIndex = 0xff;
    if (ctx) {
        for (uint i = 0; i < ctx->params.roster.size(); ++i) {
            if (ctx->params.roster[i] == target) {
                targetIndex = (uint8)i;
                break;
            }
        }
    }

    c.character.addEffect(0x4a, 0, targetIndex, true);
    target->addEffect(static_cast<uint8>(E_IMMOBILIZED), 0, 0xff, false);

    if (c.handler) {
        Effect immobilize;
        immobilize.id = static_cast<uint8>(E_IMMOBILIZED);
        immobilize.durationMin = 0;
        immobilize.power = 0xff;
        immobilize.immediate = false;
        c.handler->apply(EFF_ADD, immobilize, *target, c.combat, c.bridge);
    }

    if (c.bridge)
        c.bridge->postEffectMessage(target, "is held fast", true);
}

// Triggers the blood-drain attack: starts effect 75 (vampiric drain) on the
// attacker with duration=207 ticks and power=target roster index, then sets
// and immediately fires effect 58 (E_IMMOBILIZED) on the target.
static void handleBloodDrainingAttack(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    Goldbox::Data::PlayerCharacter *target =
        c.character.combatState ? c.character.combatState->target : nullptr;
    if (!target)
        return;

    Combat::CombatContext *ctx = Goldbox::g_engine ?
        Goldbox::g_engine->getCombatContext() : nullptr;
    uint8 targetIndex = 0xff;
    if (ctx) {
        for (uint i = 0; i < ctx->params.roster.size(); ++i) {
            if (ctx->params.roster[i] == target) {
                targetIndex = (uint8)i;
                break;
            }
        }
    }

    c.character.addEffect(0x4b, 207, targetIndex, true);
    target->addEffect(static_cast<uint8>(E_IMMOBILIZED), 0, 0xff, false);

    if (c.handler) {
        Effect immobilize;
        immobilize.id = static_cast<uint8>(E_IMMOBILIZED);
        immobilize.durationMin = 0;
        immobilize.power = 0xff;
        immobilize.immediate = false;
        c.handler->apply(EFF_ADD, immobilize, *target, c.combat, c.bridge);
    }
}

// Mirrors EFFECT_81_DragonFearAura: iterates the party and applies a
// fear effect to each enabled character on the opposite combat side.
// Level 0-3: paralyzed with fear (E_POOLRAD_HELPLESS_34, power=12).
// Level 4-5: weakened by fear (E_POOLRAD_CURSED, power=12).
// Level 6+: unaffected.
// Saving throw vs. spell (type 4) blocks the effect.
// Mirrors EFFECT_82_MummyFearAura: iterates the party, first stripping any
// prior mummy-fear-aura instance from every character, then applying a
// paralysis-fear effect to enabled characters on the opposite side.
// Save bonus = (sideCount[target->combatSide] > 5) + (race == human ? 2 : 0).
// Duration is 1d4 minutes. Save vs. paralysis (type 0) blocks the effect.
// Mirrors EFFECT_83_PetrifyingGaze.
// Announces the gaze, checks for a reflective item on the target (redirecting
// the gaze back at the attacker when found), then rolls save vs. petrification
// (type 1). On failure, sets the final target's status to S_STONED.
//
// TODO: GFX_LoadEffectTileQuad(18) and AnimateGaze() are not yet implemented;
//       the visual gaze animation is skipped until the gfx effect system is ported.
// Mirrors EFFECT_84_CharmingGaze.
// Requires a valid combat target. Guards on canEngageTarget and line-of-sight
// (both TODO: not yet ported). Announces the gaze, sets activeSpellId=10,
// rolls save vs. spell (type 4, modifier -2), then applies charm effect (0x0B)
// via checkUnaffected. If the effect was accepted, fires its EFF_ADD handler
// directly to trigger the charm side-effects (combatSide swap, ai_control, etc.).
//
// TODO: COMBAT_canEngageTarget and COMBAT_checkLineOfSight are not yet ported;
//       the engagement and LOS guards are skipped until the combat system
//       provides these queries.
// TODO: GFX_LoadEffectTileQuad(18) and COMBAT_AnimateMissilePath are not yet
//       implemented; the visual gaze animation is skipped.
static void handleCharmingGaze(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    if (!c.character.combatState || !c.character.combatState->target)
        return;

    Goldbox::Data::PlayerCharacter *target = c.character.combatState->target;

    // TODO: if (!COMBAT_canEngageTarget(attacker, target)) return;
    // TODO: if (!COMBAT_checkLineOfSight(attackerCol, attackerRow,
    //                                    targetCol, targetRow)) return;

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "Gazes...", false);

    // TODO: GFX_LoadEffectTileQuad(18);
    // TODO: COMBAT_AnimateMissilePath(attackerCol, attackerRow,
    //                                 targetCol, targetRow, 4, 4);

    if (c.combat)
        c.combat->activeSpellId = 10;

    // power = attacker's combat side * 0x80 + 12
    const uint8 power = static_cast<uint8>(
        static_cast<uint8>(c.character.combatSide) * 0x80 + 12);

    Effect dummy = c.effect;
    EffectCall tc(EFF_ADD, dummy, *target, c.combat, c.bridge,
        c.damage, c.handler);

    const bool saved = checkSavingThrow(tc, 4, -2);

    checkUnaffected(tc, E_POOLRAD_CHARM_PERSON, 0, power, true,
        true, saved, "is charmed");

    // If the charm effect was accepted, fire its EFF_ADD handler directly.
    // This mirrors UTIL_CallEffectHandler(target, effect, EO_ADD, 11) which
    // triggers the combatSide swap and ai_control logic in handleCharm.
    if (c.handler) {
        CharacterEffects *fx = target->getEffects();
        Effect *charmEffect = fx ?
            fx->findEffectById(E_POOLRAD_CHARM_PERSON) : nullptr;
        if (charmEffect)
            c.handler->apply(EFF_ADD, *charmEffect, *target,
                c.combat, c.bridge);
    }
}

static void handlePetrifyingGaze(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    if (!c.character.combatState || !c.character.combatState->target)
        return;

    Goldbox::Data::PlayerCharacter *target = c.character.combatState->target;

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "gazes...", false);

    // TODO: GFX_LoadEffectTileQuad(18); AnimateGaze(attacker, target);

    // If the attacker carries the reflective-gaze marker (raw 0x7F), check
    // whether the target has a readied item with nameCode 'v' (0x76).
    // If found, reflect the gaze back at the attacker.
    CharacterEffects *attackerFx = c.character.getEffects();
    if (attackerFx && attackerFx->hasEffect(E_POOLRAD_REFLECTIVE_GAZE_MARKER)) {
        const Goldbox::Data::ADnDCharacter *adndTarget =
            static_cast<const Goldbox::Data::ADnDCharacter *>(target);
        for (const Goldbox::Data::Items::CharacterItem &item :
                adndTarget->inventory.items()) {
            if (!item.readied)
                continue;
            if (item.nameCode1 == 'v' ||
                    item.nameCode2 == 'v' ||
                    item.nameCode3 == 'v') {
                if (c.bridge)
                    c.bridge->postEffectMessage(target, "reflects it!", false);
                // TODO: AnimateGaze(target, attacker);
                target = &c.character;
                break;
            }
        }
    }

    Effect dummy = c.effect;
    EffectCall tc(EFF_ADD, dummy, *target, c.combat, c.bridge,
        c.damage, c.handler);

    if (!checkSavingThrow(tc, 1, 0)) {
        Goldbox::Data::Effects::EffectSystem effectSystem(
            const_cast<EffectHandlerBase *>(c.handler), c.bridge);
        effectSystem.setStatus(*target, Goldbox::Data::S_STONED, "is Stoned");
    }
}

static void handleMummyFearAura(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return;

    const Goldbox::Data::CombatSide sourceSide = c.character.combatSide;

    for (Goldbox::Data::PlayerCharacter *ch : *party) {
        if (!ch)
            continue;

        // Always strip any prior mummy fear aura instance first.
        CharacterEffects *fx = ch->getEffects();
        if (fx)
            fx->eraseEffectById(E_POOLRAD_MUMMY_FEAR_AURA);

        if (!ch->enabled)
            continue;

        // Only affect characters on the opposite side from the mummy.
        if (ch->combatSide == sourceSide)
            continue;

        // saveBonus = 1 if the target's side has more than 5 members, else 0.
        // Humans receive an additional +2.
        int8 saveBonus = 0;
        if (c.combat && c.combat->sideCount[ch->combatSide] > 5)
            saveBonus = 1;
        if (ch->race == Goldbox::Data::R_HUMAN)
            saveBonus += 2;

        const uint16 duration = Goldbox::g_engine ?
            static_cast<uint16>(Goldbox::g_engine->rollDice(1, 4)) : 1;

        Effect dummy = c.effect;
        EffectCall tc(EFF_ADD, dummy, *ch, c.combat, c.bridge,
            c.damage, c.handler);

        const bool saved = checkSavingThrow(tc, 0, saveBonus);

        checkUnaffected(tc, E_POOLRAD_HELPLESS_34, duration, 12, false,
            true, saved, "is paralyzed with fear");
    }
}

static void handleDragonFearAura(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return;

    const Goldbox::Data::CombatSide dragonSide = c.character.combatSide;

    for (Goldbox::Data::PlayerCharacter *ch : *party) {
        if (!ch || !ch->enabled)
            continue;

        // Only affect characters on the opposite side from the dragon.
        if (ch->combatSide == dragonSide)
            continue;

        const Data::PoolradCharacter *pch =
            static_cast<const Data::PoolradCharacter *>(ch);
        const uint8 level = pch->highestLevel;

        // Level 6+ are immune to dragon fear.
        if (level >= 6)
            continue;

        // Build a minimal EffectCall for the target so checkUnaffected and
        // checkSavingThrow operate on the correct character.
        Effect dummy = c.effect;
        EffectCall tc(EFF_ADD, dummy, *ch, c.combat, c.bridge,
            c.damage, c.handler);

        const bool saved = checkSavingThrow(tc, 4, 0);

        if (level <= 3) {
            checkUnaffected(tc, E_POOLRAD_HELPLESS_34, 0, 12, false,
                true, saved, "is paralyzed with fear");
        } else {
            // level == 4 or 5
            checkUnaffected(tc, E_POOLRAD_CURSED, 0, 12, false,
                true, saved, "is weakened by fear");
        }
    }
}

static void handleAnkhegAcidMeleeAttack(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.combat)
        return;

    Goldbox::Data::PlayerCharacter *target =
        c.character.combatState ? c.character.combatState->target : nullptr;
    if (!target || !target->enabled)
        return;

    c.combat->behaviorFlags = Combat::CombatGlobals::DMG_ACID;

    const uint8 damage = Goldbox::g_engine ?
        Goldbox::g_engine->rollDice(1, 4) : 1;
    if (c.damage)
        c.damage->applyLegacy(*target, damage, Goldbox::Data::DAMAGE_NORMAL, false);
}

static void handleFireTouchAttack(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.combat)
        return;

    Goldbox::Data::PlayerCharacter *target =
        c.character.combatState ? c.character.combatState->target : nullptr;
    if (!target || !target->enabled)
        return;

    c.combat->behaviorFlags = 9; // DMG_FIRE | DMG_MAGIC

    const uint8 damage = Goldbox::g_engine ?
        Goldbox::g_engine->rollDice(2, 10) : 2;
    if (c.damage)
        c.damage->applyLegacy(*target, damage, Goldbox::Data::DAMAGE_NORMAL, false);
}

static void handleStudyManualBodilyHealth(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "starts to train", true);
    c.character.addEffect(7, 43200, 0xff, true);
}

static void handleTrainingManualBodilyHealth(const EffectCall &c) {
    if (c.op == EFF_TICK) {
        // On the first tick (immediate flag still set), apply the one-time
        // constitution and HP bonus, matching the original which fired the
        // handler body on the first game tick after CHARACTER_addEffect.
        if (c.effect.immediate) {
            c.effect.immediate = 0;

            Data::PoolradCharacter &ch = asPoolrad(c.character);

            if (c.bridge)
                c.bridge->postEffectMessage(&ch, "is hardier", true);

            ch.abilities.constitution.current += 1;

            if (ch.abilities.constitution.current >= 20) {
                ch.addEffect(0x3e, 0x3c, 0xff, true);
                return;
            }

            if (ch.abilities.constitution.current > 14) {
                uint8 divisor = 0;
                for (uint8 i = 0; ; ++i) {
                    const uint8 *slots = &ch.spellSlots.cleric.level1;
                    uint8 slotVal = (i < 6) ? slots[i] : 0;
                    if ((int8)slotVal > 0) {
                        if (i == 2) {
                            divisor += (ch.abilities.constitution.current - 15) *
                                       ch.levels[Goldbox::Data::C_FIGHTER];
                        } else if (ch.abilities.constitution.current < 16) {
                            divisor += slotVal;
                        } else {
                            divisor += slotVal * 2;
                        }
                    }
                    if (i == 7) break;
                }
                if (divisor == 0)
                    divisor = 1;
                uint8 hpBonus = (ch.hitPoints.max - ch.hitPointsRolled) / divisor;
                if (ch.abilities.constitution.current < 17 ||
                        (int8)ch.levels[Goldbox::Data::C_FIGHTER] > 0) {
                    ch.hitPoints.max     += hpBonus;
                    ch.hitPoints.current += hpBonus;
                }
            }
        }
        c.character.heal(1);
        return;
    }
}

} // namespace

EffectHandler::EffectHandler() {
    setupHandlers();
}

void EffectHandler::setupHandlers() {
    clearHandlers();
    setDefaultHandler(&EffectHandler::handleNoop);

    // Register shared common handlers first; game-specific ones below override.
    setupCommonHandlers(*this);

    // Poolrad-specific raw ID registrations.
    // IDs that identity-map correctly to a common handler are omitted:
    //   0x14 (20=E_RESIST_FIRE), 0x15 (21=E_SILENCE_15_RADIUS),
    //   0x16 (22=E_SLOW_POISON), 0x3B (59=E_REGENERATE_3_HPS).
    setSpecHandler(E_POOLRAD_DETECT_MAGIC,          handleNotImplemented);
    setSpecHandler(E_POOLRAD_ENLARGE_STRENGTHEN,    handleEnlargeStrengthened);
    setSpecHandler(E_POOLRAD_UNIMPLEMENTED_0D,      handleNotImplemented);
    setSpecHandler(E_POOLRAD_FRIENDLY,              handleFriendly);
    setSpecHandler(E_POOLRAD_POISON_DAMAGE,         handlePoisonDamage);
    setSpecHandler(E_POOLRAD_READ_MAGIC,            handleNotImplemented);
    setSpecHandler(E_POOLRAD_SHIELD,                handleShield);
    setSpecHandler(E_POOLRAD_SMALL_VS_NORMAL,       handleBonusVsSmall);
    setSpecHandler(E_POOLRAD_UNIMPLEMENTED_13,      handleNotImplemented);
    setSpecHandler(E_POOLRAD_SPIRITUAL_HAMMER,      handleSpiritualHammer);
    setSpecHandler(E_POOLRAD_TRUE_SEEING,           handleNotImplemented);
    setSpecHandler(E_POOLRAD_BLUR,                  handleBlur);
    setSpecHandler(E_POOLRAD_DWARF_TARGET_BONUS,    handleDwarfTargetBonus);
    setSpecHandler(E_POOLRAD_DUPLICATED,            handleDuplicated);
    setSpecHandler(E_POOLRAD_ENFEEBLED,             handleEnfeebled);
    setSpecHandler(E_POOLRAD_NAUSEATED,             handleNauseated);
    setSpecHandler(E_POOLRAD_HELPLESS,              handleHelplessPoolrad);
    setSpecHandler(E_POOLRAD_ANIMATING_DEAD,        handleAnimatingDead);
    setSpecHandler(E_POOLRAD_BLINDED,               handleBlinded);
    setSpecHandler(E_POOLRAD_DISEASED,              handleDiseased);
    setSpecHandler(E_POOLRAD_PRAYER,                handlePrayer);
    setSpecHandler(E_POOLRAD_ACCURSED,              handleAccursed);
    setSpecHandler(E_POOLRAD_PROT_NORMAL_WEAPONS,   handleProtNormalWeapons);
    setSpecHandler(E_POOLRAD_SLOWED,                handleSlow);
    setSpecHandler(E_POOLRAD_PROT_FROM_EVIL_10,     handleProtectionFromEvil);
    setSpecHandler(E_POOLRAD_PROT_FROM_GOOD_10,     handleProtectionFromGood);
    setSpecHandler(E_POOLRAD_DWARF_VS_GIANT,        handleDwarfVsGiant);
    setHandler(E_DWARF_AND_GNOME_VS_GIANTS,          handleDwarfVsLargeMonster);
    setHandler(E_GNOME_LARGE_MONSTER,                 handleGnomeVsBugbearGnoll);
    setSpecHandler(E_POOLRAD_GNOME_VS_LARGE,        handleGnomeVsLarge);
    setSpecHandler(E_POOLRAD_PRAYER_2,              handlePrayer);
    setSpecHandler(E_POOLRAD_ENDLESS_REGEN,         handleEndlessRegen);
    setSpecHandler(E_POOLRAD_INVISIBLE_RING,         handleInvisibleRing);
    setSpecHandler(E_POOLRAD_HELPLESS_33,           handleHelpless);
    setSpecHandler(E_POOLRAD_HELPLESS_34,           handleHelpless);
    setSpecHandler(E_POOLRAD_HELPLESS_35,           handleHelpless);
    setSpecHandler(0x36,                            handleNotImplemented);
    setSpecHandler(0x37,                            handleNotImplemented);
    setSpecHandler(E_POOLRAD_FIRE_RESISTANCE,        handleFireResistance);
    setSpecHandler(E_POOLRAD_REGENERATING,          handleRegenerating);
    setSpecHandler(E_POOLRAD_ROT,                   handleRot);
    // Raw 0x3A identity-maps to E_IMMOBILIZED=58; no Poolrad-specific alias needed.
    setSpecHandler(E_IMMOBILIZED,                   handleImmobilized);
    setSpecHandler(E_POOLRAD_REGEN_3_HP,            handleRegeneration);
    setSpecHandler(E_POOLRAD_FLAME_TONGUE_WEAPON,   handleFlameTongue);
    setSpecHandler(E_POOLRAD_SWORD_VS_UNDEAD,       handleSwordVsUndead);
    setSpecHandler(E_POOLRAD_STUDY_MANUAL_BODILY_HEALTH, handleStudyManualBodilyHealth);
    setSpecHandler(E_POOLRAD_TRAIN_MANUAL_BODILY_HEALTH, handleTrainingManualBodilyHealth);
    setHandler(E_STINKING_CLOUD_EXPAIR,             handleInStinkingCloudExpire);
    setHandler(E_SILENCE_15_RADIUS,                 handleSilence);
    setHandler(E_INVISIBILITY,                      handleInvisibility);
    setHandler(E_INVISIBLE,                         handleInvisibility);
    setHandler(E_ITEM_INVISIBILITY,                 handleItemInvisibility);
    setHandler(E_CAMOUFLAGE,                        handleCamouflage);
    setHandler(E_IMMUNE_TO_ELECTRICITY,             handleImmuneElec);
    setHandler(E_RESIST_FIRE,                       handleResistFire);
    setHandler(E_RESIST_FIRE_AND_COLD,              handleResistFireAndCold);
    setHandler(E_FIRE_RESIST,                       handleFireResist);
    setHandler(E_PROT_FROM_NORMAL_MISSILES,         handleProtNormalMissiles);
    setHandler(E_PROT_DRAG_BREATH,                  handleProtDragBreath);
    setHandler(E_MINOR_GLOBE_OF_INVULNERABILITY,    handleMinorGlobe);
    setHandler(E_RAKSHASA_RESIST_NORMAL_WEAPONS,    handleRakshasaResist);
    setHandler(E_DISPLACE,                          handleDisplace);
    setHandler(E_HALF_DAMAGE,                       handleHalfDamage);
    setHandler(E_HALF_FIRE_DAMAGE,                  handleHalfFireDamage);
    setHandler(E_DAMAGE_REDUCTION,                  handleDamageReduction);
    setHandler(E_FEAR_IMMUNITY,                     handleFearImmunity);
    setHandler(E_SLOW_POISON,                       handleSlowPoison);
    setHandler(E_ENTANGLE,                          handleEntangle);
    setHandler(E_PETRIFYING_GAZE,                   handleAttackBonus2);
    setHandler(E_BEHOLDER_RAYS_AFFECT_57,           handleAttackBonus2);
    setHandler(E_AFFECT_4A,                         handleAttackBonus2);
    setHandler(E_AFFECT_4E,                         handleAttackBonus2);
    setHandler(E_FIRE_ATTACK_2D10,                  handleAttackDamageBonus);
    setHandler(E_ANKHEG_ACID_ATTACK,                handleAttackDamageBonus);
    setHandler(E_GIANT_SLUG_SPIT_ACID,              handleAttackDamageBonus);
    setHandler(E_BREATH_ELEC,                       handleAttackDamageBonus);
    setHandler(E_BREATH_ACID,                       handleAttackDamageBonus);
    setHandler(E_CLOUD_KILL,                        handleAttackDamageBonus);
    setHandler(E_ANKHEG_ACID_SQUIRT_ATTACK,         handleAttackDamageBonus);
    setHandler(E_WILD_BOAR_DIE_AFTER_EXTRA_FIGHT_TIME_AFFECT_5F, handleAttackDamageBonus);
    setHandler(E_OWLBEAR_HUG_CHECK,                 handleAttackDamageBonus);
    setHandler(E_WILD_BOAR_AND_BULLETTE_AFFECT_63,  handleAttackDamageBonus);
    setHandler(E_THRI_KREEN_MISSILE_EVASION,        handleSaveBonus2);
    setHandler(E_BOULDER_EVASION,                   handleSaveBonus2);
    setHandler(E_RESIST_MAGIC_15,                   handleSaveBonus1);
    setHandler(E_RESIST_SLEEP_CHARM_30,             handleSaveBonus2);
    setHandler(E_RESIST_MAGIC_50,                   handleSaveBonus3);
    setHandler(E_RESIST_SLEEP_CHARM_90,             handleSaveBonus5);
    setHandler(E_IMMUNITY_SLEEP_CHARM,              handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_PARALYSIS,                handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_SLEEP_CHARM_PARALYSIS_POISON, handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_GAZE_ATTACKS,             handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_COLD,                     handleImmunityCold);
    setHandler(E_IMMUNITY_FIRE,                     handleImmunityFire);
    setHandler(E_EFREETI_FIRE_RESISTANCE,           handleEfreetiFireResistance);
    setHandler(E_IMMUNITY_PARALYSIS_POISON,         handleImmunityParalysisPoisonFear);
    setHandler(E_IMMUNITY_NONMAGICAL_WEAPONS,       handleImmunityNonmagical);
    setHandler(E_IMMUNITY_NONMAGICAL_HALF_SILVER,   handleImmunityNonmagical);
    setHandler(E_HALF_DAMAGE_ELECTRICITY,           handleHalfDamage);
    setHandler(E_HALF_DAMAGE_PIERCING_SLASHING,     handleHalfDamage);
    setHandler(E_HALF_DAMAGE_MAGICAL_WEAPONS,       handleHalfDamage);
    setHandler(E_HALF_DAMAGE_COLD,                  handleHalfDamage);
    setHandler(E_VULNERABILITY_HOLY_WATER,          handleSavePenalty2);
    setHandler(E_VULNERABILITY_FIRE,                handleSavePenalty2);
    setHandler(E_TROLL_FIRE_OR_ACID,                handleSavePenalty2);
    setHandler(E_EXTRA_STRENGTH_130,                handleExtraStrength);
    // Poolrad raw IDs 64/65/66/70: poison attack effects with varying save modifiers.
    setSpecHandler(0x40,                            handlePoisonAttack);
    setSpecHandler(0x41,                            handlePoisonAttackSaveBonus4);
    setSpecHandler(0x42,                            handlePoisonAttackSaveBonus2);
    setSpecHandler(0x46,                            handlePoisonMeleeSavePenalty2);
    // Poolrad raw IDs 67/68/69: paralysis melee attack effects.
    setSpecHandler(0x43,                            handleParalysisMeleeAttack);
    setSpecHandler(0x44,                            handleParalysisMeleeAttackNoElves);
    setSpecHandler(0x45,                            handleParalysisMeleeAttackStrong);
    // Poolrad raw ID 71: invisible attacker, target unavailable + attack penalty.
    setSpecHandler(0x47,                            handleInvisibleAttack);
    // Poolrad raw ID 72: chance to camouflage on attack.
    setSpecHandler(0x48,                            handleCamouflagedAttack);
    // Poolrad raw ID 73: rear-claw rake bonus attack.
    setSpecHandler(0x49,                            handleRearClawRake);
    // Poolrad raw ID 74: grapple attack against a roster-indexed target.
    setSpecHandler(0x4a,                            handleGrappleAttack);
    // Poolrad raw ID 75: vampiric blood-drain attack.
    setSpecHandler(0x4b,                            handleSucksBlood);
    // Poolrad raw ID 76: blood-drain attack trigger; sets up effects 75/58.
    setSpecHandler(0x4c,                            handleBloodDrainingAttack);
    // Poolrad raw ID 77: bite-and-hold attack; sets up effects 74/58.
    setSpecHandler(0x4d,                            handleBiteAndHold);
    // Poolrad raw ID 78: revive — re-place on combat map, retry via effect.power on failure.
    setSpecHandler(0x4e,                            handleRevive);
    // Poolrad raw ID 79: fire-touch attack — 2d10 fire+magic damage, no saving throw.
    setSpecHandler(0x4f,                            handleFireTouchAttack);
    // Poolrad raw ID 80: ankheg acid melee attack — 1d4 acid damage, no saving throw.
    setSpecHandler(0x50,                            handleAnkhegAcidMeleeAttack);
    // Poolrad raw ID 81: dragon fear aura — fear effect on all opposite-side party members.
    setSpecHandler(0x51,                            handleDragonFearAura);
    // Poolrad raw ID 82: mummy fear aura — strips prior aura, then paralysis-fear on opposite side.
    setSpecHandler(0x52,                            handleMummyFearAura);
    // Poolrad raw ID 83: petrifying gaze — save vs. petrification or stoned; reflective item check.
    setSpecHandler(0x53,                            handlePetrifyingGaze);
    // Poolrad raw ID 84: charming gaze — save vs. spell or charmed; LOS + engage guards (TODO).
    setSpecHandler(0x54,                            handleCharmingGaze);
}

Goldbox::Data::Effects::Effects EffectHandler::mapRawEffectId(uint8 rawId) const {
    using namespace Goldbox::Data::Effects;

    switch (rawId) {
    case E_POOLRAD_BLESSED:
        return E_BLESSED;
    case E_POOLRAD_CURSED:
        return E_CURSED;
    case E_POOLRAD_PROTECTION_FROM_EVIL:
        return E_PROTECTION_FROM_EVIL;
    case E_POOLRAD_PROTECTION_FROM_GOOD:
        return E_PROTECTION_FROM_GOOD;
    case E_POOLRAD_RESIST_COLD:
        return E_RESIST_COLD;
    case E_POOLRAD_CHARM_PERSON:
        return E_CHARM_PERSON;
    default:
        return static_cast<Effects>(rawId);
    }
}

void EffectHandler::handleNoop(const Goldbox::Data::Effects::EffectCall &) {
}

} // namespace Poolrad
} // namespace Goldbox
