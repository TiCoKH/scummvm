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

namespace Goldbox {
namespace Data {
namespace Effects {

namespace {

enum : uint8 {
    // Raw effect ids as stored in CharacterEffects::Effect::type.
    // Keep these aligned with Poolrad raw mapping in poolrad/effect_handler.cpp.
    kRawEffectMirrorImage = 0x1C, // E_MIRROR_IMAGE
    kRawEffectHaste = 0x27        // E_HASTE
};

struct EffectStackingRule {
    uint8 effectType;
    EffectStacking policy;
};

static const EffectStackingRule kStackingRules[] = {
    { kRawEffectMirrorImage, STACK_ADD },
    { kRawEffectHaste, STACK_IGNORE }
};

static void notifyBridgeOnEffectApply(EffectHostBridge *bridge,
        EffectOp op, PlayerCharacter &character,
        uint8 oldStatus, uint32 oldFlags) {
    if (!bridge)
        return;

    bool statusPanelDirty = false;
    if (oldFlags != character.effectState.flags)
        statusPanelDirty = true;

    if (oldStatus != character.healthStatus) {
        if (op == EFF_REMOVE) {
            bridge->notifyStatusChanged(&character, oldStatus,
                character.healthStatus);
        }
        statusPanelDirty = true;
    }

    if (statusPanelDirty)
        bridge->requestRefresh(EffectHostBridge::RF_STATUS_PANEL);
}

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

void EffectSystem::applyEffect(CharacterEffects &effects,
        Goldbox::Data::PlayerCharacter &character, uint8 type,
        uint16 durationMin, uint8 power, bool immediate) {
    if (!_handler)
        return;

    EffectStacking stacking = getStackingPolicy(type);
    if (stacking != STACK_ADD) {
        int idx = findEffectIndex(effects, type);
        if (idx >= 0) {
            Effect &existing = effects.effectAt(static_cast<uint>(idx));
            if (stacking == STACK_IGNORE)
                return;
            if (durationMin != 0xFFFF) {
                if (existing.durationMin == 0xFFFF)
                    existing.durationMin = durationMin;
                else if (durationMin > existing.durationMin)
                    existing.durationMin = durationMin;
            }
            if (power != 0xFF)
                existing.power = power;
            existing.immediate = immediate ? 1 : 0;
            return;
        }
    }

    effects.addEffect(type, durationMin, power, immediate ? 1 : 0);
    Effect &added = effects.lastEffect();
    if (immediate) {
        const uint8 oldStatus = character.healthStatus;
        const uint32 oldFlags = character.effectState.flags;
        _handler->apply(EFF_ADD, added, character);
        notifyBridgeOnEffectApply(_bridge, EFF_ADD, character,
            oldStatus, oldFlags);
    }
}

void EffectSystem::tick(CharacterEffects &effects,
        Goldbox::Data::PlayerCharacter &character) {
    if (!_handler)
        return;

    for (uint i = 0; i < effects.effectCount();) {
        Effect &effect = effects.effectAt(i);
        uint8 oldStatus = character.healthStatus;
        uint32 oldFlags = character.effectState.flags;
        _handler->apply(EFF_TICK, effect, character);
        notifyBridgeOnEffectApply(_bridge, EFF_TICK, character,
            oldStatus, oldFlags);

        if (effect.durationMin != 0xFFFF) {
            if (effect.durationMin > 0)
                --effect.durationMin;
            if (effect.durationMin == 0) {
                oldStatus = character.healthStatus;
                oldFlags = character.effectState.flags;
                _handler->apply(EFF_REMOVE, effect, character);
                notifyBridgeOnEffectApply(_bridge, EFF_REMOVE,
                    character, oldStatus, oldFlags);
                effects.removeEffectAt(i);
                continue;
            }
        }
        ++i;
    }
}

void EffectSystem::removeEffectsByType(CharacterEffects &effects,
        Goldbox::Data::PlayerCharacter &character, uint8 type) {
    if (!_handler)
        return;

    for (uint i = 0; i < effects.effectCount();) {
        if (effects.effectAt(i).type != type) {
            ++i;
            continue;
        }
        const uint8 oldStatus = character.healthStatus;
        const uint32 oldFlags = character.effectState.flags;
        _handler->apply(EFF_REMOVE, effects.effectAt(i), character);
        notifyBridgeOnEffectApply(_bridge, EFF_REMOVE, character,
            oldStatus, oldFlags);
        effects.removeEffectAt(i);
    }
}

EffectStacking EffectSystem::getStackingPolicy(uint8 type) const {
    for (uint i = 0; i < ARRAYSIZE(kStackingRules); ++i) {
        if (kStackingRules[i].effectType == type)
            return kStackingRules[i].policy;
    }

    return STACK_REFRESH;
}

int EffectSystem::findEffectIndex(const CharacterEffects &effects,
        uint8 type) const {
    return effects.findEffectIndexByType(type);
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
