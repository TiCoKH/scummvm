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

#include "goldbox/data/effects/effect_common_handler.h"

#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
namespace Effects {

namespace {

static void applyFlag(EffectOp op, PlayerCharacter &character, uint32 flag) {
    if (op == EFF_ADD)
        character.effectState.flags |= flag;
    else if (op == EFF_REMOVE)
        character.effectState.flags &= ~flag;
}

static void applyModifiers(EffectOp op, PlayerCharacter &character,
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

static void applySimpleModifiers(EffectOp op, PlayerCharacter &character,
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

static void applyHeldFlag(EffectOp op, PlayerCharacter &character, uint32 flag) {
    applyFlag(op, character, flag | CEF_HELD);
}

static void applyRegen(EffectOp op, PlayerCharacter &character, uint8 amount) {
    if (op != EFF_TICK)
        return;
    if (amount == 0)
        amount = 1;
    character.heal(amount);
}

static void applyTickDamage(EffectOp op, PlayerCharacter &character,
        uint8 amount) {
    if (op != EFF_TICK)
        return;
    if (amount == 0)
        amount = 1;
    character.damage(amount);
}

} // namespace

bool tryApplyCommonEffect0(const EffectCall0 &call, Effects effectId) {
    PlayerCharacter &character = call.character;

    switch (effectId) {
    case E_BLESS:
        applySimpleModifiers(call.op, character, 1, 0, 0, 0, 5, 0);
        return true;
    case E_CURSED:
        applySimpleModifiers(call.op, character, -1, 0, 0, 0, -5, 0);
        return true;
    case E_PRAYER:
    case E_CHANT:
        applySimpleModifiers(call.op, character, 1, 1, 1, 0, 0, 0);
        return true;
    case E_HASTE:
        applySimpleModifiers(call.op, character, 0, 0, 0, 0, 0, 1);
        return true;
    case E_SLOW:
        applySimpleModifiers(call.op, character, 0, 0, 0, 0, 0, -1);
        return true;
    case E_PARALYZE:
        applyHeldFlag(call.op, character, CEF_PARALYZED);
        return true;
    case E_SLEEP:
        applyHeldFlag(call.op, character, CEF_SLEEPING);
        return true;
    case E_HELPLESS:
        applyHeldFlag(call.op, character, CEF_HELPLESS);
        return true;
    case E_BLINDED:
        applyFlag(call.op, character, CEF_BLINDED);
        return true;
    case E_CONFUSE:
        applyFlag(call.op, character, CEF_CONFUSED);
        return true;
    case E_FUMBLING:
        applySimpleModifiers(call.op, character, -2, 0, 0, 0, 0, 0);
        applyFlag(call.op, character, CEF_FUMBLING);
        return true;
    case E_WEAKEN:
        applySimpleModifiers(call.op, character, -1, -1, 0, 0, 0, 0);
        return true;
    case E_FEEBLEMIND:
        applySimpleModifiers(call.op, character, -2, -2, -2, 0, -10, 0);
        return true;
    case E_STRENGTH:
        applySimpleModifiers(call.op, character, 1, 1, 0, 0, 0, 0);
        return true;
    case E_ENLARGE:
        applySimpleModifiers(call.op, character, 0, 1, 0, 0, 0, 0);
        return true;
    case E_REDUCE:
        applySimpleModifiers(call.op, character, 0, -1, 0, 0, 0, 0);
        return true;
    case E_BERSERK:
        applySimpleModifiers(call.op, character, 2, 2, 0, -2, 10, 0);
        return true;
    case E_POISONED:
        applyFlag(call.op, character, CEF_POISONED);
        return true;
    case E_POISON_PLUS_0:
        applyFlag(call.op, character, CEF_POISONED);
        applyTickDamage(call.op, character, 1);
        return true;
    case E_POISON_PLUS_2:
        applyFlag(call.op, character, CEF_POISONED);
        applyTickDamage(call.op, character, 2);
        return true;
    case E_POISON_PLUS_4:
        applyFlag(call.op, character, CEF_POISONED);
        applyTickDamage(call.op, character, 3);
        return true;
    case E_POISON_NEG_2:
        applyFlag(call.op, character, CEF_POISONED);
        applyTickDamage(call.op, character, 1);
        applySimpleModifiers(call.op, character, 0, 0, -2, 0, 0, 0);
        return true;
    case E_CON_SAVING_BONUS:
        applySimpleModifiers(call.op, character, 0, 0, 2, 0, 0, 0);
        return true;
    case E_REGENERATE_1_HPS:
        applyFlag(call.op, character, CEF_REGEN_1);
        applyRegen(call.op, character, 1);
        return true;
    case E_REGENERATE_3_HPS:
    case E_REGEN_3_HP:
        applyFlag(call.op, character, CEF_REGEN_3);
        applyRegen(call.op, character, 3);
        return true;
    default:
        return false;
    }
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
