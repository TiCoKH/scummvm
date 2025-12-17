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

#ifndef GOLDBOX_GFX_PIC_H
#define GOLDBOX_GFX_PIC_H

#include "common/stream.h"
#include "graphics/managed_surface.h"
#include "graphics/palette.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Gfx {

class Pic : public Graphics::ManagedSurface {

public:
	Pic(int w, int h) : Graphics::ManagedSurface(w, h) {}

	static Pic *read(Data::DaxBlockPic *daxBlock);

	/**
	 * Render the picture at the given pixel coordinates
	 * @param dst Destination surface
	 * @param x X position in pixels
	 * @param y Y position in pixels
	 */
	void draw(Graphics::ManagedSurface *dst, int x, int y) const;

	/**
	 * Render the picture at the given pixel coordinates with transparent color
	 * @param dst Destination surface
	 * @param x X position in pixels
	 * @param y Y position in pixels
	 * @param tpColorIndex Color index to treat as transparent
	 */
	void trDraw(Graphics::ManagedSurface *dst, int x, int y, uint32 tpColorIndex) const;

	/**
	 * Render the picture at the given character grid coordinates (8x8 cells)
	 * @param dst Destination surface
	 * @param charX Character column position
	 * @param charY Character row position
	 */
	void drawAtCharPos(Graphics::ManagedSurface *dst, int charX, int charY) const;

	/**
	 * Render the picture at the given character grid coordinates with transparent color
	 * @param dst Destination surface
	 * @param charX Character column position
	 * @param charY Character row position
	 * @param tpColorIndex Color index to treat as transparent
	 */
	void trDrawAtCharPos(Graphics::ManagedSurface *dst, int charX, int charY, uint32 tpColorIndex) const;

	/**
	 * Render the picture at Gold Box's 3x3-tile grid coordinates.
	 * Each tile is 3x3 pixels; rendering is offset by +1 pixel from tile origin.
	 * @param dst Destination surface
	 * @param iconX Tile column
	 * @param iconY Tile row
	 */
	void drawAtIconPos(Graphics::ManagedSurface *dst, int iconX, int iconY) const;

	/**
	 * Render at 3x3-tile grid coordinates with a transparent color.
	 * Each tile is 3x3 pixels; rendering is offset by +1 pixel from tile origin.
	 * @param dst Destination surface
	 * @param iconX Tile column
	 * @param iconY Tile row
	 * @param tpColorIndex Color index to treat as transparent
	 */
	void trDrawAtIconPos(Graphics::ManagedSurface *dst, int iconX, int iconY, uint32 tpColorIndex) const;

	/**
	 * Creates a copy of a picture
	 */
	Pic *clone() const;
};

} // namespace Gfx
} // namespace Goldbox
#endif
