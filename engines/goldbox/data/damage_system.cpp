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

#include "goldbox/data/damage_system.h"

#include "goldbox/combat/damage_utils.h"
#include "goldbox/combat/combat_state.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Data {

namespace {

static bool isKilledStatus(uint8 status) {
    return status == Goldbox::Data::S_DEAD
        || status == Goldbox::Data::S_GONE
        || status == Goldbox::Data::S_STONED;
}

static uint8 applyDamageModifier(uint8 amount, DamageModifier modifier,
        bool applyModifier) {
    if (!applyModifier)
        return amount;

    switch (modifier) {
    case DAMAGE_NULLIFY:
        return 0;
    case DAMAGE_HALF:
        return amount >> 1;
    case DAMAGE_NORMAL:
    default:
        return amount;
    }
}

} // namespace

DamageSystem::DamageSystem(Effects::EffectHostBridge *bridge) :
    _bridge(bridge) {
}

void DamageSystem::setHostBridge(Effects::EffectHostBridge *bridge) {
    _bridge = bridge;
}

DamageResult DamageSystem::apply(PlayerCharacter &target,
        const DamageRequest &request) const {
    DamageResult result;
    result.requested = request.amount;

    int damage = request.amount;
    if (request.savingThrow && request.applyModifier) {
        switch (request.modifier) {
        case DAMAGE_NULLIFY:
            damage = 0;
            break;

        case DAMAGE_HALF:
            damage /= 2;
            break;

        case DAMAGE_NORMAL:
        default:
            break;
        }
    } else if (!request.savingThrow) {
        const uint8 baseDamage = request.amount > 0xff
            ? 0xff
            : static_cast<uint8>(request.amount);
        damage = applyDamageModifier(baseDamage, request.modifier,
            request.applyModifier);
    }
    const uint8 rawDamage = damage <= 0
        ? 0
        : damage > 0xff
        ? 0xff
        : static_cast<uint8>(damage);
    result.applied = rawDamage;
    result.resisted = request.amount > 0 && rawDamage == 0;

    if (rawDamage == 0)
        return result;

    // Build the gameplay message before applying HP/status changes. The UI
    // layer decides when and where result.message is displayed.
    result.message = DamageUtils::buildDamageMessage(
        rawDamage, request.behaviorFlags);

    if (_bridge) {
        // All modifiers have already been resolved above. Passing the original
        // modifier here would apply it a second time in the host.
        _bridge->applyDamage(&target, rawDamage, DAMAGE_NORMAL, false);
    } else {
        target.damage(rawDamage);
    }

    result.killed = isKilledStatus(target.healthStatus);
    result.wentDown = !target.enabled;

    // The original only interrupts an active spell during GS_COMBAT.
    if (g_engine && VmInterface::getGameStatus() == GS_COMBAT
            && target.combatState
            && target.combatState->spellId != 0) {
        target.combatState->canCast = false;
        const uint8 interruptedSpellId = target.combatState->spellId;
        if (Effects::CharacterEffects *fx = target.getEffects()) {
            if (fx->findEffectById(interruptedSpellId))
                fx->eraseEffectById(interruptedSpellId);
        }
        target.combatState->spellId = 0;

        result.interruptedSpell = true;
        result.spellLostMessage = "lost a spell";
    }

    if (result.wentDown) {
        result.downMessage = result.killed ? "is killed" : "Goes Down";
        if (!result.killed && target.healthStatus == Goldbox::Data::S_DYING)
            result.downMessage += " and is Dying";
    }

    return result;
}

DamageResult DamageSystem::applyLegacy(PlayerCharacter &target,
        uint8 baseDamage,
        DamageModifier modifier,
        bool applyModifier,
        uint8 behaviorFlags) const {
    return apply(target, DamageRequest(baseDamage,
    false, modifier, applyModifier, behaviorFlags));
}

} // namespace Data
} // namespace Goldbox