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

class EffectHostBridge;

enum EffectStacking {
    STACK_REFRESH = 0,
    STACK_ADD = 1,
    STACK_IGNORE = 2
};

class EffectSystem {
public:
    // Raw effect IDs that affect character status display.
    static uint8 kStatusEffects[];

    explicit EffectSystem(EffectHandlerBase *handler,
            EffectHostBridge *bridge = nullptr);

    void setHandler(EffectHandlerBase *handler);
    void setHostBridge(EffectHostBridge *bridge);

    /**
     * Add or refresh a timed effect record on a character.
     *
     * This mutates the CharacterEffects list (append/refresh/ignore based on
     * stacking rules). It does not implement legacy "does this effect apply to
     * target now (possibly by party propagation)" semantics.
     */
    void addOrRefreshEffect(CharacterEffects &effects,
                            Goldbox::Data::PlayerCharacter &character,
                            uint8 type,
                            uint16 durationMin,
                            uint8 power,
                            bool immediate);

    void tick(CharacterEffects &effects,
              Goldbox::Data::PlayerCharacter &character);

    // Remove the first effect matching id, calling EFF_REMOVE if immediate.
    bool removeEffectById(Goldbox::Data::PlayerCharacter &character,
            CharacterEffects &effects, uint8 id);

    // Remove a specific effect instance, calling EFF_REMOVE if immediate.
    bool removeEffect(Goldbox::Data::PlayerCharacter &character,
            CharacterEffects &effects, Effect &effect);

private:
    EffectHandlerBase *_handler;
    EffectHostBridge *_bridge;

    bool removeEffectImpl(Goldbox::Data::PlayerCharacter &character,
            CharacterEffects &effects, Effect &effect);

    EffectStacking getStackingPolicy(uint8 type, uint8 power) const;
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_SYSTEM_H
