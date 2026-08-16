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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_HOST_BRIDGE_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_HOST_BRIDGE_H

#include "common/scummsys.h"
#include "common/str.h"
#include "goldbox/data/damage_system.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;

namespace Effects {

/**
 * Bridge for effect runtime -> host communication.
 *
 * Implement this in the game host layer (e.g. Poolrad engine host) to let
 * effect execution request UI/ECL side effects without hard-coding view/VM
 * dependencies inside data/effects runtime code.
 */
class EffectHostBridge {
public:
    enum RefreshFlags : uint32 {
        RF_NONE = 0,
        RF_STATUS_PANEL = 1 << 0,
        RF_CHARACTER_PANEL = 1 << 1,
        RF_VIEWPORT = 1 << 2,
        RF_AREA_MAP = 1 << 3
    };

    virtual ~EffectHostBridge() {}

    // Effect text/message output.
    virtual void postEffectMessage(PlayerCharacter *character,
            const Common::String &text, bool withDelay) = 0;

    // Request redraw/update of UI surfaces impacted by effects.
    virtual void requestRefresh(uint32 refreshFlags) = 0;

    // Apply damage through the host combat UI path when available.
    virtual void applyDamage(PlayerCharacter *character,
            uint8 baseDamage, DamageModifier modifier,
            bool applyModifier) = 0;

    // Notify host that a character status changed due to effects.
    virtual void notifyStatusChanged(PlayerCharacter *character,
            uint8 oldStatus, uint8 newStatus) = 0;

    // Full death/incapacitation sequence: remove from combat map, wait,
    // clear message area, refresh party UI. Called after status is set.
    virtual void onCharacterDied(PlayerCharacter *character) = 0;

    // Optional VM-state signal channel (for ECL/event bus integration).
    virtual void postVmState(uint16 tag, uint16 value,
            uint8 valueType) = 0;
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_HOST_BRIDGE_H
