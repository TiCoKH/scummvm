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

#include "common/scummsys.h"
#include "common/array.h"
#include "graphics/managed_surface.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {

namespace Gfx {
	class Icon;
}

/**
 * Icon slot allocation constants (matches specification).
 */
enum IconSlot : uint16 {
	SLOT_PLAYER_START   = 0,
	SLOT_PLAYER_END     = 9,
	SLOT_OVERLAY_BUFFER = 11,
	SLOT_EDITOR_WORKING = 12,
	SLOT_RESERVED_START = 13,
	SLOT_RESERVED_END   = 24,
	SLOT_SELECTFRAME    = 25,
	SLOT_DYNAMIC_START  = 26,
	SLOT_DYNAMIC_END    = 254,
	SLOT_COUNT          = 256
};

/**
 * Icon slot lifecycle states.
 */
enum SlotState : uint8 {
	SLOT_EMPTY = 0,   // Slot is unallocated
	SLOT_READY = 1,   // Icon loaded and ready for rendering
	SLOT_IN_USE = 2   // Icon currently being rendered
};

/**
 * Icon content kind - specifies which DAX container to load from.
 */
enum IconKind : uint8 {
	ICON_KIND_HEAD   = 0,
	ICON_KIND_BODY   = 1,
	ICON_KIND_SPRITE = 2,
	ICON_KIND_CPIC   = 3
};

/**
 * IconManager provides a fixed 256-slot buffer for icon storage.
 * This matches the original Gold Box icon buffer architecture.
 *
 * Slot allocation:
 * - [0-9]:   Player character icons
 * - [11]:    Overlay/composite buffer
 * - [12]:    Editor working copy
 * - [13-24]: Reserved for system use
 * - [25]:    Editor reference icon
 * - [26-254]: Dynamic monster/NPC icons
 * - [255]:   Reserved/unused
 */
class IconManager {
public:
	IconManager();
	~IconManager();

	/**
	 * Get icon from slot by ID.
	 * @param slotId Slot index (0-255)
	 * @return Icon pointer or nullptr if slot is empty
	 */
	Gfx::Icon *getIcon(uint8 slotId);

	/**
	 * Get pic from slot by ID (for raw loaded sprites).
	 * @param slotId Slot index (0-255)
	 * @return Pic pointer or nullptr if slot is empty
	 */
	const Gfx::Pic *getPic(uint8 slotId) const;

	/**
	 * Access the underlying Pic for the Icon's ready state.
	 * @param slotId Slot index (0-255)
	 * @return Const Pic pointer or nullptr if unavailable
	 */
	const Gfx::Pic *getReadyPic(uint8 slotId) const;

	/**
	 * Access the underlying Pic for the Icon's action state.
	 * @param slotId Slot index (0-255)
	 * @return Const Pic pointer or nullptr if unavailable
	 */
	const Gfx::Pic *getActionPic(uint8 slotId) const;

	/**
	 * Draw an icon from a slot at Gold Box icon-grid coordinates.
	 * Mimics original GFX_DrawTile signature semantics.
	 * @param dst Destination surface
	 * @param slotId Icon slot ID (0-255)
	 * @param actionFlag 0 = ready, 1 = action
	 * @param reserved Currently unused (kept for compatibility)
	 * @param iconY Icon-grid Y position
	 * @param iconX Icon-grid X position
	 * @return true if drawn, false on error
	 */
	bool drawAtPos(Graphics::ManagedSurface *dst, uint8 slotId, uint8 actionFlag,
	              uint8 reserved, int iconY, int iconX);

	/**
	 * Store icon in specified slot.
	 * Takes ownership of the icon pointer.
	 * @param slotId Slot index (0-255)
	 * @param icon Icon to store (ownership transferred)
	 */
	void setIcon(uint8 slotId, Gfx::Icon *icon);

	/**
	 * Release icon from slot and free memory.
	 * @param slotId Slot index (0-255)
	 */
	void releaseIcon(uint8 slotId);

	/**
	 * Get slot state.
	 * @param slotId Slot index (0-255)
	 * @return Current state of the slot
	 */
	SlotState getSlotState(uint8 slotId) const;

	/**
	 * Check if slot is empty.
	 * @param slotId Slot index (0-255)
	 * @return true if slot is empty
	 */
	bool isSlotEmpty(uint8 slotId) const;

	/**
	 * Clear all icons and reset buffer.
	 */
	void clear();

	/**
	 * Load an icon from DAX file and store in slot.
	 * Automatically loads both ready (blockId) and action (blockId + 128) blocks.
	 * @param slotId Target slot (0-255)
	 * @param kind Icon content kind (SPRITE for COMSPR, CPIC for CPIC)
	 * @param blockId DAX block ID for ready state (action = blockId + 128)
	 * @return true if successfully loaded, false on error
	 */
	bool loadIcon(uint8 slotId, IconKind kind, uint16 blockId);

	/**
	 * Build a composite Icon from CombatIconData and store in a slot.
	 * @param slotId Target slot (0-255)
	 * @param iconData Combat icon configuration (head/body/colors/size)
	 * @param isAction Whether to build the action state icon
	 * @return true if successfully stored
	 */
	bool loadIcon(uint8 slotId, const Data::CombatIconData &iconData, bool isAction);

private:
	Common::Array<Gfx::Icon *> _iconBuffer;      // Fixed 256-slot array
	Common::Array<SlotState> _slotStates;   // Lifecycle tracking
};

} // namespace Goldbox

#endif // GOLDBOX_CORE_ICON_MANAGER_H
