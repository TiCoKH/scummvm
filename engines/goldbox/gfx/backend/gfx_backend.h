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

#ifndef GOLDBOX_GFX_BACKEND_GFX_BACKEND_H
#define GOLDBOX_GFX_BACKEND_GFX_BACKEND_H

#include "common/platform.h"
#include "graphics/font.h"
#include "graphics/managed_surface.h"

namespace Goldbox {

namespace Data {
class DaxFileManager;
}

namespace Gfx {

/**
 * Platform side of the graphics: screen mode, palette, text font and the
 * platform's own artwork. Common rendering code asks the backend instead of
 * checking the platform; each platform backend stands alone.
 */
class GfxBackend {
public:
	virtual ~GfxBackend() {}

	/** Create the backend of a platform; DOS for any other platform. */
	static GfxBackend *create(Common::Platform platform);

	/** Switch to the platform's screen mode. */
	virtual void initScreen() = 0;

	/** Load the platform palette (entries 0-254). */
	virtual void setupPalette() = 0;

	/**
	 * Create the game font from the loaded DAX data or platform files.
	 * The caller owns the font.
	 */
	virtual Graphics::Font *createFont(Data::DaxFileManager &dax) = 0;

	/** Load platform artwork that isn't in the DAX files. */
	virtual void loadArtwork() {}

	/** Glyph of the game font for a character of a game string, 0xFF for none. */
	virtual uint8 mapChar(uint8 chr) const = 0;

	/**
	 * Draw the standard window frame around the interior character cells
	 * startX..endX, startY..endY.
	 * @return false to draw it with the common frame tiles.
	 */
	virtual bool drawFrame(Graphics::ManagedSurface &dst, int startX, int startY,
		int endX, int endY) const {
		return false;
	}

	/** Whether the title screens are pictures in the title DAX family. */
	virtual bool hasTitlePictures() const = 0;
};

} // namespace Gfx
} // namespace Goldbox

#endif
