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

void EffectHandlerBase::apply(EffectOp op, Effect &effect, Goldbox::Data::PlayerCharacter &character) const {
    apply(op, effect, character, nullptr);
}

void EffectHandlerBase::apply(EffectOp op, Effect &effect,
        Goldbox::Data::PlayerCharacter &character,
        const EffectExecutionContext *ctx) const {
    apply(EffectCall0(op, effect, character, ctx));
}

void EffectHandlerBase::apply(EffectOp op, Effect &effect,
        Goldbox::Data::PlayerCharacter &character,
        const EffectExecutionContext *ctx, int32 arg0) const {
    apply(EffectCall1(op, effect, character, ctx, arg0));
}

void EffectHandlerBase::apply(EffectOp op, Effect &effect,
        Goldbox::Data::PlayerCharacter &character,
        const EffectExecutionContext *ctx, int32 arg0,
        int32 arg1) const {
    apply(EffectCall2(op, effect, character, ctx, arg0, arg1));
}

void EffectHandlerBase::apply(EffectOp op, Effect &effect,
        Goldbox::Data::PlayerCharacter &character,
        const EffectExecutionContext *ctx, int32 arg0,
        int32 arg1, int32 arg2) const {
    apply(EffectCall3(op, effect, character, ctx, arg0, arg1, arg2));
}

void EffectHandlerBase::apply(const EffectCall0 &call) const {
    Effects internalId = mapRawEffectId(call.effect.type);
    Handler0 handler = getHandler0((uint8)internalId);
    if (handler)
        handler(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

void EffectHandlerBase::apply(const EffectCall1 &call) const {
    Effects internalId = mapRawEffectId(call.effect.type);
    Handler1 handler = getHandler1((uint8)internalId);
    if (handler)
        handler(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

void EffectHandlerBase::apply(const EffectCall2 &call) const {
    Effects internalId = mapRawEffectId(call.effect.type);
    Handler2 handler = getHandler2((uint8)internalId);
    if (handler)
        handler(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

void EffectHandlerBase::apply(const EffectCall3 &call) const {
    Effects internalId = mapRawEffectId(call.effect.type);
    Handler3 handler = getHandler3((uint8)internalId);
    if (handler)
        handler(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

bool EffectHandlerBase::hasHandler(uint8 effectType) const {
    return _handlers0.contains(effectType) || _handlers1.contains(effectType)
        || _handlers2.contains(effectType) || _handlers3.contains(effectType);
}

void EffectHandlerBase::clearHandlers() {
    _handlers0.clear();
    _handlers1.clear();
    _handlers2.clear();
    _handlers3.clear();
}

void EffectHandlerBase::setHandler(Effects effectId, Handler0 handler) {
    _handlers0.setVal((uint8)effectId, handler);
}

void EffectHandlerBase::setHandler(Effects effectId, Handler1 handler) {
    _handlers1.setVal((uint8)effectId, handler);
}

void EffectHandlerBase::setHandler(Effects effectId, Handler2 handler) {
    _handlers2.setVal((uint8)effectId, handler);
}

void EffectHandlerBase::setHandler(Effects effectId, Handler3 handler) {
    _handlers3.setVal((uint8)effectId, handler);
}

void EffectHandlerBase::setDefaultHandler(Handler0 handler) {
    _defaultHandler = handler;
}

EffectHandlerBase::Handler0 EffectHandlerBase::getHandler0(uint8 effectType) const {
    if (_handlers0.contains(effectType))
        return _handlers0.getVal(effectType);
    return nullptr;
}

EffectHandlerBase::Handler1 EffectHandlerBase::getHandler1(uint8 effectType) const {
    if (_handlers1.contains(effectType))
        return _handlers1.getVal(effectType);
    return nullptr;
}

EffectHandlerBase::Handler2 EffectHandlerBase::getHandler2(uint8 effectType) const {
    if (_handlers2.contains(effectType))
        return _handlers2.getVal(effectType);
    return nullptr;
}

EffectHandlerBase::Handler3 EffectHandlerBase::getHandler3(uint8 effectType) const {
    if (_handlers3.contains(effectType))
        return _handlers3.getVal(effectType);
    return nullptr;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
