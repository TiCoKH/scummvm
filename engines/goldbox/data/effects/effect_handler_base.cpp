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
    Effects internalId = mapRawEffectId(effect.type);
    Handler handler = getHandler((uint8)internalId);
    if (!handler)
        handler = _defaultHandler;
    if (handler)
        handler(op, effect, character);
}

bool EffectHandlerBase::hasHandler(uint8 effectType) const {
    return _handlers.contains(effectType);
}

void EffectHandlerBase::clearHandlers() {
    _handlers.clear();
}

void EffectHandlerBase::setHandler(Effects effectId, Handler handler) {
    _handlers.setVal((uint8)effectId, handler);
}

void EffectHandlerBase::setDefaultHandler(Handler handler) {
    _defaultHandler = handler;
}

EffectHandlerBase::Handler EffectHandlerBase::getHandler(uint8 effectType) const {
    if (_handlers.contains(effectType))
        return _handlers.getVal(effectType);
    return nullptr;
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
