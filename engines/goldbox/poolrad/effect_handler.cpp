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
#include "goldbox/core/tile_pos.h"
#include "goldbox/spells/spell_aoe.h"
#include "goldbox/combat/cloud_effect_manager.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_common_handler.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/data/rules/saving_throw.h"
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
    if (!c.character.combatState)
        return;
    const uint8 count = c.character.combatState->attackCount;
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

    const uint8 count = c.character.combatState ? c.character.combatState->attackCount : 0;
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
    if (c.combat)
        c.combat->effectSet18.value = 0;
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

// Thin adapter over the shared Rules::checkSavingThrow (mirrors the
// original UTIL_CheckSavingThrow), bound to this handler's EffectCall.
static bool checkSavingThrow(const EffectCall &c,
        Goldbox::Data::Spells::SaveVerseType saveType, int8 saveModifier) {
    Goldbox::Data::ADnDCharacter &adnd =
        static_cast<Goldbox::Data::ADnDCharacter &>(c.character);
    return Goldbox::Data::Rules::checkSavingThrow(adnd, c.combat, c.handler,
            c.bridge, saveType, saveModifier);
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

    if (checkSavingThrow(c, Goldbox::Data::Spells::SVS_POISON, saveModifier))
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
    if (checkSavingThrow(c, Goldbox::Data::Spells::SVS_POISON, 0))
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
            col = rec.pos.col;
            row = rec.pos.row;
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
    ctx->table.setPosition(idx, TilePos(col, row));

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

    const bool saved = checkSavingThrow(tc, Goldbox::Data::Spells::SVS_SPELL, -2);

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

    if (!checkSavingThrow(tc, Goldbox::Data::Spells::SVS_PETRIFICATION, 0)) {
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

        const bool saved = checkSavingThrow(tc, Goldbox::Data::Spells::SVS_POISON, saveBonus);

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

        const bool saved = checkSavingThrow(tc, Goldbox::Data::Spells::SVS_SPELL, 0);

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

static uint8 selectLevelDrainClass(const Data::PoolradCharacter &ch, int32 &outXp) {
    int32 bestXp = 0;
    uint8 bestClass = 0;
    uint8 bestLevel = 0;

    for (uint8 classId = 0; classId < BASE_CLASS_NUM; ++classId) {
        uint8 classLevel = ch.levels[static_cast<Goldbox::Data::ClassADnD>(classId)];
        if (classLevel < 2)
            continue;

        int32 xp = Goldbox::Data::Rules::xpForClassAtLevel(classId, classLevel);

        if (bestLevel <= classLevel) {
            if (bestXp <= xp || xp == -1) {
                bestXp = xp;
                bestLevel = classLevel;
                bestClass = classId;
            }
        }
    }

    outXp = bestXp;
    return bestClass;
}

static void applyLevelDrain(Data::PoolradCharacter &ch, uint8 levelCount,
        EffectHostBridge *bridge) {
    if (bridge)
        bridge->postEffectMessage(&ch, "loses a level", true);

    if (ch.healthStatus == Goldbox::Data::S_ANIMATED) {
        Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, bridge);
        effectSystem.setStatus(ch, Goldbox::Data::S_GONE, "is dead forever");
        ch.hitPoints.max = 0;
        return;
    }

    for (uint8 drain = 0; drain < levelCount; ++drain) {
        if (ch.healthStatus == Goldbox::Data::S_GONE)
            break;

        ch.drainedLevels++;

        uint8 totalLevels = 0;
        for (uint8 i = 0; i < BASE_CLASS_NUM; ++i)
            totalLevels += ch.levels[static_cast<Goldbox::Data::ClassADnD>(i)];
        if (totalLevels == 0)
            totalLevels = 1;

        uint8 hpLoss = ch.hitPoints.max / totalLevels;

        for (uint8 i = 0; i < hpLoss; ++i) {
            if (ch.hitPoints.max > 0)      ch.hitPoints.max--;
            if (ch.hitPointsRolled > 0)    ch.hitPointsRolled--;
            if (ch.hitPoints.current > 0)  ch.hitPoints.current--;
            ch.drainedHPs++;
        }

        int32 newXp = 0;
        uint8 classId = selectLevelDrainClass(ch, newXp);

        if (ch.highestLevel < 2) {
            Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, bridge);
            effectSystem.setStatus(ch, Goldbox::Data::S_GONE, "is dead forever");
            ch.hitPoints.max = 0;

            if (ch.levels[static_cast<Goldbox::Data::ClassADnD>(classId)] > 0 &&
                    levelCount == 2) {
                int32 dummy = 0;
                classId = selectLevelDrainClass(ch, dummy);
                ch.levels[static_cast<Goldbox::Data::ClassADnD>(classId)] = 0;
            }
        } else {
            ch.levels[static_cast<Goldbox::Data::ClassADnD>(classId)]--;

            if (ch.hitPoints.current == 0 &&
                    ch.healthStatus != Goldbox::Data::S_DEAD) {
                Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, bridge);
                effectSystem.setStatus(ch, Goldbox::Data::S_DEAD, "is killed");
            }

            ch.recalcCombatStats();
        }

        int32 updatedXp = 0;
        classId = selectLevelDrainClass(ch, updatedXp);
        ch.highestLevel = ch.levels[static_cast<Goldbox::Data::ClassADnD>(classId)];
        ch.experiencePoints = static_cast<uint32>(updatedXp > 0 ? updatedXp : 0);

        ch.recalcCombatStats();
    }
}

// Mirrors EFFECT_89_CombatTurnTrigger: gates the electricity breath attack
// on the combat turn counter. When both attackRollCount and attackRoll are
// zero the trigger flag is cleared (effect ready to fire again next turn).
// Otherwise, if the trigger has not yet fired this turn, sets attackRoll to
// 0xFF (attack blocked/redirected) and marks the trigger as fired.
static void handleConstitutionSavingBonus(const EffectCall &c) {
    if (!c.combat)
        return;
    const uint8 con = c.character.abilities.constitution.current;
    uint8 bonus = 0;
    if      (con >= 18) bonus = 5;
    else if (con >= 14) bonus = 4;
    else if (con >= 11) bonus = 3;
    else if (con >=  7) bonus = 2;
    else if (con >=  4) bonus = 1;
    c.combat->savingThrow += bonus;
}

static void handleHalflingPoisonBonus(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->activeSpellId == 0x0F) {
        rollAvoid(c.character, *c.combat, c.bridge, 100);
        if (c.bridge)
            c.bridge->postEffectMessage(&c.character, "is unaffected", true);
    } else if (c.combat->behaviorFlags & Combat::CombatGlobals::DMG_ELECTRICITY) {
        rollAvoid(c.character, *c.combat, c.bridge, 100);
    }
}

static void handleThriKreenMissileEvasion60(const EffectCall &c) {
    if (!c.combat)
        return;
    rollAvoid(c.character, *c.combat, c.bridge, 60);
}

static void applyMagicResistance(const EffectCall &c, uint8 baseChance) {
    if (!c.combat)
        return;
    if (c.combat->activeSpellId == 0 &&
            !(c.combat->behaviorFlags & Combat::CombatGlobals::DMG_MAGIC))
        return;
    const Data::PoolradCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Data::PoolradCharacter *>(c.combat->attacker) : nullptr;
    const uint8 castingLevel = attacker ? attacker->highestLevel : 0;
    const int resistanceChance = baseChance + (castingLevel - 11) * 5;
    const int roll = Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 100) : 1;
    if (roll <= resistanceChance)
        c.combat->damage = 0;
}

static void handleResistMagic50(const EffectCall &c) {
    applyMagicResistance(c, 50);
}

static void handleResistMagic15(const EffectCall &c) {
    applyMagicResistance(c, 15);
}

static void protectionIf(const EffectCall &c, uint8 spellId) {
    if (!c.combat)
        return;
    if (spellId == 0 || c.combat->activeSpellId == spellId)
        c.combat->damage = 0;
}

static void handleResistSleepCharm90(const EffectCall &c) {
    if (!c.combat)
        return;
    const int roll = Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 100) : 1;
    if (roll < 91) {
        protectionIf(c, 53);
        protectionIf(c, 11);
    }
}

static void handleImmunitySleepCharmSpell(const EffectCall &c) {
    protectionIf(c, 11);
    protectionIf(c, 53);
}

static void handleImmunityParalysisSpell(const EffectCall &c) {
    protectionIf(c, 52);
}

static void handleImmunityColdSpell(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & Combat::CombatGlobals::DMG_COLD))
        c.combat->damage = 0;
}

static void handleImmunityParalysisPoisonSpell(const EffectCall &c) {
    protectionIf(c, 55);
    protectionIf(c, 52);
    if (c.combat && c.combat->savingThrowType == Goldbox::Data::Spells::SVS_POISON)
        c.combat->savingThrow = 100;
}

static void handleImmunityFireSpell(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & Combat::CombatGlobals::DMG_FIRE))
        c.combat->damage = 0;
}

static void handleEfreetiFireResistanceSpell(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & Combat::CombatGlobals::DMG_FIRE))
        return;
    const uint8 count = c.character.combatState ? c.character.combatState->attackCount : 0;
    uint8 dmg = c.combat->damage;
    for (uint8 i = 0; i < count; ++i) {
        dmg = (dmg < 1) ? 0 : dmg - 1;
        if (dmg < count)
            dmg = count;
    }
    c.combat->damage = dmg;
}

static void handleHalfDamageFromElectricity(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & Combat::CombatGlobals::DMG_ELECTRICITY))
        c.combat->damage >>= 1;
}

static void handleHalfDamageFromPiercingSlashing(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        const_cast<Goldbox::Data::ADnDCharacter *>(attacker)->getWeaponOrAmmo() : nullptr;
    if (!weapon)
        return;
    const uint8 wpnType = weapon->prop().wpnType;
    if (wpnType == 0 || (wpnType & 0x01))
        c.combat->damage >>= 1;
}

static void handleHalfDamageFromMagicalWeapons(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        const_cast<Goldbox::Data::ADnDCharacter *>(attacker)->getWeaponOrAmmo() : nullptr;
    if (weapon && weapon->bonus > 0)
        c.combat->damage >>= 1;
}

static void handleVulnerabilityToHolyWater(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        attacker->getEquippedItem(Goldbox::Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon && weapon->typeIndex == 85) {
        c.combat->damage = static_cast<uint8>(
            (Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 6) : 1) + 1);
    }
}

static void handleHalfDamageFromCold(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & Combat::CombatGlobals::DMG_COLD))
        c.combat->damage >>= 1;
}

static void handleImmunityNonMagicalWeaponsSpell(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        const_cast<Goldbox::Data::ADnDCharacter *>(attacker)->getWeaponOrAmmo() : nullptr;
    if ((weapon == nullptr || weapon->bonus == 0) &&
            (c.combat->attacker->race != 0 ||
             static_cast<const Data::PoolradCharacter *>(c.combat->attacker)->highestLevel < 4))
        c.combat->damage = 0;
}

static void handleBoulderEvasion(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        attacker->getEquippedItem(Goldbox::Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon && (weapon->typeIndex == 87 || weapon->typeIndex == 88))
        rollAvoid(c.character, *c.combat, c.bridge, 50);
}

static void handleAnkhegAcidSquirtAttack(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.character.combatState)
        return;

    Goldbox::Data::PlayerCharacter *target = c.character.combatState->target;
    if (!target || !target->enabled)
        return;

    // 25% chance to perform the acid squirt.
    const int roll = Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 100) : 100;
    if (roll > 25)
        return;

    // TODO: COMBAT_FindTargetFacing(char_ptr, target) < 4 — facing arc check not yet ported.
    // TODO: COMBAT_resetActionState(char_ptr)
    // TODO: GFX_LoadEffectTileQuad(23); COMBAT_AnimateMissilePath(...)

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "Spits Acid", true);

    const uint8 damage = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(4, 8)) : 4;

    // Build a target-side EffectCall for checkSavingThrow (type 3 = vs. breath weapon).
    Effect dummy = c.effect;
    EffectCall tc(EFF_ADD, dummy, *target, c.combat, c.bridge, c.damage, c.handler);
    const bool saved = checkSavingThrow(tc, Goldbox::Data::Spells::SVS_BREATH, 0);

    if (c.damage)
        c.damage->applyLegacy(*target, damage, Goldbox::Data::DAMAGE_HALF, saved);

    // Consume this effect (121) and the related acid-melee setup effect (raw 0x50).
    CharacterEffects *fx = c.character.getEffects();
    if (fx) {
        c.effect.immediate = 0;
        fx->eraseEffectById(c.effect.id);
        fx->eraseEffectById(0x50);
    }
}

static void handleVulnerabilityToFire(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        attacker->getEquippedItem(Goldbox::Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon && weapon->typeIndex == 86)
        c.combat->damage = static_cast<uint8>(
            Goldbox::g_engine ? Goldbox::g_engine->rollDice(3, 8) : 3);
    if (c.combat->behaviorFlags & (Combat::CombatGlobals::DMG_FIRE |
                                   Combat::CombatGlobals::DMG_MAGIC))
        c.combat->damage += c.character.combatState ? c.character.combatState->attackCount : 0;
}

static void handleImmunityNonMagicalHalfSilver(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        const_cast<Goldbox::Data::ADnDCharacter *>(attacker)->getWeaponOrAmmo() : nullptr;
    if (weapon == nullptr) {
        c.combat->damage = 0;
    } else if (weapon->bonus == 0 && weapon->nameCode3 == 177) {
        c.combat->damage >>= 1;
    } else if (weapon->bonus == 0) {
        c.combat->damage = 0;
    }
}

static void handleResistSleepCharm30(const EffectCall &c) {
    if (!c.combat)
        return;
    const int roll = Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 100) : 100;
    if (roll < 31) {
        protectionIf(c, 11);
        protectionIf(c, 53);
    }
}

static void handleImmunitySleepCharmParalysisPoison(const EffectCall &c) {
    protectionIf(c, 11);
    protectionIf(c, 53);
    protectionIf(c, 52);
    protectionIf(c, 55);
    if (c.combat && c.combat->savingThrowType == Goldbox::Data::Spells::SVS_POISON)
        c.combat->savingThrow = 100;
}

static void handleImmuneToGazeAttacks(const EffectCall &c) {
    if (!c.combat || !c.character.combatState)
        return;

    Goldbox::Data::PlayerCharacter *target = c.character.combatState->target;
    if (!target)
        return;

    const Goldbox::Data::ADnDCharacter *adndTarget =
        static_cast<const Goldbox::Data::ADnDCharacter *>(target);

    for (const Goldbox::Data::Items::CharacterItem &item :
            adndTarget->inventory.items()) {
        if (!item.readied)
            continue;
        if ((item.nameCode1 == 0x98 && item.nameCode3 == 0xFC) ||
                (item.nameCode2 == 0x98 && item.nameCode3 == 0xFC) ||
                (item.nameCode3 == 0x98) ||
                item.nameCode1 == 0x76 ||
                item.nameCode2 == 0x76 ||
                item.nameCode3 == 0x76) {
            c.combat->targetUnavailable = true;
            return;
        }
    }
}

// Mirrors EFFECT_128_ItemEffects and its aliases (effects 128-130, 133, 134, 136, 138, 139).
// The item's effect2 field is stored in c.effect.power by the item-ready dispatch path.
// On EFF_ADD: add the sub-effect (power=12, duration=0) then fire its EFF_ADD handler.
// On EFF_REMOVE: erase the sub-effect from the character's effect list.
static void handleItemEffect(const EffectCall &c) {
    const uint8 subEffectId = c.effect.power;
    if (subEffectId == 0)
        return;

    CharacterEffects *fx = c.character.getEffects();
    if (!fx)
        return;

    if (c.op == EFF_ADD) {
        c.character.addEffect(subEffectId, 0, 12, false);
        if (c.handler) {
            Effect sub;
            sub.id = subEffectId;
            sub.durationMin = 0;
            sub.power = 12;
            sub.immediate = 0;
            c.handler->apply(EFF_ADD, sub, c.character, c.combat, c.bridge);
        }
    } else if (c.op == EFF_REMOVE) {
        fx->eraseEffectById(subEffectId);
    }
}

static void handleExtraStrengthItem(const EffectCall &c) {
    if (c.op == EFF_ADD) {
        uint8 enc = 0;
        const bool applied = c.character.applyStrengthChange(18, 100, enc);
        if (applied) {
            if (c.bridge)
                c.bridge->postEffectMessage(&c.character, "is stronger", true);
            Data::PoolradCharacter &ch = asPoolrad(c.character);
            ch.recalcCombatStats();
        }
        c.character.addEffect(E_STRENGTH, 0, enc, true);
    } else if (c.op == EFF_REMOVE) {
        CharacterEffects *fx = c.character.getEffects();
        if (!fx)
            return;
        const bool atExtra = (c.character.abilities.strength.current == 18 &&
                              c.character.abilities.strException.current == 100);
        for (Effect &e : fx->effects()) {
            if (e.id != static_cast<uint8>(E_STRENGTH))
                continue;
            uint8 str = 0, ext = 0;
            strengthDecode(e.power & 0x7f, str, ext);
            if ((atExtra && e.power < 0x80) ||
                    (!atExtra && str == 18 && ext == 100)) {
                fx->eraseEffectById(e.id);
                return;
            }
        }
    }
}

static void handleRequiresGiantStrength(const EffectCall &c) {
    if (c.op != EFF_ADD || c.character.abilities.strength.current >= 19)
        return;
    Goldbox::Data::ADnDCharacter &adnd =
        static_cast<Goldbox::Data::ADnDCharacter &>(c.character);
    for (Goldbox::Data::Items::CharacterItem &item : adnd.inventory.items()) {
        if (item.readied && item.effect3 == 0x87) {
            item.readied = 0;
            if (c.bridge)
                c.bridge->postEffectMessage(&c.character, "Must have Giant Strength", true);
            return;
        }
    }
}

static void handleRemoveEffect23OnRemove(const EffectCall &c) {
    if (c.op == EFF_REMOVE) {
        CharacterEffects *fx = c.character.getEffects();
        if (fx)
            fx->eraseEffectById(23);
    }
}

static void handleLingeringBreath(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    // 50% chance to skip unless the turn counter is zero (always fires then).
    if (c.combat && c.combat->turnCounter != 0) {
        const int roll = Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 100) : 51;
        if (roll <= 50)
            return;
    }

    (void)(Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr); // ctx reserved for GFX/spell path


    // TODO: COMBAT_GetCharacterX/Y — position lookup not yet used by unported GFX/spell path.
    // uint8 ax = ctx ? ctx->table.getCharacterCol(&c.character) : 0;
    // uint8 ay = ctx ? ctx->table.getCharacterRow(&c.character) : 0;

    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "Breathes!", true);

    Combat::CombatContext *ctx = Goldbox::g_engine ?
        Goldbox::g_engine->getCombatContext() : nullptr;

    if (ctx) {
        const TilePos attackerPos(
            ctx->table.getCharacterCol(&c.character),
            ctx->table.getCharacterRow(&c.character));

        // Target position is the current combat target stored in globals.
        // TODO(spell_aoe): expose targetX/Y on CombatGlobals or CombatContext
        // and replace the placeholder zeros once that field is added.
        const TilePos targetPos(0, 0); // TODO: ctx->globals.targetX/Y

        const uint8 damage = c.character.hitPoints.max;

        // TODO(spell_aoe): pass presenter (ICombatSpellPresenter*) once
        // CombatView implements the interface and is reachable here.

        // Initial hit at the target tile (mirrors SPELL_ResolveAoEHitAtTile
        // called before SPELL_TraceSpellPath in the original).
        bool hitObstacle = false;
        Goldbox::Spells::resolveAoEHitAtTile(*ctx, targetPos, damage, 3, 19,
                                             nullptr, hitObstacle);

        // Continue the breath along its path.
        Goldbox::Spells::traceSpellPath(*ctx, attackerPos, targetPos,
                                        10,      // initialAnimFrame
                                        3,       // savingThrowMod
                                        damage,
                                        3,       // pathLength
                                        19,      // effectTileId
                                        nullptr);
    }

    // Remove the poison status that the breath attack inflicts on the attacker
    // (mirrors UTIL_RemovePoison called after SPELL_TraceSpellPath).
    CharacterEffects *fx = c.character.getEffects();
    if (fx) {
        fx->eraseEffectById(static_cast<uint8>(E_POISONED));
        asPoolrad(c.character).effectState.flags &= ~Data::PoolradCharacter::EF_POISONED;
    }

    // power >= 0xFE: persistent state — decrement and keep the effect.
    // Otherwise: consume the effect.
    if (c.effect.power >= 0xfe) {
        --c.effect.power;
    } else {
        c.effect.immediate = 0;
        if (fx)
            fx->eraseEffectById(c.effect.id);
    }

    c.character.resetCombatAction();
}

static void handleTrollFireAcidVulnerability(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->behaviorFlags & (Combat::CombatGlobals::DMG_FIRE |
                                   Combat::CombatGlobals::DMG_ACID))
        return;
    const uint8 duration = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(3, 6)) : 3;
    c.character.addEffect(E_POOLRAD_RETURN_FROM_DEATH, duration, 0xff, true);
}

static void handleApplyDeathRecovery(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    CharacterEffects *fx = c.character.getEffects();
    if (!fx)
        return;
    if (fx->hasEffect(static_cast<uint8>(E_POOLRAD_REGEN_3_HP_ROUND)))
        return;
    if (!fx->hasEffect(static_cast<uint8>(E_REGENERATE_3_HPS)))
        c.character.addEffect(static_cast<uint8>(E_REGENERATE_3_HPS), 3, 0xff, true);
}

static void handleReturnFromDeath(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    // UTIL_Revive heals hp_max and restores status. It fails (returns false)
    // when the character cannot receive healing (S_DEAD, S_GONE, S_STONED, etc.).
    const uint8 status = c.character.healthStatus;
    const bool canRevive = (status == Goldbox::Data::S_OKAY ||
                            status == Goldbox::Data::S_ANIMATED ||
                            status == Goldbox::Data::S_UNCONSCIOUS ||
                            status == Goldbox::Data::S_DYING);
    if (!canRevive) {
        tryAddEffect(c.character, c.effect.id, c.effect.power, 1);
        return;
    }
    c.character.hitPoints.current = c.character.hitPoints.max;
    c.character.healthStatus = Goldbox::Data::S_OKAY;
    c.character.enabled = true;
}

static void handleGaseousEscape(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    c.effect.immediate = 0;
    // TODO: COMBAT_FocusCharacter(char_ptr, 3, 0) — viewport focus not yet ported.
    Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, c.bridge);
    effectSystem.setStatus(c.character, Goldbox::Data::S_RUNNING,
        "Turns gaseous and escapes");
}

static void handleRegen3HpRound(const EffectCall &c) {
    if (c.op != EFF_TICK)
        return;
    if (c.character.healHp(3, true) && c.bridge)
        c.bridge->showHealResult(&c.character);
}

static void handleKeepFightingAfterUnconscious(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    uint8 reviveAmount = 0;
    if (c.character.healthStatus == Goldbox::Data::S_DYING &&
            c.character.combatState &&
            c.character.combatState->bleeding < 6)
        reviveAmount = 6 - c.character.combatState->bleeding;
    else if (c.character.healthStatus == Goldbox::Data::S_UNCONSCIOUS)
        reviveAmount = 6;

    if (reviveAmount == 0)
        return;

    // Revive: restore HP, status, and enabled flag.
    c.character.hitPoints.current =
        MIN<uint8>(c.character.hitPoints.max,
                   c.character.hitPoints.current + reviveAmount);
    c.character.healthStatus = Goldbox::Data::S_OKAY;
    c.character.enabled = true;

    const uint8 duration = static_cast<uint8>(
        (Goldbox::g_engine ? Goldbox::g_engine->rollDice(1, 4) : 1) + 1);
    c.character.addEffect(E_POOLRAD_FIGHT_ON_AT_ZERO_HP, duration, 0xff, true);

    c.effect.immediate = 0;
    CharacterEffects *fx = c.character.getEffects();
    if (fx)
        fx->eraseEffectById(c.effect.id);
}

static void handleHalfDamageFromFire(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & Combat::CombatGlobals::DMG_FIRE))
        return;
    c.combat->damage >>= 1;
}

static void handleHalfDamageBluntPiercing(const EffectCall &c) {
    if (!c.combat || !c.combat->attacker)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker);
    const Goldbox::Data::Items::CharacterItem *weapon =
        const_cast<Goldbox::Data::ADnDCharacter *>(attacker)->getWeaponOrAmmo();
    if (weapon == nullptr)
        return;
    const Goldbox::Data::Items::ItemProperty &prop = weapon->prop();
    if (prop.wpnType & 0x81)
        c.combat->damage >>= 1;
}

static void handleImmunityNonSilverNonMagical(const EffectCall &c) {
    if (!c.combat)
        return;
    const Goldbox::Data::ADnDCharacter *attacker =
        c.combat->attacker ?
        static_cast<const Goldbox::Data::ADnDCharacter *>(c.combat->attacker) : nullptr;
    const Goldbox::Data::Items::CharacterItem *weapon = attacker ?
        const_cast<Goldbox::Data::ADnDCharacter *>(attacker)->getWeaponOrAmmo() : nullptr;
    if (weapon == nullptr ||
            (weapon->bonus == 0 && weapon->nameCode3 != 0xB1))
        c.combat->damage = 0;
}

static void handleFightOnAtZeroHp(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    c.effect.immediate = 0;
    if (!c.character.enabled)
        return;
    Goldbox::Data::Effects::EffectSystem effectSystem(nullptr, c.bridge);
    effectSystem.setStatus(c.character, Goldbox::Data::S_DEAD, "Falls dead");
}

static void handleDwarfSaveBonus(const EffectCall &c) {
    if (!c.combat)
        return;
    const uint8 type = static_cast<uint8>(c.combat->savingThrowType);
    if (type != static_cast<uint8>(Goldbox::Data::Spells::SVS_ROD_STAFF_WAND) &&
            type != static_cast<uint8>(Goldbox::Data::Spells::SVS_SPELL))
        return;
    const uint8 con = c.character.abilities.constitution.current;
    uint8 bonus = 0;
    if      (con >= 18) bonus = 5;
    else if (con >= 14) bonus = 4;
    else if (con >= 11) bonus = 3;
    else if (con >=  7) bonus = 2;
    else if (con >=  4) bonus = 1;
    c.combat->savingThrow += bonus;
}

static void handleCombatTurnTrigger(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->turnCounter == 0 && c.combat->attackRoll == 0) {
        c.effect.power &= 0x0F;
    } else if (!(c.effect.power & 0x10)) {
        c.combat->attackRoll = 0xFF;
        c.effect.power |= 0x10;
    }
}

static void handleDiseaseMeleeAttack(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;

    Goldbox::Data::PlayerCharacter *target =
        c.character.combatState ? c.character.combatState->target : nullptr;
    if (!target || !target->enabled)
        return;

    CharacterEffects *fx = target->getEffects();
    if (fx && fx->hasEffect(E_POOLRAD_DISEASED))
        return;

    target->addEffect(E_POOLRAD_DISEASED, 14400, 0xff, true);

    const uint8 roll = Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(1, 6)) : 1;
    target->addEffect(E_POOLRAD_ROT, 43200, roll * 16 + 15, true);

    if (c.bridge)
        c.bridge->postEffectMessage(target, "is Diseased", true);
}

static void handleDrain1Level(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    Data::PoolradCharacter *target = c.character.combatState
        ? static_cast<Data::PoolradCharacter *>(c.character.combatState->target)
        : nullptr;
    if (target)
        applyLevelDrain(*target, 1, c.bridge);
}

static void handleDrain2Levels(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    Data::PoolradCharacter *target = c.character.combatState
        ? static_cast<Data::PoolradCharacter *>(c.character.combatState->target)
        : nullptr;
    if (target)
        applyLevelDrain(*target, 2, c.bridge);
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
    setHandler(E_AFFECT_4A,                         handleAttackBonus2);
    setHandler(E_AFFECT_4E,                         handleAttackBonus2);
    setHandler(E_FIRE_ATTACK_2D10,                  handleAttackDamageBonus);
    setHandler(E_ANKHEG_ACID_ATTACK,                handleAttackDamageBonus);
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
    // Poolrad raw ID 85: drain 1 level from combat target.
    setSpecHandler(0x55,                            handleDrain1Level);
    // Poolrad raw ID 86: drain 2 levels from combat target.
    setSpecHandler(0x56,                            handleDrain2Levels);
    // Poolrad raw ID 87: disease melee attack — applies disease + disease damage to target.
    setSpecHandler(0x57,                            handleDiseaseMeleeAttack);
    // Poolrad raw ID 88: lingering breath - fires breath attack each turn; power>=0xFE persists.
    setSpecHandler(0x58,                            handleLingeringBreath);
    // Poolrad raw ID 89: combat turn trigger — gates electricity breath on turn counter.
    setSpecHandler(0x59,                            handleCombatTurnTrigger);
    // Poolrad raw ID 90: constitution saving throw bonus (racial).
    setSpecHandler(E_POOLRAD_CON_SAVING_BONUS,      handleConstitutionSavingBonus);
    // Poolrad raw ID 91: halfling poison/electricity immunity (racial).
    setSpecHandler(E_POOLRAD_HALFLING_POISON_BONUS, handleHalflingPoisonBonus);
    // Poolrad raw ID 93: half damage from fire attacks.
    setSpecHandler(E_POOLRAD_HALF_DAMAGE_FROM_FIRE,      handleHalfDamageFromFire);
    // Poolrad raw ID 94: half damage from blunt/piercing weapons (wpnType & 0x81).
    setSpecHandler(E_POOLRAD_HALF_DAMAGE_BLUNT_PIERCING, handleHalfDamageBluntPiercing);
    // Poolrad raw ID 95: fight on after being reduced to 0 HP; clears immediate, then kills if enabled.
    setSpecHandler(E_POOLRAD_FIGHT_ON_AT_ZERO_HP,        handleFightOnAtZeroHp);
    // Poolrad raw ID 96: immunity to non-silver, non-magical weapons.
    setSpecHandler(E_POOLRAD_IMMUNITY_NON_SILVER_NON_MAGICAL, handleImmunityNonSilverNonMagical);
    // Poolrad raw ID 97: dwarf constitution saving throw bonus (vs. rod/staff/wand and vs. spell).
    setSpecHandler(E_POOLRAD_DWARF_SAVE_BONUS,           handleDwarfSaveBonus);
    // Poolrad raw ID 98: regenerate 3 HP per round.
    setSpecHandler(E_POOLRAD_REGEN_3_HP_ROUND,           handleRegen3HpRound);
    // Poolrad raw ID 99: keep fighting after becoming unconscious/dying; revives and adds fight-on effect.
    setSpecHandler(E_POOLRAD_KEEP_FIGHTING_AFTER_UNCONSCIOUS, handleKeepFightingAfterUnconscious);
    // Poolrad raw ID 100: troll fire/acid vulnerability — adds return-from-death effect if not fire/acid hit.
    setSpecHandler(E_POOLRAD_TROLL_FIRE_ACID_VULNERABILITY,  handleTrollFireAcidVulnerability);
    // Poolrad raw ID 101: apply death recovery — bootstraps E_REGENERATE_3_HPS if not already regenerating.
    setSpecHandler(E_POOLRAD_APPLY_DEATH_RECOVERY,           handleApplyDeathRecovery);
    // Poolrad raw ID 102: return from death — revive with full HP, retry next tick on failure.
    setSpecHandler(E_POOLRAD_RETURN_FROM_DEATH,              handleReturnFromDeath);
    // Poolrad raw ID 103: running — clears immediate, sets S_RUNNING status (gaseous escape).
    setSpecHandler(E_POOLRAD_GASEOUS_ESCAPE,                        handleGaseousEscape);
    // Poolrad raw ID 104: thri-kreen missile evasion at 60% — reuses rollAvoid helper.
    setSpecHandler(E_POOLRAD_THRI_KREEN_MISSILE_EVASION_60,         handleThriKreenMissileEvasion60);
    // Poolrad raw ID 105: resist magic 50% — base 50% chance scaled by caster level.
    setSpecHandler(0x69,                                             handleResistMagic50);
    // Poolrad raw ID 106: resist magic 15% — base 15% chance scaled by caster level.
    setSpecHandler(0x6a,                                             handleResistMagic15);
    // Poolrad raw ID 107: 90% resistance to sleep/charm spells.
    setSpecHandler(0x6b,                                             handleResistSleepCharm90);
    // Poolrad raw ID 108: full immunity to sleep and charm spells.
    setSpecHandler(0x6c,                                             handleImmunitySleepCharmSpell);
    // Poolrad raw ID 109: full immunity to paralysis spells.
    setSpecHandler(0x6d,                                             handleImmunityParalysisSpell);
    // Poolrad raw ID 110: full immunity to cold damage.
    setSpecHandler(0x6e,                                             handleImmunityColdSpell);
    // Poolrad raw ID 111: immunity to paralysis and poison saving throws.
    setSpecHandler(0x6f,                                             handleImmunityParalysisPoisonSpell);
    // Poolrad raw ID 112: full immunity to fire damage.
    setSpecHandler(0x70,                                             handleImmunityFireSpell);
    // Poolrad raw ID 113: Efreeti fire resistance — reduce damage by 1 per attack count.
    setSpecHandler(0x71,                                             handleEfreetiFireResistanceSpell);
    // Poolrad raw ID 114: half damage from electricity.
    setSpecHandler(0x72,                                             handleHalfDamageFromElectricity);
    // Poolrad raw ID 115: half damage from piercing/slashing weapons (wpnType==0 or bit 0 set).
    setSpecHandler(0x73,                                             handleHalfDamageFromPiercingSlashing);
    // Poolrad raw ID 116: half damage from magical weapons (bonus > 0).
    setSpecHandler(0x74,                                             handleHalfDamageFromMagicalWeapons);
    // Poolrad raw ID 117: vulnerability to holy water (typeIndex==85) — replace damage with 1d6+1.
    setSpecHandler(0x75,                                             handleVulnerabilityToHolyWater);
    // Poolrad raw ID 118: half damage from cold.
    setSpecHandler(0x76,                                             handleHalfDamageFromCold);
    // Poolrad raw ID 119: immunity to non-magical weapons; bypassed by non-human or level >= 4 attacker.
    setSpecHandler(0x77,                                             handleImmunityNonMagicalWeaponsSpell);
    // Poolrad raw ID 120: boulder evasion — 50% avoid chance when attacker wields typeIndex 87 or 88.
    setSpecHandler(0x78,                                             handleBoulderEvasion);
    // Poolrad raw ID 121: ankheg acid squirt — 25% chance, 4d8 acid, save vs. breath for half, self-removes.
    setSpecHandler(0x79,                                             handleAnkhegAcidSquirtAttack);
    // Poolrad raw ID 122: vulnerability to fire — typeIndex 86 replaces damage with 3d8; DMG_FIRE|DMG_MAGIC adds attackCount.
    setSpecHandler(0x7a,                                             handleVulnerabilityToFire);
    // Poolrad raw ID 123: immunity to non-magical weapons; silver weapons deal half damage.
    setSpecHandler(0x7b,                                             handleImmunityNonMagicalHalfSilver);
    // Poolrad raw ID 124: 30% resistance to sleep/charm spells.
    setSpecHandler(0x7c,                                             handleResistSleepCharm30);
    // Poolrad raw ID 125: immunity to sleep, charm, paralysis, and poison; auto-pass vs. paralysis saves.
    setSpecHandler(0x7d,                                             handleImmunitySleepCharmParalysisPoison);
    // Poolrad raw ID 126: gaze immunity — set targetUnavailable if target carries a mirror/anti-gaze item.
    setSpecHandler(0x7e,                                             handleImmuneToGazeAttacks);
    // Poolrad raw ID 127: not implemented.
    setSpecHandler(0x7f,                                             handleNotImplemented);
    // Poolrad raw IDs 128-130, 131, 133, 134, 135, 136, 137, 138, 139: item-granted sub-effect (effect2 in power).
    setSpecHandler(0x80,                                             handleItemEffect);
    setSpecHandler(0x81,                                             handleItemEffect);
    setSpecHandler(0x82,                                             handleItemEffect);
    setSpecHandler(0x83,                                             handleExtraStrengthItem);
    setSpecHandler(0x85,                                             handleItemEffect);
    setSpecHandler(0x86,                                             handleItemEffect);
    setSpecHandler(0x87,                                             handleRequiresGiantStrength);
    setSpecHandler(0x88,                                             handleItemEffect);
    setSpecHandler(0x89,                                             handleRemoveEffect23OnRemove);
    setSpecHandler(0x8a,                                             handleItemEffect);
    setSpecHandler(0x8b,                                             handleItemEffect);
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
