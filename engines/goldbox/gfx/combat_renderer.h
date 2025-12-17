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

#ifndef GOLDBOX_GFX_COMBAT_RENDERER_H
#define GOLDBOX_GFX_COMBAT_RENDERER_H

#include "common/scummsys.h"
#include "common/hashmap.h"
#include "graphics/managed_surface.h"
#include "goldbox/gfx/icon.h"
#include "goldbox/gfx/dax_renderer.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace Gfx {

/**
 * CombatRenderer is the main dispatcher for rendering character icons in combat.
 *
 * Responsibilities:
 * 1. Icon lookup and construction (via Icon + DaxRenderer)
 * 2. Sprite rendering at specified screen/tile coordinates
 * 3. Visual effects (grey out, color filter)
 * 4. Caching for performance (optional via IconManager)
 *
 * Current implementation delegates to:
 * - Icon: sprite composition, color remapping, state management
 * - DaxRenderer: DAX block decoding abstraction
 * - Pic: rendering at pixel/tile coordinates
 */
class CombatRenderer {
public:
	CombatRenderer();
	CombatRenderer(DaxRenderer *renderer);
	~CombatRenderer();

	/**
	 * Draw a combat icon using IconRenderParams.
	 * Coordinates are computed via params.screenPixelX/Y().
	 * @param params Render parameters (icon data, state, position, effects)
	 * @param dst Destination surface
	 */
	void drawCombatIcon(const IconRenderParams &params,
	                    Graphics::ManagedSurface *dst);

	/**
	 * Draw an icon at pixel coordinates with optional color/grey effects.
	 * @param iconData Character icon data
	 * @param state Action state (ready/attack)
	 * @param flipped Horizontal mirror (true = facing left)
	 * @param x Pixel X coordinate
	 * @param y Pixel Y coordinate
	 * @param dst Destination surface
	 * @param colorOverride Color filter (-1 = no override)
	 * @param greyEffect Apply greyscale effect
	 */
	void drawIcon(const Goldbox::Data::CombatIconData &iconData,
	              IconState state,
	              IconDirection direction,
	              int x, int y,
	              Graphics::ManagedSurface *dst,
	              int32 colorOverride = -1);

	/**
	 * Draw an icon at 3x3 tile grid coordinates with optional effects.
	 * @param iconData Character icon data
	 * @param state Action state (ready/attack)
	 * @param flipped Horizontal mirror
	 * @param tileX Tile column (0-based)
	 * @param tileY Tile row (0-based)
	 * @param dst Destination surface
	 * @param colorOverride Color filter (-1 = no override)
	 * @param greyEffect Apply greyscale effect
	 */
	void drawIconAtTile(const Goldbox::Data::CombatIconData &iconData,
	                    IconState state,
	                    IconDirection direction,
	                    int tileX, int tileY,
	                    Graphics::ManagedSurface *dst,
	                    int32 colorOverride = -1);

	/**
	 * Apply color filter to a surface (modifies in-place).
	 * Remaps all non-transparent pixels to the given palette index.
	 * @param surface Surface to colorize
	 * @param colorIndex Palette index to use (0-255)
	 */
	static void applyColorFilter(Graphics::ManagedSurface *surface, uint8 colorIndex);

private:
	DaxRenderer *_renderer; // DAX abstraction (may be null; Icon will use VmInterface)
	Common::HashMap<uint32, Icon *> _iconCache; // Cache by CombatIconData hash + state + flip
	uint32 _lastIconIndex; // Optimization: track last rendered icon

	/**
	 * Compute cache key from icon data, state, and orientation.
	 */
	uint32 computeCacheKey(const Goldbox::Data::CombatIconData &iconData,
	                       IconState state,
	                       IconDirection direction) const;

	/**
	 * Get or create cached icon.
	 */
	Icon *getOrCreateIcon(const Goldbox::Data::CombatIconData &iconData,
	                      IconState state,
	                      IconDirection direction);
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_COMBAT_RENDERER_H

