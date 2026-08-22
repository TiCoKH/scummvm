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
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Data {
namespace Effects {

namespace {

struct EffectStackingRule {
    uint8 effectType;
    EffectStacking policy;
};

static const EffectStackingRule kStackingRules[] = {
    { static_cast<uint8>(E_MIRROR_IMAGE), STACK_ADD },
    { static_cast<uint8>(E_HASTE), STACK_IGNORE }
};

} // namespace

EffectSystem::EffectSystem(EffectHandlerBase *handler,
        EffectHostBridge *bridge, EffectRuntime *runtime)
        : _handler(handler), _bridge(bridge), _runtime(runtime) {
}

void EffectSystem::setHandler(EffectHandlerBase *handler) {
    _handler = handler;
}

void EffectSystem::setHostBridge(EffectHostBridge *bridge) {
    _bridge = bridge;
}

void EffectSystem::setRuntime(EffectRuntime *runtime) {
    _runtime = runtime;
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

    // Snapshot id and immediate flag before calling the handler: the handler
    // (or a bridge callback it triggers) may call eraseEffectById on the same
    // node, invalidating the reference. We erase by id afterward so the node
    // is always cleaned up regardless of what the handler does.
    const uint8 savedId = effect.id;
    const uint8 wasImmediate = effect.immediate;

    const uint8 oldStatus = character.healthStatus;
    const uint32 oldFlags = character.effectState.flags;
    if (wasImmediate)
        _handler->apply(EFF_REMOVE, effect, character, nullptr, _bridge);
    character.onEffectsChanged();
    notifyBridge(_bridge, EFF_REMOVE, character,
        oldStatus, oldFlags, false, true);

    effects.eraseEffectById(savedId);
    return true;
}

void EffectSystem::setStatus(Goldbox::Data::PlayerCharacter &character,
        uint8 newStatus, const Common::String &message) {
    // 1. Display the message before any state change.
    if (_bridge && !message.empty())
        _bridge->postEffectMessage(&character, message, true);

    // 2. Guard: already in a terminal state, nothing more to do.
    if (character.healthStatus == Goldbox::Data::S_DEAD ||
            character.healthStatus == Goldbox::Data::S_GONE ||
            character.healthStatus == Goldbox::Data::S_STONED)
        return;

    // 3. Mutate character state.
    character.healthStatus = newStatus;
    character.enabled = false;
    character.hitPoints.current = 0;

    // 4. Fire EFF_REMOVE for every immediate effect, then strip the list.
    // The original removeEffect loop called the handler for each immediate
    // effect (e.g. handleCharm restores combatSide on EFF_REMOVE). We
    // replicate that here before the raw clear so cleanup handlers run.
    CharacterEffects *fx = character.getEffects();
    if (_handler && fx) {
        Common::List<Effect> &list = fx->effects();
        for (Effect &e : list) {
            if (e.immediate)
                _handler->apply(EFF_REMOVE, e, character, nullptr, _bridge);
        }
    }
    character.clearStatusEffects();

    // 5. Fire ETS_ON_DEATH trigger set (on-death passive effects).
    if (_runtime && fx)
        _runtime->checkEffectSet(ES_ON_DEATH, *fx, character);

    // 6. Notify host: combat map removal, timing delay, UI refresh.
    if (_bridge)
        _bridge->onCharacterDied(&character);
}


EffectStacking EffectSystem::getStackingPolicy(uint8 id, uint8 power) const {
    (void)power;
    for (uint i = 0; i < ARRAYSIZE(kStackingRules); ++i) {
        if (kStackingRules[i].effectType == id)
            return kStackingRules[i].policy;
    }

    return STACK_REFRESH;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
