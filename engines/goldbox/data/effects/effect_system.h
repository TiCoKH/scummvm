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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_SYSTEM_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_SYSTEM_H

#include "common/types.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_handler_base.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
namespace Effects {

enum EffectStacking {
    STACK_REFRESH = 0,
    STACK_ADD = 1,
    STACK_IGNORE = 2
};

class EffectSystem {
public:
    explicit EffectSystem(EffectHandlerBase *handler);

    void setHandler(EffectHandlerBase *handler);

    void applyEffect(CharacterEffects &effects,
                     Goldbox::Data::PlayerCharacter &character,
                     uint8 type,
                     uint16 durationMin,
                     uint8 power,
                     bool immediate);

    void tick(CharacterEffects &effects,
              Goldbox::Data::PlayerCharacter &character);

    void removeEffectsByType(CharacterEffects &effects,
                             Goldbox::Data::PlayerCharacter &character,
                             uint8 type);

private:
    EffectHandlerBase *_handler;

    EffectStacking getStackingPolicy(uint8 type) const;
    int findEffectIndex(const CharacterEffects &effects, uint8 type) const;
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_SYSTEM_H
