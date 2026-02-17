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

namespace Goldbox {
namespace Data {
namespace Effects {

EffectSystem::EffectSystem(EffectHandlerBase *handler) : _handler(handler) {
}

void EffectSystem::setHandler(EffectHandlerBase *handler) {
    _handler = handler;
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
            Effect &existing = effects.effects()[idx];
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
    Effect &added = effects.effects().back();
    if (immediate)
        _handler->apply(EFF_ADD, added, character);
}

void EffectSystem::tick(CharacterEffects &effects,
        Goldbox::Data::PlayerCharacter &character) {
    if (!_handler)
        return;

    Common::Array<Effect> &list = effects.effects();
    for (uint i = 0; i < list.size();) {
        Effect &effect = list[i];
        _handler->apply(EFF_TICK, effect, character);

        if (effect.durationMin != 0xFFFF) {
            if (effect.durationMin > 0)
                --effect.durationMin;
            if (effect.durationMin == 0) {
                _handler->apply(EFF_REMOVE, effect, character);
                list.remove_at(i);
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

    Common::Array<Effect> &list = effects.effects();
    for (uint i = 0; i < list.size();) {
        if (list[i].type != type) {
            ++i;
            continue;
        }
        _handler->apply(EFF_REMOVE, list[i], character);
        list.remove_at(i);
    }
}

EffectStacking EffectSystem::getStackingPolicy(uint8 type) const {
    switch (type) {
    default:
        return STACK_REFRESH;
    }
}

int EffectSystem::findEffectIndex(const CharacterEffects &effects,
        uint8 type) const {
    const Common::Array<Effect> &list = effects.effects();
    for (uint i = 0; i < list.size(); ++i) {
        if (list[i].type == type)
            return static_cast<int>(i);
    }
    return -1;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
