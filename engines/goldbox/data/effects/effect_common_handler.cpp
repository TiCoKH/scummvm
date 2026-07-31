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

static void applyFlag(EffectOp op, PlayerCharacter &ch, uint32 flag) {
    if (op == EFF_ADD)
        ch.effectState.flags |= flag;
    else if (op == EFF_REMOVE)
        ch.effectState.flags &= ~flag;
}

// --- Individual effect handlers ---

static void handleBless(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
    c.combat->moraleModifier += 5;
}

static void handleCursed(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 1;
    c.combat->moraleModifier -= 5;
}

static void handlePrayerChant(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
    c.combat->damage += 1;
}

static void handleHaste(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
}

static void handleSlow(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 1;
}

static void handleParalyze(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_PARALYZED | CEF_HELD);
}

static void handleSleep(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_SLEEPING | CEF_HELD);
}

static void handleHelpless(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_HELPLESS | CEF_HELD);
}

static void handleBlinded(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_BLINDED);
}

static void handleConfuse(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_CONFUSED);
}

static void handleFumbling(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_FUMBLING);
    if (!c.combat)
        return;
    c.combat->attackRoll -= 2;
}

static void handleWeaken(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 1;
    c.combat->damage -= 1;
}

static void handleFeeblemind(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 2;
    c.combat->damage -= 2;
    c.combat->moraleModifier -= 10;
}

static void handleStrength(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
    c.combat->damage += 1;
}

static void handleEnlarge(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->damage += 1;
}

static void handleReduce(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->damage -= 1;
}

static void handleBerserk(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 2;
    c.combat->damage += 2;
    c.combat->moraleModifier += 10;
}

static void handlePoisoned(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_POISONED);
}

static void handlePoisonPlus0(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_POISONED);
    if (c.op == EFF_TICK)
        c.character.damage(1);
}

static void handlePoisonPlus2(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_POISONED);
    if (c.op == EFF_TICK)
        c.character.damage(2);
}

static void handlePoisonPlus4(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_POISONED);
    if (c.op == EFF_TICK)
        c.character.damage(3);
}

static void handlePoisonNeg2(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_POISONED);
    if (c.op == EFF_TICK)
        c.character.damage(1);
}

static void handleConSavingBonus(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 2;
}

static void handleRegen1(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_REGEN_1);
    if (c.op == EFF_TICK)
        c.character.heal(1);
}

static void handleRegen3(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_REGEN_3);
    if (c.op == EFF_TICK)
        c.character.heal(3);
}

} // namespace

void setupCommonHandlers(EffectHandlerBase &base) {
    base.setHandler(E_BLESS,            handleBless);
    base.setHandler(E_CURSED,           handleCursed);
    base.setHandler(E_PRAYER,           handlePrayerChant);
    base.setHandler(E_CHANT,            handlePrayerChant);
    base.setHandler(E_HASTE,            handleHaste);
    base.setHandler(E_SLOW,             handleSlow);
    base.setHandler(E_PARALYZE,         handleParalyze);
    base.setHandler(E_SLEEP,            handleSleep);
    base.setHandler(E_HELPLESS,         handleHelpless);
    base.setHandler(E_BLINDED,          handleBlinded);
    base.setHandler(E_CONFUSE,          handleConfuse);
    base.setHandler(E_FUMBLING,         handleFumbling);
    base.setHandler(E_WEAKEN,           handleWeaken);
    base.setHandler(E_FEEBLEMIND,       handleFeeblemind);
    base.setHandler(E_STRENGTH,         handleStrength);
    base.setHandler(E_ENLARGE,          handleEnlarge);
    base.setHandler(E_REDUCE,           handleReduce);
    base.setHandler(E_BERSERK,          handleBerserk);
    base.setHandler(E_POISONED,         handlePoisoned);
    base.setHandler(E_POISON_PLUS_0,    handlePoisonPlus0);
    base.setHandler(E_POISON_PLUS_2,    handlePoisonPlus2);
    base.setHandler(E_POISON_PLUS_4,    handlePoisonPlus4);
    base.setHandler(E_POISON_NEG_2,     handlePoisonNeg2);
    base.setHandler(E_CON_SAVING_BONUS, handleConSavingBonus);
    base.setHandler(E_REGENERATE_1_HPS, handleRegen1);
    base.setHandler(E_REGENERATE_3_HPS, handleRegen3);
    base.setHandler(E_REGEN_3_HP,       handleRegen3);
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
