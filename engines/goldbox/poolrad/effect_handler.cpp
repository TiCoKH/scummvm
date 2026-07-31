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
        Goldbox::Data::Effects::E_STICKS_TO_SNAKES,
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
}

EffectHandler::EffectHandler() {
    setupHandlers();
}

void EffectHandler::setupHandlers() {
    clearHandlers();
    setDefaultHandler(&EffectHandler::handleNoop);

    for (uint rawId = 0; rawId < kRawEffectMap.size; ++rawId) {
        setHandler(kRawEffectMap.mapRaw((uint8)rawId), &EffectHandler::handleEffect);
    }

    setHandler(Goldbox::Data::Effects::E_DAMAGE, &EffectHandler::handleNoop);
    setHandler(Goldbox::Data::Effects::E_HEAL, &EffectHandler::handleNoop);
    setHandler(Goldbox::Data::Effects::E_BUFF, &EffectHandler::handleNoop);
    setHandler(Goldbox::Data::Effects::E_DEBUFF, &EffectHandler::handleNoop);
    setHandler(Goldbox::Data::Effects::E_SUMMON, &EffectHandler::handleNoop);
    setHandler(Goldbox::Data::Effects::E_MISC, &EffectHandler::handleNoop);
}

Goldbox::Data::Effects::Effects EffectHandler::mapRawEffectId(uint8 rawId) const {
    return kRawEffectMap.mapRaw(rawId);
}

void EffectHandler::handleNoop(const Goldbox::Data::Effects::EffectCall &) {
}

namespace {
using namespace Goldbox::Data::Effects;
using Goldbox::Data::EffectModifiers;

static void applyFlag(EffectOp op, Data::PoolradCharacter &character, uint32 flag) {
    if (op == Goldbox::Data::Effects::EFF_ADD)
        character.effectState.flags |= flag;
    else if (op == Goldbox::Data::Effects::EFF_REMOVE)
        character.effectState.flags &= ~flag;
}

static void applyModifiers(EffectOp op, Data::PoolradCharacter &character,
                   const EffectModifiers &delta) {
    int sign = (op == EFF_REMOVE) ? -1 : (op == EFF_ADD ? 1 : 0);
    if (sign == 0)
        return;
    character.effectState.mods.attackRoll += delta.attackRoll * sign;
    character.effectState.mods.damage += delta.damage * sign;
    character.effectState.mods.savingThrow += delta.savingThrow * sign;
    character.effectState.mods.armorClass += delta.armorClass * sign;
    character.effectState.mods.morale += delta.morale * sign;
    character.effectState.mods.movement += delta.movement * sign;
}

static void applySimpleModifiers(EffectOp op, Data::PoolradCharacter &character,
                 int8 attackRoll, int8 damage, int8 savingThrow,
                 int8 armorClass, int8 morale, int8 movement) {
    EffectModifiers delta;
    delta.attackRoll = attackRoll;
    delta.damage = damage;
    delta.savingThrow = savingThrow;
    delta.armorClass = armorClass;
    delta.morale = morale;
    delta.movement = movement;
    applyModifiers(op, character, delta);
}

static void applyHeldFlag(EffectOp op, Data::PoolradCharacter &character, uint32 flag) {
    applyFlag(op, character, flag | Data::PoolradCharacter::EF_HELD);
}

static void applyRegen(EffectOp op, Data::PoolradCharacter &character, uint8 amount) {
    if (op != EFF_TICK)
        return;
    if (amount == 0)
        amount = 1;
    character.heal(amount);
}

static void applyTickDamage(EffectOp op, Data::PoolradCharacter &character, uint8 amount) {
    if (op != EFF_TICK)
        return;
    if (amount == 0)
        amount = 1;
    character.damage(amount);
}
}

void EffectHandler::handleEffect(const Goldbox::Data::Effects::EffectCall &call) {
    const EffectOp op = call.op;
    Effect &effect = call.effect;
    Goldbox::Data::PlayerCharacter &character = call.character;
    Data::PoolradCharacter &poolradCharacter = static_cast<Data::PoolradCharacter &>(character);
    Effects internalId = kRawEffectMap.mapRaw(effect.type);

    // Shared defaults first: other games can invert this order (override first,
    // common fallback) when an effect id has game-specific divergence.
    if (Goldbox::Data::Effects::tryApplyCommonEffect0(call, internalId))
        return;

    switch (internalId) {
    case Effects::E_SILENCE_15_RADIUS:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_SILENCED);
        break;
    case Effects::E_INVISIBILITY:
    case Effects::E_INVISIBLE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_INVISIBLE);
        break;
    case Effects::E_ITEM_INVISIBILITY:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_ITEM_INVISIBLE);
        break;
    case Effects::E_CAMOUFLAGE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_CAMOUFLAGE);
        break;
    case Effects::E_IMMUNE_TO_ELECTRICITY:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_IMMUNE_ELEC);
        break;
    case Effects::E_RESIST_COLD:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_RESIST_COLD);
        break;
    case Effects::E_RESIST_FIRE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_RESIST_FIRE);
        break;
    case Effects::E_RESIST_FIRE_AND_COLD:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_RESIST_FIRE_COLD);
        break;
    case Effects::E_FIRE_RESIST:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_FIRE_RESIST);
        break;
    case Effects::E_PROT_FROM_NORMAL_MISSILES:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_PROT_NORMAL_MISSILES);
        break;
    case Effects::E_PROT_DRAG_BREATH:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_PROT_DRAG_BREATH);
        break;
    case Effects::E_MINOR_GLOBE_OF_INVULNERABILITY:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_MINOR_GLOBE);
        break;
    case Effects::E_RAKSHASA_RESIST_NORMAL_WEAPONS:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_RAKSHASA_RESIST);
        break;
    case Effects::E_DISPLACE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_DISPLACE);
        break;
    case Effects::E_HALF_DAMAGE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_HALF_DAMAGE);
        break;
    case Effects::E_HALF_FIRE_DAMAGE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_HALF_FIRE_DAMAGE);
        break;
    case Effects::E_DAMAGE_REDUCTION:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_DAMAGE_REDUCTION);
        break;
    case Effects::E_FEAR_IMMUNITY:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_FEAR_IMMUNE);
        break;
    case Effects::E_SLOW_POISON:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_SLOW_POISON);
        break;
    case Effects::E_ENTANGLE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_ENTANGLED);
        break;
    case Effects::E_PETRIFYING_GAZE:
    case Effects::E_BEHOLDER_RAYS_AFFECT_57:
    case Effects::E_AFFECT_4A:
    case Effects::E_AFFECT_4E: {
        applySimpleModifiers(op, poolradCharacter, 2, 0, 0, 0, 0, 0);
        break;
    }
    case Effects::E_FIRE_ATTACK_2D10:
    case Effects::E_ANKHEG_ACID_ATTACK:
    case Effects::E_GIANT_SLUG_SPIT_ACID:
    case Effects::E_BREATH_ELEC:
    case Effects::E_BREATH_ACID:
    case Effects::E_CLOUD_KILL:
    case Effects::E_ANKHEG_ACID_SQUIRT_ATTACK:
    case Effects::E_WILD_BOAR_DIE_AFTER_EXTRA_FIGHT_TIME_AFFECT_5F:
    case Effects::E_OWLBEAR_HUG_CHECK:
    case Effects::E_WILD_BOAR_AND_BULLETTE_AFFECT_63: {
        applySimpleModifiers(op, poolradCharacter, 1, 3, 0, 0, 0, 0);
        break;
    }
    case Effects::E_THRI_KREEN_MISSILE_EVASION:
    case Effects::E_BOULDER_EVASION: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 2, 0, 0, 0);
        break;
    }
    case Effects::E_RESIST_MAGIC_15: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 1, 0, 0, 0);
        break;
    }
    case Effects::E_RESIST_SLEEP_CHARM_30: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 2, 0, 0, 0);
        break;
    }
    case Effects::E_RESIST_MAGIC_50: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 3, 0, 0, 0);
        break;
    }
    case Effects::E_RESIST_SLEEP_CHARM_90: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 5, 0, 0, 0);
        break;
    }
    case Effects::E_IMMUNITY_SLEEP_CHARM:
    case Effects::E_IMMUNITY_PARALYSIS:
    case Effects::E_IMMUNITY_SLEEP_CHARM_PARALYSIS_POISON:
    case Effects::E_IMMUNITY_GAZE_ATTACKS: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 6, 0, 0, 0);
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_FEAR_IMMUNE);
        break;
    }
    case Effects::E_IMMUNITY_COLD: {
        applyFlag(op, poolradCharacter,
            Data::PoolradCharacter::EF_RESIST_COLD |
            Data::PoolradCharacter::EF_HALF_DAMAGE);
        break;
    }
    case Effects::E_IMMUNITY_FIRE:
    case Effects::E_EFREETI_FIRE_RESISTANCE: {
        applyFlag(op, poolradCharacter,
            Data::PoolradCharacter::EF_RESIST_FIRE |
            Data::PoolradCharacter::EF_FIRE_RESIST |
            Data::PoolradCharacter::EF_HALF_FIRE_DAMAGE);
        break;
    }
    case Effects::E_IMMUNITY_PARALYSIS_POISON: {
        applyFlag(op, poolradCharacter,
            Data::PoolradCharacter::EF_SLOW_POISON |
            Data::PoolradCharacter::EF_FEAR_IMMUNE);
        applySimpleModifiers(op, poolradCharacter, 0, 0, 6, 0, 0, 0);
        break;
    }
    case Effects::E_HALF_DAMAGE_ELECTRICITY:
    case Effects::E_HALF_DAMAGE_PIERCING_SLASHING:
    case Effects::E_HALF_DAMAGE_MAGICAL_WEAPONS:
    case Effects::E_HALF_DAMAGE_COLD: {
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_HALF_DAMAGE);
        break;
    }
    case Effects::E_IMMUNITY_NONMAGICAL_WEAPONS:
    case Effects::E_IMMUNITY_NONMAGICAL_HALF_SILVER: {
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_RAKSHASA_RESIST);
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_DAMAGE_REDUCTION);
        break;
    }
    case Effects::E_VULNERABILITY_HOLY_WATER:
    case Effects::E_VULNERABILITY_FIRE:
    case Effects::E_TROLL_FIRE_OR_ACID: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, -2, 0, 0, 0);
        break;
    }
    case Effects::E_UNKNOWN_101:
    case Effects::E_UNKNOWN_102:
    case Effects::E_UNIMPLEMENTED_126:
    case Effects::E_ITEM_EFFECT_127:
    case Effects::E_ITEM_EFFECT_128:
    case Effects::E_ITEM_EFFECT_129:
    case Effects::E_ITEM_131:
    case Effects::E_UNKNOWN_132:
    case Effects::E_UNKNOWN_133:
    case Effects::E_UNKNOWN_134:
    case Effects::E_UNKNOWN_135:
    case Effects::E_UNKNOWN_136:
        break;
    case Effects::E_EXTRA_STRENGTH_130: {
        applySimpleModifiers(op, poolradCharacter, 1, 2, 0, 0, 0, 0);
        break;
    }
    case Effects::E_POISON_DAMAGE:
        applyTickDamage(op, poolradCharacter, effect.power);
        break;
    case Effects::E_HIGH_CON_REGEN:
        applyRegen(op, poolradCharacter, 1);
        break;
    default:
        break;
    }
}

} // namespace Poolrad
} // namespace Goldbox
