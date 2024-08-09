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

	static Pic *read(DaxBlockPic *daxBlock);
//	void toScreen(Graphics::Surface *dst, uint32 chr, int x, int y, uint32 color) const override;


	/**
	 * Creates a copy of a picture
	 */
	Pic *clone() const;
};

} // namespace Gfx
} // namespace Goldbox
#endif
