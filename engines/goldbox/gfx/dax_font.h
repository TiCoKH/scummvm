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

#ifndef GOLDBOX_GFX_DAX_FONT_H
#define GOLDBOX_GFX_DAX_FONT_H

#include "common/rect.h"
#include "graphics/font.h"
#include "graphics/managed_surface.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Gfx {

/**
 * Implements a plain fixed width font from the 8X8D font block: square
 * glyphs of one bit per pixel, 8x8 on DOS and 16x16 on the PC-98.
 */
class DaxFont : public Graphics::Font {
private:
	Data::DaxBlock8x8D *_daxBlock;
	uint _glyphCount;
	int _glyphSize;

	void load();
	void drawChar(Graphics::Surface *dst, uint32 chr, int x, int y, uint32 color) const override;

public:
	DaxFont(Data::DaxBlock8x8D *daxBlock, int glyphSize = 8);
	~DaxFont() override {}

	/**
	 * Get the font height
	 */
	int getFontHeight() const override {
		return _glyphSize;
	}

	/**
	 * Get the maximum character width
	 */
	int getMaxCharWidth() const override {
		return _glyphSize;
	}

	/**
	 * Get the width of the given character
	 */
	int getCharWidth(uint32 chr) const override {
		return _glyphSize;
	}
};

} // namespace Gfx
} // namespace Goldbox

#endif
