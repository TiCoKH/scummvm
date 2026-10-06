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

#include "goldbox/combat/combat_turn.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_state.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combat_damage.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/items/base_items.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/items/character_inventory.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/vm_interface.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

uint8 calcAttackCountWithEvenTurnBonus(uint8 attacks, uint8 turnCounter) {
    if (turnCounter & 1)
        attacks++;
    return attacks >> 1;
}

void recalcPrimaryAttacks(Data::ADnDCharacter *adnd) {
    if (!adnd || !adnd->combatState)
        return;

    CombatContext *ctx = VmInterface::getCombatContext();
    CombatGlobals *globals = ctx ? &ctx->globals : nullptr;
    Data::Effects::EffectRuntime *effectRuntime = ctx ? ctx->params.effectRuntime : nullptr;

    const uint8 oldAttacks = adnd->curPrimaryRoll.attacks;
    adnd->curPrimaryRoll.attacks = adnd->basePrimaryRoll.attacks;

    bool isRanged = false;
    Data::Items::CharacterItem *ammoItem = nullptr;
    adnd->getRangedAttackItem(&ammoItem);

    const Data::Items::CharacterItem *weapon =
        adnd->getEquippedItem(Data::Items::Slot::S_MAIN_HAND);
    bool hasRangedWeapon = weapon && (weapon->prop().missileType != 0);

    uint8 exchangeValue;
    if (hasRangedWeapon) {
        isRanged = true;
        exchangeValue = weapon->prop().fireRate;
        if (exchangeValue < 2)
            exchangeValue = 2;
    } else {
        exchangeValue = adnd->curPrimaryRoll.attacks;
    }

    if (globals) {
        globals->effectSet18.value      = exchangeValue;
        globals->effectSet18.isMovement = false;
    }

    if (effectRuntime && adnd->getEffects())
        effectRuntime->checkEffectSet(Data::Effects::ES_COMBAT_RATE_MODIFIER,
                                      *adnd->getEffects(), *adnd, globals);

    uint8 result = calcAttackCountWithEvenTurnBonus(
        globals ? globals->effectSet18.value : exchangeValue,
        globals ? globals->turnCounter : 0);

    if (isRanged && ammoItem) {
        uint8 cap = (ammoItem->stackSize > 1) ? ammoItem->stackSize : 1;
        if (cap < result)
            result = cap;
    }

    const bool unknownBool = adnd->combatState->unknownBool;
    if (!unknownBool || result < oldAttacks ||
            (unknownBool && result < (uint8)(oldAttacks * 2) && !isRanged))
        adnd->curPrimaryRoll.attacks = result;
}

bool isSideAmbushed(Data::CombatSide side, uint8 ambushFlags) {
    switch (side) {
    case Data::CS_PARTY: return (ambushFlags & 1) != 0;
    case Data::CS_ENEMY: return (ambushFlags & 2) != 0;
    default:             return false;
    }
}

uint8 calcMoveBudget(Data::PlayerCharacter *ch) {
    CombatContext *ctx = VmInterface::getCombatContext();
    CombatGlobals *globals = ctx ? &ctx->globals : nullptr;
    Data::Effects::EffectRuntime *effectRuntime = ctx ? ctx->params.effectRuntime : nullptr;
    ECL::AddressSpace *eclMemory = ctx ? ctx->params.eclMemory : nullptr;
    const VmGlobalLayout *vmLayout = ctx ? ctx->params.vmGlobalLayout : nullptr;

    uint8 moveValue = ch->movement.current;

    // Party characters receive the VM party movement modifier.
    if (ch->combatSide == Data::CS_PARTY && eclMemory && vmLayout) {
        const VmFieldLocation field = vmLayout->field(kVmGlobalFieldPartyMoveModifier);
        if (VmLayout::isValid(field))
            moveValue += eclMemory->read8(field.vmAddr);
    }

    // Movement must be within the valid range.
    if (moveValue == 0 || moveValue > 0x60)
        moveValue = 1;

    // Effect Set 18 operates on movement in doubled units.
    if (globals) {
        globals->effectSet18.value      = moveValue * 2;
        globals->effectSet18.isMovement = true;
    }

    if (effectRuntime && ch->getEffects())
        effectRuntime->checkEffectSet(Data::Effects::ES_COMBAT_RATE_MODIFIER,
                                      *ch->getEffects(),
                                      *ch,
                                      globals);

    if (globals)
        globals->effectSet18.isMovement = false;

    return globals ? globals->effectSet18.value : (uint8)(moveValue * 2);
}

uint8 getOpposingSideMaxReach(const Data::PlayerCharacter *ch,
                              const Common::Array<Data::PlayerCharacter *> &roster) {
    const Data::CombatSide opposingSide =
        (ch->combatSide == Data::CS_PARTY) ? Data::CS_ENEMY : Data::CS_PARTY;

    uint8 maxReach = 0;
    for (uint i = 0; i < roster.size(); ++i) {
        Data::PlayerCharacter *member = roster[i];
        if (!member || !member->enabled || member->combatSide != opposingSide)
            continue;
        const uint8 reach = calcMoveBudget(member) >> 1;
        if (reach > maxReach)
            maxReach = reach;
    }
    return maxReach;
}

void initCharacterTurnState(Data::PlayerCharacter *ch) {
    if (!ch || !ch->combatState)
        return;

    CombatContext *ctx = VmInterface::getCombatContext();
    CombatGlobals *globals = ctx ? &ctx->globals : nullptr;
    Data::Effects::EffectRuntime *effectRuntime = ctx ? ctx->params.effectRuntime : nullptr;
    ECL::AddressSpace *eclMemory = ctx ? ctx->params.eclMemory : nullptr;
    const VmGlobalLayout *vmLayout = ctx ? ctx->params.vmGlobalLayout : nullptr;

    Data::CombatAction &cs = *ch->combatState;

    cs.spellId     = 0;
    cs.canCast     = true;
    cs.canUse      = true;
    cs.unknownBool = false;
    cs.attackId    = 2;

    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(ch);

    recalcPrimaryAttacks(adnd);

    // EFFECT_SET18_EXCHANGE_VALUE = sec_attack; EFFECT_SET18_EXCHANGE_MODE = false.
    // Handlers (HASTE, SLOW, IMMOBILIZED) read/write globals->effectSet18.value.
    uint8 baseAttacks = adnd ? adnd->baseSecondaryRoll.attacks : 1;

    if (globals) {
        globals->effectSet18.value      = baseAttacks;
        globals->effectSet18.isMovement = false;
    }

    if (effectRuntime && ch->getEffects())
        effectRuntime->checkEffectSet(Data::Effects::ES_COMBAT_RATE_MODIFIER,
                                      *ch->getEffects(), *ch, globals);

    if (adnd)
        adnd->curSecondaryRoll.attacks = calcAttackCountWithEvenTurnBonus(
            globals ? globals->effectSet18.value : baseAttacks,
            globals ? globals->turnCounter : 0);

    cs.maxTargets = adnd ? adnd->attackLevel : 1;

    if (!ch->enabled) {
        cs.initiative = 0;
    } else {
        int init = VmInterface::rollDice(1, 6)
                 + (int)ch->getDexSpeedBonus();
        cs.initiative = (uint8)init;
        if ((int8)cs.initiative < 1)
            cs.initiative = 1;
        uint8 ambushFlags = 0;
        if (eclMemory && vmLayout) {
            const VmFieldLocation field =
                vmLayout->field(kVmGlobalFieldCombatIsAmbush);
            if (VmLayout::isValid(field))
                ambushFlags = eclMemory->read8(field.vmAddr);
        }
        if (isSideAmbushed(ch->combatSide, ambushFlags))
            cs.initiative -= 6;
        if ((int8)cs.initiative < 0 || cs.initiative > 20)
            cs.initiative = 0;
    }

    cs.movePoints = calcMoveBudget(ch);
}

void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster) {
    for (uint i = 0; i < roster.size(); i++)
        initCharacterTurnState(roster[i]);
}

Data::PlayerCharacter *selectNextActor(
        const Common::Array<Data::PlayerCharacter *> &roster,
        const CombatGlobals &globals) {
    // Both sides must still have members for combat to continue.
    if (globals.sideCount[0] == 0 || globals.sideCount[1] == 0)
        return nullptr;

    // Find the enabled character with the highest initiative that has not
    // yet acted (initiative != 0xFF means acted; 0 means skipped/cancelled).
    Data::PlayerCharacter *best = nullptr;
    uint8 bestInitiative = 0;

    for (uint i = 0; i < roster.size(); i++) {
        Data::PlayerCharacter *ch = roster[i];
        if (!ch || !ch->enabled || !ch->combatState)
            continue;
        if (ch->combatState->initiative == 0xFF)
            continue;
        if (ch->combatState->initiative > bestInitiative) {
            bestInitiative = ch->combatState->initiative;
            best = ch;
        }
    }
    return best;
}

void resolveAttack(Data::PlayerCharacter *attacker,
                   Data::PlayerCharacter *target,
                   bool useBehindAC,
                   Data::Items::CharacterItem *item,
                   bool *result,
                   CombatViewDelegate *view) {
    CombatContext *ctx = VmInterface::getCombatContext();
    if (!ctx || !attacker->combatState || !target->combatState)
        return;

    Data::CombatAction &acs = *attacker->combatState;
    Data::CombatAction &tcs = *target->combatState;

    // Phase 1: turn target toward attacker when appropriate.
    if (tcs.attackCount < 3 && !useBehindAC) {
        uint8 targetFacing = (uint8)ctx->getFacingToward(target, attacker);
        if (ctx->isCharacterInBounds(target, false)) {
            if (view)
                view->updateCharacterFacingAndRedraw(target, targetFacing, 0, false);
        } else {
            tcs.direction = targetFacing;
        }
    }

    // Phase 2: turn attacker toward target.
    uint8 attackerFacing = (uint8)ctx->getFacingToward(attacker, target);
    acs.direction = attackerFacing;

    if (view) {
        view->drawCombatInfo(attacker);
        view->updateCharacterFacingAndRedraw(attacker, attackerFacing, 1, false);
    }

    // Phase 3: store target.
    acs.target = target;

    // Phase 4: animate explicitly supplied item.
    if (item && view)
        view->animateRangedAttack(attacker, target, item);

    // Phase 5: animate equipped ranged weapon with typeIndex 0x2F.
    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(attacker);
    if (adnd) {
        Data::Items::CharacterItem *weapon =
            adnd->getEquippedItem(Data::Items::Slot::S_MAIN_HAND);
        if (weapon && weapon->typeIndex == 0x2F && view)
            view->animateRangedAttack(attacker, target, weapon);
    }

    // Phase 6: default result.
    *result = true;

    // Phase 7: resolve attack sequence if attacker has attacks remaining.
    if (adnd &&
        (adnd->curPrimaryRoll.attacks != 0 || adnd->curSecondaryRoll.attacks != 0)) {

        Data::PlayerCharacter *savedSelected = ctx->globals.attacker;
        ctx->globals.attacker = attacker;

        resolveAttackSequence(attacker, target, useBehindAC, result, view);

        // Phase 8: consume supplied item stack.
        if (item) {
            if (item->stackSize > 0)
                --item->stackSize;

            if (item->stackSize == 0) {
                if (adnd && adnd->isEquippedRangedWeapon() && item->effect3 != 0x89) {
                    // Mirrors original: copy item to PTR_USED_ITEM list before removal
                    // so it can be recovered after combat. Engine-level used-item list
                    // not yet implemented; fall through to plain removal for now.
                    adnd->removeItem(item);
                } else {
                    adnd->removeItem(item);
                }
            }
        }

        // Phase 9: recalculate combat stats.
        adnd->recalcCombatStats();

        ctx->globals.attacker = savedSelected;
    }

    // Phase 10: post-attack cleanup, then reset turn-action state.
    if (*result) {
        acs.attackId    = 0;
        acs.unknownBool = false;
        acs.target      = nullptr;
        if (adnd) {
            adnd->curPrimaryRoll.attacks   = 0;
            adnd->curSecondaryRoll.attacks = 0;
        }
        acs.endTurn();
    }

    // Phase 11: restore attacker facing if visible.
    if (ctx->isCharacterInBounds(attacker, false) && view) {
        view->updateCharacterFacingAndRedraw(attacker, acs.direction, 1, true);
        view->updateCharacterFacingAndRedraw(attacker, acs.direction, 0, false);
    }
}

void resolveAttackSequence(Data::PlayerCharacter *attacker,
                           Data::PlayerCharacter *target,
                           bool useBehindAC,
                           bool *result,
                           CombatViewDelegate *view) {
    CombatContext *ctx = VmInterface::getCombatContext();
    CombatGlobals &globals = ctx->globals;
    Data::Effects::EffectRuntime *effectRuntime = ctx->params.effectRuntime;

    bool attackHit = false;
    bool stopAttacking = false;

    *result = false;
    globals.attacksLeft = 0;
    globals.attackRollSlots[1] = 0;
    globals.attackRollSlots[2] = 0;
    globals.attackDisplayCount[1] = 0;
    globals.attackDisplayCount[2] = 0;
    globals.damage = 0;

    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(attacker);
    Data::CombatAction &cs = *attacker->combatState;
    cs.unknownBool = true;

    // ----------------------------------------------------------------
    // Fast path: target already incapacitated.
    // ----------------------------------------------------------------
    if (target->hasNegativeEffect()) {
        // Find the highest slot that still has attacks.
        while (cs.attackId > 0 && adnd->getCurRoll(cs.attackId).attacks == 0)
            --cs.attackId;

        ++globals.attackDisplayCount[cs.attackId];

        if (view)
            view->drawAttackResult(attacker, target, 3, 0, 0, 0);
        VmInterface::soundPlay(0x08);

        if (Data::Effects::CharacterEffects *fx = attacker->getEffects()) {
            uint8 blurId = Data::Rules::getBlurEffectId();
            if (blurId != Data::Rules::EFFECT_ID_NONE)
                fx->eraseEffectById(blurId);
        }

        adnd->curPrimaryRoll.attacks   = 0;
        adnd->curSecondaryRoll.attacks = 0;

        *result = true;

    // ----------------------------------------------------------------
    // Normal attack sequence.
    // ----------------------------------------------------------------
    } else {
        // Large-target weapon swap: replace primary damage profile.
        const Data::Items::CharacterItem *weapon =
            adnd ? adnd->getEquippedItem(Data::Items::Slot::S_MAIN_HAND) : nullptr;
        if (weapon && adnd &&
            (target->iconDimension > 0x80 || (target->iconDimension & 7) > 1)) {
            const Data::Items::ItemProperty &props = weapon->prop();
            int8 oldBonus = props.dmgSmallMed.bonus;
            adnd->curPrimaryRoll.action.roll.diceNum   = props.dmgLarge.dices;
            adnd->curPrimaryRoll.action.roll.diceSides = props.dmgLarge.sides;
            adnd->curPrimaryRoll.action.modifier       = adnd->curPrimaryRoll.action.modifier
                                                         - oldBonus + props.dmgLarge.bonus;
        }

        Data::ADnDCharacter *targetAdnd = dynamic_cast<Data::ADnDCharacter *>(target);
        if (targetAdnd)
            targetAdnd->recalcCombatStats();

        if (effectRuntime && target->getEffects())
            effectRuntime->checkEffectSet(Data::Effects::ES_ALIGN_PROTECTION,
                                          *target->getEffects(), *target, &globals);

        // Backstab check.
        bool backstab = ctx->checkBackstab(attacker, target);

        // AC selection.
        if (!backstab && cs.attackCount > 1) {
            Direction facing = ctx->getFacingToward(attacker, target);
            if (facing == (Direction)cs.direction && cs.directionChange > 4)
                useBehindAC = true;
        }

        uint8 targetAC;
        if (backstab)
            targetAC = (uint8)((int)target->armorClass.getCurrent() - 2 - 2); // rear AC - 2
        else if (useBehindAC)
            targetAC = (uint8)((int)target->armorClass.getCurrent() - 2);     // rear AC
        else
            targetAC = (uint8)target->armorClass.getCurrent();                 // front AC

        adjustAcForFacingAndRange(attacker, target, &targetAC);

        uint8 attackResultMode = backstab ? 2 : (useBehindAC ? 1 : 0);

        // ----------------------------------------------------------------
        // Per-slot attack loop: slot 2 down to slot 1.
        // ----------------------------------------------------------------
        for (uint8 slot = cs.attackId; slot != 0; --slot) {
            while (adnd->getCurRoll(slot).attacks != 0 && !stopAttacking) {
                --adnd->getCurRoll(slot).attacks;
                cs.attackId = slot;
                ++globals.attackDisplayCount[slot];

                bool hit = rollToHit(attacker, target, targetAC);

                if (hit || target->hasNegativeEffect()) {
                    ++globals.attackRollSlots[slot];
                    VmInterface::soundPlay(0x08);
                    attackHit = true;

                    rollAttackDamage(attacker, target, slot);

                    if (view)
                        view->drawAttackResult(attacker, target, attackResultMode,
                                               globals.damage, globals.damage,
                                               hit ? 1 : 0);

                    if (target->enabled && effectRuntime && attacker->getEffects())
                        effectRuntime->checkEffectSet(
                                static_cast<Data::Effects::EffectSet>(slot + 1),
                                *attacker->getEffects(), *attacker, &globals);

                    if (!target->enabled)
                        stopAttacking = true;
                }
            }
        }

        if (!attackHit)
            VmInterface::soundPlay(0x0A);

        // All slots exhausted?
        *result = true;
        for (uint8 id = 1; id <= 2; ++id) {
            if (adnd->getCurRoll(id).attacks != 0) {
                *result = false;
                break;
            }
        }

        cs.maxTargets = 0;
    }
}

void rollAttackDamage(Data::PlayerCharacter *attacker,
                      Data::PlayerCharacter *target,
                      uint8 slot) {
    CombatContext *ctx = VmInterface::getCombatContext();
    CombatGlobals &globals = ctx->globals;

    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(attacker);
    if (!adnd)
        return;

    const Data::CombatRoll &roll = adnd->getCurRoll(slot);
    int total = 0;
    for (uint i = 0; i < roll.action.roll.diceNum; i++)
        total += VmInterface::rollDice(1, roll.action.roll.diceSides);
    total += roll.action.modifier;
    if (total < 1)
        total = 1;

    // Backstab multiplier: ((thiefLevel >> 2) + 2) * damage.
    if (ctx->checkBackstab(attacker, target)) {
        int thiefLevel = (int)adnd->levels[Data::C_THIEF];
        if (thiefLevel < 0)
            thiefLevel += 3;
        int multiplier = (thiefLevel >> 2) + 2;
        total = multiplier * total;
    }

    globals.behaviorFlags = 0;

    if (Data::Effects::EffectRuntime *er = ctx->params.effectRuntime) {
        if (Data::Effects::CharacterEffects *fx = attacker->getEffects())
            er->checkEffectSet(Data::Effects::ES_ATTACKER_OFFENSE, *fx, *attacker, &globals);
        if (Data::Effects::CharacterEffects *fx = target->getEffects())
            er->checkEffectSet(Data::Effects::ES_DEFENDER_REACTIVE, *fx, *target, &globals);
    }

    globals.damage = (uint8)CLIP(total, 1, 255);
    applyDamage(*ctx, target, globals.damage, DAMAGE_NORMAL, false,
                ctx->params.effectRuntime);
}

void adjustAcForFacingAndRange(Data::PlayerCharacter *attacker,
                               Data::PlayerCharacter *target,
                               uint8 *targetAC) {
    CombatContext *ctx = VmInterface::getCombatContext();
    if (!ctx || !targetAC || !target->combatState)
        return;

    // facingValue: arc distance of attacker relative to target's facing.
    // 0 = front, 1 = side, 2 = rear. Mirrors COMBAT_FindTargetFacing.
    Direction attackerDir = ctx->getFacingToward(target, attacker);
    int diff = (int)attackerDir - (int)target->combatState->direction;
    if (diff < 0) diff += 8;
    if (diff > 4) diff = 8 - diff;
    int8 facingValue = (int8)(diff >> 1); // 0, 1, or 2

    int8 effectiveValue = facingValue;

    // For ranged weapons, effectiveValue = (weaponRange - 1) / 3.
    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(attacker);
    const Data::Items::CharacterItem *weapon =
        adnd ? adnd->getEquippedItem(Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon && weapon->prop().missileType != 0) {
        uint8 range = weapon->prop().range;
        effectiveValue = (int8)((range - 1) / 3);
    }

    if (effectiveValue < facingValue) {
        facingValue -= effectiveValue;
        *targetAC += 2;
    }

    if (effectiveValue < facingValue)
        *targetAC += 3;
}




bool rollToHit(Data::PlayerCharacter *attacker,
               Data::PlayerCharacter *defender,
               uint8 targetAC) {
    CombatContext *ctx = VmInterface::getCombatContext();
    CombatGlobals &globals = ctx->globals;
    Data::Effects::EffectRuntime *effectRuntime = ctx->params.effectRuntime;
    ECL::AddressSpace *eclMemory = ctx->params.eclMemory;
    const VmGlobalLayout *vmLayout = ctx->params.vmGlobalLayout;

    // Remove Blur from attacker before rolling.
    if (Data::Effects::CharacterEffects *fx = attacker->getEffects()) {
        uint8 blurId = Data::Rules::getBlurEffectId();
        if (blurId != Data::Rules::EFFECT_ID_NONE)
            fx->eraseEffectById(blurId);
    }

    globals.attackRoll = (uint8)VmInterface::rollDice(1, 20);

    // Natural 1 always misses.
    if (globals.attackRoll <= 1)
        return false;

    // Natural 20 is promoted to 100 for the hit comparison.
    if (globals.attackRoll == 20)
        globals.attackRoll = 100;

    // Attacker effect set 10, then defender effect set 16.
    if (effectRuntime) {
        if (Data::Effects::CharacterEffects *fx = attacker->getEffects())
            effectRuntime->checkEffectSet(Data::Effects::ES_ATTACKER_TO_HIT,
                                          *fx, *attacker, &globals);
        if (Data::Effects::CharacterEffects *fx = defender->getEffects())
            effectRuntime->checkEffectSet(Data::Effects::ES_DEFENDER_TO_HIT,
                                          *fx, *defender, &globals);
    }

    // Read side-specific THAC0/damage bonus from VM globals.
    int8 thac0Bonus = 0;
    if (eclMemory && vmLayout) {
        const VmGlobalFieldId fieldId = (attacker->combatSide == Data::CS_PARTY)
            ? kVmGlobalFieldPartyThac0DmgBonus
            : kVmGlobalFieldMonsterThac0Bonus;
        const VmFieldLocation loc = vmLayout->field(fieldId);
        if (VmLayout::isValid(loc))
            thac0Bonus = (int8)eclMemory->read8(loc.vmAddr);
    }

    // (int8)attackRoll >= 0 guard mirrors the original signed comparison.
    if ((int8)globals.attackRoll < 0)
        return false;

    return (int)targetAC <= (int)attacker->thac0.getCurrent()
                          + (int)thac0Bonus
                          + (int)globals.attackRoll;
}

bool executeMultiAttack(Data::PlayerCharacter *attacker,
                        Data::PlayerCharacter *selectedTarget,
                        CombatContext &ctx,
                        CombatViewDelegate *view) {
    if (!attacker || !attacker->combatState || !selectedTarget)
        return false;

    Data::CombatAction &cs = *attacker->combatState;

    // Initial eligibility: attacker must not have reached max target count,
    // and the selected target must be a valid living combatant.
    if (cs.attackCount >= cs.maxTargets)
        return false;

    if (!selectedTarget->enabled)
        return false;

    // Multi-attack is specifically a range-1 melee sweep.
    if (ctx.getTargetRange(attacker, selectedTarget) != 1)
        return false;

    ctx.buildTargetList(attacker, 1);
    const uint8 targetCount = (uint8)ctx.targetList.targetOrder.size();

    // Scan the target list: find the selected target's position and count
    // eligible (enabled) targets.
    uint8 selectedIdx    = 0;
    uint8 eligibleCount  = 0;

    for (uint8 i = 0; i < targetCount; ++i) {
        Data::PlayerCharacter *candidate =
            ctx.table.getCharacter(ctx.targetList.targetOrder[i]);
        if (!candidate)
            continue;

        if (candidate == selectedTarget)
            selectedIdx = i;

        if (candidate->enabled)
            ++eligibleCount;
    }

    // Multi-attack only fires when eligible targets exceed current attack count.
    if (cs.attackCount >= eligibleCount)
        return false;

    // Cap by max_target.
    if (eligibleCount > cs.maxTargets)
        eligibleCount = cs.maxTargets;

    // Presentation: notify the player that the attacker sweeps.
    if (view)
        view->drawCombatInfo(attacker);

    // Move the selected target to the front of the target order so it is
    // always attacked first. This is a swap, not a shift.
    if (targetCount > 0 &&
        ctx.table.getCharacter(ctx.targetList.targetOrder[0]) != selectedTarget) {
        const int tableIdx = ctx.table.findIndex(selectedTarget);
        if (tableIdx >= 0) {
            ctx.targetList.targetOrder[selectedIdx] =
                ctx.targetList.targetOrder[0];
            ctx.targetList.targetOrder[0] = (uint8)tableIdx;
        }
    }

    // Attack each eligible target once. The loop iterates the full target
    // list while eligibleCount limits how many valid targets receive attacks.
    for (uint8 i = 0; i < targetCount; ++i) {
        if (eligibleCount == 0)
            break;

        Data::PlayerCharacter *currentTarget =
            ctx.table.getCharacter(ctx.targetList.targetOrder[i]);
        if (!currentTarget || !currentTarget->enabled)
            continue;

        ctx.applyAttackFacingChange(attacker, currentTarget);

        // Force one primary attack per target.
        cs.attackCount = 1;

        bool attackResult = false;
        resolveAttack(attacker, currentTarget, false, nullptr, &attackResult, view);

        --eligibleCount;
    }

    return true;
}

void surrenderSetup(Data::PlayerCharacter *ch) {
    Data::Effects::CharacterEffects *fx = ch ? ch->getEffects() : nullptr;
    if (!fx)
        return;
    fx->eraseEffectById(0x4A);
    fx->eraseEffectById(0x4B);
}

bool checkSurrender(Data::PlayerCharacter *ch,
                    CombatContext &ctx,
                    CombatViewDelegate *view) {
    if (!ch || !ch->combatState)
        return false;

    Data::CombatAction &cs = *ch->combatState;
    CombatGlobals &globals  = ctx.globals;

    cs.moralFailure = false;

    // Remove surrender-state effects before evaluation.
    surrenderSetup(ch);

    // Already fleeing: cannot surrender normally.
    if (cs.fleeing) {
        cs.moralFailure = true;
        // Presentation only — "is forced to flee" message.
        if (view)
            view->drawAttackResult(ch, nullptr, 0, 0, 0, 0);
        // TODO: replace drawAttackResult with a proper message delegate
        //       once a showMessage callback is added to CombatViewDelegate.
        return false;
    }

    // Only NPC values with bit 7 set (unsigned > 0x7F) participate.
    if ((uint8)ch->npc <= 0x7F)
        return false;

    // Initial morale modifier from NPC value; cap at 0x66 → 0.
    globals.moraleModifier = (int8)(ch->npc * 2);
    if ((uint8)globals.moraleModifier > 0x66)
        globals.moraleModifier = 0;

    // First effect-set evaluation (ES_SIMPLE_BLESS_CURSE = 17).
    // Handlers may modify globals.moraleModifier.
    if (ctx.params.effectRuntime && ch->getEffects())
        ctx.params.effectRuntime->checkEffectSet(
            Data::Effects::ES_SIMPLE_BLESS_CURSE,
            *ch->getEffects(), *ch, &globals);

    // First morale gate: proceed when modifier < HP-loss% or modifier == 0.
    const uint8 hpLossPercent = (ch->hitPoints.max > 0)
        ? (uint8)(100 - ((uint16)ch->hitPoints.current * 100) / ch->hitPoints.max)
        : 100;

    if ((uint8)globals.moraleModifier >= hpLossPercent && globals.moraleModifier != 0)
        return false;

    // Replace modifier with global handicap value and re-evaluate.
    globals.moraleModifier = (int8)globals.handicapValue;

    if (ctx.params.effectRuntime && ch->getEffects())
        ctx.params.effectRuntime->checkEffectSet(
            Data::Effects::ES_SIMPLE_BLESS_CURSE,
            *ch->getEffects(), *ch, &globals);

    // Second morale gate: party characters bypass the threshold comparison.
    if (ch->combatSide != Data::CS_PARTY) {
        uint8 moraleThreshold = 0;
        if (ctx.params.eclMemory && ctx.params.vmGlobalLayout) {
            const VmFieldLocation loc =
                ctx.params.vmGlobalLayout->field(kVmGlobalFieldMoraleThreshold);
            if (VmLayout::isValid(loc))
                moraleThreshold = ctx.params.eclMemory->read8(loc.vmAddr);
        }
        const uint8 thresholdValue = (uint8)(100 - moraleThreshold);
        if ((uint8)globals.moraleModifier >= thresholdValue && globals.moraleModifier != 0)
            return false;
    }

    // Compare movement reach against opposing side.
    const uint8 opposingReach = getOpposingSideMaxReach(ch, ctx.params.roster);
    const uint8 movementReach = calcMoveBudget(ch) >> 1;

    if (movementReach < opposingReach) {
        // Cannot escape: surrender if intelligent enough.
        if (ch->abilities.intelligence.current > 5) {
            if (g_combatSession)
                g_combatSession->setCharacterStatus(ch, Data::S_UNCONSCIOUS);
            // End actor turn.
            cs.endTurn();
            return true;
        }
    } else {
        // Can be reached: moral failure, remove surrender effects.
        cs.moralFailure = true;
        surrenderSetup(ch);
    }

    return false;
}

bool castSpell(Data::PlayerCharacter *actor,
               uint8 spellId,
               bool aiControl,
               CastSpellViewDelegate *view) {
    if (!actor || !actor->combatState || spellId == 0)
        return false;

    const Common::Array<Data::Spells::SpellEntry> &spells =
        Data::Rules::getSpellEntries();
    if (spellId >= spells.size())
        return false;

    const Data::Spells::SpellEntry &spell = spells[spellId];

    // Reject spells that cannot be cast during combat (c_avail == 0).
    if (spell.whenCast == Data::Spells::IN_CAMP)
        return false;

    // Player-controlled casting refreshes the combat presentation.
    if (!aiControl && view)
        view->focusCaster(actor);

    // Integer division: cast_time / 3 gives initiative units consumed.
    const uint8 castingTime = spell.castTime / 3;

    if (castingTime == 0) {
        // Immediate spell: execute and end the actor's turn.
        // TODO: call useSpell(actor, spellId, aiControl) when implemented.
        actor->combatState->endTurn();
        return true;
    }

    // Timed spell: enter casting state.
    if (!aiControl && view)
        view->showBeginsCasting(actor);

    actor->combatState->spellId = spellId;

    // Consume initiative; floor at 1 (strict < preserves original oddity).
    if (castingTime < actor->combatState->initiative)
        actor->combatState->initiative -= castingTime;
    else
        actor->combatState->initiative = 1;

    return true;
}

} // namespace Combat
} // namespace Goldbox
