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
#include "common/bitarray.h"
#include "graphics/managed_surface.h"
#include "graphics/palette.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Gfx {

class Pic : public Graphics::ManagedSurface {

public:
	Pic(int w, int h) : Graphics::ManagedSurface(w, h), _transparencyMask(nullptr), _transparentIndex(0) {}

	~Pic();

	static Pic *read(Data::DaxBlockPic *daxBlock);

	/**
	 * Read a PIC with two-pass rendering for remappable sprites.
	 * Pass 1: Decode palette (0-15); color 0 marks transparent pixels (mask created).
	 * Pass 2: Apply color remapping (e.g., 13→0 to make black drawable).
	 * Result: Color 0 is drawable after remapping, transparency preserved via mask.
	 * @param daxBlock DAX block containing sprite data
	 * @param sourceColor Source palette index to remap (usually 13 = bright magenta)
	 * @param targetColor Target palette index for remapping
	 * @return Decoded Pic with transparency mask and applied remapping
	 */
	static Pic *readWithRemapping(Data::DaxBlockPic *daxBlock, uint8 sourceColor, uint8 targetColor);

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
	 * Draw using transparency mask (if available).
	 * Respects the mask created by readWithRemapping().
	 * @param dst Destination surface
	 * @param x X position in pixels
	 * @param y Y position in pixels
	 */
	void drawWithMask(Graphics::ManagedSurface *dst, int x, int y) const;

	/**
	 * Creates a copy of a picture
	 */
	Pic *clone() const;

	/**
	 * Ensure a transparency mask exists for this pic.
	 * Allocates a mask if missing and clears it to false.
	 * @return Pointer to the mask (always non-null after call)
	 */
	Common::BitArray *ensureTransparencyMask();

	/**
	 * Get transparency mask for this pic.
	 * @return Pointer to transparency mask (nullptr if none created)
	 */
	Common::BitArray *getTransparencyMask() const { return _transparencyMask; }

	/**
	 * Check if a pixel is transparent via mask.
	 * @param x X coordinate
	 * @param y Y coordinate
	 * @return true if pixel is marked as transparent, false otherwise
	 */
	bool isPixelTransparent(int x, int y) const;

	/**
	 * Get the transparent color index for this picture.
	 * Defaults to 0 for normal PIC, 255 for remapped CTILE.
	 */
	uint8 getTransparentIndex() const { return _transparentIndex; }

	/**
	 * Set the transparent color index for this picture.
	 */
	void setTransparentIndex(uint8 idx) { _transparentIndex = idx; }

private:
	Common::BitArray *_transparencyMask;  // Mask for original color 0 pixels
	uint8 _transparentIndex;              // Current colorkey used for transparency when drawing
};

} // namespace Gfx
} // namespace Goldbox
#endif
