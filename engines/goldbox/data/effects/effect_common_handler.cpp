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

#include "goldbox/combat/combat_globals.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Data {
namespace Effects {

// File-local helper, not exposed.
namespace {

void applyFlag(EffectOp op, PlayerCharacter &ch, uint32 flag) {
    if (op == EFF_ADD)
        ch.effectState.flags |= flag;
    else if (op == EFF_REMOVE)
        ch.effectState.flags &= ~flag;
}

// Handlers used only within this file (not reused by game-specific handlers).

static void handleBlessed(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->moraleModifier += 5;
    c.combat->attackRoll += 1;
}

static void handleCursed(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->moraleModifier < 5)
        c.combat->moraleModifier = 0;
    else
        c.combat->moraleModifier -= 5;
    c.combat->attackRoll -= 1;
}

static void handleChant(const EffectCall &c) {
    if (!c.combat)
        return;
    const CombatSide prayerSide = static_cast<CombatSide>((c.effect.power & 0x10) >> 4);
    if (c.character.combatSide == prayerSide) {
        c.combat->attackRoll  += 1;
        c.combat->savingThrow += 1;
    } else {
        c.combat->attackRoll  -= 1;
        c.combat->savingThrow -= 1;
    }
}

static void handleHaste(const EffectCall &c) {
    if (!(c.effect.power & 0x10)) {
        c.effect.power |= 0x10;
        c.character.age++;
        if (c.bridge)
            c.bridge->postEffectMessage(&c.character, "ages", true);
    }
    if (c.combat)
        c.combat->attackCount += c.combat->attackCount;
}

static void handleSleep(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_SLEEPING | CEF_HELD);
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

// Effect 43 (Weakened): on EFF_ADD, drains one point of strength permanently.
// If strength is already below 4, applies terminal disease (E_CAUSE_DISEASE_1)
// instead. On EFF_REMOVE, restores the drained strength point.
static void handleWeakened(const EffectCall &c) {
    if (c.op == EFF_REMOVE) {
        c.character.abilities.strength.current++;
        return;
    }
    if (c.op != EFF_ADD)
        return;
    if (c.character.abilities.strength.current < 4) {
        CharacterEffects *fx = c.character.getEffects();
        if (fx && !fx->hasEffect(static_cast<uint8>(E_CAUSE_DISEASE_1)))
            tryAddEffect(c.character, static_cast<uint8>(E_CAUSE_DISEASE_1), 0xff, 0);
        return;
    }
    if (c.bridge)
        c.bridge->postEffectMessage(&c.character, "is weakened", true);
    c.character.abilities.strength.current--;
}

// Effect 44 (CauseWound): on EFF_ADD, deals 1 HP damage. If the character is
// already at 1 HP or below, applies terminal disease (E_CAUSE_DISEASE_1)
// instead. Outside combat, requests a character panel refresh.
static void handleCauseWound(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    if (c.character.hitPoints.current < 2) {
        CharacterEffects *fx = c.character.getEffects();
        if (fx && !fx->hasEffect(static_cast<uint8>(E_CAUSE_DISEASE_1)))
            tryAddEffect(c.character, static_cast<uint8>(E_CAUSE_DISEASE_1), 0xff, 0);
        return;
    }
    if (c.damage)
        c.damage->apply(c.character, Goldbox::Data::DamageRequest(1, false));
    if (!c.combat && c.bridge)
        c.bridge->requestRefresh(EffectHostBridge::RF_CHARACTER_PANEL);
}

static void handleFeeblemind(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 2;
    c.combat->damage -= 2;
    c.combat->moraleModifier -= 10;
}

static void handleNotImplemented(const EffectCall &c) {
    (void)c;
}

static void handleReduce(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->damage -= 1;
}

static void handleBlink(const EffectCall &c) {
	if (!c.combat || !c.character.combatState || c.character.combatState->initiative == 0)
        return;
    c.combat->targetUnavailable = true;
    c.combat->attackRoll = 0xff;
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

static void handleResistCold(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->behaviorFlags & Combat::CombatGlobals::DMG_COLD) {
        c.combat->damage >>= 1;
        c.combat->savingThrow += 3;
    }
}

static void handleRegen1(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_REGEN_1);
    if (c.op == EFF_TICK)
        c.character.heal(1);
}

static void handleCharm(const EffectCall &c) {
    if (c.op == EFF_REMOVE) {
        c.character.combatSide = (c.effect.power & 0x40)
            ? Goldbox::Data::CS_ENEMY : Goldbox::Data::CS_PARTY;
        if (c.character.npc == (int8)0xb3)
            c.character.npc = 0;
    } else if (c.op == EFF_ADD) {
        if (c.effect.power & 0x20)
            return;
        c.effect.power = (uint8)(0x20 +
            (c.character.combatSide == Goldbox::Data::CS_ENEMY ? 0x40 : 0x00) +
            c.effect.power);
        c.character.combatSide = Goldbox::Data::CS_PARTY;
        c.character.ai_control = true;
        if (!(c.character.npc & (int8)0x80))
            c.character.npc = (int8)0xb3;
        if (c.character.combatState)
            c.character.combatState->target = nullptr;
        if (c.combat)
            c.combat->moraleModifier = 100;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Shared helpers.
// ---------------------------------------------------------------------------

bool tryAddEffect(Goldbox::Data::PlayerCharacter &ch,
        uint8 effectId, uint8 power, uint16 duration) {
    if (Goldbox::g_engine && Goldbox::g_engine->isInSpellProcess())
        return false;
    ch.addEffect(effectId, duration, power, true);
    return true;
}

// ---------------------------------------------------------------------------
// Handlers exposed for reuse by game-specific handler files.
// ---------------------------------------------------------------------------

void handleAccursed(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 4;
    c.combat->savingThrow -= 4;
}

void handlePrayer(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll += 1;
    c.combat->savingThrow += 1;
}

void handleHelpless(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_HELPLESS | CEF_HELD);
}

void handleBlinded(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_BLINDED);
}

void handleSlow(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 1;
    c.combat->attackCount >>= 1;
}

void handleParalyze(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_PARALYZED | CEF_HELD);
}

void handleShield(const EffectCall &c) {
    if (c.op != EFF_ADD)
        return;
    if (c.character.armorClass.current < 0x39)
        c.character.armorClass.current = 0x39;
    if (!c.combat)
        return;
    ++c.combat->savingThrow;
    if (c.combat->activeSpellId == 0x0F)
        c.combat->damage = 0;
}

void handleFriendly(const EffectCall &c) {
    c.character.abilities.charisma.current = c.effect.power;
}

void handleEnlargeStrengthened(const EffectCall &c) {
    // power >= 0x80 means this effect is storing a saved-off old strength
    // value (set by applyStrengthChange); skip it during normal eval.
    if (c.effect.power >= 0x80)
        return;

    uint8 newStr, newExt;
    strengthDecode(c.effect.power, newStr, newExt);

    uint8 outEncoded;
    c.character.applyStrengthChange(newStr, newExt, outEncoded);
}

void handlePoisonDamage(const EffectCall &c) {
    if (c.op == EFF_ADD)
        c.character.effectState.flags |= CEF_POISONED;
    else if (c.op == EFF_REMOVE)
        c.character.effectState.flags &= ~CEF_POISONED;

    if (c.op != EFF_ADD)
        return;
    if (c.character.hitPoints.current <= 1)
        return;
    if (!c.damage)
        return;

    c.damage->apply(c.character, Goldbox::Data::DamageRequest(1, false));

    if (!c.combat && c.bridge)
        c.bridge->requestRefresh(EffectHostBridge::RF_CHARACTER_PANEL);
}

void handleProtNormalWeapons(const EffectCall &c) {
    if (c.op != EFF_ADD || !c.combat || !c.combat->attacker)
        return;
    ADnDCharacter *attacker = static_cast<ADnDCharacter *>(c.combat->attacker);
    Items::CharacterItem *attackItem = attacker->getWeaponOrAmmo();
    if (attackItem && attackItem->bonus == 0)
        rollAvoid(c.character, *c.combat, c.bridge, 100);
}

void handleRegen3(const EffectCall &c) {
    applyFlag(c.op, c.character, CEF_REGEN_3);
    if (c.op == EFF_TICK)
        c.character.heal(3);
}

void handleProtectionFromEvil(const EffectCall &c) {
    if (!c.combat || !c.combat->attacker)
        return;
    const uint8 alignment = c.combat->attacker->alignment;
    if (alignment == Goldbox::Data::A_LAWFUL_EVIL ||
            alignment == Goldbox::Data::A_NEUTRAL_EVIL ||
            alignment == Goldbox::Data::A_CHAOTIC_EVIL) {
        c.combat->savingThrow += 2;
        c.combat->attackRoll  -= 2;
    }
}

void handleProtectionFromGood(const EffectCall &c) {
    if (!c.combat || !c.combat->attacker)
        return;
    const uint8 alignment = c.combat->attacker->alignment;
    if (alignment == Goldbox::Data::A_LAWFUL_GOOD ||
            alignment == Goldbox::Data::A_NEUTRAL_GOOD ||
            alignment == Goldbox::Data::A_CHAOTIC_GOOD) {
        c.combat->savingThrow += 2;
        c.combat->attackRoll  -= 2;
    }
}

void handleSpiritualHammer(const EffectCall &c) {
    ADnDCharacter &character = static_cast<ADnDCharacter &>(c.character);
    Items::CharacterItem *hammer = nullptr;

    for (Items::CharacterItem &item : character.inventory.items()) {
        if (item.typeIndex == 20 && item.nameCode3 == 243) {
            hammer = &item;
            break;
        }
    }

    if (c.op != EFF_ADD) {
        if (hammer)
            character.removeItem(hammer);
    } else if (!hammer && character.inventory.count() < 16) {
        Items::CharacterItem item = {};
        item.typeIndex = 20;
        item.nameCode2 = 20;
        item.nameCode3 = 243;
        item.bonus = 1;
        item.effect2 = 23;
        item.effect3 = 137;

        if (character.addItem(item) && c.bridge)
            c.bridge->postEffectMessage(&c.character, "Gains an item...", true);
    }

    character.onEffectsChanged();
}

void handleBonusVsSmall(const EffectCall &c) {
    // Base implementation: no-op. Game-specific handlers override via
    // setSpecHandler or setHandler after setupCommonHandlers().
    (void)c;
}

void handleDwarfVsGiant(const EffectCall &c) {
    // Base implementation: no-op. Game-specific handlers override via
    // setSpecHandler or setHandler after setupCommonHandlers().
    (void)c;
}

// ---------------------------------------------------------------------------

// EFFECT_rollAvoid: chance to fully avoid the current hit. Returns true
// if the attack was avoided.
bool rollAvoid(Goldbox::Data::PlayerCharacter &targetChar,
        Combat::CombatGlobals &combat, EffectHostBridge *bridge, uint8 percent) {
    if (!combat.attacker)
        return false;

    ADnDCharacter *attacker = static_cast<ADnDCharacter *>(combat.attacker);
    if (!attacker->getEquippedItem(Items::Slot::S_MAIN_HAND))
        return false;

    if (!Goldbox::g_engine || Goldbox::g_engine->rollDice(1, 100) > percent)
        return false;

    if (bridge)
        bridge->postEffectMessage(&targetChar, "Avoids it.", true);

    combat.damage = 0;
    combat.attackRoll = 0xff;
    --combat.attacksLeft;

    return true;
}

void setupCommonHandlers(EffectHandlerBase &base) {
    base.setHandler(E_BLESSED,                  handleBlessed);
    base.setHandler(E_CURSED,                   handleCursed);
    base.setHandler(E_BESTOW_CURSE,             handleAccursed);
    base.setHandler(E_PRAYER,                   handlePrayer);
    base.setHandler(E_CHANT,                    handleChant);
    base.setHandler(E_HASTE,                    handleHaste);
    base.setHandler(E_SLOW,                     handleSlow);
    base.setHandler(E_PARALYZE,                 handleParalyze);
    base.setHandler(E_SLEEP,                    handleSleep);
    base.setHandler(E_HELPLESS,                 handleHelpless);
    base.setHandler(E_BLINDED,                  handleBlinded);
    base.setHandler(E_CONFUSE,                  handleConfuse);
    base.setHandler(E_FUMBLING,                 handleFumbling);
    base.setHandler(E_WEAKEN,                   handleWeaken);
    base.setHandler(E_WEAKENED,                 handleWeakened);
    base.setHandler(E_CAUSE_WOUND,              handleCauseWound);
    base.setHandler(E_FEEBLEMIND,               handleFeeblemind);
    base.setHandler(E_FRIENDS,                  handleFriendly);
    base.setHandler(E_READ_MAGIC,               handleNotImplemented);
    base.setHandler(E_SHIELD,                   handleShield);
    base.setHandler(E_STRENGTH,                 handleEnlargeStrengthened);
    base.setHandler(E_ENLARGE,                  handleEnlargeStrengthened);
    base.setHandler(E_BLINK,                    handleBlink);
    base.setHandler(E_REDUCE,                   handleReduce);
    base.setHandler(E_BERSERK,                  handleBerserk);
    base.setHandler(E_POISON_DAMAGE,            handlePoisonDamage);
    base.setHandler(E_POISONED,                 handlePoisoned);
    base.setHandler(E_POISON_PLUS_0,            handlePoisonPlus0);
    base.setHandler(E_POISON_PLUS_2,            handlePoisonPlus2);
    base.setHandler(E_POISON_PLUS_4,            handlePoisonPlus4);
    base.setHandler(E_POISON_NEG_2,             handlePoisonNeg2);
    base.setHandler(E_CON_SAVING_BONUS,         handleConSavingBonus);
    base.setHandler(E_PROTECTION_FROM_EVIL,     handleProtectionFromEvil);
    base.setHandler(E_PROTECTION_FROM_GOOD,     handleProtectionFromGood);
    base.setHandler(E_RESIST_COLD,              handleResistCold);
    base.setHandler(E_IMMUNITY_NONMAGICAL_WEAPONS, handleProtNormalWeapons);
    base.setHandler(E_SPIRITUAL_HAMMER,         handleSpiritualHammer);
    base.setHandler(E_HUMAN_VS_SMALL,           handleBonusVsSmall);
    base.setHandler(E_DWARF_AND_GNOME_VS_GIANTS, handleDwarfVsGiant);
    base.setHandler(E_REGENERATE_1_HPS,         handleRegen1);
    base.setHandler(E_REGENERATE_3_HPS,         handleRegen3);
    base.setHandler(E_REGEN_3_HP,               handleRegen3);
    base.setHandler(E_CHARM_PERSON,             handleCharm);
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
