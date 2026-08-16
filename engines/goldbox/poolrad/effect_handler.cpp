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
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_common_handler.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"

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

static void handlePoisonDamage(const EffectCall &c) {
    if (c.op == EFF_ADD)
        c.character.effectState.flags |= CEF_POISONED;
    else if (c.op == EFF_REMOVE)
        c.character.effectState.flags &= ~CEF_POISONED;

    // The original EFFECT_add path applies poison damage once when the
    // effect is successfully added, not on every poison-cycle tick.
    if (c.op != EFF_ADD)
        return;

    if (c.character.hitPoints.current <= 1)
        return;

    if (!c.damage)
        return;

    c.damage->apply(c.character,
            Goldbox::Data::DamageRequest(1, false));

    if (!c.combat && c.bridge)
        c.bridge->requestRefresh(
                Goldbox::Data::Effects::EffectHostBridge::RF_CHARACTER_PANEL);
}

static void handleStudyManualBodilyHealth(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "starts to train", true);
    c.character.setEffect(7, 43200, 0xff, true);
}

static void handleTrainingManualBodilyHealth(const EffectCall &c) {
    if (c.op == EFF_TICK) {
        // On the first tick (immediate flag still set), apply the one-time
        // constitution and HP bonus, matching the original which fired the
        // handler body on the first game tick after CHARACTER_setEffect.
        if (c.effect.immediate) {
            c.effect.immediate = 0;

            Data::PoolradCharacter &ch = asPoolrad(c.character);

            if (c.bridge)
                c.bridge->postEffectMessage(&ch, "is hardier", true);

            ch.abilities.constitution.current += 1;

            if (ch.abilities.constitution.current >= 20) {
                ch.setEffect(0x3e, 0x3c, 0xff, true);
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

    setHandler(E_SILENCE_15_RADIUS,             handleSilence);
    setHandler(E_INVISIBILITY,                  handleInvisibility);
    setHandler(E_INVISIBLE,                     handleInvisibility);
    setHandler(E_ITEM_INVISIBILITY,             handleItemInvisibility);
    setHandler(E_CAMOUFLAGE,                    handleCamouflage);
    setHandler(E_IMMUNE_TO_ELECTRICITY,         handleImmuneElec);
    setHandler(E_RESIST_FIRE,                   handleResistFire);
    setHandler(E_RESIST_FIRE_AND_COLD,          handleResistFireAndCold);
    setHandler(E_FIRE_RESIST,                   handleFireResist);
    setHandler(E_PROT_FROM_NORMAL_MISSILES,     handleProtNormalMissiles);
    setHandler(E_PROT_DRAG_BREATH,              handleProtDragBreath);
    setHandler(E_MINOR_GLOBE_OF_INVULNERABILITY, handleMinorGlobe);
    setHandler(E_RAKSHASA_RESIST_NORMAL_WEAPONS, handleRakshasaResist);
    setHandler(E_DISPLACE,                      handleDisplace);
    setHandler(E_HALF_DAMAGE,                   handleHalfDamage);
    setHandler(E_HALF_FIRE_DAMAGE,              handleHalfFireDamage);
    setHandler(E_DAMAGE_REDUCTION,              handleDamageReduction);
    setHandler(E_FEAR_IMMUNITY,                 handleFearImmunity);
    setHandler(E_SLOW_POISON,                   handleSlowPoison);
    setHandler(E_ENTANGLE,                      handleEntangle);
    setHandler(E_PETRIFYING_GAZE,               handleAttackBonus2);
    setHandler(E_BEHOLDER_RAYS_AFFECT_57,       handleAttackBonus2);
    setHandler(E_AFFECT_4A,                     handleAttackBonus2);
    setHandler(E_AFFECT_4E,                     handleAttackBonus2);
    setHandler(E_FIRE_ATTACK_2D10,              handleAttackDamageBonus);
    setHandler(E_ANKHEG_ACID_ATTACK,            handleAttackDamageBonus);
    setHandler(E_GIANT_SLUG_SPIT_ACID,          handleAttackDamageBonus);
    setHandler(E_BREATH_ELEC,                   handleAttackDamageBonus);
    setHandler(E_BREATH_ACID,                   handleAttackDamageBonus);
    setHandler(E_CLOUD_KILL,                    handleAttackDamageBonus);
    setHandler(E_ANKHEG_ACID_SQUIRT_ATTACK,     handleAttackDamageBonus);
    setHandler(E_WILD_BOAR_DIE_AFTER_EXTRA_FIGHT_TIME_AFFECT_5F, handleAttackDamageBonus);
    setHandler(E_OWLBEAR_HUG_CHECK,             handleAttackDamageBonus);
    setHandler(E_WILD_BOAR_AND_BULLETTE_AFFECT_63, handleAttackDamageBonus);
    setHandler(E_THRI_KREEN_MISSILE_EVASION,    handleSaveBonus2);
    setHandler(E_BOULDER_EVASION,               handleSaveBonus2);
    setHandler(E_RESIST_MAGIC_15,               handleSaveBonus1);
    setHandler(E_RESIST_SLEEP_CHARM_30,         handleSaveBonus2);
    setHandler(E_RESIST_MAGIC_50,               handleSaveBonus3);
    setHandler(E_RESIST_SLEEP_CHARM_90,         handleSaveBonus5);
    setHandler(E_IMMUNITY_SLEEP_CHARM,          handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_PARALYSIS,            handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_SLEEP_CHARM_PARALYSIS_POISON, handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_GAZE_ATTACKS,         handleImmunitySleepCharm);
    setHandler(E_IMMUNITY_COLD,                 handleImmunityCold);
    setHandler(E_IMMUNITY_FIRE,                 handleImmunityFire);
    setHandler(E_EFREETI_FIRE_RESISTANCE,       handleImmunityFire);
    setHandler(E_IMMUNITY_PARALYSIS_POISON,     handleImmunityParalysisPoisonFear);
    setHandler(E_IMMUNITY_NONMAGICAL_WEAPONS,   handleImmunityNonmagical);
    setHandler(E_IMMUNITY_NONMAGICAL_HALF_SILVER, handleImmunityNonmagical);
    setHandler(E_HALF_DAMAGE_ELECTRICITY,       handleHalfDamage);
    setHandler(E_HALF_DAMAGE_PIERCING_SLASHING, handleHalfDamage);
    setHandler(E_HALF_DAMAGE_MAGICAL_WEAPONS,   handleHalfDamage);
    setHandler(E_HALF_DAMAGE_COLD,              handleHalfDamage);
    setHandler(E_VULNERABILITY_HOLY_WATER,      handleSavePenalty2);
    setHandler(E_VULNERABILITY_FIRE,            handleSavePenalty2);
    setHandler(E_TROLL_FIRE_OR_ACID,            handleSavePenalty2);
    setHandler(E_EXTRA_STRENGTH_130,            handleExtraStrength);
    setHandler(E_HUMAN_VS_SMALL,                handleBonusVsSmall);
    setSpecHandler(E_POOLRAD_FLAME_TONGUE_WEAPON,    handleFlameTongue);
    setSpecHandler(E_POOLRAD_SWORD_VS_UNDEAD,    handleSwordVsUndead);
    setHandler(E_POISON_DAMAGE,                          handlePoisonDamage);
    setSpecHandler(E_POOLRAD_STUDY_MANUAL_BODILY_HEALTH,       handleStudyManualBodilyHealth);
    setSpecHandler(E_POOLRAD_TRAIN_MANUAL_BODILY_HEALTH,    handleTrainingManualBodilyHealth);
}

Goldbox::Data::Effects::Effects EffectHandler::mapRawEffectId(uint8 rawId) const {
    using namespace Goldbox::Data::Effects;

    switch (rawId) {
    case E_POOLRAD_BLESSED:
        return E_BLESSED;
    case E_POOLRAD_CURSED:
        return E_CURSED;
    case E_POOLRAD_DETECT_MAGIC:
        return E_DETECT_MAGIC;
    case E_POOLRAD_PROTECTION_FROM_EVIL:
        return E_PROTECTION_FROM_EVIL;
    case E_POOLRAD_PROTECTION_FROM_GOOD:
        return E_PROTECTION_FROM_GOOD;
    case E_POOLRAD_RESIST_COLD:
        return E_RESIST_COLD;
    case E_POOLRAD_CHARM_PERSON:
        return E_CHARM_PERSON;
    default:
        // Most Poolrad effect IDs share the common table. Keep identity
        // mapping for those IDs; exceptions are listed explicitly above.
        return static_cast<Effects>(rawId);
    }
}

void EffectHandler::handleNoop(const Goldbox::Data::Effects::EffectCall &) {
}

} // namespace Poolrad
} // namespace Goldbox
