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

#include "goldbox/data/effects/effect_notify.h"

namespace Goldbox {
namespace Data {
namespace Effects {

bool isStatusPanelEffect(Effects effectType) {
    switch (effectType) {
    case E_PARALYZE:
    case E_SLEEP:
    case E_HELPLESS:
    case E_BLINDED:
    case E_POISON_DAMAGE:
    case E_POISONED:
    case E_POISON_PLUS_0:
    case E_POISON_PLUS_2:
    case E_POISON_PLUS_4:
    case E_POISON_NEG_2:
    case E_SLOW_POISON:
        return true;
    default:
        return false;
    }
}

void notifyBridge(EffectHostBridge *bridge,
        EffectOp op, PlayerCharacter &character,
        uint8 oldStatus, uint32 oldFlags,
        bool notifyStatus, bool flagsDirty) {
    if (!bridge)
        return;

    bool statusPanelDirty = false;

    if (oldFlags != character.effectState.flags && flagsDirty)
        statusPanelDirty = true;

    if (oldStatus != character.healthStatus) {
        if (notifyStatus || op == EFF_REMOVE)
            bridge->notifyStatusChanged(&character, oldStatus,
                character.healthStatus);
        statusPanelDirty = true;
    }

    if (statusPanelDirty)
        bridge->requestRefresh(EffectHostBridge::RF_STATUS_PANEL);
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox
