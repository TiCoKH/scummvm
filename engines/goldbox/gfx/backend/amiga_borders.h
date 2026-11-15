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

#ifndef GOLDBOX_GFX_BACKEND_AMIGA_BORDERS_H
#define GOLDBOX_GFX_BACKEND_AMIGA_BORDERS_H

#include "common/path.h"
#include "graphics/managed_surface.h"

namespace Goldbox {
namespace Gfx {

/**
 * Window border bitmaps of the Amiga version, which blits these
 * (DRAW_BitmapBorder) instead of drawing the DOS 8x8 frame tiles.
 * Loaded from poolradamigagfx.dat, built by
 * devtools/create_goldbox/poolrad/create_amiga_gfx.py.
 */
class AmigaBorders {
public:
	enum ImageId {
		kTop = 0,
		kBottom = 1,
		kLeftEdge = 2,
		kRightEdge = 3,
		kCorner = 4,
		kImageCount = 5
	};

	bool load(const Common::Path &fileName);
	bool isLoaded() const { return _loaded; }

	/**
	 * Draw a frame one cell outside the interior character cells
	 * startX..endX, startY..endY, in the same order as DRAW_BitmapBorder.
	 */
	void drawFrame(Graphics::ManagedSurface &dst, int startX, int startY,
		int endX, int endY) const;

private:
	void blit(Graphics::ManagedSurface &dst, ImageId id, int x, int y,
		int w, int h) const;

	Graphics::ManagedSurface _images[kImageCount];
	bool _loaded = false;
};

} // namespace Gfx
} // namespace Goldbox

#endif
