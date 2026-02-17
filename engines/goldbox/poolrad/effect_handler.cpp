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
        Goldbox::Data::Effects::E_TROLL_FIRE_OR_ACID
};

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

void EffectHandler::apply(Goldbox::Data::Effects::EffectOp op, Goldbox::Data::Effects::Effect &effect,
                          Data::PoolradCharacter &character) const {
    Goldbox::Data::Effects::EffectHandlerBase::apply(op, effect, character);
}

bool EffectHandler::hasHandler(uint8 effectType) const {
    return Goldbox::Data::Effects::EffectHandlerBase::hasHandler(effectType);
}

Goldbox::Data::Effects::Effects EffectHandler::mapRawEffectId(uint8 rawId) const {
    return kRawEffectMap.mapRaw(rawId);
}

void EffectHandler::handleNoop(Goldbox::Data::Effects::EffectOp, Goldbox::Data::Effects::Effect &,
                               Goldbox::Data::PlayerCharacter &) {
}

namespace {
static void applyFlag(Goldbox::Data::Effects::EffectOp op, Data::PoolradCharacter &character, uint32 flag) {
    if (op == Goldbox::Data::Effects::EFF_ADD)
        character.effectState.flags |= flag;
    else if (op == Goldbox::Data::Effects::EFF_REMOVE)
        character.effectState.flags &= ~flag;
}

static void applyModifiers(Goldbox::Data::Effects::EffectOp op, Data::PoolradCharacter &character,
                   const Goldbox::Data::EffectModifiers &delta) {
    int sign = (op == Goldbox::Data::Effects::EFF_REMOVE) ? -1 :
               (op == Goldbox::Data::Effects::EFF_ADD ? 1 : 0);
    if (sign == 0)
        return;
    character.effectState.mods.attackRoll += delta.attackRoll * sign;
    character.effectState.mods.damage += delta.damage * sign;
    character.effectState.mods.savingThrow += delta.savingThrow * sign;
    character.effectState.mods.armorClass += delta.armorClass * sign;
    character.effectState.mods.morale += delta.morale * sign;
    character.effectState.mods.movement += delta.movement * sign;
}

static void applySimpleModifiers(Goldbox::Data::Effects::EffectOp op, Data::PoolradCharacter &character,
                 int8 attackRoll, int8 damage, int8 savingThrow,
                 int8 armorClass, int8 morale, int8 movement) {
    Goldbox::Data::EffectModifiers delta;
    delta.attackRoll = attackRoll;
    delta.damage = damage;
    delta.savingThrow = savingThrow;
    delta.armorClass = armorClass;
    delta.morale = morale;
    delta.movement = movement;
    applyModifiers(op, character, delta);
}

static void applyHeldFlag(Goldbox::Data::Effects::EffectOp op, Data::PoolradCharacter &character, uint32 flag) {
    applyFlag(op, character, flag | Data::PoolradCharacter::EF_HELD);
}

static void applyRegen(Goldbox::Data::Effects::EffectOp op, Data::PoolradCharacter &character, uint8 amount) {
    if (op != Goldbox::Data::Effects::EFF_TICK)
        return;
    if (amount == 0)
        amount = 1;
    character.heal(amount);
}

static void applyTickDamage(Goldbox::Data::Effects::EffectOp op, Data::PoolradCharacter &character, uint8 amount) {
    if (op != Goldbox::Data::Effects::EFF_TICK)
        return;
    if (amount == 0)
        amount = 1;
    character.damage(amount);
}
}

void EffectHandler::handleEffect(Goldbox::Data::Effects::EffectOp op, Goldbox::Data::Effects::Effect &effect,
                                 Goldbox::Data::PlayerCharacter &character) {
    using Goldbox::Data::Effects::Effects;
    Data::PoolradCharacter &poolradCharacter = static_cast<Data::PoolradCharacter &>(character);
    Effects internalId = kRawEffectMap.mapRaw(effect.type);
    switch (internalId) {
    case Effects::E_BLESS: {
        applySimpleModifiers(op, poolradCharacter, 1, 0, 0, 0, 5, 0);
        break;
    }
    case Effects::E_CURSED: {
        applySimpleModifiers(op, poolradCharacter, -1, 0, 0, 0, -5, 0);
        break;
    }
    case Effects::E_PRAYER:
    case Effects::E_CHANT: {
        applySimpleModifiers(op, poolradCharacter, 1, 1, 1, 0, 0, 0);
        break;
    }
    case Effects::E_HASTE: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 0, 0, 0, 1);
        break;
    }
    case Effects::E_SLOW: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 0, 0, 0, -1);
        break;
    }
    case Effects::E_PARALYZE:
        applyHeldFlag(op, poolradCharacter, Data::PoolradCharacter::EF_PARALYZED);
        break;
    case Effects::E_SLEEP:
        applyHeldFlag(op, poolradCharacter, Data::PoolradCharacter::EF_SLEEPING);
        break;
    case Effects::E_HELPLESS:
        applyHeldFlag(op, poolradCharacter, Data::PoolradCharacter::EF_HELPLESS);
        break;
    case Effects::E_BLINDED:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_BLINDED);
        break;
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
    case Effects::E_FUMBLING: {
        applySimpleModifiers(op, poolradCharacter, -2, 0, 0, 0, 0, 0);
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_FUMBLING);
        break;
    }
    case Effects::E_CONFUSE:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_CONFUSED);
        break;
    case Effects::E_WEAKEN: {
        applySimpleModifiers(op, poolradCharacter, -1, -1, 0, 0, 0, 0);
        break;
    }
    case Effects::E_STRENGTH: {
        applySimpleModifiers(op, poolradCharacter, 1, 1, 0, 0, 0, 0);
        break;
    }
    case Effects::E_ENLARGE: {
        applySimpleModifiers(op, poolradCharacter, 0, 1, 0, 0, 0, 0);
        break;
    }
    case Effects::E_REDUCE: {
        applySimpleModifiers(op, poolradCharacter, 0, -1, 0, 0, 0, 0);
        break;
    }
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
    case Effects::E_POISONED:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_POISONED);
        break;
    case Effects::E_REGENERATE_1_HPS:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_REGEN_1);
        applyRegen(op, poolradCharacter, 1);
        break;
    case Effects::E_REGENERATE_3_HPS:
    case Effects::E_REGEN_3_HP:
        applyFlag(op, poolradCharacter, Data::PoolradCharacter::EF_REGEN_3);
        applyRegen(op, poolradCharacter, 3);
        break;
    case Effects::E_CON_SAVING_BONUS: {
        applySimpleModifiers(op, poolradCharacter, 0, 0, 2, 0, 0, 0);
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
