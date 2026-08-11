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

#include "goldbox/data/damage_utils.h"
#include "goldbox/data/combat_state.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_host_bridge.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules_types.h"

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

    if (request.amount <= 0)
        return result;

    result.requested = request.amount;

    const int beforeHp = target.hitPoints.current;
    const uint8 beforeStatus = target.healthStatus;

    bool hadSpell = false;
    if (target.combatState)
        hadSpell = target.combatState->spellId != 0;

    const uint8 rawDamage = request.amount > 0xff
        ? 0xff
        : static_cast<uint8>(request.amount);

    if (_bridge) {
        DamageModifier modifier = request.modifier;
        bool applyModifier = request.applyModifier;

        // TODO: Wire saving throw outcomes to damage modifiers.
        if (request.savingThrow) {
            applyModifier = false;
        }

        _bridge->applyDamage(&target, rawDamage, modifier, applyModifier);
    } else {
        target.damage(applyDamageModifier(rawDamage, request.modifier,
            request.applyModifier));
    }

    const int afterHp = target.hitPoints.current;
    result.applied = (beforeHp > afterHp) ? (beforeHp - afterHp) : 0;
    result.resisted = result.applied < result.requested;
    result.killed = isKilledStatus(target.healthStatus);
    result.wentDown = !target.enabled;

    result.message = DamageUtils::buildDamageMessage(
        static_cast<uint8>(MIN<int>(result.applied, 0xff)),
        request.behaviorFlags);

    if (hadSpell && target.combatState) {
        const uint8 interruptedSpellId = target.combatState->spellId;
        if (interruptedSpellId != 0) {
            if (Effects::CharacterEffects *fx = target.getEffects()) {
                int idx = fx->findEffectIndexById(interruptedSpellId);
                if (idx >= 0)
                    fx->removeEffectAt(static_cast<uint>(idx));
            }
            target.combatState->spellId = 0;
        }

        result.interruptedSpell = interruptedSpellId != 0;
        if (result.interruptedSpell)
            result.spellLostMessage = "lost a spell";
    }

    if (result.wentDown) {
        result.downMessage = result.killed ? "is killed" : "Goes Down";
        if (!result.killed && target.healthStatus == Goldbox::Data::S_DYING)
            result.downMessage += " and is Dying";
    }

    // If HP did not change but status collapsed from alive to dead,
    // preserve semantic "damage happened" by mirroring requested amount.
    if (result.applied == 0 && beforeStatus == Goldbox::Data::S_OKAY
            && isKilledStatus(target.healthStatus)) {
        result.applied = result.requested;
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