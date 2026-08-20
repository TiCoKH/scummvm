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

EffectHandlerBase::EffectHandlerBase() : _defaultHandler(nullptr) {
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
    Handler handler = getRawHandler(call.effect.id);
    if (!handler) {
        Effects internalId = mapRawEffectId(call.effect.id);
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
