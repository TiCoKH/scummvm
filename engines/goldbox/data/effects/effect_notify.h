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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_NOTIFY_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_NOTIFY_H

#include "goldbox/data/effects/effect_handler_base.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
namespace Effects {

bool isStatusPanelEffect(Effects effectType);

void notifyBridge(EffectHostBridge *bridge,
        EffectOp op, PlayerCharacter &character,
        uint8 oldStatus, uint32 oldFlags,
        bool notifyStatus, bool flagsDirty);

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_NOTIFY_H
