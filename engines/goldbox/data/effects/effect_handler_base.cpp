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

#include "goldbox/data/effects/effect_handler_base.h"

namespace Goldbox {
namespace Data {
namespace Effects {

EffectHandlerBase *g_effectHandler = nullptr;

EffectHandlerBase::EffectHandlerBase() : _defaultHandler(nullptr) {
    g_effectHandler = this;
}

EffectHandlerBase::~EffectHandlerBase() {
    if (g_effectHandler == this)
        g_effectHandler = nullptr;
}

void EffectHandlerBase::apply(EffectOp op, Effect &effect,
        Goldbox::Data::PlayerCharacter &character,
        Combat::CombatGlobals *combat) const {
    DamageSystem damageSystem(nullptr);
    apply(EffectCall(op, effect, character, combat, nullptr,
        &damageSystem, this));
}

void EffectHandlerBase::apply(EffectOp op, Effect &effect,
        Goldbox::Data::PlayerCharacter &character,
        Combat::CombatGlobals *combat,
        EffectHostBridge *bridge) const {
    DamageSystem damageSystem(bridge);
    apply(EffectCall(op, effect, character, combat, bridge,
        &damageSystem, this));
}

void EffectHandlerBase::apply(const EffectCall &call) const {
    dispatch(call.effect.id, call);
}

void EffectHandlerBase::dispatch(uint8 effectId, const EffectCall &call) const {
    Handler handler = getRawHandler(effectId);
    if (!handler) {
        Effects internalId = mapRawEffectId(effectId);
        handler = getHandler((uint8)internalId);
    }
    if (handler)
        handler(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

bool EffectHandlerBase::hasHandler(uint8 effectType) const {
    if (_rawHandlers.contains(effectType))
        return true;

    return _handlers.contains((uint8)mapRawEffectId(effectType));
}

const uint8 *EffectHandlerBase::getTriggerSet(uint8 setId, uint &count) const {
    (void)setId;
    count = 0;
    return nullptr;
}

bool EffectHandlerBase::isGroupRadiatingEffect(uint8 rawId, uint8 &range) const {
    range = 1;
    switch (rawId) {
    case E_SILENCE_15_RADIUS:
    case E_PROT_FROM_EVIL_10_RADIUS:
    case E_PROT_FROM_GOOD_10_RADIUS:
        return true;
    case E_PRAYER:
        range = 6;
        return true;
    default:
        return false;
    }
}

const uint8 *EffectHandlerBase::getHpZeroEffects(uint &count) const {
    count = 0;
    return nullptr;
}

void EffectHandlerBase::clearHandlers() {
    _handlers.clear();
    _rawHandlers.clear();
}

void EffectHandlerBase::setHandler(Effects effectId, Handler handler) {
    _handlers.setVal((uint8)effectId, handler);
}

void EffectHandlerBase::setSpecHandler(uint8 rawId, Handler handler) {
    _rawHandlers.setVal(rawId, handler);
}

void EffectHandlerBase::setDefaultHandler(Handler handler) {
    _defaultHandler = handler;
}

EffectHandlerBase::Handler EffectHandlerBase::getHandler(uint8 effectType) const {
    if (_handlers.contains(effectType))
        return _handlers.getVal(effectType);
    return nullptr;
}

EffectHandlerBase::Handler EffectHandlerBase::getRawHandler(uint8 rawId) const {
    if (_rawHandlers.contains(rawId))
        return _rawHandlers.getVal(rawId);
    return nullptr;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
