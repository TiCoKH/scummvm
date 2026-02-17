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

#ifndef GOLDBOX_POOLRAD_EFFECT_HANDLER_H
#define GOLDBOX_POOLRAD_EFFECT_HANDLER_H

#include "goldbox/data/effects/effect_handler_base.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {
class PoolradCharacter;
}

class EffectHandler : public Goldbox::Data::Effects::EffectHandlerBase {
public:
    EffectHandler();

    void setupHandlers() override;
    void apply(Goldbox::Data::Effects::EffectOp op, Goldbox::Data::Effects::Effect &effect,
               Data::PoolradCharacter &character) const;
    bool hasHandler(uint8 effectType) const;

    Goldbox::Data::Effects::Effects mapRawEffectId(uint8 rawId) const override;

private:
    static void handleNoop(Goldbox::Data::Effects::EffectOp op, Goldbox::Data::Effects::Effect &effect,
                           Goldbox::Data::PlayerCharacter &character);
    static void handleEffect(Goldbox::Data::Effects::EffectOp op, Goldbox::Data::Effects::Effect &effect,
                             Goldbox::Data::PlayerCharacter &character);
};

} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_EFFECT_HANDLER_H
