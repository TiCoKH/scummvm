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

#ifndef GOLDBOX_GFX_VIEWPORT_BACKGROUND_H
#define GOLDBOX_GFX_VIEWPORT_BACKGROUND_H

#include "common/scummsys.h"
#include "graphics/managed_surface.h"

namespace Goldbox {
namespace Gfx {

/**
 * Manages the cached background surface for the 3D first-person viewport.
 *
 * The viewport occupies an 88x88 pixel region at screen position (24,24),
 * corresponding to character-grid columns 3..13 and rows 3..13.
 *
 * The background is divided into four horizontal bands:
 *   Sky     : y 16-58  (43 rows)
 *   Skyline : y 59     ( 1 row)
 *   Horizon : y 60-61  ( 2 rows)
 *   Floor   : y 62-103 (42 rows)
 *
 * Colors are EGA palette indices. The surface is rebuilt only when colors
 * change, then blitted as a fast base layer before wall regions are drawn.
 */
class ViewportBackground {
public:
	/** Viewport geometry constants (in pixels). */
	static const int kViewportX = 24;
	static const int kViewportY = 24;
	static const int kViewportSize = 88;

	ViewportBackground();

	/**
	 * Set the four band colors. Rebuilds the cached surface only if
	 * any color actually changed.
	 * @param sky      EGA palette index for sky band
	 * @param skyline  EGA palette index for skyline band
	 * @param horizon  EGA palette index for horizon band
	 * @param floor    EGA palette index for floor band
	 */
	void set3DViewportColors(uint8 sky, uint8 skyline, uint8 horizon,
			uint8 floor);

	/**
	 * Return the cached background surface. The surface covers the full
	 * screen area (320x200) but only the viewport region is filled.
	 * Caller should blitFrom() this before drawing wall regions.
	 */
	const Graphics::ManagedSurface &surface() const { return _surface; }

	/** Current color accessors. */
	uint8 colorSky() const { return _colorSky; }
	uint8 colorSkyline() const { return _colorSkyline; }
	uint8 colorHorizon() const { return _colorHorizon; }
	uint8 colorFloor() const { return _colorFloor; }

private:
	void rebuild();

	Graphics::ManagedSurface _surface;
	uint8 _colorSky;
	uint8 _colorSkyline;
	uint8 _colorHorizon;
	uint8 _colorFloor;
};

} // namespace Gfx
} // namespace Goldbox

#endif // GOLDBOX_GFX_VIEWPORT_BACKGROUND_H
