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

#ifndef GOLDBOX_GFX_ICON_H
#define GOLDBOX_GFX_ICON_H

#include "common/scummsys.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/gfx/dax_renderer.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Data {
	class DaxBlockContainer;
	enum IconKind : uint8;
}

namespace Gfx {

/**
 * Icon content kind - specifies which DAX container to load from.
 * (Re-exported from Data namespace for convenience)
 */
enum IconKind : uint8 {
	ICON_KIND_HEAD   = 0,
	ICON_KIND_BODY   = 1,
	ICON_KIND_SPRITE = 2,
	ICON_KIND_CPIC   = 3
};

/**
 * Represents an icon size category.
 * Small icons use different sprite ranges than Large icons.
 */
enum IconSize {
	ICON_SIZE_SMALL = 1,
	ICON_SIZE_LARGE = 2
};

/**
 * Represents an icon state.
 * Ready state uses base sprite IDs (0-13 heads, 0-31 bodies).
 * Attack state adds 128 offset to sprite IDs.
 */
enum IconState {
	ICON_STATE_READY  = 0,
	ICON_STATE_ATTACK = 1
};

/**
 * Represents icon facing direction.
 * Controls horizontal flipping of the sprite.
 */
enum IconDirection {
	ICON_DIRECTION_RIGHT = 0,  // Normal orientation (not flipped)
	ICON_DIRECTION_LEFT  = 1   // Horizontally flipped
};

struct IconRenderParams {
    uint8 iconIndex;
    IconState state;
    IconDirection direction;
    int16 screenTileY;
    int16 screenTileX;
    int32 colorOverride;      // -1 = no override
    int16 pixelOffsetX;
    int16 pixelOffsetY;
    void *overrideDax;        // Optional: DAX block override (nullptr = use default)

    IconRenderParams() : iconIndex(0), state(ICON_STATE_READY), direction(ICON_DIRECTION_RIGHT),
                        screenTileY(0), screenTileX(0), colorOverride(-1),
                        pixelOffsetX(0), pixelOffsetY(0), overrideDax(nullptr) {}

    // Computed screen coordinates (in pixels)
    int32 screenPixelX() const { return screenTileX * 3 + 1 + pixelOffsetX; }
    int32 screenPixelY() const { return screenTileY * 3 + 1 + pixelOffsetY; }
};


/**
 * Icon encapsulates the visual representation of a character
 * for use in combat and other views. It handles sprite compositing,
 * color remapping, and action state management.
 *
 * Design principles:
 * - Immutable rendering: Once constructed, the icon doesn't change
 * - Reusable: Can be drawn multiple times to different surfaces
 * - Stateless rendering: No internal frame counters or animations
 * - Works directly with CombatIconData for storage/rendering
 */
class Icon {
public:
	/**
	 * Construct an icon from CombatIconData.
	 * DAX containers accessed via VmInterface.
	 * @param iconData Icon data structure with head/body/colors
	 * @param state Icon state (ready or attack)
	 */
	Icon(const Data::CombatIconData &iconData,
	              IconState state = ICON_STATE_READY);

	/**
	 * Construct an icon with an injected DaxRenderer abstraction.
	 * When provided, sprite decoding uses the renderer instead of VmInterface.
	 */
	Icon(const Data::CombatIconData &iconData,
	              DaxRenderer *renderer,
	              IconState state = ICON_STATE_READY);

	/**
	 * Construct an icon from explicit ready/action Pic surfaces.
	 * Pics are cloned internally; caller retains ownership of inputs.
	 */
	Icon(const Pic *readyPic, const Pic *actionPic);

	/**
	 * Construct an icon by loading both ready and action blocks from a DAX container.
	 * Automatically loads ready state from blockId and action state from blockId + 128.
	 * Supports COMSPR (sprites) and CPIC containers.
	 * @param blockId Base block ID for ready state (action state = blockId + 128)
	 * @param kind Container type (ICON_KIND_SPRITE for COMSPR, ICON_KIND_CPIC for CPIC)
	 */
	Icon(uint16 blockId, IconKind kind);

	~Icon();

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
	 * Draw icon at icon coordinate position using 3x3 tile grid.
	 * Icon coordinates are converted to character positions: charPos = iconPos * 3 + 1
	 * Character positions are then converted to pixels: pixelPos = charPos * 8
	 * @param surface Target surface to draw on
	 * @param iconX Icon X coordinate (0-based)
	 * @param iconY Icon Y coordinate (0-based)
	 */
	void drawAtIconPos(Graphics::ManagedSurface *surface, int iconX, int iconY) const;

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

	/**
	 * Set the icon state (ready or attack).
	 * Updates the composite pointer to the appropriate bitmap.
	 * @param state New icon state
	 */
	void setActionState(IconState state);

	/**
	 * Set the orientation (normal or flipped).
	 * Updates the composite pointer to the appropriate bitmap.
	 * @param direction Facing direction
	 */
	void setOrientation(IconDirection direction);

	/**
	 * Toggle the horizontal orientation.
	 */
	void toggleOrientation();

	/**
	 * Get current orientation state.
	 * @return Current facing direction
	 */
	IconDirection getDirection() const { return _direction; }

	/**
	 * Get current icon state.
	 * @return Current icon state
	 */
	IconState getState() const { return _state; }

	/**
	 * Update icon from new CombatIconData without recreating the object.
	 * Rebuilds all bitmaps with new sprite IDs and colors.
	 * Preserves current action state and orientation.
	 * @param iconData New icon data
	 */
	void updateFromData(const Data::CombatIconData &iconData) {
		// Clean up old bitmaps
		delete _readyIcon;
		delete _actionIcon;
		delete _readyIconFlipped;
		delete _actionIconFlipped;
		delete _readyHeadPic;
		delete _readyBodyPic;
		delete _actionHeadPic;
		delete _actionBodyPic;

		_readyIcon = nullptr;
		_actionIcon = nullptr;
		_readyIconFlipped = nullptr;
		_actionIconFlipped = nullptr;
		_readyHeadPic = nullptr;
		_readyBodyPic = nullptr;
		_actionHeadPic = nullptr;
		_actionBodyPic = nullptr;
		_composite = nullptr;

		// Update size if it changed
		_size = static_cast<IconSize>(iconData.iconSize);

		// Rebuild sprites with new data
		uint8 sizeOffset = (_size == ICON_SIZE_LARGE) ? 64 : 0;
		uint8 readyHeadId = iconData.iconHead + sizeOffset;
		uint8 readyBodyId = iconData.iconBody + sizeOffset;
		uint8 actionHeadId = readyHeadId + 128;
		uint8 actionBodyId = readyBodyId + 128;

		if (!loadBaseSprites(readyHeadId, readyBodyId, actionHeadId, actionBodyId)) {
			_readyIcon = new Pic(24, 24);
			_readyIcon->clear(0);
			_actionIcon = new Pic(24, 24);
			_actionIcon->clear(0);
		} else {
			_readyIcon = buildComposite(false, iconData);
			_actionIcon = buildComposite(true, iconData);
		}

		_readyIconFlipped = createFlipped(_readyIcon);
		_actionIconFlipped = createFlipped(_actionIcon);

		// Restore composite pointer based on current state
		updateComposite();
	}

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
	static const uint8 ICON_HEAD_MIN = 0;
	static const uint8 ICON_HEAD_MAX = 13;
	static const uint8 ICON_BODY_MIN = 0;
	static const uint8 ICON_BODY_MAX = 31;
	static const uint8 HEAD_SPRITE_COUNT = 14;
	static const uint8 BODY_SPRITE_COUNT = 32;
	static const uint8 SIZE_OFFSET_LARGE = 64;
	static const uint8 ACTION_OFFSET = 128;
	static const uint8 SIZE_SMALL = 1;
	static const uint8 SIZE_LARGE = 2;
	static const uint8 COLOR_MIN = 0;
	static const uint8 COLOR_MAX = 15;
	static const uint8 TRANSPARENT_COLOR_INDEX = 0;

private:
	IconSize _size;
	IconState _state;
	IconDirection _direction;
	DaxRenderer *_renderer; // optional decoding abstraction (nullptr when not used)
	Pic *_composite;  // Current state (for compatibility)

	// Bitmap cache: ready and action states
	Pic *_readyIcon;          // Ready state icon (normal)
	Pic *_actionIcon;         // Action state icon (normal)
	Pic *_readyIconFlipped;   // Ready state icon (vertically flipped)
	Pic *_actionIconFlipped;  // Action state icon (vertically flipped)

	// Base sprite cache: pre-made Pics from DAX, reused across all states
	Pic *_readyHeadPic;       // Ready head sprite from DAX
	Pic *_readyBodyPic;       // Ready body sprite from DAX
	Pic *_actionHeadPic;      // Action head sprite from DAX
	Pic *_actionBodyPic;      // Action body sprite from DAX

	/**
	 * Load base Pic sprites from DAX containers.
	 * Caches head and body Pics for reuse in compositing.
	 * @param headId Head sprite ID
	 * @param bodyId Body sprite ID
	 * @return true if both sprites loaded successfully
	 */
	bool loadBaseSprites(uint8 readyHeadId, uint8 readyBodyId, uint8 actionHeadId, uint8 actionBodyId);

	/**
	 * Build the composite icon from pre-made head and body Pics with color mapping.
	 * Uses cached _readyHeadPic/_readyBodyPic or action variants.
	 * @param isAction Whether to use action state sprites
	 * @param iconData Icon data with color information
	 * @return Composite Pic with colors applied
	 */
	Pic *buildComposite(bool isAction, const Data::CombatIconData &iconData);

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
	// Legacy-accurate recolorization over the final composite (head+body as one)
	void remapComposite(Pic *composite, const Data::CombatIconData &iconData) const;

	/**
	 * Debug helper: Print hexadecimal content of a Pic frame.
	 * Outputs pixel data row by row for inspection.
	 * @param pic Pic to dump
	 * @param label Label for the output
	 */
	void debugDumpPicHex(const Pic *pic, const char *label) const;

	/**
	 * Update the composite pointer based on current icon state and orientation.
	 */
	void updateComposite() {
		if (_state == ICON_STATE_READY) {
			_composite = (_direction == ICON_DIRECTION_LEFT) ? _readyIconFlipped : _readyIcon;
		} else {
			_composite = (_direction == ICON_DIRECTION_LEFT) ? _actionIconFlipped : _actionIcon;
		}
	}
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_ICON_H
