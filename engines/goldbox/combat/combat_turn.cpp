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
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/items/base_items.h"
#include "goldbox/data/items/character_inventory.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/vm_interface.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

uint8 calcAttackCountWithEvenTurnBonus(uint8 attacks, uint8 turnCounter) {
    if (turnCounter & 1)
        attacks++;
    return attacks >> 1;
}

void recalcPrimaryAttacks(Data::ADnDCharacter *adnd, CombatGlobals *globals,
                          Data::Effects::EffectRuntime *effectRuntime) {
    if (!adnd || !adnd->combatState)
        return;

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

uint8 calcMoveBudget(const Data::PlayerCharacter *ch) {
    int budget = (int)ch->movement.current + (int)ch->effectState.mods.movement;
    if (budget < 0)   budget = 0;
    if (budget > 255) budget = 255;
    return (uint8)budget;
}

void initCharacterTurnState(Data::PlayerCharacter *ch,
                            Data::Effects::EffectRuntime *effectRuntime,
                            CombatGlobals *globals,
                            ECL::AddressSpace *eclMemory,
                            const VmGlobalLayout *vmLayout) {
    if (!ch || !ch->combatState)
        return;

    Data::CombatAction &cs = *ch->combatState;

    cs.spellId     = 0;
    cs.canCast     = true;
    cs.canUse      = true;
    cs.unknownBool = false;
    cs.attackId    = 2;

    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(ch);

    recalcPrimaryAttacks(adnd, globals, effectRuntime);

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

    // Move budget set after effect set — not part of the exchange.
    cs.movePoints = calcMoveBudget(ch);
}

void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster,
                       Data::Effects::EffectRuntime *effectRuntime,
                       CombatGlobals *globals,
                       ECL::AddressSpace *eclMemory,
                       const VmGlobalLayout *vmLayout) {
    for (uint i = 0; i < roster.size(); i++)
        initCharacterTurnState(roster[i], effectRuntime, globals,
                               eclMemory, vmLayout);
}

Data::PlayerCharacter *selectNextActor(
        const Common::Array<Data::PlayerCharacter *> &roster,
        const CombatGlobals &globals) {
    // Both sides must still have members for combat to continue.
    if (globals.sideCount[0] == 0 || globals.sideCount[1] == 0)
        return nullptr;

    // Find the eligible character with the lowest delay value.
    // delay == 0xFF means the character has already acted this round.
    Data::PlayerCharacter *best = nullptr;
    uint8 bestInitiative = 0xFF;

    for (uint i = 0; i < roster.size(); i++) {
        Data::PlayerCharacter *ch = roster[i];
        if (!ch || !ch->enabled || !ch->combatState)
            continue;
		if (ch->combatState->initiative == 0xFF)
            continue;
		if (ch->combatState->initiative < bestInitiative) {
			bestInitiative = ch->combatState->initiative;
            best = ch;
        }
    }

    return best;
}

} // namespace Combat
} // namespace Goldbox
