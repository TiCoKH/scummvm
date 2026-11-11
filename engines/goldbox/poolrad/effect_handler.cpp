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

// Pool of Radiance effect handlers, ported from EffHndlr.pas.
//
// Handlers ignore the operation unless the original tests `removed`:
// EFF_REMOVE is the original removed = True (the node expired or was
// removed with its onRemove flag set), every other op is removed = False
// (a trigger-set hit or a direct CallEffectHandler). The periodic effects
// therefore act and re-arm themselves when their timer runs out.

#include "goldbox/poolrad/effect_handler.h"
#include "goldbox/core/coords.h"
#include "goldbox/spells/spell_aoe.h"
#include "goldbox/combat/cloud_effect_manager.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/poolrad/data/poolrad_effects.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/data/rules/saving_throw.h"
#include "goldbox/engine.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {

namespace {

using namespace Goldbox::Data::Effects;
using Goldbox::Data::PlayerCharacter;
using Goldbox::Combat::CombatGlobals;

// TBaseItemRec.wpnClass melee damage type bits; slash is zero.
enum {
    kWpnSlash = 0x00,
    kWpnPierce = 0x01,
    kWpnBash = 0x80
};

// Item name components and base item types tested by the handlers.
enum {
    kNameMirror = 118,
    kNameSilver = 177,
    kNameHolySymbol = 152,
    kNameOfTyr = 252,
    kNameSpiritualHammer = 243,
    kItemHammer = 20,
    kItemHolyWater = 85,
    kItemFlaskOfOil = 86,
    kItemBoulder = 87,
    kItemLargeBoulder = 88
};

// Monster type (charType) values tested by the handlers.
enum {
    kTypeHumanoid = 1,
    kTypeUndead = 4
};

// --- Shared helpers (EffHndlr / FxEngine counterparts) ---

static Data::PoolradCharacter &asPoolrad(PlayerCharacter &ch) {
    return static_cast<Data::PoolradCharacter &>(ch);
}

static Goldbox::Data::ADnDCharacter &asAdnd(PlayerCharacter &ch) {
    return static_cast<Goldbox::Data::ADnDCharacter &>(ch);
}

static uint8 rollDice(uint8 count, uint8 sides) {
    return Goldbox::g_engine ?
        static_cast<uint8>(Goldbox::g_engine->rollDice(count, sides)) : count;
}

// FxEngine.RollDamageDice: also records the dice count.
static uint8 rollDamageDice(const EffectCall &c, uint8 count, uint8 sides) {
    if (c.combat)
        c.combat->damageDice = count;
    return rollDice(count, sides);
}

static bool effectLocked() {
    return Goldbox::g_engine && Goldbox::g_engine->isInSpellProcess();
}

static void setEffectLock(bool locked) {
    if (Goldbox::g_engine)
        Goldbox::g_engine->setInSpellProcess(locked);
}

static void say(const EffectCall &c, PlayerCharacter *ch,
        const char *text, bool withDelay = true) {
    if (c.bridge)
        c.bridge->postEffectMessage(ch, text, withDelay);
}

static bool inCombat() {
    return Goldbox::g_engine &&
        Goldbox::VmInterface::getGameStatus() == GS_COMBAT;
}

static Combat::CombatContext *combatContext() {
    return Goldbox::g_engine ? Goldbox::g_engine->getCombatContext() : nullptr;
}

// GDVars.curChar: the creature acting (attacking or casting).
static PlayerCharacter *actingCreature(const EffectCall &c) {
    return c.combat ? c.combat->attacker : nullptr;
}

// combatAddr^.other: the creature's current combat target.
static PlayerCharacter *combatTarget(PlayerCharacter &ch) {
    return ch.combatState ? ch.combatState->target : nullptr;
}

static bool hasEffect(PlayerCharacter &ch, uint8 effectId) {
    CharacterEffects *fx = ch.getEffects();
    return fx && fx->hasEffect(effectId);
}

static EffectSystem effectSystemFor(const EffectCall &c) {
    return EffectSystem(const_cast<EffectHandlerBase *>(c.handler), c.bridge);
}

// FxEngine.RemoveEffect: node when given, else the first node with effectId.
static void removeEffect(const EffectCall &c, PlayerCharacter &ch,
        uint8 effectId, Effect *node = nullptr) {
    CharacterEffects *fx = ch.getEffects();
    if (!fx)
        return;
    EffectSystem remover = effectSystemFor(c);
    if (node)
        remover.removeEffect(ch, *fx, *node);
    else
        remover.removeEffectById(ch, *fx, effectId);
}

// EffHndlr.AddEffectIfNoLock: the node always has onRemove set.
static bool addEffectIfNoLock(PlayerCharacter &ch, uint8 effectId,
        uint8 strength, uint16 minutes) {
    if (effectLocked())
        return false;
    ch.addEffect(effectId, minutes, strength, true);
    return true;
}

// FxEngine.CallEffectHandler. node may be null, as in the original.
static void callEffectHandler(const EffectCall &c, uint8 effectId,
        PlayerCharacter &ch, Effect *node, bool removed) {
    if (!c.handler)
        return;
    Effect none;
    none.id = effectId;
    none.durationMin = 0;
    none.power = 0xFF;
    none.immediate = 0;
    Effect &effect = node ? *node : none;
    c.handler->dispatch(effectId, EffectCall(removed ? EFF_REMOVE : EFF_ADD,
        effect, ch, c.combat, c.bridge, c.damage, c.handler, c.item));
}

// EffHndlr.NullifyAttack: id 0 always matches, otherwise only the effect
// under the immunity check.
static void nullifyAttack(const EffectCall &c, uint8 effectId) {
    if (!c.combat)
        return;
    if (effectId == 0 || c.combat->pendingEffectId == effectId) {
        c.combat->damage = 0;
        c.combat->pendingEffectId = 0;
    }
}

// EffHndlr.InflictStatusMsg.
static void inflictStatusMsg(const EffectCall &c, PlayerCharacter &ch,
        uint8 status, const char *message) {
    effectSystemFor(c).setStatus(ch, status, message);
}

// FxEngine.InflictNonWeaponDamage: NONWEAPON_DAMAGE handlers adjust the
// damage, then the save reduction (1 = none, 2 = half) applies.
static void inflictNonWeaponDamage(const EffectCall &c, PlayerCharacter &target,
        uint8 amount, uint8 element, uint8 saveReduction, bool saved) {
    CombatGlobals local;
    CombatGlobals *globals = c.combat ? c.combat : &local;
    globals->behaviorFlags = element;
    globals->damage = amount;

    CharacterEffects *fx = target.getEffects();
    if (fx && c.handler) {
        EffectRuntime runtime(const_cast<EffectHandlerBase *>(c.handler),
            c.bridge);
        runtime.checkEffectSet(ES_ON_DAMAGE_TAKEN, *fx, target, globals);
    }

    uint8 damage = globals->damage;
    if (saved) {
        if (saveReduction == 1)
            damage = 0;
        else if (saveReduction == 2)
            damage /= 2;
    }
    if (damage == 0)
        return;

    if (c.damage)
        c.damage->applyLegacy(target, damage, Goldbox::Data::DAMAGE_NORMAL,
            false, element);
    else
        target.damage(damage);
}

static bool rollSavingThrow(const EffectCall &c, PlayerCharacter &ch,
        Goldbox::Data::Spells::SaveVerseType saveType, int8 modifier) {
    return Goldbox::Data::Rules::checkSavingThrow(asAdnd(ch), c.combat,
        c.handler, c.bridge, saveType, modifier);
}

// CharHlp.CalcDistance, in tiles.
static uint8 calcDistance(PlayerCharacter &from, PlayerCharacter &to) {
    Combat::CombatContext *ctx = combatContext();
    if (!ctx)
        return 0xFF;
    return ctx->getTargetRange(&from, &to) >> 1;
}

// Every creature in play (GDVars.creatureList): the combat roster during
// combat, the party otherwise.
static Common::Array<PlayerCharacter *> allCreatures() {
    Common::Array<PlayerCharacter *> list;
    Combat::CombatContext *ctx = combatContext();
    if (ctx && !ctx->params.roster.empty())
        return ctx->params.roster;
    Common::List<PlayerCharacter *> *party = Goldbox::VmInterface::getParty();
    if (party) {
        for (PlayerCharacter *ch : *party) {
            if (ch)
                list.push_back(ch);
        }
    }
    return list;
}

static uint8 combatSlot(PlayerCharacter *ch) {
    Combat::CombatContext *ctx = combatContext();
    if (ctx) {
        for (uint i = 0; i < ctx->params.roster.size(); ++i) {
            if (ctx->params.roster[i] == ch)
                return static_cast<uint8>(i);
        }
    }
    return MonsterSpecialAttacks::kInvalidRosterIndex;
}

static PlayerCharacter *creatureInSlot(uint8 slot) {
    Combat::CombatContext *ctx = combatContext();
    if (ctx && MonsterSpecialAttacks::victimIndexInRange(slot,
            ctx->params.roster.size()))
        return ctx->params.roster[slot];
    return nullptr;
}

static void countTeams() {
    Combat::CombatContext *ctx = combatContext();
    if (ctx)
        ctx->updateSideCount();
}

static void refreshPartyDisplay(const EffectCall &c) {
    if (!inCombat() && c.bridge)
        c.bridge->requestRefresh(EffectHostBridge::RF_CHARACTER_PANEL);
}

// FxEngine.ReviveDeadCreature: put the creature back on its last cell.
static bool reviveDeadCreature(const EffectCall &c, PlayerCharacter &ch,
        uint8 hp) {
    Combat::CombatContext *ctx = combatContext();
    if (ctx) {
        uint8 col = ctx->table.getCharacterCol(&ch);
        uint8 row = ctx->table.getCharacterRow(&ch);
        const Common::Array<CombatGlobals::DownedMemberRecord> &downed =
            ctx->globals.downedMembers;
        for (uint i = 0; i < downed.size(); ++i) {
            if (downed[i].character == &ch) {
                col = downed[i].pos.col;
                row = downed[i].pos.row;
                break;
            }
        }
        const uint8 size = MAX<uint8>(1, ctx->table.getCharacterSize(&ch));
        const int idx = ctx->table.addCombatant(&ch, size);
        if (idx < 0)
            return false;
        ctx->table.setPosition(idx, TilePos(col, row));
    }

    ch.healthStatus = Goldbox::Data::S_OKAY;
    ch.enabled = true;
    ch.hitPoints.current = hp;
    say(c, &ch, ch.combatSide == Goldbox::Data::CS_ENEMY ?
        "stands up and grins" : "gets back up");
    countTeams();
    return true;
}

// FxEngine.PutOutOfPlayWithStatus.
static void putOutOfPlayWithStatus(const EffectCall &c, PlayerCharacter &ch,
        uint8 status, const char *message) {
    if (!ch.enabled)
        return;
    say(c, &ch, message);
    if (Combat::g_combatSession) {
        Combat::g_combatSession->setCharacterStatus(&ch, status);
        return;
    }
    ch.enabled = false;
    ch.healthStatus = status;
    if (status != Goldbox::Data::S_RUNNING)
        ch.hitPoints.current = 0;
}

// EffHndlr.FindDamagingWeapon: the ammunition for a bow or crossbow,
// otherwise the readied weapon; null without one.
static Goldbox::Data::Items::CharacterItem *findDamagingWeapon(
        PlayerCharacter *ch) {
    if (!ch)
        return nullptr;
    return asAdnd(*ch).getWeaponOrAmmo();
}

static Goldbox::Data::Items::CharacterItem *readiedWeapon(PlayerCharacter *ch) {
    if (!ch)
        return nullptr;
    return asAdnd(*ch).getEquippedItem(Goldbox::Data::Items::Slot::S_MAIN_HAND);
}

// EffHndlr.AvoidRanged: only against an armed attacker more than a cell away.
static void avoidRanged(const EffectCall &c, uint8 percent) {
    PlayerCharacter *attacker = actingCreature(c);
    if (!c.combat || !attacker || !readiedWeapon(attacker))
        return;
    if (calcDistance(*attacker, c.character) <= 1)
        return;
    if (rollDice(1, 100) > percent)
        return;
    say(c, &c.character, "Avoids it");
    c.combat->damage = 0;
    c.combat->attackRoll = 0xFF;
    --c.combat->hitsMade[0];
}

static bool nameIn(const Common::String &name, const char *const *list,
        uint count) {
    for (uint i = 0; i < count; ++i) {
        if (name == list[i])
            return true;
    }
    return false;
}

static const char *const kGnomeFoes[] = {
    "KOBOLD", "KOBOLD LEADER", "GOBLIN", "GOBLIN LEADER"
};

static const char *const kDwarfFoes[] = {
    "ORC", "ORC LEADER", "GOBLIN", "GOBLIN LEADER",
    "HOBGOBLIN", "HOBGOBLIN CHIEF", "GAGOOL", "MACE"
};

static const char *const kGiantFoes[] = {
    "OGRE", "OGRE LEADER", "TROLL", "FIRE GIANT", "HILL GIANT"
};

// EffHndlr.MagicResistance. The chance is a byte, so it wraps for casters
// far enough below 11th level and the resistance then always succeeds.
static void magicResistance(const EffectCall &c, uint8 percent) {
    if (!c.combat)
        return;
    uint8 level = 0;
    PlayerCharacter *caster = actingCreature(c);
    if (caster)
        level = asPoolrad(*caster).highestLevel;
    const uint8 chance = static_cast<uint8>(percent - (11 - level) * 5);
    if (c.combat->pendingEffectId == 0 &&
            !(c.combat->behaviorFlags & CombatGlobals::DMG_MAGIC))
        return;
    if (rollDice(1, 100) <= chance)
        nullifyAttack(c, 0);
}

// Constitution bonus of the Con save effects. The original leaves it
// unset outside Constitution 4..20; treat that as no bonus.
static uint8 conSaveBonus(uint8 con) {
    if (con < 4 || con > 20)
        return 0;
    if (con <= 6)
        return 1;
    if (con <= 10)
        return 2;
    if (con <= 13)
        return 3;
    if (con <= 17)
        return 4;
    return 5;
}

static bool hasEquippedMirror(PlayerCharacter &ch) {
    for (const Goldbox::Data::Items::CharacterItem &item :
            asAdnd(ch).inventory.items()) {
        if (item.readied && (item.nameCode1 == kNameMirror ||
                item.nameCode2 == kNameMirror || item.nameCode3 == kNameMirror))
            return true;
    }
    return false;
}

// --- Handlers, in EffHndlr order ---

static void handleNull(const EffectCall &) {
}

// Effect 1
static void handleBless(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->moraleModifier += 5;
    ++c.combat->attackRoll;
}

// Effect 2
static void handleCurse(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->moraleModifier < 5)
        c.combat->moraleModifier = 0;
    else
        c.combat->moraleModifier -= 5;
    --c.combat->attackRoll;
}

// Effect 3
static void handleUndeadSlayer(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!c.combat || !target)
        return;
    if (asPoolrad(*target).monsterType == kTypeUndead) {
        c.combat->attackRoll += 2;
        c.combat->damage += 2;
    }
}

// Effect 4: start training with the Manual of Bodily Health.
static void handleTrainStart(const EffectCall &c) {
    say(c, &c.character, "starts to train");
    c.character.addEffect(E_POOLRAD_TRAIN_MANUAL, 43200, 0xFF, true);
}

// Effect 6
static void handleFlameTongue(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!c.combat || !target)
        return;
    uint8 bonus = 0;
    switch (asPoolrad(*target).monsterType) {
    case 10:
        bonus = 1;
        break;
    case 9:
    case 12:
        bonus = 2;
        break;
    case kTypeUndead:
        bonus = 3;
        break;
    default:
        break;
    }
    c.combat->attackRoll += bonus;
    c.combat->damage += bonus;
    c.combat->behaviorFlags = CombatGlobals::DMG_FIRE | CombatGlobals::DMG_MAGIC;
}

// Effect 7: Manual of Bodily Health training done.
static void handleTrainManual(const EffectCall &c) {
    Data::PoolradCharacter &ch = asPoolrad(c.character);
    say(c, &ch, "is hardier");
    ++ch.abilities.constitution.current;
    const uint8 con = ch.abilities.constitution.current;
    if (con >= 20) {
        ch.addEffect(E_POOLRAD_CON_REGEN, 60, 0xFF, true);
        return;
    }
    uint8 hpGain = static_cast<uint8>(ch.hitPoints.max - ch.hitPointsRolled);
    if (con < 15)
        return;
    uint8 hpBonus = 0;
    for (uint j = 0; j < BASE_CLASS_NUM; ++j) {
        const uint8 level = ch.levels.levels[j];
        if (level == 0)
            continue;
        if (j == Goldbox::Data::C_FIGHTER)
            hpBonus += (con - 15) * level;
        else if (con > 15)
            hpBonus += level * 2;
        else
            hpBonus += level;
    }
    if (hpBonus == 0)
        hpBonus = 1;
    hpGain /= hpBonus;
    if (con < 17 || ch.levels[Goldbox::Data::C_FIGHTER] > 0) {
        ch.hitPoints.max += hpGain;
        ch.hitPoints.current += hpGain;
    }
}

static bool isEvil(uint8 alignment) {
    return alignment == Goldbox::Data::A_LAWFUL_EVIL ||
        alignment == Goldbox::Data::A_NEUTRAL_EVIL ||
        alignment == Goldbox::Data::A_CHAOTIC_EVIL;
}

static bool isGood(uint8 alignment) {
    return alignment == Goldbox::Data::A_LAWFUL_GOOD ||
        alignment == Goldbox::Data::A_NEUTRAL_GOOD ||
        alignment == Goldbox::Data::A_CHAOTIC_GOOD;
}

// Effects 8, 45
static void handleProtEvil(const EffectCall &c) {
    PlayerCharacter *attacker = actingCreature(c);
    if (!c.combat || !attacker || !isEvil(attacker->alignment))
        return;
    c.combat->savingThrow += 2;
    c.combat->attackRoll -= 2;
}

// Effects 9, 46
static void handleProtGood(const EffectCall &c) {
    PlayerCharacter *attacker = actingCreature(c);
    if (!c.combat || !attacker || !isGood(attacker->alignment))
        return;
    c.combat->savingThrow += 2;
    c.combat->attackRoll -= 2;
}

// Effect 10
static void handleResistCold(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & CombatGlobals::DMG_COLD))
        return;
    c.combat->damage /= 2;
    c.combat->savingThrow += 3;
}

// Effect 11. strength: bit 5 = applied, bit 6 = saved side, bit 7 = charmer.
static void handleCharm(const EffectCall &c) {
    PlayerCharacter &ch = c.character;
    if (c.op == EFF_REMOVE) {
        ch.combatSide = (c.effect.power & 0x40) ?
            Goldbox::Data::CS_ENEMY : Goldbox::Data::CS_PARTY;
        if (ch.npc == (int8)0xB3)
            ch.npc = 0;
        return;
    }

    if (!(c.effect.power & 0x20)) {
        c.effect.power = static_cast<uint8>(c.effect.power + 0x20 +
            static_cast<uint8>(ch.combatSide) * 0x40);
        ch.combatSide = (c.effect.power & 0x80) ?
            Goldbox::Data::CS_ENEMY : Goldbox::Data::CS_PARTY;
        ch.ai_control = true;
        if (!(ch.npc & (int8)0x80))
            ch.npc = (int8)0xB3;
        if (ch.combatState)
            ch.combatState->target = nullptr;
        countTeams();
    }
    if (c.combat)
        c.combat->moraleModifier = 100;
}

// Effects 12, 38: set Strength from this node, then let the strongest other
// Enlarge/Strength node take over. Nodes above 0x7F are skipped.
static void handleExtraStr(const EffectCall &c) {
    if (c.effect.power > 0x7F)
        return;
    PlayerCharacter &ch = c.character;
    uint8 base = 0, exc = 0;
    strengthDecode(c.effect.power, base, exc);
    ch.abilities.strength.current = base;
    ch.abilities.strException.current = exc;

    CharacterEffects *fx = ch.getEffects();
    if (!fx)
        return;
    Effect *best = nullptr;
    uint8 bestBase = 0, bestExc = 0;
    for (Effect &e : fx->effects()) {
        if ((e.id != E_POOLRAD_ENLARGE && e.id != E_POOLRAD_STRENGTH) ||
                &e == &c.effect)
            continue;
        strengthDecode(e.power, base, exc);
        if (base > bestBase || (bestBase == 18 && exc > bestExc)) {
            bestBase = base;
            bestExc = exc;
            best = &e;
        }
    }
    if (best) {
        best->power = strengthEncode(ch.abilities.strength.current,
            ch.abilities.strException.current);
        ch.abilities.strength.current = bestBase;
        ch.abilities.strException.current = bestExc;
    }
}

// Effect 14
static void handleFriends(const EffectCall &c) {
    c.character.abilities.charisma.current = c.effect.power;
}

// Effect 15: Slow Poison, 1 damage every 10 minutes while above 1 HP.
static void handleSlowPoisonHit(const EffectCall &c) {
    if (!addEffectIfNoLock(c.character, E_POOLRAD_SLOW_POISON_HIT,
            c.effect.power, 10))
        return;
    if (c.character.hitPoints.current > 1) {
        inflictNonWeaponDamage(c, c.character, 1, 0, 0, false);
        refreshPartyDisplay(c);
    }
}

// Effect 17
static void handleShield(const EffectCall &c) {
    // 60-biased AC 3.
    if (c.character.armorClass.current < 57)
        c.character.armorClass.current = 57;
    if (!c.combat)
        return;
    ++c.combat->savingThrow;
    // Magic Missile
    if (c.combat->activeSpellId == 15)
        c.combat->damage = 0;
}

// Effect 18
static void handleGnomeThac0(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!c.combat || !target)
        return;
    if (asPoolrad(*target).monsterType != kTypeHumanoid ||
            target->iconDimension != 1)
        return;
    if (nameIn(target->name, kGnomeFoes, ARRAYSIZE(kGnomeFoes)))
        ++c.combat->attackRoll;
}

// Effect 20
static void handleResistFire(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & CombatGlobals::DMG_FIRE))
        return;
    c.combat->damage /= 2;
    c.combat->savingThrow += 3;
}

// Effect 21: Silence 15' radius.
static void handleSilence(const EffectCall &c) {
    Goldbox::Data::CombatAction *cs = c.character.combatState;
    if (!cs)
        return;
    if (cs->canUse)
        say(c, &c.character, "is silenced");
    cs->canUse = false;
    cs->canCast = false;
}

// Effect 22: Slow Poison ran out; a creature still poisoned dies.
static void handleSlowPoisonEnd(const EffectCall &c) {
    if (hasEffect(c.character, E_POOLRAD_POISONED))
        inflictStatusMsg(c, c.character, Goldbox::Data::S_DEAD,
            "dies from poison");
    setEffectLock(true);
    removeEffect(c, c.character, E_POOLRAD_SLOW_POISON_HIT);
    setEffectLock(false);
}

// Effect 23: Spiritual Hammer.
static void handleSpiritHammer(const EffectCall &c) {
    Goldbox::Data::ADnDCharacter &ch = asAdnd(c.character);
    Goldbox::Data::Items::CharacterItem *hammer = nullptr;
    for (Goldbox::Data::Items::CharacterItem &item : ch.inventory.items()) {
        if (item.typeIndex == kItemHammer &&
                item.nameCode3 == kNameSpiritualHammer) {
            hammer = &item;
            break;
        }
    }

    if (c.op == EFF_REMOVE) {
        if (hammer)
            ch.removeItem(hammer);
    } else if (!hammer && ch.inventory.count() < 16) {
        Goldbox::Data::Items::CharacterItem item = {};
        item.typeIndex = kItemHammer;
        item.nameCode2 = 20;
        item.nameCode3 = kNameSpiritualHammer;
        item.bonus = 1;
        item.effect2 = E_POOLRAD_SPIRIT_HAMMER;
        item.effect3 = E_POOLRAD_SPIRIT_HAMMER_GONE;
        if (ch.addItem(item))
            say(c, &ch, "Gains an item");
    }
    ch.recalcCombatStats();
}

// Effect 25: Invisibility.
static void handleInvisible(const EffectCall &c) {
    if (!c.combat)
        return;
    PlayerCharacter *attacker = actingCreature(c);
    if (!attacker || !hasEffect(*attacker, E_POOLRAD_SEE_INVISIBLE))
        c.combat->targetUnavailable = true;
    c.combat->attackRoll -= 4;
}

// Effect 26
static void handleDwarfThac0(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!c.combat || !target)
        return;
    if (asPoolrad(*target).monsterType != kTypeHumanoid ||
            target->iconDimension != 1)
        return;
    if (nameIn(target->name, kDwarfFoes, ARRAYSIZE(kDwarfFoes)))
        ++c.combat->attackRoll;
}

// Effect 28: an image absorbs a single-target spell unless 1d(images+1)
// rolls 1.
static void handleMirrorImage(const EffectCall &c) {
    if (rollDice(1, c.effect.power + 1) <= 1)
        return;
    if (!c.combat || c.combat->activeSpellId == 0 || c.combat->multiTarget)
        return;
    nullifyAttack(c, 0);
    say(c, &c.character, "lost an image");
    --c.effect.power;
    if (c.effect.power == 0)
        removeEffect(c, c.character, E_POOLRAD_MIRROR_IMAGE);
}

// Effect 29: Ray of Enfeeblement.
static void handleEnfeeble(const EffectCall &c) {
    if (c.combat)
        c.combat->damage -= c.combat->damage / 4;
}

// Effect 30: coughing in a Stinking Cloud.
static void handleStinkCough(const EffectCall &c) {
    Goldbox::Data::ADnDCharacter &ch = asAdnd(c.character);
    if (ch.combatState) {
        if (ch.combatState->canUse)
            say(c, &ch, "is coughing");
        ch.combatState->canUse = false;
        ch.combatState->canCast = false;
    }
    ch.recalcCombatStats();
    if (ch.armorClassRear > 52)
        ch.armorClassRear -= 2;
    else
        ch.armorClassRear = 50;
    ch.armorClass.current = ch.armorClassRear;
}

// Effect 32: the animation ends (runs on removal).
static void handleAnimateDead(const EffectCall &c) {
    c.effect.immediate = 0;
    const uint8 side = c.effect.power >> 4;
    Data::PoolradCharacter &ch = asPoolrad(c.character);
    if (!effectLocked())
        inflictStatusMsg(c, ch, Goldbox::Data::S_DEAD, "collapses");
    ch.combatSide = side ? Goldbox::Data::CS_ENEMY : Goldbox::Data::CS_PARTY;
    ch.ai_control = true;
    ch.levelUndead = 0;
    ch.attackLevel = ch.levels[Goldbox::Data::C_FIGHTER];
    ch.movement.base = 12;
    if (ch.npc == (int8)0xB3)
        ch.npc = 0;
    ch.monsterType = 0;
}

// Effect 33
static void handleBlind(const EffectCall &c) {
    Goldbox::Data::ADnDCharacter &ch = asAdnd(c.character);
    ch.armorClass.current -= 4;
    ch.armorClassRear -= 4;
    if (!c.combat)
        return;
    c.combat->attackRoll -= 4;
    c.combat->savingThrow -= 4;
}

// Effect 34: chains the disease handlers 43 and 44 on this node.
static void handleDiseased(const EffectCall &c) {
    const bool removed = c.op == EFF_REMOVE;
    callEffectHandler(c, E_POOLRAD_DISEASE_STR, c.character, &c.effect, removed);
    callEffectHandler(c, E_POOLRAD_DISEASE_HP, c.character, &c.effect, removed);
}

// Effect 35: Prayer's bonus for the caster's side.
static void handleAlliedPrayer(const EffectCall &c) {
    if (!c.combat)
        return;
    ++c.combat->savingThrow;
    ++c.combat->attackRoll;
}

// Effect 36
static void handleBestowCurse(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->attackRoll -= 4;
    c.combat->savingThrow -= 4;
}

// Effect 37
static void handleBlinking(const EffectCall &c) {
    if (!c.combat || !c.character.combatState ||
            c.character.combatState->initiative == 0)
        return;
    c.combat->targetUnavailable = true;
    c.combat->attackRoll = 0xFF;
}

// Effect 39: ages the creature once (strength bit 4) and doubles the rate.
static void handleHasted(const EffectCall &c) {
    if (!(c.effect.power & 0x10)) {
        c.effect.power += 0x10;
        say(c, &c.character, "ages");
        ++c.character.age;
    }
    if (c.combat)
        c.combat->effectSet18.value *= 2;
}

// Effect 40: the caster's Stinking Cloud clears.
static void handleCastStink(const EffectCall &c) {
    Combat::CombatContext *ctx = combatContext();
    if (ctx)
        ctx->clouds.expire(&c.character, c.effect.power >> 4, c.bridge);
}

// Effect 41: Protection from Normal Missiles.
static void handleProtMissiles(const EffectCall &c) {
    Goldbox::Data::Items::CharacterItem *weapon =
        findDamagingWeapon(actingCreature(c));
    if (weapon && weapon->bonus == 0)
        avoidRanged(c, 100);
}

// Effect 42
static void handleSlowed(const EffectCall &c) {
    if (c.combat)
        c.combat->effectSet18.value /= 2;
}

static void addHelplessOnce(PlayerCharacter &ch) {
    if (!hasEffect(ch, E_POOLRAD_HELPLESS))
        ch.addEffect(E_POOLRAD_HELPLESS, 0, 0xFF, false);
}

// Effect 43: disease, -1 Strength an hour; helpless at Strength 3.
static void handleDiseaseStr(const EffectCall &c) {
    if (!addEffectIfNoLock(c.character, E_POOLRAD_DISEASE_STR,
            c.effect.power, 60))
        return;
    if (c.character.abilities.strength.current > 3) {
        say(c, &c.character, "is weakened");
        --c.character.abilities.strength.current;
    } else {
        addHelplessOnce(c.character);
    }
}

// Effect 44: disease, 1 damage every 10 minutes; helpless at 1 HP.
static void handleDiseaseHp(const EffectCall &c) {
    if (!addEffectIfNoLock(c.character, E_POOLRAD_DISEASE_HP,
            c.effect.power, 10))
        return;
    if (c.character.hitPoints.current > 1) {
        inflictNonWeaponDamage(c, c.character, 1, 0, 0, false);
        refreshPartyDisplay(c);
    } else {
        addHelplessOnce(c.character);
    }
}

// Effect 47: the original tests the attacker's type and size, but the name
// of the defender's own target.
static void handleDwarfAcGiant(const EffectCall &c) {
    PlayerCharacter *attacker = actingCreature(c);
    PlayerCharacter *other = combatTarget(c.character);
    if (!c.combat || !attacker || !other)
        return;
    if (asPoolrad(*attacker).monsterType != kTypeHumanoid ||
            (attacker->iconDimension & 0x7F) != 2)
        return;
    if (nameIn(other->name, kGiantFoes, ARRAYSIZE(kGiantFoes)))
        c.combat->attackRoll -= 4;
}

// Effect 48
static void handleGnomeAcBug(const EffectCall &c) {
    PlayerCharacter *attacker = actingCreature(c);
    if (!c.combat || !attacker ||
            asPoolrad(*attacker).monsterType != kTypeHumanoid)
        return;
    if (attacker->name == "BUGBEAR" || attacker->name == "GNOLL")
        c.combat->attackRoll -= 4;
}

// Effect 49: strength bit 4 is the caster's side.
static void handlePrayer(const EffectCall &c) {
    const uint8 side = (c.effect.power & 0x10) >> 4;
    if (static_cast<uint8>(c.character.combatSide) == side) {
        handleAlliedPrayer(c);
    } else if (c.combat) {
        --c.combat->attackRoll;
        --c.combat->savingThrow;
    }
}

// Effect 50: mummy rot blocks healing; 1 HP back every 10 days.
static void handleRotNoHeal(const EffectCall &c) {
    if (!addEffectIfNoLock(c.character, E_POOLRAD_ROT_NO_HEAL,
            c.effect.power, 14400))
        return;
    if (c.character.healHp(1, true) && c.bridge)
        c.bridge->showHealResult(&c.character);
}

// Effects 31, 51, 52, 53
static void handleHelpless(const EffectCall &c) {
    c.character.resetCombatAction();
}

// Effect 56: Ring of Invisibility.
static void handleRingInvis(const EffectCall &c) {
    c.character.addEffect(E_POOLRAD_INVISIBLE, 1, 12, false);
}

// Effect 57: mummy rot, -2 Charisma a month for 1d6 months, then death.
static void handleMummyRot(const EffectCall &c) {
    if (effectLocked())
        return;
    PlayerCharacter &ch = c.character;
    say(c, &ch, "Rots...");
    uint8 &cha = ch.abilities.charisma.current;
    cha = cha > 2 ? cha - 2 : 0;
    if (cha < 3)
        cha = 3;
    if ((c.effect.power >> 4) > 1) {
        c.effect.power -= 0x10;
        addEffectIfNoLock(ch, E_POOLRAD_MUMMY_ROT, c.effect.power, 43200);
    } else {
        inflictStatusMsg(c, ch, Goldbox::Data::S_GONE, "Dies rots away");
    }
}

// Effect 58
static void handleImmobile(const EffectCall &c) {
    if (c.character.combatState)
        c.character.combatState->movePoints = 0;
    if (c.combat && c.combat->effectSet18.isMovement)
        c.combat->effectSet18.value = 0;
}

// Effect 59: the troll starts regenerating for good.
static void handleTrollRegen(const EffectCall &c) {
    c.character.addEffect(E_POOLRAD_REGEN3, 0, 0xFF, false);
}

// Effect 61: Ring of Fire Resistance, -2 per die (minimum 1 per die) and
// +4 to save; non-magical fire is nullified.
static void handleRingFireRes(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & CombatGlobals::DMG_FIRE))
        return;
    const uint8 dice = c.combat->damageDice;
    for (uint8 i = 0; i < dice; ++i) {
        c.combat->damage -= 2;
        if (c.combat->damage < dice)
            c.combat->damage = dice;
    }
    c.combat->savingThrow += 4;
    if (!(c.combat->behaviorFlags & CombatGlobals::DMG_MAGIC))
        nullifyAttack(c, 0);
}

// Effect 62: Constitution 20+, 1 HP back an hour.
static void handleConRegen(const EffectCall &c) {
    if (!addEffectIfNoLock(c.character, E_POOLRAD_CON_REGEN,
            c.effect.power, 60))
        return;
    if (c.character.healHp(1, true) && c.bridge)
        c.bridge->showHealResult(&c.character);
}

// EffHndlr.PoisonTouch: the target saves or dies of poison.
static void poisonTouch(const EffectCall &c, int8 saveModifier) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target)
        return;
    if (rollSavingThrow(c, *target, Goldbox::Data::Spells::SVS_POISON,
            saveModifier))
        return;
    say(c, target, "is Poisoned", false);
    target->addEffect(E_POOLRAD_POISONED, 0, 0xFF, false);
    inflictStatusMsg(c, *target, Goldbox::Data::S_DEAD, "is killed");
}

// EffHndlr.ParalysisTouch. The original takes a save modifier but never
// uses it.
static void paralysisTouch(const EffectCall &c, uint16 minutes) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target)
        return;
    if (rollSavingThrow(c, *target, Goldbox::Data::Spells::SVS_POISON, 0))
        return;
    say(c, target, "is Paralyzed");
    target->addEffect(E_POOLRAD_PARALYSIS, minutes, 12, false);
}

// Effects 64, 65, 66, 70
static void handlePoisonHit0(const EffectCall &c) {
    poisonTouch(c, 0);
}

static void handlePoisonHit4(const EffectCall &c) {
    poisonTouch(c, 4);
}

static void handlePoisonHit2(const EffectCall &c) {
    poisonTouch(c, 2);
}

static void handlePoisonHitM2(const EffectCall &c) {
    poisonTouch(c, -2);
}

// Effect 67: thri-kreen, 2d8 minutes.
static void handleKreenPara(const EffectCall &c) {
    paralysisTouch(c, rollDice(2, 8));
}

// Effect 68: ghoul, 63 minutes, not against elves.
static void handleGhoulPara(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (target && target->race != Goldbox::Data::R_ELF)
        paralysisTouch(c, 63);
}

// Effect 69: drider, 1d9+10 minutes (its -2 save modifier is unused).
static void handleDriderPara(const EffectCall &c) {
    paralysisTouch(c, rollDice(1, 9) + 10);
}

// Effect 71: Dust of Disappearance.
static void handleDustDisappear(const EffectCall &c) {
    if (!c.combat)
        return;
    c.combat->targetUnavailable = true;
    c.combat->attackRoll -= 4;
}

// Effect 72: camouflage / Cloak of Elvenkind.
static void handleCamouflage(const EffectCall &c) {
    if (rollDice(1, 100) <= 95)
        c.character.addEffect(E_POOLRAD_INVISIBLE, 1, 12, false);
}

// CbtActs attack helper: run a temporary attack profile through the real
// attack resolver, then restore the creature's own profile.
static bool executeTemporaryAttack(PlayerCharacter &attacker,
        PlayerCharacter *target, uint8 attacks, uint8 diceNum,
        uint8 diceSides) {
    Combat::CombatSession *session = Combat::g_combatSession;
    if (!target || !session || !attacker.combatState)
        return false;

    Goldbox::Data::ADnDCharacter &adnd = asAdnd(attacker);
    Goldbox::Data::CombatRoll saved = adnd.getCurRoll(1);
    const uint8 savedAttackId = attacker.combatState->attackId;

    Goldbox::Data::CombatRoll &roll = adnd.getCurRoll(1);
    roll.attacks = attacks;
    roll.action.roll.diceNum = diceNum;
    roll.action.roll.diceSides = diceSides;
    roll.action.modifier = 0;
    attacker.combatState->attackId = 1;

    session->executeAttackOnTarget(&attacker, target, true);

    adnd.getCurRoll(1) = saved;
    attacker.combatState->attackId = savedAttackId;
    return true;
}

// Effect 73 (CbtActs.RearClawRake): after both claws hit, two 2d4 rakes.
static void handleRearClawRake(const EffectCall &c) {
    if (!c.combat || c.combat->hitsMade[1] != 2)
        return;
    PlayerCharacter *target = combatTarget(c.character);
    if (!target || !target->enabled)
        return;
    say(c, &c.character, "Rakes...");
    executeTemporaryAttack(c.character, target, 2, 2, 4);
    c.character.resetCombatAction();
}

// Effect 74 (CbtActs.BiteAndHold). strength = the victim's combat slot.
static void handleBiteHold(const EffectCall &c) {
    PlayerCharacter *victim = creatureInSlot(c.effect.power);
    const bool removed = c.op == EFF_REMOVE;
    if (removed || !c.character.enabled || !victim || !victim->enabled) {
        if (victim)
            removeEffect(c, *victim, E_POOLRAD_IMMOBILE);
        if (!removed) {
            c.effect.immediate = 0;
            removeEffect(c, c.character, E_POOLRAD_BITE_HOLD, &c.effect);
        }
        return;
    }

    executeTemporaryAttack(c.character, victim, 1, 3, 4);
    c.character.resetCombatAction();
    if (!victim->enabled)
        removeEffect(c, c.character, E_POOLRAD_BITE_HOLD, &c.effect);
}

// Effect 75: blood drain. strength = the victim's combat slot; the
// duration's upper bits are the remaining damage budget (x16).
static void handleBloodDrain(const EffectCall &c) {
    PlayerCharacter *victim = creatureInSlot(c.effect.power);
    const bool removed = c.op == EFF_REMOVE;
    if (removed || !c.character.enabled || !victim || !victim->enabled) {
        if (victim)
            removeEffect(c, *victim, E_POOLRAD_IMMOBILE);
        if (!removed) {
            c.effect.immediate = 0;
            removeEffect(c, c.character, E_POOLRAD_BLOOD_DRAIN, &c.effect);
        }
        if (c.character.enabled && victim && victim->enabled &&
                c.character.combatState)
            c.character.combatState->fleeing = true;
        return;
    }

    say(c, &c.character, "Sucks some Blood");
    const uint8 damage = rollDamageDice(c, 1, 4);
    inflictNonWeaponDamage(c, *victim, damage, 0, 0, false);
    c.character.resetCombatAction();
    if (victim->enabled && (c.effect.durationMin >> 4) > damage)
        c.effect.durationMin -= damage * 16;
    else
        removeEffect(c, c.character, E_POOLRAD_BLOOD_DRAIN, &c.effect);
}

// Effect 76: latch on with a 12 HP budget and immobilize the target.
static void handleBloodAttach(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target)
        return;
    c.character.addEffect(E_POOLRAD_BLOOD_DRAIN, 12 * 16 + 15,
        combatSlot(target), true);
    target->addEffect(E_POOLRAD_IMMOBILE, 0, 0xFF, false);
    callEffectHandler(c, E_POOLRAD_IMMOBILE, *target, nullptr, false);
}

// Effect 77: bite and hold, no effect on Strength 19+.
static void handleBiteAttach(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target || target->abilities.strength.current >= 19)
        return;
    c.character.addEffect(E_POOLRAD_BITE_HOLD, 0, combatSlot(target), true);
    target->addEffect(E_POOLRAD_IMMOBILE, 0, 0xFF, false);
    callEffectHandler(c, E_POOLRAD_IMMOBILE, *target, nullptr, false);
    say(c, target, "is held fast");
}

// Effect 78: healed from unconscious; retry a minute later if blocked.
static void handleHealWake(const EffectCall &c) {
    if (!reviveDeadCreature(c, c.character, c.character.hitPoints.current))
        addEffectIfNoLock(c.character, E_POOLRAD_HEAL_WAKE, c.effect.power, 1);
}

// Effect 79: fire touch, 2d10.
static void handleFireTouch(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (target)
        inflictNonWeaponDamage(c, *target, rollDamageDice(c, 2, 10),
            CombatGlobals::DMG_FIRE | CombatGlobals::DMG_MAGIC, 0, false);
}

// Effect 80: acid squirt, 1d4.
static void handleAcidSquirt(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (target)
        inflictNonWeaponDamage(c, *target, rollDamageDice(c, 1, 4),
            CombatGlobals::DMG_ACID, 0, false);
}

// FxEngine.AddEffectWithMessage: the EFFECT_IMMUNITY set may veto the
// effect; an existing node is replaced when the new duration is longer or
// permanent, and the new node is always added.
static bool addEffectWithMessage(const EffectCall &c, PlayerCharacter &ch,
        uint8 effectId, uint16 minutes, uint8 strength, bool onRemove,
        bool saveNegates, bool saved, const char *message) {
    CombatGlobals local;
    CombatGlobals *globals = c.combat ? c.combat : &local;
    globals->pendingEffectId = effectId;

    CharacterEffects *fx = ch.getEffects();
    if (fx && c.handler) {
        EffectRuntime runtime(const_cast<EffectHandlerBase *>(c.handler),
            c.bridge);
        runtime.checkEffectSet(ES_APPLY_WITH_MESSAGE_GUARDS, *fx, ch, globals);
    }

    if (globals->pendingEffectId == 0 || (saved && saveNegates)) {
        say(c, &ch, "is Unaffected");
        return false;
    }

    Effect *existing = fx ? fx->findEffectById(effectId) : nullptr;
    if (existing && ((existing->durationMin > 0 &&
            minutes > existing->durationMin) || minutes == 0))
        removeEffect(c, ch, effectId, existing);
    ch.addEffect(effectId, minutes, strength, onRemove);
    if (message && *message)
        say(c, &ch, message);
    return true;
}

// Effect 81: each enemy saves vs. spell or, by level, is paralyzed (0..3)
// or cursed (4, 5) for good.
static void handleDragonFear(const EffectCall &c) {
    const Common::Array<PlayerCharacter *> creatures = allCreatures();
    for (PlayerCharacter *ch : creatures) {
        if (!ch || !ch->enabled || ch->combatSide == c.character.combatSide)
            continue;
        const bool saved = rollSavingThrow(c, *ch,
            Goldbox::Data::Spells::SVS_SPELL, 0);
        const uint8 level = asPoolrad(*ch).highestLevel;
        if (level <= 3)
            addEffectWithMessage(c, *ch, E_POOLRAD_PARALYSIS, 0, 12, false,
                true, saved, "is paralyzed with fear");
        else if (level <= 5)
            addEffectWithMessage(c, *ch, E_POOLRAD_CURSE, 0, 12, false,
                true, saved, "is weakened by fear");
    }
}

// Effect 82: removes the aura from every creature, then each enemy saves
// vs. paralysis (+1 with 6+ on its side, +2 if human) or is paralyzed for
// 1d4 minutes.
static void handleMummyFear(const EffectCall &c) {
    const Common::Array<PlayerCharacter *> creatures = allCreatures();
    const Goldbox::Data::CombatSide side = c.character.combatSide;
    for (PlayerCharacter *ch : creatures) {
        if (!ch)
            continue;
        removeEffect(c, *ch, E_POOLRAD_MUMMY_FEAR);
        if (!ch->enabled || ch->combatSide == side)
            continue;
        int8 bonus = 0;
        if (c.combat && c.combat->sideCount[ch->combatSide] >= 6)
            ++bonus;
        if (ch->race == Goldbox::Data::R_HUMAN)
            bonus += 2;
        const bool saved = rollSavingThrow(c, *ch,
            Goldbox::Data::Spells::SVS_POISON, bonus);
        addEffectWithMessage(c, *ch, E_POOLRAD_PARALYSIS, rollDice(1, 4), 12,
            false, true, saved, "is paralyzed with fear");
    }
}

// Effect 83: save vs. petrification or be stoned; a reflectable gaze is
// turned back by an equipped mirror.
static void handlePetrifyGaze(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target)
        return;
    say(c, &c.character, "gazes...", false);
    if (hasEffect(c.character, E_POOLRAD_GAZE_REFLECTABLE) &&
            hasEquippedMirror(*target)) {
        say(c, target, "reflects it!", false);
        target = &c.character;
    }
    if (!rollSavingThrow(c, *target, Goldbox::Data::Spells::SVS_PETRIFICATION, 0))
        inflictStatusMsg(c, *target, Goldbox::Data::S_STONED, "is Stoned");
}

// Effect 84: Charm Person by gaze (save vs. spell at -2), no action used.
static void handleCharmGaze(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target || !target->enabled ||
            target->combatSide == c.character.combatSide)
        return;
    Combat::CombatContext *ctx = combatContext();
    if (ctx) {
        const TilePos from(ctx->table.getCharacterCol(&c.character),
            ctx->table.getCharacterRow(&c.character));
        const TilePos to(ctx->table.getCharacterCol(target),
            ctx->table.getCharacterRow(target));
        uint16 range = 0xFF;
        if (!ctx->lineOfSightCheck(from, to, range))
            return;
    }
    say(c, &c.character, "Gazes...", false);
    if (c.combat)
        c.combat->activeSpellId = 10;
    const uint8 strength = static_cast<uint8>(
        static_cast<uint8>(c.character.combatSide) * 0x80 + 12);
    const bool saved = rollSavingThrow(c, *target,
        Goldbox::Data::Spells::SVS_SPELL, -2);
    addEffectWithMessage(c, *target, E_POOLRAD_CHARM, 0, strength, true, true,
        saved, "is charmed");
    CharacterEffects *fx = target->getEffects();
    Effect *charm = fx ? fx->findEffectById(E_POOLRAD_CHARM) : nullptr;
    if (charm)
        callEffectHandler(c, E_POOLRAD_CHARM, *target, charm, false);
}

// DrainLevels.ClassToDrain: later classes win ties; xp is the XP floor of
// the chosen class level.
static uint8 classToDrain(const Data::PoolradCharacter &ch, int32 &xp) {
    xp = 0;
    uint8 best = 0;
    uint8 bestLevel = 0;
    for (uint8 j = 0; j < BASE_CLASS_NUM; ++j) {
        const uint8 level = ch.levels.levels[j];
        const int32 xpTry = level > 1 ?
            Goldbox::Data::Rules::xpForClassAtLevel(j, level) : 0;
        if (level >= bestLevel && (xpTry >= xp || xpTry == -1)) {
            xp = xpTry;
            best = j;
            bestLevel = level;
        }
    }
    return best;
}

// EffHndlr.DrainLevels.
static void drainLevels(const EffectCall &c, Data::PoolradCharacter &ch,
        uint8 levels) {
    Common::String text = levels > 1 ?
        Common::String::format("is drained %u levels", (unsigned)levels) :
        Common::String("is drained a level");
    say(c, &ch, text.c_str());

    if (ch.healthStatus == Goldbox::Data::S_ANIMATED) {
        inflictStatusMsg(c, ch, Goldbox::Data::S_GONE, "is dead forever");
        ch.hitPoints.max = 0;
        return;
    }

    for (uint8 i = 0; i < levels; ++i) {
        if (ch.healthStatus == Goldbox::Data::S_GONE)
            continue;
        ++ch.drainedLevels;
        uint8 totalLevels = 0;
        for (uint j = 0; j < BASE_CLASS_NUM; ++j)
            totalLevels += ch.levels.levels[j];
        const uint8 hpPerLevel = totalLevels ? ch.hitPoints.max / totalLevels : 0;
        for (uint8 k = 0; k < hpPerLevel; ++k) {
            if (ch.hitPoints.max > 0)
                --ch.hitPoints.max;
            if (ch.hitPointsRolled > 0)
                --ch.hitPointsRolled;
            if (ch.hitPoints.current > 0)
                --ch.hitPoints.current;
            ++ch.drainedHPs;
        }

        int32 xp = 0;
        if (ch.highestLevel < 2) {
            inflictStatusMsg(c, ch, Goldbox::Data::S_GONE, "is dead forever");
            ch.hitPoints.max = 0;
            if (ch.levels.levels[classToDrain(ch, xp)] > 0 && levels == 2)
                ch.levels.levels[classToDrain(ch, xp)] = 0;
        } else {
            --ch.levels.levels[classToDrain(ch, xp)];
            if (ch.hitPoints.current == 0 &&
                    ch.healthStatus != Goldbox::Data::S_DEAD)
                inflictStatusMsg(c, ch, Goldbox::Data::S_DEAD, "is killed");
            ch.recalcCombatStats();
        }
        ch.highestLevel = ch.levels.levels[classToDrain(ch, xp)];
        ch.experiencePoints = static_cast<uint32>(xp);
        ch.recalcCombatStats();
    }
}

// Effects 85, 86
static void handleDrainOne(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (target)
        drainLevels(c, asPoolrad(*target), 1);
}

static void handleDrainTwo(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (target)
        drainLevels(c, asPoolrad(*target), 2);
}

// Effect 87: mummy rot touch; seeds effect 57's 1d6 countdown.
static void handleRotTouch(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target || hasEffect(*target, E_POOLRAD_ROT_NO_HEAL))
        return;
    target->addEffect(E_POOLRAD_ROT_NO_HEAL, 14400, 0xFF, true);
    target->addEffect(E_POOLRAD_MUMMY_ROT, 43200,
        static_cast<uint8>(rollDice(1, 6) * 16 + 15), true);
    say(c, target, "is Diseased");
}

// Effect 88 (Magic.DragonBreathHandler).
static void handleDragonBreath(const EffectCall &c) {
    if (c.combat && c.combat->turnCounter != 0 && rollDice(1, 100) <= 50)
        return;

    say(c, &c.character, "Breathes!");

    // FxEngine.RemoveInvisibility
    while (hasEffect(c.character, E_POOLRAD_INVISIBLE))
        removeEffect(c, c.character, E_POOLRAD_INVISIBLE);

    Combat::CombatContext *ctx = combatContext();
    if (ctx) {
        const TilePos from(ctx->table.getCharacterCol(&c.character),
            ctx->table.getCharacterRow(&c.character));
        PlayerCharacter *target = combatTarget(c.character);
        const TilePos to = target ?
            TilePos(ctx->table.getCharacterCol(target),
                ctx->table.getCharacterRow(target)) : from;
        const uint8 damage = c.character.hitPoints.max;
        (void)Goldbox::Spells::resolveAoEHitAtTile(*ctx, to, damage,
            Goldbox::Data::Spells::SVS_BREATH, 19, nullptr);
        ctx->globals.targetPos = to;
        Goldbox::Spells::traceSpellPath(*ctx, from, 10, damage,
            Goldbox::Data::Spells::SVS_BREATH, false, 19, nullptr);
    }

    // Remaining breath charges
    if (c.effect.power > 253)
        --c.effect.power;
    else
        removeEffect(c, c.character, E_POOLRAD_DRAGON_BREATH, &c.effect);
    c.character.resetCombatAction();
}

// Effect 89: strength bit 4 = already displaced an attack this round.
static void handleDisplaced(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->turnCounter == 0 && c.combat->attackRoll == 0) {
        c.effect.power &= 0x0F;
    } else if (!(c.effect.power & 0x10)) {
        c.combat->attackRoll = 0xFF;
        c.effect.power |= 0x10;
    }
}

// Effect 90: Constitution bonus vs. paralysis/poison.
static void handleConPsnSave(const EffectCall &c) {
    if (c.combat &&
            c.combat->savingThrowType == Goldbox::Data::Spells::SVS_POISON)
        c.combat->savingThrow +=
            conSaveBonus(c.character.abilities.constitution.current);
}

// Effect 91: immune to electricity and Magic Missile.
static void handleImmuneMM(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->activeSpellId == 15) {
        nullifyAttack(c, 0);
        say(c, &c.character, "is unaffected");
    } else if (c.combat->behaviorFlags & CombatGlobals::DMG_ELECTRICITY) {
        nullifyAttack(c, 0);
    }
}

// Effect 93
static void handleHalfFire(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & CombatGlobals::DMG_FIRE))
        c.combat->damage /= 2;
}

// Effect 94: half damage from piercing or bashing weapons.
static void handleHalfBash(const EffectCall &c) {
    Goldbox::Data::Items::CharacterItem *weapon =
        findDamagingWeapon(actingCreature(c));
    if (c.combat && weapon && (weapon->prop().wpnType & (kWpnPierce | kWpnBash)))
        c.combat->damage /= 2;
}

// Effect 95: fighting on at 0 HP ran out.
static void handleFightOn(const EffectCall &c) {
    c.effect.immediate = 0;
    if (c.character.enabled)
        inflictStatusMsg(c, c.character, Goldbox::Data::S_DEAD, "Falls dead");
}

// Effect 96: only silver or magical weapons hurt it.
static void handleNeedSilver(const EffectCall &c) {
    Goldbox::Data::Items::CharacterItem *weapon =
        findDamagingWeapon(actingCreature(c));
    if (c.combat && (!weapon ||
            (weapon->bonus == 0 && weapon->nameCode3 != kNameSilver)))
        c.combat->damage = 0;
}

// Effect 97: Constitution bonus vs. spells and wands.
static void handleConSpellSave(const EffectCall &c) {
    if (!c.combat)
        return;
    if (c.combat->savingThrowType == Goldbox::Data::Spells::SVS_SPELL ||
            c.combat->savingThrowType == Goldbox::Data::Spells::SVS_ROD_STAFF_WAND)
        c.combat->savingThrow +=
            conSaveBonus(c.character.abilities.constitution.current);
}

// Effect 98: regenerate 3 HP a round.
static void handleRegen3(const EffectCall &c) {
    c.character.hitPoints.current = MIN<uint8>(c.character.hitPoints.max,
        c.character.hitPoints.current + 3);
}

// Effect 99: revive a dying (overkill < 6) or unconscious creature to fight
// on for 1d4+1 minutes.
static void handleCanFightOn(const EffectCall &c) {
    PlayerCharacter &ch = c.character;
    uint8 hp = 0;
    if (ch.healthStatus == Goldbox::Data::S_DYING && ch.combatState &&
            ch.combatState->bleeding < 6)
        hp = 6 - ch.combatState->bleeding;
    if (ch.healthStatus == Goldbox::Data::S_UNCONSCIOUS)
        hp = 6;
    if (hp == 0 || !reviveDeadCreature(c, ch, hp))
        return;
    ch.addEffect(E_POOLRAD_FIGHT_ON, rollDice(1, 4) + 1, 0xFF, true);
    c.effect.immediate = 0;
    removeEffect(c, ch, E_POOLRAD_CAN_FIGHT_ON, &c.effect);
}

// Effect 100: unless burnt or dissolved, the troll comes back in 3d6 minutes.
static void handleTrollBurn(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags &
            (CombatGlobals::DMG_FIRE | CombatGlobals::DMG_ACID)))
        return;
    c.character.addEffect(E_POOLRAD_TROLL_RETURN, rollDice(3, 6), 0xFF, true);
}

// Effect 101
static void handleTrollHeal(const EffectCall &c) {
    if (!hasEffect(c.character, E_POOLRAD_REGEN3) &&
            !hasEffect(c.character, E_POOLRAD_TROLL_REGEN))
        c.character.addEffect(E_POOLRAD_TROLL_REGEN, 3, 0xFF, true);
}

// Effect 102: the troll gets back up at full HP.
static void handleTrollReturn(const EffectCall &c) {
    if (!reviveDeadCreature(c, c.character, c.character.hitPoints.max))
        addEffectIfNoLock(c.character, E_POOLRAD_TROLL_RETURN,
            c.effect.power, 1);
}

// Effect 103. It runs from HP_ZERO on a creature already out of play, so
// PutOutOfPlayWithStatus returns at once in the original as well.
static void handleGaseous(const EffectCall &c) {
    c.effect.immediate = 0;
    putOutOfPlayWithStatus(c, c.character, Goldbox::Data::S_RUNNING,
        "Turns gaseous and escapes");
}

// Effect 104: thri-kreen dodge missiles 60%.
static void handleKreenDodge(const EffectCall &c) {
    avoidRanged(c, 60);
}

// Effect 105
static void handleMagicRes50(const EffectCall &c) {
    magicResistance(c, 50);
}

// Effect 106: the original passes 15, not 85.
static void handleMagicRes85(const EffectCall &c) {
    magicResistance(c, 15);
}

// Effect 107: elves resist sleep and charm 90%.
static void handleElfResist(const EffectCall &c) {
    if (rollDice(1, 100) <= 90) {
        nullifyAttack(c, E_POOLRAD_SLEEP);
        nullifyAttack(c, E_POOLRAD_CHARM);
    }
}

// Effect 108
static void handleNoSleepCharm(const EffectCall &c) {
    nullifyAttack(c, E_POOLRAD_CHARM);
    nullifyAttack(c, E_POOLRAD_SLEEP);
}

// Effect 109
static void handleNoParalysis(const EffectCall &c) {
    nullifyAttack(c, E_POOLRAD_PARALYSIS);
}

// Effect 110
static void handleNoCold(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & CombatGlobals::DMG_COLD))
        nullifyAttack(c, 0);
}

// Effect 111: paralysis/poison immunity; those saves always succeed.
static void handleNoParaPoison(const EffectCall &c) {
    nullifyAttack(c, E_POOLRAD_POISONED);
    nullifyAttack(c, E_POOLRAD_PARALYSIS);
    if (c.combat &&
            c.combat->savingThrowType == Goldbox::Data::Spells::SVS_POISON)
        c.combat->savingThrow = 100;
}

// Effect 112
static void handleNoFire(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & CombatGlobals::DMG_FIRE))
        nullifyAttack(c, 0);
}

// Effect 113: -1 per die, at least 1 per die.
static void handleEfreetiFire(const EffectCall &c) {
    if (!c.combat || !(c.combat->behaviorFlags & CombatGlobals::DMG_FIRE))
        return;
    const uint8 dice = c.combat->damageDice;
    for (uint8 i = 0; i < dice; ++i) {
        --c.combat->damage;
        if (c.combat->damage < dice)
            c.combat->damage = dice;
    }
}

// Effect 114
static void handleHalfElec(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & CombatGlobals::DMG_ELECTRICITY))
        c.combat->damage /= 2;
}

// Effect 115: half damage from piercing or slashing weapons.
static void handleHalfPierce(const EffectCall &c) {
    Goldbox::Data::Items::CharacterItem *weapon =
        findDamagingWeapon(actingCreature(c));
    if (!c.combat || !weapon)
        return;
    const uint8 wpnClass = weapon->prop().wpnType;
    if (wpnClass == kWpnSlash || (wpnClass & kWpnPierce))
        c.combat->damage /= 2;
}

// Effect 116
static void handleHalfMagical(const EffectCall &c) {
    Goldbox::Data::Items::CharacterItem *weapon =
        findDamagingWeapon(actingCreature(c));
    if (c.combat && weapon && weapon->bonus > 0)
        c.combat->damage /= 2;
}

// Effect 117: a vial of holy water does 1d6+1.
static void handleHolyWater(const EffectCall &c) {
    Goldbox::Data::Items::CharacterItem *weapon = readiedWeapon(actingCreature(c));
    if (c.combat && weapon && weapon->typeIndex == kItemHolyWater)
        c.combat->damage = rollDamageDice(c, 1, 6) + 1;
}

// Effect 118
static void handleHalfCold(const EffectCall &c) {
    if (c.combat && (c.combat->behaviorFlags & CombatGlobals::DMG_COLD))
        c.combat->damage /= 2;
}

// Effect 119: non-magical and unarmed attacks fail, unless a monster of
// level 4+ makes them.
static void handleNeedMagic(const EffectCall &c) {
    PlayerCharacter *attacker = actingCreature(c);
    Goldbox::Data::Items::CharacterItem *weapon = findDamagingWeapon(attacker);
    if (!c.combat || !attacker || (weapon && weapon->bonus != 0))
        return;
    if (attacker->race > Goldbox::Data::R_MONSTER ||
            asPoolrad(*attacker).highestLevel < 4)
        c.combat->damage = 0;
}

// Effect 120: dodge a thrown boulder 50%.
static void handleBoulderDodge(const EffectCall &c) {
    Goldbox::Data::Items::CharacterItem *weapon = readiedWeapon(actingCreature(c));
    if (weapon && (weapon->typeIndex == kItemBoulder ||
            weapon->typeIndex == kItemLargeBoulder))
        avoidRanged(c, 50);
}

// Effect 121: anhkheg acid spit, 25% within 4 cells, 8d4, save vs. breath
// for half; then it is used up.
static void handleAnhkhegAcid(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!target)
        return;
    if (rollDice(1, 100) > 25)
        return;
    if (calcDistance(c.character, *target) >= 4)
        return;
    c.character.resetCombatAction();
    say(c, &c.character, "Spits Acid");
    const uint8 damage = rollDamageDice(c, 8, 4);
    const bool saved = rollSavingThrow(c, *target,
        Goldbox::Data::Spells::SVS_BREATH, 0);
    inflictNonWeaponDamage(c, *target, damage,
        c.combat ? c.combat->behaviorFlags : 0, 2, saved);
    removeEffect(c, c.character, E_POOLRAD_ANHKHEG_ACID, &c.effect);
    removeEffect(c, c.character, E_POOLRAD_ACID_SQUIRT);
}

// Effect 122: a flask of oil does 3d8; magic or fire +1 per die.
static void handleMummyFire(const EffectCall &c) {
    if (!c.combat)
        return;
    Goldbox::Data::Items::CharacterItem *weapon = readiedWeapon(actingCreature(c));
    if (weapon && weapon->typeIndex == kItemFlaskOfOil)
        c.combat->damage = rollDamageDice(c, 3, 8);
    if (c.combat->behaviorFlags &
            (CombatGlobals::DMG_FIRE | CombatGlobals::DMG_MAGIC))
        c.combat->damage += c.combat->damageDice;
}

// Effect 123: silver does half, magic full, anything else nothing.
static void handleSilverMagic(const EffectCall &c) {
    if (!c.combat)
        return;
    Goldbox::Data::Items::CharacterItem *weapon =
        findDamagingWeapon(actingCreature(c));
    if (!weapon)
        c.combat->damage = 0;
    else if (weapon->bonus == 0 && weapon->nameCode3 == kNameSilver)
        c.combat->damage /= 2;
    else if (weapon->bonus == 0)
        c.combat->damage = 0;
}

// Effect 124: half-elves resist sleep and charm 30%.
static void handleHalfElfResist(const EffectCall &c) {
    if (rollDice(1, 100) <= 30) {
        nullifyAttack(c, E_POOLRAD_CHARM);
        nullifyAttack(c, E_POOLRAD_SLEEP);
    }
}

// Effect 125: sleep/charm/paralysis/poison immunity.
static void handleNoCharmPoison(const EffectCall &c) {
    nullifyAttack(c, E_POOLRAD_CHARM);
    nullifyAttack(c, E_POOLRAD_SLEEP);
    nullifyAttack(c, E_POOLRAD_PARALYSIS);
    nullifyAttack(c, E_POOLRAD_POISONED);
    if (c.combat &&
            c.combat->savingThrowType == Goldbox::Data::Spells::SVS_POISON)
        c.combat->savingThrow = 100;
}

// Effect 126: the gaze is blocked by the target's equipped mirror or
// Wooden Holy Symbol of Tyr.
static void handleGazeBlock(const EffectCall &c) {
    PlayerCharacter *target = combatTarget(c.character);
    if (!c.combat || !target)
        return;
    for (const Goldbox::Data::Items::CharacterItem &item :
            asAdnd(*target).inventory.items()) {
        if (c.combat->targetUnavailable)
            break;
        if (!item.readied)
            continue;
        const uint8 names[3] = { item.nameCode1, item.nameCode2, item.nameCode3 };
        for (uint j = 0; j < 3; ++j) {
            if ((names[j] == kNameHolySymbol && item.nameCode3 == kNameOfTyr) ||
                    names[j] == kNameMirror)
                c.combat->targetUnavailable = true;
        }
    }
}

// Effects 128..130, 133, 134, 136, 138, 139: readying the item adds its
// effect byte as a permanent effect and runs that handler.
static void handleReadyItem(const EffectCall &c) {
    const uint8 effectId = c.item ? c.item->effect2 : c.effect.power;
    if (c.op == EFF_REMOVE) {
        removeEffect(c, c.character, effectId);
        return;
    }
    c.character.addEffect(effectId, 0, 12, false);
    callEffectHandler(c, effectId, c.character, nullptr, false);
}

// Effect 131: Gauntlets of Ogre Power. Unreadying removes the first Strength
// node when at 18/00, else the first one holding 18/00.
static void handleGauntlets(const EffectCall &c) {
    PlayerCharacter &ch = c.character;
    if (c.op == EFF_REMOVE) {
        CharacterEffects *fx = ch.getEffects();
        if (!fx)
            return;
        const bool isOgre = ch.abilities.strength.current == 18 &&
            ch.abilities.strException.current == 100;
        for (Effect &e : fx->effects()) {
            if (e.id != E_POOLRAD_STRENGTH)
                continue;
            uint8 base = 0, exc = 0;
            strengthDecode(e.power, base, exc);
            if (isOgre || (base == 18 && exc == 100)) {
                removeEffect(c, ch, E_POOLRAD_STRENGTH, &e);
                return;
            }
        }
        return;
    }

    uint8 encoded = 0;
    if (ch.applyStrengthChange(18, 100, encoded))
        say(c, &ch, "is stronger");
    if (c.bridge)
        c.bridge->requestRefresh(EffectHostBridge::RF_CHARACTER_PANEL);
    ch.addEffect(E_POOLRAD_STRENGTH, 0, encoded, true);
}

// Effect 132: the item's effect byte holds the required alignment (low
// nibble); another wielder drops it and takes the upper nibble in damage.
static void handleAlignedWeapon(const EffectCall &c) {
    if (c.op == EFF_REMOVE || !c.item)
        return;
    if ((c.item->effect2 & 0x0F) == c.character.alignment)
        return;
    c.item->readied = 0;
    inflictNonWeaponDamage(c, c.character, c.item->effect2 >> 4,
        CombatGlobals::DMG_MAGIC, 0, false);
    if (c.bridge)
        c.bridge->requestRefresh(EffectHostBridge::RF_CHARACTER_PANEL);
}

// Effect 135: a boulder needs giant Strength.
static void handleBoulder(const EffectCall &c) {
    if (c.op == EFF_REMOVE || !c.item)
        return;
    if (c.character.abilities.strength.current < 19) {
        c.item->readied = 0;
        say(c, &c.character, "Must have Giant Strength");
    }
}

// Effect 137: the Spiritual Hammer was unreadied.
static void handleHammerGone(const EffectCall &c) {
    if (c.op == EFF_REMOVE)
        removeEffect(c, c.character, E_POOLRAD_SPIRIT_HAMMER);
}

// --- Tables (FxEngine.TriggerEffectsWithCondition, GDConsts) ---

struct TriggerSet {
    const uint8 *ids;
    uint count;
};

#define TRIGGER_SET(name, ...) \
    static const uint8 name[] = { __VA_ARGS__ }

TRIGGER_SET(kAttackerVeto, E_POOLRAD_GAZE_BLOCK);
TRIGGER_SET(kTargetVeto, E_POOLRAD_UNUSED_63, E_POOLRAD_BLINKING,
    E_POOLRAD_INVISIBLE, E_POOLRAD_DUST_DISAPPEAR);
TRIGGER_SET(kHitPhase1, E_POOLRAD_DRAIN_ONE, E_POOLRAD_DRAIN_TWO,
    E_POOLRAD_ROT_TOUCH, E_POOLRAD_GHOUL_PARA, E_POOLRAD_FIRE_TOUCH,
    E_POOLRAD_ACID_SQUIRT, E_POOLRAD_BLOOD_ATTACH, E_POOLRAD_REAR_CLAW_RAKE,
    E_POOLRAD_BITE_ATTACH);
TRIGGER_SET(kHitPhase2, E_POOLRAD_POISON_HIT0, E_POOLRAD_POISON_HIT4,
    E_POOLRAD_POISON_HIT2, E_POOLRAD_KREEN_PARA, E_POOLRAD_GHOUL_PARA,
    E_POOLRAD_DRIDER_PARA, E_POOLRAD_POISON_HIT_M2, E_POOLRAD_FIRE_TOUCH,
    E_POOLRAD_DRAIN_ONE, E_POOLRAD_DRAIN_TWO, E_POOLRAD_ROT_TOUCH);
TRIGGER_SET(kWeaponDamageAtt, E_POOLRAD_ENFEEBLE, E_POOLRAD_UNDEAD_SLAYER,
    E_POOLRAD_FLAME_TONGUE);
TRIGGER_SET(kWeaponDamageDef, E_POOLRAD_MIRROR_IMAGE, E_POOLRAD_PROT_MISSILES,
    E_POOLRAD_KREEN_DODGE, E_POOLRAD_BOULDER_DODGE, E_POOLRAD_TROLL_HEAL,
    E_POOLRAD_HALF_PIERCE, E_POOLRAD_HALF_MAGICAL, E_POOLRAD_NEED_MAGIC,
    E_POOLRAD_SILVER_MAGIC, E_POOLRAD_NEED_SILVER, E_POOLRAD_HALF_BASH,
    E_POOLRAD_UNUSED_60, E_POOLRAD_MUMMY_FIRE, E_POOLRAD_HOLY_WATER);
TRIGGER_SET(kNonweaponDamage, E_POOLRAD_EFREETI_FIRE, E_POOLRAD_RING_FIRE_RES,
    E_POOLRAD_MUMMY_FIRE, E_POOLRAD_UNUSED_60, E_POOLRAD_IMMUNE_MM,
    E_POOLRAD_RESIST_COLD, E_POOLRAD_RESIST_FIRE, E_POOLRAD_MAGIC_RES50,
    E_POOLRAD_MAGIC_RES85, E_POOLRAD_NO_FIRE, E_POOLRAD_HALF_ELEC,
    E_POOLRAD_HALF_COLD, E_POOLRAD_SHIELD, E_POOLRAD_HALF_FIRE,
    E_POOLRAD_TROLL_HEAL, E_POOLRAD_MIRROR_IMAGE);
TRIGGER_SET(kTurnStart, E_POOLRAD_SNAKE_CHARM, E_POOLRAD_PARALYSIS,
    E_POOLRAD_SLEEP, E_POOLRAD_HELPLESS);
TRIGGER_SET(kCombatStart, E_POOLRAD_CAN_FIGHT_ON, E_POOLRAD_DRAGON_FEAR,
    E_POOLRAD_MUMMY_FEAR, E_POOLRAD_DISPLACED, E_POOLRAD_CAMOUFLAGE,
    E_POOLRAD_RING_INVIS);
TRIGGER_SET(kEffectImmunity, E_POOLRAD_MAGIC_RES50, E_POOLRAD_MAGIC_RES85,
    E_POOLRAD_ELF_RESIST, E_POOLRAD_NO_SLEEP_CHARM, E_POOLRAD_NO_PARALYSIS,
    E_POOLRAD_NO_COLD, E_POOLRAD_NO_PARA_POISON, E_POOLRAD_NO_FIRE,
    E_POOLRAD_HALF_ELF_RESIST, E_POOLRAD_NO_CHARM_POISON);
TRIGGER_SET(kAttackRollAtt, E_POOLRAD_BLESS, E_POOLRAD_CURSE, E_POOLRAD_BLIND,
    E_POOLRAD_BESTOW_CURSE, E_POOLRAD_PRAYER, E_POOLRAD_UNDEAD_SLAYER,
    E_POOLRAD_FLAME_TONGUE, E_POOLRAD_GNOME_THAC0, E_POOLRAD_DWARF_THAC0);
TRIGGER_SET(kDefenderAc, E_POOLRAD_BLIND, E_POOLRAD_SHIELD, E_POOLRAD_PROT_EVIL,
    E_POOLRAD_PROT_GOOD, E_POOLRAD_PROT_EVIL_10FT, E_POOLRAD_PROT_GOOD_10FT,
    E_POOLRAD_STINK_COUGH);
TRIGGER_SET(kSavingThrow, E_POOLRAD_PROT_EVIL, E_POOLRAD_PROT_GOOD,
    E_POOLRAD_RESIST_COLD, E_POOLRAD_SHIELD, E_POOLRAD_RESIST_FIRE,
    E_POOLRAD_BLIND, E_POOLRAD_BESTOW_CURSE, E_POOLRAD_PROT_EVIL_10FT,
    E_POOLRAD_PROT_GOOD_10FT, E_POOLRAD_PRAYER, E_POOLRAD_RING_FIRE_RES,
    E_POOLRAD_NO_PARA_POISON, E_POOLRAD_NO_CHARM_POISON,
    E_POOLRAD_CON_PSN_SAVE, E_POOLRAD_CON_SPELL_SAVE);
TRIGGER_SET(kHpZero, E_POOLRAD_CAN_FIGHT_ON, E_POOLRAD_TROLL_BURN,
    E_POOLRAD_GASEOUS, E_POOLRAD_BLOOD_DRAIN, E_POOLRAD_BITE_HOLD);
TRIGGER_SET(kMonsterSpecial, E_POOLRAD_PETRIFY_GAZE, E_POOLRAD_CHARM_GAZE,
    E_POOLRAD_DRAGON_BREATH, E_POOLRAD_ANHKHEG_ACID);
TRIGGER_SET(kActionStart, E_POOLRAD_SILENCE, E_POOLRAD_STINK_COUGH,
    E_POOLRAD_BITE_HOLD, E_POOLRAD_BLOOD_DRAIN, E_POOLRAD_CHARM);
TRIGGER_SET(kAttackRollDef, E_POOLRAD_INVISIBLE, E_POOLRAD_DUST_DISAPPEAR,
    E_POOLRAD_BLINKING, E_POOLRAD_DWARF_AC_GIANT, E_POOLRAD_GNOME_AC_BUG,
    E_POOLRAD_DISPLACED);
TRIGGER_SET(kMoraleCheck, E_POOLRAD_BLESS, E_POOLRAD_CURSE, E_POOLRAD_CHARM);
TRIGGER_SET(kMoveAttackRate, E_POOLRAD_HASTED, E_POOLRAD_SLOWED,
    E_POOLRAD_IMMOBILE);
TRIGGER_SET(kRoundEnd, E_POOLRAD_REGEN3, E_POOLRAD_SPIRIT_HAMMER,
    E_POOLRAD_CAMOUFLAGE, E_POOLRAD_RING_INVIS, E_POOLRAD_CHARM);

#undef TRIGGER_SET

// Indexed by C_Cond (= EffectSet).
static const TriggerSet kTriggerSets[] = {
    { kAttackerVeto, ARRAYSIZE(kAttackerVeto) },
    { kTargetVeto, ARRAYSIZE(kTargetVeto) },
    { kHitPhase1, ARRAYSIZE(kHitPhase1) },
    { kHitPhase2, ARRAYSIZE(kHitPhase2) },
    { kWeaponDamageAtt, ARRAYSIZE(kWeaponDamageAtt) },
    { kWeaponDamageDef, ARRAYSIZE(kWeaponDamageDef) },
    { kNonweaponDamage, ARRAYSIZE(kNonweaponDamage) },
    { kTurnStart, ARRAYSIZE(kTurnStart) },
    { kCombatStart, ARRAYSIZE(kCombatStart) },
    { kEffectImmunity, ARRAYSIZE(kEffectImmunity) },
    { kAttackRollAtt, ARRAYSIZE(kAttackRollAtt) },
    { kDefenderAc, ARRAYSIZE(kDefenderAc) },
    { kSavingThrow, ARRAYSIZE(kSavingThrow) },
    { kHpZero, ARRAYSIZE(kHpZero) },
    { kMonsterSpecial, ARRAYSIZE(kMonsterSpecial) },
    { kActionStart, ARRAYSIZE(kActionStart) },
    { kAttackRollDef, ARRAYSIZE(kAttackRollDef) },
    { kMoraleCheck, ARRAYSIZE(kMoraleCheck) },
    { kMoveAttackRate, ARRAYSIZE(kMoveAttackRate) },
    { kRoundEnd, ARRAYSIZE(kRoundEnd) }
};

static_assert(ARRAYSIZE(kTriggerSets) == ES_SPELL_POST_PROCESS + 1,
    "One trigger set per C_Cond value");

// GDConsts.hpZeroEffects
static const uint8 kHpZeroEffects[] = {
    E_POOLRAD_TRAIN_MANUAL, E_POOLRAD_CHARM, E_POOLRAD_STINK_COUGH,
    E_POOLRAD_HELPLESS, E_POOLRAD_ANIMATE_DEAD, E_POOLRAD_SNAKE_CHARM,
    E_POOLRAD_PARALYSIS, E_POOLRAD_SLEEP, E_POOLRAD_DRAGON_AURA,
    E_POOLRAD_IMMOBILE, E_POOLRAD_TROLL_REGEN, E_POOLRAD_FIGHT_ON,
    E_POOLRAD_REGEN3, E_POOLRAD_SPIRIT_HAMMER_GONE, E_POOLRAD_BITE_HOLD,
    E_POOLRAD_BLOOD_DRAIN
};

} // namespace

// --- FxEngine.ConsiderStinkingCloud ---

void checkCloudEffect(Goldbox::Data::PlayerCharacter &ch,
                      Goldbox::Data::Effects::EffectHandlerBase *handler,
                      Goldbox::Data::Effects::EffectHostBridge *bridge) {
    if (!ch.enabled)
        return;

    Combat::CombatContext *ctx = combatContext();
    if (!ctx)
        return;

    int occupant = 0;
    uint8 groundTile = 0;
    ctx->getGroundInfo(&ch, 0xFF, &occupant, &groundTile);
    if (groundTile != Combat::CloudEffectManager::kTileCloud)
        return;

    if (hasEffect(ch, E_POOLRAD_STINK_COUGH) ||
            hasEffect(ch, E_POOLRAD_HELPLESS) ||
            hasEffect(ch, E_POOLRAD_ANIMATE_DEAD) ||
            hasEffect(ch, E_POOLRAD_NO_PARA_POISON) ||
            hasEffect(ch, E_POOLRAD_NO_CHARM_POISON))
        return;

    Effect none;
    none.id = 0;
    none.durationMin = 0;
    none.power = 0;
    none.immediate = 0;
    Goldbox::Data::DamageSystem damageSystem(bridge);
    EffectCall c(EFF_ADD, none, ch, &ctx->globals, bridge, &damageSystem,
        handler);

    const bool saved = rollSavingThrow(c, ch, Goldbox::Data::Spells::SVS_POISON, 0);
    CharacterEffects *fx = ch.getEffects();
    if (saved) {
        addEffectWithMessage(c, ch, E_POOLRAD_STINK_COUGH, 1, 0xFF, false,
            false, saved, "starts to cough");
        callEffectHandler(c, E_POOLRAD_STINK_COUGH, ch,
            fx ? fx->findEffectById(E_POOLRAD_STINK_COUGH) : nullptr, false);
    } else {
        addEffectWithMessage(c, ch, E_POOLRAD_HELPLESS, rollDice(1, 4) + 1,
            0xFF, false, false, saved, "chokes and gags from nausea");
        callEffectHandler(c, E_POOLRAD_HELPLESS, ch,
            fx ? fx->findEffectById(E_POOLRAD_HELPLESS) : nullptr, false);
    }
}

bool checkUnaffectedEffect(Goldbox::Data::PlayerCharacter &ch,
                           const EffectHandlerBase *handler,
                           EffectHostBridge *bridge,
                           Goldbox::Combat::CombatGlobals *combat,
                           uint8 effectId, uint16 duration, uint8 power,
                           bool immediate, bool checkSave, bool saved,
                           const char *message) {
    Effect none;
    none.id = effectId;
    none.durationMin = duration;
    none.power = power;
    none.immediate = immediate ? 1 : 0;
    Goldbox::Data::DamageSystem damageSystem(bridge);
    EffectCall c(EFF_ADD, none, ch, combat, bridge, &damageSystem, handler);
    return addEffectWithMessage(c, ch, effectId, duration, power, immediate,
        checkSave, saved, message);
}

EffectHandler::EffectHandler() {
    setupHandlers();
}

void EffectHandler::setupHandlers() {
    clearHandlers();
    setDefaultHandler(&EffectHandler::handleNoop);

    // EffHndlr.InitEffectHandlers. Every id 1..139 is registered by its
    // raw value, so no generic id mapping is involved.
    static const struct {
        uint8 id;
        Handler handler;
    } kHandlers[] = {
        { E_POOLRAD_BLESS, handleBless },
        { E_POOLRAD_CURSE, handleCurse },
        { E_POOLRAD_UNDEAD_SLAYER, handleUndeadSlayer },
        { E_POOLRAD_TRAIN_START, handleTrainStart },
        { E_POOLRAD_DETECT_MAGIC, handleNull },
        { E_POOLRAD_FLAME_TONGUE, handleFlameTongue },
        { E_POOLRAD_TRAIN_MANUAL, handleTrainManual },
        { E_POOLRAD_PROT_EVIL, handleProtEvil },
        { E_POOLRAD_PROT_GOOD, handleProtGood },
        { E_POOLRAD_RESIST_COLD, handleResistCold },
        { E_POOLRAD_CHARM, handleCharm },
        { E_POOLRAD_ENLARGE, handleExtraStr },
        { E_POOLRAD_REDUCE, handleNull },
        { E_POOLRAD_FRIENDS, handleFriends },
        { E_POOLRAD_SLOW_POISON_HIT, handleSlowPoisonHit },
        { E_POOLRAD_READ_MAGIC, handleNull },
        { E_POOLRAD_SHIELD, handleShield },
        { E_POOLRAD_GNOME_THAC0, handleGnomeThac0 },
        { E_POOLRAD_FIND_TRAPS, handleNull },
        { E_POOLRAD_RESIST_FIRE, handleResistFire },
        { E_POOLRAD_SILENCE, handleSilence },
        { E_POOLRAD_SLOW_POISON_END, handleSlowPoisonEnd },
        { E_POOLRAD_SPIRIT_HAMMER, handleSpiritHammer },
        { E_POOLRAD_SEE_INVISIBLE, handleNull },
        { E_POOLRAD_INVISIBLE, handleInvisible },
        { E_POOLRAD_DWARF_THAC0, handleDwarfThac0 },
        { E_POOLRAD_FEATHER_FALL, handleNull },
        { E_POOLRAD_MIRROR_IMAGE, handleMirrorImage },
        { E_POOLRAD_ENFEEBLE, handleEnfeeble },
        { E_POOLRAD_STINK_COUGH, handleStinkCough },
        { E_POOLRAD_HELPLESS, handleHelpless },
        { E_POOLRAD_ANIMATE_DEAD, handleAnimateDead },
        { E_POOLRAD_BLIND, handleBlind },
        { E_POOLRAD_DISEASED, handleDiseased },
        { E_POOLRAD_ALLIED_PRAYER, handleAlliedPrayer },
        { E_POOLRAD_BESTOW_CURSE, handleBestowCurse },
        { E_POOLRAD_BLINKING, handleBlinking },
        { E_POOLRAD_STRENGTH, handleExtraStr },
        { E_POOLRAD_HASTED, handleHasted },
        { E_POOLRAD_CAST_STINK, handleCastStink },
        { E_POOLRAD_PROT_MISSILES, handleProtMissiles },
        { E_POOLRAD_SLOWED, handleSlowed },
        { E_POOLRAD_DISEASE_STR, handleDiseaseStr },
        { E_POOLRAD_DISEASE_HP, handleDiseaseHp },
        { E_POOLRAD_PROT_EVIL_10FT, handleProtEvil },
        { E_POOLRAD_PROT_GOOD_10FT, handleProtGood },
        { E_POOLRAD_DWARF_AC_GIANT, handleDwarfAcGiant },
        { E_POOLRAD_GNOME_AC_BUG, handleGnomeAcBug },
        { E_POOLRAD_PRAYER, handlePrayer },
        { E_POOLRAD_ROT_NO_HEAL, handleRotNoHeal },
        { E_POOLRAD_SNAKE_CHARM, handleHelpless },
        { E_POOLRAD_PARALYSIS, handleHelpless },
        { E_POOLRAD_SLEEP, handleHelpless },
        { E_POOLRAD_DRAGON_AURA, handleNull },
        { E_POOLRAD_POISONED, handleNull },
        { E_POOLRAD_RING_INVIS, handleRingInvis },
        { E_POOLRAD_MUMMY_ROT, handleMummyRot },
        { E_POOLRAD_IMMOBILE, handleImmobile },
        { E_POOLRAD_TROLL_REGEN, handleTrollRegen },
        { E_POOLRAD_UNUSED_60, handleNull },
        { E_POOLRAD_RING_FIRE_RES, handleRingFireRes },
        { E_POOLRAD_CON_REGEN, handleConRegen },
        { E_POOLRAD_UNUSED_63, handleNull },
        { E_POOLRAD_POISON_HIT0, handlePoisonHit0 },
        { E_POOLRAD_POISON_HIT4, handlePoisonHit4 },
        { E_POOLRAD_POISON_HIT2, handlePoisonHit2 },
        { E_POOLRAD_KREEN_PARA, handleKreenPara },
        { E_POOLRAD_GHOUL_PARA, handleGhoulPara },
        { E_POOLRAD_DRIDER_PARA, handleDriderPara },
        { E_POOLRAD_POISON_HIT_M2, handlePoisonHitM2 },
        { E_POOLRAD_DUST_DISAPPEAR, handleDustDisappear },
        { E_POOLRAD_CAMOUFLAGE, handleCamouflage },
        { E_POOLRAD_REAR_CLAW_RAKE, handleRearClawRake },
        { E_POOLRAD_BITE_HOLD, handleBiteHold },
        { E_POOLRAD_BLOOD_DRAIN, handleBloodDrain },
        { E_POOLRAD_BLOOD_ATTACH, handleBloodAttach },
        { E_POOLRAD_BITE_ATTACH, handleBiteAttach },
        { E_POOLRAD_HEAL_WAKE, handleHealWake },
        { E_POOLRAD_FIRE_TOUCH, handleFireTouch },
        { E_POOLRAD_ACID_SQUIRT, handleAcidSquirt },
        { E_POOLRAD_DRAGON_FEAR, handleDragonFear },
        { E_POOLRAD_MUMMY_FEAR, handleMummyFear },
        { E_POOLRAD_PETRIFY_GAZE, handlePetrifyGaze },
        { E_POOLRAD_CHARM_GAZE, handleCharmGaze },
        { E_POOLRAD_DRAIN_ONE, handleDrainOne },
        { E_POOLRAD_DRAIN_TWO, handleDrainTwo },
        { E_POOLRAD_ROT_TOUCH, handleRotTouch },
        { E_POOLRAD_DRAGON_BREATH, handleDragonBreath },
        { E_POOLRAD_DISPLACED, handleDisplaced },
        { E_POOLRAD_CON_PSN_SAVE, handleConPsnSave },
        { E_POOLRAD_IMMUNE_MM, handleImmuneMM },
        { E_POOLRAD_UNUSED_92, handleNull },
        { E_POOLRAD_HALF_FIRE, handleHalfFire },
        { E_POOLRAD_HALF_BASH, handleHalfBash },
        { E_POOLRAD_FIGHT_ON, handleFightOn },
        { E_POOLRAD_NEED_SILVER, handleNeedSilver },
        { E_POOLRAD_CON_SPELL_SAVE, handleConSpellSave },
        { E_POOLRAD_REGEN3, handleRegen3 },
        { E_POOLRAD_CAN_FIGHT_ON, handleCanFightOn },
        { E_POOLRAD_TROLL_BURN, handleTrollBurn },
        { E_POOLRAD_TROLL_HEAL, handleTrollHeal },
        { E_POOLRAD_TROLL_RETURN, handleTrollReturn },
        { E_POOLRAD_GASEOUS, handleGaseous },
        { E_POOLRAD_KREEN_DODGE, handleKreenDodge },
        { E_POOLRAD_MAGIC_RES50, handleMagicRes50 },
        { E_POOLRAD_MAGIC_RES85, handleMagicRes85 },
        { E_POOLRAD_ELF_RESIST, handleElfResist },
        { E_POOLRAD_NO_SLEEP_CHARM, handleNoSleepCharm },
        { E_POOLRAD_NO_PARALYSIS, handleNoParalysis },
        { E_POOLRAD_NO_COLD, handleNoCold },
        { E_POOLRAD_NO_PARA_POISON, handleNoParaPoison },
        { E_POOLRAD_NO_FIRE, handleNoFire },
        { E_POOLRAD_EFREETI_FIRE, handleEfreetiFire },
        { E_POOLRAD_HALF_ELEC, handleHalfElec },
        { E_POOLRAD_HALF_PIERCE, handleHalfPierce },
        { E_POOLRAD_HALF_MAGICAL, handleHalfMagical },
        { E_POOLRAD_HOLY_WATER, handleHolyWater },
        { E_POOLRAD_HALF_COLD, handleHalfCold },
        { E_POOLRAD_NEED_MAGIC, handleNeedMagic },
        { E_POOLRAD_BOULDER_DODGE, handleBoulderDodge },
        { E_POOLRAD_ANHKHEG_ACID, handleAnhkhegAcid },
        { E_POOLRAD_MUMMY_FIRE, handleMummyFire },
        { E_POOLRAD_SILVER_MAGIC, handleSilverMagic },
        { E_POOLRAD_HALF_ELF_RESIST, handleHalfElfResist },
        { E_POOLRAD_NO_CHARM_POISON, handleNoCharmPoison },
        { E_POOLRAD_GAZE_BLOCK, handleGazeBlock },
        { E_POOLRAD_GAZE_REFLECTABLE, handleNull },
        { E_POOLRAD_READY_FEATHER_FALL, handleReadyItem },
        { E_POOLRAD_READY_FIRE_RES, handleReadyItem },
        { E_POOLRAD_READY_ITEM_130, handleReadyItem },
        { E_POOLRAD_GAUNTLETS, handleGauntlets },
        { E_POOLRAD_ALIGNED_WEAPON, handleAlignedWeapon },
        { E_POOLRAD_READY_DISPLACEMENT, handleReadyItem },
        { E_POOLRAD_READY_ELVENKIND, handleReadyItem },
        { E_POOLRAD_BOULDER, handleBoulder },
        { E_POOLRAD_READY_ITEM_136, handleReadyItem },
        { E_POOLRAD_SPIRIT_HAMMER_GONE, handleHammerGone },
        { E_POOLRAD_READY_ITEM_138, handleReadyItem },
        { E_POOLRAD_READY_RING_INVIS, handleReadyItem }
    };

    static_assert(ARRAYSIZE(kHandlers) == E_POOLRAD_LAST,
        "One handler per Pool of Radiance effect id");

    for (uint i = 0; i < ARRAYSIZE(kHandlers); ++i)
        setSpecHandler(kHandlers[i].id, kHandlers[i].handler);
}

Goldbox::Data::Effects::Effects EffectHandler::mapRawEffectId(uint8 rawId) const {
    // Every raw id has its own handler; anything else is unknown.
    (void)rawId;
    return Goldbox::Data::Effects::E_MISC;
}

const uint8 *EffectHandler::getTriggerSet(uint8 setId, uint &count) const {
    if (setId >= ARRAYSIZE(kTriggerSets)) {
        count = 0;
        return nullptr;
    }
    count = kTriggerSets[setId].count;
    return kTriggerSets[setId].ids;
}

bool EffectHandler::isGroupRadiatingEffect(uint8 rawId, uint8 &range) const {
    range = 1;
    switch (rawId) {
    case E_POOLRAD_SILENCE:
    case E_POOLRAD_PROT_EVIL_10FT:
    case E_POOLRAD_PROT_GOOD_10FT:
        return true;
    case E_POOLRAD_PRAYER:
        range = 6;
        return true;
    default:
        return false;
    }
}

const uint8 *EffectHandler::getHpZeroEffects(uint &count) const {
    count = ARRAYSIZE(kHpZeroEffects);
    return kHpZeroEffects;
}

void EffectHandler::handleNoop(const Goldbox::Data::Effects::EffectCall &) {
}

} // namespace Poolrad
} // namespace Goldbox
