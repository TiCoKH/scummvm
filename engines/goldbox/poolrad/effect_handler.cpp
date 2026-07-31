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
#include "common/util.h"
#include "goldbox/data/effects/effect_common_handler.h"
#include "goldbox/data/effects/effect_mapping.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {

namespace {

static const Goldbox::Data::Effects::Effects kRawMap[] = {
        Goldbox::Data::Effects::E_NONE,
        Goldbox::Data::Effects::E_BLESS,
        Goldbox::Data::Effects::E_CURSED,
        Goldbox::Data::Effects::E_SWORD_VS_UNDEAD,
        Goldbox::Data::Effects::E_DISPEL_EVIL,
        Goldbox::Data::Effects::E_DETECT_MAGIC,
        Goldbox::Data::Effects::E_IMMUNE_TO_ELECTRICITY,
        Goldbox::Data::Effects::E_FAERIE_FIRE,
        Goldbox::Data::Effects::E_PROTECTION_FROM_EVIL,
        Goldbox::Data::Effects::E_PROTECTION_FROM_GOOD,
        Goldbox::Data::Effects::E_RESIST_COLD,
        Goldbox::Data::Effects::E_CHARM_PERSON,
        Goldbox::Data::Effects::E_ENLARGE,
        Goldbox::Data::Effects::E_SUFFOCATE,
        Goldbox::Data::Effects::E_FRIENDS,
        Goldbox::Data::Effects::E_POISON_DAMAGE,
        Goldbox::Data::Effects::E_READ_MAGIC,
        Goldbox::Data::Effects::E_SHIELD,
        Goldbox::Data::Effects::E_GNOME_VS_MAN_SIZED_GIANT,
        Goldbox::Data::Effects::E_FIND_TRAPS,
        Goldbox::Data::Effects::E_RESIST_FIRE,
        Goldbox::Data::Effects::E_SILENCE_15_RADIUS,
        Goldbox::Data::Effects::E_SLOW_POISON,
        Goldbox::Data::Effects::E_SPIRITUAL_HAMMER,
        Goldbox::Data::Effects::E_DETECT_INVISIBILITY,
        Goldbox::Data::Effects::E_INVISIBILITY,
        Goldbox::Data::Effects::E_DWARF_VS_ORC,
        Goldbox::Data::Effects::E_FUMBLING,
        Goldbox::Data::Effects::E_MIRROR_IMAGE,
        Goldbox::Data::Effects::E_RAY_OF_ENFEEBLEMENT,
        Goldbox::Data::Effects::E_STINKING_CLOUD,
        Goldbox::Data::Effects::E_HELPLESS,
        Goldbox::Data::Effects::E_ANIMATE_DEAD,
        Goldbox::Data::Effects::E_BLINDED,
        Goldbox::Data::Effects::E_CAUSE_DISEASE_1,
        Goldbox::Data::Effects::E_CONFUSE,
        Goldbox::Data::Effects::E_BESTOW_CURSE,
        Goldbox::Data::Effects::E_BLINK,
        Goldbox::Data::Effects::E_STRENGTH,
        Goldbox::Data::Effects::E_HASTE,
        Goldbox::Data::Effects::E_COUGHING_FROM_STINKING_CLOUD,
        Goldbox::Data::Effects::E_PROT_FROM_NORMAL_MISSILES,
        Goldbox::Data::Effects::E_SLOW,
        Goldbox::Data::Effects::E_WEAKEN,
        Goldbox::Data::Effects::E_DISEASE_CONFUSED,
        Goldbox::Data::Effects::E_PROT_FROM_EVIL_10_RADIUS,
        Goldbox::Data::Effects::E_PROT_FROM_GOOD_10_RADIUS,
        Goldbox::Data::Effects::E_DWARF_AND_GNOME_VS_GIANTS,
        Goldbox::Data::Effects::E_GNOME_LARGE_MONSTER,
        Goldbox::Data::Effects::E_PRAYER,
        Goldbox::Data::Effects::E_HOT_FIRE_SHIELD,
        Goldbox::Data::Effects::E_SNAKE_CHARM,
        Goldbox::Data::Effects::E_PARALYZE,
        Goldbox::Data::Effects::E_SLEEP,
        Goldbox::Data::Effects::E_COLD_FIRE_SHIELD,
        Goldbox::Data::Effects::E_POISONED,
        Goldbox::Data::Effects::E_ITEM_INVISIBILITY,
        Goldbox::Data::Effects::E_ENGULFS,
        Goldbox::Data::Effects::E_CLEAR_MOVEMENT,
        Goldbox::Data::Effects::E_REGENERATE_3_HPS,
        Goldbox::Data::Effects::E_RAKSHASA_RESIST_NORMAL_WEAPONS,
        Goldbox::Data::Effects::E_FIRE_RESIST,
        Goldbox::Data::Effects::E_HIGH_CON_REGEN,
        Goldbox::Data::Effects::E_MINOR_GLOBE_OF_INVULNERABILITY,
        Goldbox::Data::Effects::E_POISON_PLUS_0,
        Goldbox::Data::Effects::E_POISON_PLUS_4,
        Goldbox::Data::Effects::E_POISON_PLUS_2,
        Goldbox::Data::Effects::E_THRI_KREEN_PARALYZE,
        Goldbox::Data::Effects::E_FEEBLEMIND,
        Goldbox::Data::Effects::E_INVISIBLE_TO_ANIMALS,
        Goldbox::Data::Effects::E_POISON_NEG_2,
        Goldbox::Data::Effects::E_INVISIBLE,
        Goldbox::Data::Effects::E_CAMOUFLAGE,
        Goldbox::Data::Effects::E_PROT_DRAG_BREATH,
        Goldbox::Data::Effects::E_AFFECT_4A,
        Goldbox::Data::Effects::E_WEAP_DRAGON_SLAYER,
        Goldbox::Data::Effects::E_WEAP_FROST_BRAND,
        Goldbox::Data::Effects::E_BERSERK,
        Goldbox::Data::Effects::E_AFFECT_4E,
        Goldbox::Data::Effects::E_FIRE_ATTACK_2D10,
        Goldbox::Data::Effects::E_ANKHEG_ACID_ATTACK,
        Goldbox::Data::Effects::E_HALF_DAMAGE,
        Goldbox::Data::Effects::E_RESIST_FIRE_AND_COLD,
        Goldbox::Data::Effects::E_PETRIFYING_GAZE,
        Goldbox::Data::Effects::E_SHAMBLING_ABSORB_LIGHTNING,
        Goldbox::Data::Effects::E_REDUCE_DAMAGE_TO_ONE_IF_ITEM_FIELD7_AFFECT_55,
        Goldbox::Data::Effects::E_GIANT_SLUG_SPIT_ACID,
        Goldbox::Data::Effects::E_BEHOLDER_RAYS_AFFECT_57,
        Goldbox::Data::Effects::E_BREATH_ELEC,
        Goldbox::Data::Effects::E_DISPLACE,
        Goldbox::Data::Effects::E_BREATH_ACID,
        Goldbox::Data::Effects::E_CLOUD_KILL,
        Goldbox::Data::Effects::E_FEAR_IMMUNITY,
        Goldbox::Data::Effects::E_HALF_FIRE_DAMAGE,
        Goldbox::Data::Effects::E_DAMAGE_REDUCTION,
        Goldbox::Data::Effects::E_WILD_BOAR_DIE_AFTER_EXTRA_FIGHT_TIME_AFFECT_5F,
        Goldbox::Data::Effects::E_OWLBEAR_HUG_CHECK,
        Goldbox::Data::Effects::E_CON_SAVING_BONUS,
        Goldbox::Data::Effects::E_REGEN_3_HP,
        Goldbox::Data::Effects::E_WILD_BOAR_AND_BULLETTE_AFFECT_63,
        Goldbox::Data::Effects::E_TROLL_FIRE_OR_ACID,
        Goldbox::Data::Effects::E_UNKNOWN_101,
        Goldbox::Data::Effects::E_UNKNOWN_102,
        Goldbox::Data::Effects::E_THRI_KREEN_MISSILE_EVASION,
        Goldbox::Data::Effects::E_RESIST_MAGIC_50,
        Goldbox::Data::Effects::E_RESIST_MAGIC_15,
        Goldbox::Data::Effects::E_RESIST_SLEEP_CHARM_90,
        Goldbox::Data::Effects::E_IMMUNITY_SLEEP_CHARM,
        Goldbox::Data::Effects::E_IMMUNITY_PARALYSIS,
        Goldbox::Data::Effects::E_IMMUNITY_COLD,
        Goldbox::Data::Effects::E_IMMUNITY_PARALYSIS_POISON,
        Goldbox::Data::Effects::E_IMMUNITY_FIRE,
        Goldbox::Data::Effects::E_EFREETI_FIRE_RESISTANCE,
        Goldbox::Data::Effects::E_HALF_DAMAGE_ELECTRICITY,
        Goldbox::Data::Effects::E_HALF_DAMAGE_PIERCING_SLASHING,
        Goldbox::Data::Effects::E_HALF_DAMAGE_MAGICAL_WEAPONS,
        Goldbox::Data::Effects::E_VULNERABILITY_HOLY_WATER,
        Goldbox::Data::Effects::E_HALF_DAMAGE_COLD,
        Goldbox::Data::Effects::E_IMMUNITY_NONMAGICAL_WEAPONS,
        Goldbox::Data::Effects::E_BOULDER_EVASION,
        Goldbox::Data::Effects::E_ANKHEG_ACID_SQUIRT_ATTACK,
        Goldbox::Data::Effects::E_VULNERABILITY_FIRE,
        Goldbox::Data::Effects::E_IMMUNITY_NONMAGICAL_HALF_SILVER,
        Goldbox::Data::Effects::E_RESIST_SLEEP_CHARM_30,
        Goldbox::Data::Effects::E_IMMUNITY_SLEEP_CHARM_PARALYSIS_POISON,
        Goldbox::Data::Effects::E_IMMUNITY_GAZE_ATTACKS,
        Goldbox::Data::Effects::E_UNIMPLEMENTED_126,
        Goldbox::Data::Effects::E_ITEM_EFFECT_127,
        Goldbox::Data::Effects::E_ITEM_EFFECT_128,
        Goldbox::Data::Effects::E_ITEM_EFFECT_129,
        Goldbox::Data::Effects::E_EXTRA_STRENGTH_130,
        Goldbox::Data::Effects::E_ITEM_131,
        Goldbox::Data::Effects::E_UNKNOWN_132,
        Goldbox::Data::Effects::E_UNKNOWN_133,
        Goldbox::Data::Effects::E_UNKNOWN_134,
        Goldbox::Data::Effects::E_UNKNOWN_135,
        Goldbox::Data::Effects::E_UNKNOWN_136
};

static const uint kExpectedRawEffectCount = 0x89;
static_assert(ARRAYSIZE(kRawMap) == kExpectedRawEffectCount,
    "kRawMap must remain aligned with expected raw effect id count");

static const Goldbox::Data::Effects::EffectMapping kRawEffectMap = {
    kRawMap,
    ARRAYSIZE(kRawMap)
};

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

static void handleResistCold(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_RESIST_COLD);
}

static void handleResistFire(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_RESIST_FIRE);
}

static void handleResistFireAndCold(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_RESIST_FIRE_COLD);
}

static void handleFireResist(const EffectCall &c) {
    applyFlag(c.op, asPoolrad(c.character), Data::PoolradCharacter::EF_FIRE_RESIST);
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
    applyFlag(c.op, asPoolrad(c.character),
        Data::PoolradCharacter::EF_RESIST_COLD |
        Data::PoolradCharacter::EF_HALF_DAMAGE);
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

static void handlePoisonDamage(const EffectCall &c) {
    if (c.op == EFF_TICK)
        c.character.damage(c.effect.power);
}

static void handleHighConRegen(const EffectCall &c) {
    if (c.op == EFF_TICK)
        c.character.heal(1);
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
    setHandler(E_RESIST_COLD,                   handleResistCold);
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
    setHandler(E_POISON_DAMAGE,                 handlePoisonDamage);
    setHandler(E_HIGH_CON_REGEN,                handleHighConRegen);
}

Goldbox::Data::Effects::Effects EffectHandler::mapRawEffectId(uint8 rawId) const {
    return kRawEffectMap.mapRaw(rawId);
}

void EffectHandler::handleNoop(const Goldbox::Data::Effects::EffectCall &) {
}

} // namespace Poolrad
} // namespace Goldbox
