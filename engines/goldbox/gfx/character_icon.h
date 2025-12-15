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

#ifndef GOLDBOX_GFX_CHARACTER_ICON_H
#define GOLDBOX_GFX_CHARACTER_ICON_H

#include "common/scummsys.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
	class DaxBlockContainer;
}

namespace Gfx {

/**
 * Represents an icon size category.
 * Small icons use different sprite ranges than Large icons.
 */
enum IconSize {
	ICON_SIZE_SMALL = 1,
	ICON_SIZE_LARGE = 2
};

/**
 * Represents an icon action state.
 * Ready state uses base sprite IDs (0-13 heads, 0-31 bodies).
 * Action state adds 128 offset to sprite IDs.
 */
enum IconActionState {
	ICON_ACTION_READY = 0,     // No offset applied
	ICON_ACTION_ATTACK = 1,    // +128 offset applied
	ICON_ACTION_CASTING = 1,   // Same as attack (uses action sprites)
	ICON_ACTION_DEATH = 1      // Same as attack (uses action sprites)
};

/**
 * CharacterIcon encapsulates the visual representation of a character
 * for use in combat and other views. It handles sprite compositing,
 * color remapping, and action state management.
 *
 * Design principles:
 * - Immutable rendering: Once constructed, the icon doesn't change
 * - Reusable: Can be drawn multiple times to different surfaces
 * - Stateless rendering: No internal frame counters or animations
 * - Works directly with CombatIconData for storage/rendering
 */
class CharacterIcon {
public:
	/**
	 * Construct an icon from CombatIconData.
	 * DAX containers accessed via VmInterface.
	 * @param iconData Icon data structure with head/body/colors
	 * @param actionState Action state offset (0=ready, 128+=action)
	 */
	CharacterIcon(const Data::CombatIconData &iconData,
	              IconActionState actionState = ICON_ACTION_READY);

	~CharacterIcon();

	/**
	 * Render the icon at pixel coordinates.
	 * @param dst Destination surface
	 * @param x X position in pixels
	 * @param y Y position in pixels
	 */
	void draw(Graphics::ManagedSurface *dst, int x, int y) const;

	/**
	 * Render the icon at character grid coordinates.
	 * @param dst Destination surface
	 * @param charX Character column position
	 * @param charY Character row position
	 */
	void drawAtCharPos(Graphics::ManagedSurface *dst, int charX, int charY) const;

	/**
	 * Get the rendered icon as a Pic.
	 * @return Pointer to the internal composite Pic
	 */
	const Pic *getPic() const { return _composite; }

	/**
	 * Get the rendered icon as a surface.
	 * @return Pointer to the internal composite surface
	 */
	const Graphics::ManagedSurface *getSurface() const { return _composite; }

	/**
	 * Get icon dimensions.
	 */
	int getWidth() const;
	int getHeight() const;

	/**
	 * Get the icon size category.
	 */
	IconSize getSize() const { return _size; }

	/**
	 * Get the ready state icon (normal orientation).
	 * @return Pointer to the ready state composite Pic
	 */
	const Pic *getReadyIcon() const { return _readyIcon; }

	/**
	 * Get the action state icon (normal orientation).
	 * @return Pointer to the action state composite Pic
	 */
	const Pic *getActionIcon() const { return _actionIcon; }

	/**
	 * Get the ready state icon (vertically flipped).
	 * @return Pointer to the ready state flipped composite Pic
	 */
	const Pic *getReadyIconFlipped() const { return _readyIconFlipped; }

	/**
	 * Get the action state icon (vertically flipped).
	 * @return Pointer to the action state flipped composite Pic
	 */
	const Pic *getActionIconFlipped() const { return _actionIconFlipped; }

	// Static utility methods for icon manipulation

	/**
	 * Calculate actual sprite ID from base ID, size, and action state.
	 */
	static uint8 calculateSpriteId(uint8 baseId, uint8 size, bool isAction);

	/**
	 * Extract base ID from actual sprite ID.
	 */
	static uint8 extractBaseId(uint8 spriteId);

	/**
	 * Validate head sprite base ID.
	 */
	static bool isValidHeadBaseId(uint8 baseId);

	/**
	 * Validate body sprite base ID.
	 */
	static bool isValidBodyBaseId(uint8 baseId);

	/**
	 * Cycle to next/previous head sprite base ID (0-13).
	 */
	static uint8 nextHead(uint8 currentBaseId);
	static uint8 prevHead(uint8 currentBaseId);

	/**
	 * Cycle to next/previous body sprite base ID (0-31).
	 */
	static uint8 nextBody(uint8 currentBaseId);
	static uint8 prevBody(uint8 currentBaseId);

	/**
	 * Cycle to next/previous color index (0-15).
	 */
	static uint8 nextColor(uint8 currentColor);
	static uint8 prevColor(uint8 currentColor);

	/**
	 * Get pixel dimensions for icon size.
	 */
	static void getDimensions(uint8 size, int &width, int &height);

	// Constants
	static const uint8 SMALL_HEAD_MIN = 0;
	static const uint8 SMALL_HEAD_MAX = 13;
	static const uint8 SMALL_BODY_MIN = 0;
	static const uint8 SMALL_BODY_MAX = 31;
	static const uint8 HEAD_SPRITE_COUNT = 14;
	static const uint8 BODY_SPRITE_COUNT = 32;
	static const uint8 SIZE_OFFSET_LARGE = 64;
	static const uint8 ACTION_OFFSET = 128;
	static const uint8 SIZE_SMALL = 1;
	static const uint8 SIZE_LARGE = 2;
	static const int SMALL_ICON_WIDTH = 16;
	static const int SMALL_ICON_HEIGHT = 16;
	static const int LARGE_ICON_WIDTH = 24;
	static const int LARGE_ICON_HEIGHT = 24;
	static const uint8 COLOR_MIN = 0;
	static const uint8 COLOR_MAX = 15;

private:
	IconSize _size;
	IconActionState _actionState;
	Pic *_composite;  // Current state (for compatibility)

	// Bitmap cache: ready and action states
	Pic *_readyIcon;          // Ready state icon (normal)
	Pic *_actionIcon;         // Action state icon (normal)
	Pic *_readyIconFlipped;   // Ready state icon (vertically flipped)
	Pic *_actionIconFlipped;  // Action state icon (vertically flipped)

	/**
	 * Build the composite icon from head and body sprites with color mapping.
	 * DAX containers accessed via VmInterface.
	 * @return Composite Pic with colors applied
	 */
	Pic *buildComposite(uint8 headId, uint8 bodyId,
	                    const Data::CombatIconData &iconData);

	/**
	 * Create a vertically flipped copy of a Pic.
	 * @param source Source Pic to flip
	 * @return New flipped Pic (caller owns)
	 */
	Pic *createFlipped(const Pic *source) const;

	/**
	 * Apply color remapping to a sprite layer.
	 * @param layer The sprite to recolor (modified in place)
	 * @param colorMap Array of color indices [primary1, primary2, ...]
	 * @param colorCount Number of colors in colorMap
	 */
	void applyColorRemapping(Pic *layer, const uint8 *colorMap, uint8 colorCount) const;
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_CHARACTER_ICON_H
