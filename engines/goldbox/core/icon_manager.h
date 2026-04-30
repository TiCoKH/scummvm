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

#ifndef GOLDBOX_CORE_ICON_MANAGER_H
#define GOLDBOX_CORE_ICON_MANAGER_H

// Moved to gfx/icon_manager.h — include it directly for new code.
#include "goldbox/gfx/icon_manager.h"

namespace Goldbox {

// Backward-compatibility aliases so existing code including this header
// continues to compile without modification.
using Gfx::IconManager;
using Gfx::IconSlot;
using Gfx::SlotState;
using Gfx::SLOT_PLAYER_START;
using Gfx::SLOT_PLAYER_END;
using Gfx::SLOT_OVERLAY_BUFFER;
using Gfx::SLOT_EDITOR_WORKING;
using Gfx::SLOT_RESERVED_START;
using Gfx::SLOT_RESERVED_END;
using Gfx::SLOT_SELECTFRAME;
using Gfx::SLOT_DYNAMIC_START;
using Gfx::SLOT_DYNAMIC_END;
using Gfx::SLOT_COUNT;
using Gfx::SLOT_EMPTY;
using Gfx::SLOT_READY;
using Gfx::SLOT_IN_USE;
using Gfx::IconKind;
using Gfx::ICON_KIND_HEAD;
using Gfx::ICON_KIND_BODY;
using Gfx::ICON_KIND_SPRITE;
using Gfx::ICON_KIND_CPIC;

} // namespace Goldbox

#endif // GOLDBOX_CORE_ICON_MANAGER_H
