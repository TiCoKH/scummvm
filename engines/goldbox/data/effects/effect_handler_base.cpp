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
    const HandlerEntry *entry = getHandler((uint8)internalId);
    if (!entry) {
        if (_defaultHandler)
            _defaultHandler(call);
        return;
    }

    if (entry->arity == kArity0 && entry->handler.h0)
        entry->handler.h0(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

void EffectHandlerBase::apply(const EffectCall1 &call) const {
    Effects internalId = mapRawEffectId(call.effect.type);
    const HandlerEntry *entry = getHandler((uint8)internalId);
    if (!entry) {
        if (_defaultHandler)
            _defaultHandler(call);
        return;
    }

    if (entry->arity == kArity1 && entry->handler.h1)
        entry->handler.h1(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

void EffectHandlerBase::apply(const EffectCall2 &call) const {
    Effects internalId = mapRawEffectId(call.effect.type);
    const HandlerEntry *entry = getHandler((uint8)internalId);
    if (!entry) {
        if (_defaultHandler)
            _defaultHandler(call);
        return;
    }

    if (entry->arity == kArity2 && entry->handler.h2)
        entry->handler.h2(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

void EffectHandlerBase::apply(const EffectCall3 &call) const {
    Effects internalId = mapRawEffectId(call.effect.type);
    const HandlerEntry *entry = getHandler((uint8)internalId);
    if (!entry) {
        if (_defaultHandler)
            _defaultHandler(call);
        return;
    }

    if (entry->arity == kArity3 && entry->handler.h3)
        entry->handler.h3(call);
    else if (_defaultHandler)
        _defaultHandler(call);
}

bool EffectHandlerBase::hasHandler(uint8 effectType) const {
    return _handlers.contains(effectType);
}

void EffectHandlerBase::clearHandlers() {
    _handlers.clear();
}

void EffectHandlerBase::setHandler(Effects effectId, Handler0 handler) {
    HandlerEntry entry;
    entry.arity = kArity0;
    entry.handler.h0 = handler;
    _handlers.setVal((uint8)effectId, entry);
}

void EffectHandlerBase::setHandler(Effects effectId, Handler1 handler) {
    HandlerEntry entry;
    entry.arity = kArity1;
    entry.handler.h1 = handler;
    _handlers.setVal((uint8)effectId, entry);
}

void EffectHandlerBase::setHandler(Effects effectId, Handler2 handler) {
    HandlerEntry entry;
    entry.arity = kArity2;
    entry.handler.h2 = handler;
    _handlers.setVal((uint8)effectId, entry);
}

void EffectHandlerBase::setHandler(Effects effectId, Handler3 handler) {
    HandlerEntry entry;
    entry.arity = kArity3;
    entry.handler.h3 = handler;
    _handlers.setVal((uint8)effectId, entry);
}

void EffectHandlerBase::setDefaultHandler(Handler0 handler) {
    _defaultHandler = handler;
}

const EffectHandlerBase::HandlerEntry *EffectHandlerBase::getHandler(uint8 effectType) const {
    if (_handlers.contains(effectType))
        return &_handlers.getVal(effectType);
    return nullptr;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
