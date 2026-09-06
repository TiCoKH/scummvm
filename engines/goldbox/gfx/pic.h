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
	 * Decode a specific frame from a multi-frame PIC DAX block.
	 * Frame data is sequential: each frame is (width * height / 2) bytes
	 * of packed nibbles following the header.
	 * @param daxBlock The PIC DAX block
	 * @param frameIdx 0-based frame index
	 * @return Decoded Pic, or nullptr if frameIdx is out of bounds
	 */
	static Pic *readFrame(Data::DaxBlockPic *daxBlock, int frameIdx);

	/**
	 * Decode a TILE-format frame remapping for combat terrain tiles.
	 * Index 0 = background (transparent) -> sentinel 255.
	 * Index 8 = black art -> remapped to 0.
	 * Drawn with trDraw(255) so background shows through.
	 */
	static Pic *readTileFrame(Data::DaxBlockPic *daxBlock, int frameIdx);

	/**
	 * Decode a single frame from a SPRIT DAX block's EGA planar data.
	 * EGA format: 4 bitplanes interleaved per scanline row.
	 * Each row is charWidth*4 bytes (one byte per plane per 8-pixel column).
	 * Color index 0 is transparent for sprites.
	 * @param spritBlock The SPRIT DAX block
	 * @param frameIdx Frame index to decode (0-based, distance selects frame)
	 * @return Decoded Pic with transparency at color 0, or nullptr on failure
	 */
	static Pic *readSpriteFrame(Data::DaxBlockSprit *spritBlock, int frameIdx);

	/**
	 * Decode a frame from an EGAPIC DAX block (PIC.DAX / FINAL.DAX).
	 * Same EGA planar layout as SPRIT but no transparency/remapping.
	 * For multi-frame blocks, frames > 0 are XOR-decoded against frame 0.
	 * @param spritBlock The EGAPIC DAX block (isXorDecode() == true)
	 * @param frameIdx Frame index to decode (0-based)
	 * @return Decoded Pic, or nullptr on failure
	 */
	static Pic *readEgaPicFrame(Data::DaxBlockSprit *spritBlock, int frameIdx);

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
