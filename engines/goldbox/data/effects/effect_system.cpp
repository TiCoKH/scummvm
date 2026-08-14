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
 */

#include "goldbox/data/effects/effect_system.h"

#include "goldbox/data/effects/effect_host_bridge.h"
#include "goldbox/data/effects/effect_notify.h"

namespace Goldbox {
namespace Data {
namespace Effects {

uint8 EffectSystem::kStatusEffects[] = { 7, 11, 30, 31, 32, 51, 52, 53, 54, 58, 59, 95, 98, 137, 74, 75 };

namespace {

struct EffectStackingRule {
    uint8 effectType;
    EffectStacking policy;
};

static const EffectStackingRule kStackingRules[] = {
    { static_cast<uint8>(E_MIRROR_IMAGE), STACK_ADD },
    { static_cast<uint8>(E_HASTE), STACK_IGNORE }
};

// Permanent effects (power == 0xFF) are never duplicated regardless of type.
static const uint8 kPermanentPower = 0xFF;

} // namespace

EffectSystem::EffectSystem(EffectHandlerBase *handler,
        EffectHostBridge *bridge) : _handler(handler), _bridge(bridge) {
}

void EffectSystem::setHandler(EffectHandlerBase *handler) {
    _handler = handler;
}

void EffectSystem::setHostBridge(EffectHostBridge *bridge) {
    _bridge = bridge;
}

void EffectSystem::addOrRefreshEffect(CharacterEffects &effects,
        Goldbox::Data::PlayerCharacter &character, uint8 id,
        uint16 durationMin, uint8 power, bool immediate) {
    if (!_handler)
        return;

    EffectStacking stacking = getStackingPolicy(id, power);
    if (stacking != STACK_ADD) {
        Effect *existing = effects.findEffectById(id);
        if (existing) {
            if (stacking == STACK_IGNORE)
                return;
            if (durationMin != 0xFFFF) {
                if (existing->durationMin == 0xFFFF)
                    existing->durationMin = durationMin;
                else if (durationMin > existing->durationMin)
                    existing->durationMin = durationMin;
            }
            if (power != 0xFF)
                existing->power = power;
            existing->immediate = immediate ? 1 : 0;
            return;
        }
    }

    Effect newEffect;
    newEffect.id = id;
    newEffect.durationMin = durationMin;
    newEffect.power = power;
    newEffect.immediate = immediate ? 1 : 0;
    effects.appendEffect(newEffect);
    Effect &added = effects.lastEffect();
    if (immediate) {
        const uint8 oldStatus = character.healthStatus;
        const uint32 oldFlags = character.effectState.flags;
        _handler->apply(EFF_ADD, added, character, nullptr, _bridge);
        character.onEffectsChanged();
        notifyBridge(_bridge, EFF_ADD, character,
            oldStatus, oldFlags, false, true);
    }
}

void EffectSystem::tick(CharacterEffects &effects,
        Goldbox::Data::PlayerCharacter &character) {
    if (!_handler)
        return;

    Common::List<Effect> &list = effects.effects();
    for (Common::List<Effect>::iterator it = list.begin(); it != list.end();) {
        Effect &effect = *it;
        uint8 oldStatus = character.healthStatus;
        uint32 oldFlags = character.effectState.flags;
        _handler->apply(EFF_TICK, effect, character, nullptr, _bridge);
        character.onEffectsChanged();
        notifyBridge(_bridge, EFF_TICK, character,
            oldStatus, oldFlags, false, true);

        if (effect.durationMin != 0xFFFF) {
            if (effect.durationMin > 0)
                --effect.durationMin;
            if (effect.durationMin == 0) {
                oldStatus = character.healthStatus;
                oldFlags = character.effectState.flags;
                _handler->apply(EFF_REMOVE, effect, character, nullptr, _bridge);
                character.onEffectsChanged();
                notifyBridge(_bridge, EFF_REMOVE,
                    character, oldStatus, oldFlags, false, true);
                it = list.erase(it);
                continue;
            }
        }
        ++it;
    }
}

bool EffectSystem::removeEffectById(Goldbox::Data::PlayerCharacter &character,
        CharacterEffects &effects, uint8 id) {
    Effect *e = effects.findEffectById(id);
    if (e)
        return removeEffectImpl(character, effects, *e);
    return false;
}

bool EffectSystem::removeEffect(Goldbox::Data::PlayerCharacter &character,
        CharacterEffects &effects, Effect &effect) {
    return removeEffectImpl(character, effects, effect);
}

bool EffectSystem::removeEffectImpl(Goldbox::Data::PlayerCharacter &character,
        CharacterEffects &effects, Effect &effect) {
    if (!_handler)
        return false;

    const uint8 oldStatus = character.healthStatus;
    const uint32 oldFlags = character.effectState.flags;
    if (effect.immediate)
        _handler->apply(EFF_REMOVE, effect, character, nullptr, _bridge);
    character.onEffectsChanged();
    notifyBridge(_bridge, EFF_REMOVE, character,
        oldStatus, oldFlags, false, true);

    Common::List<Effect> &list = effects.effects();
    for (Common::List<Effect>::iterator it = list.begin();
            it != list.end(); ++it) {
        if (&*it == &effect) {
            list.erase(it);
            break;
        }
    }
    return true;
}

EffectStacking EffectSystem::getStackingPolicy(uint8 id, uint8 power) const {
    // Permanent effects (power == 0xFF) are never duplicated regardless of type.
    if (power == kPermanentPower)
        return STACK_IGNORE;

    for (uint i = 0; i < ARRAYSIZE(kStackingRules); ++i) {
        if (kStackingRules[i].effectType == id)
            return kStackingRules[i].policy;
    }

    return STACK_REFRESH;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
