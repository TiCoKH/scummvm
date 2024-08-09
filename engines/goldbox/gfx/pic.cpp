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

#include "common/system.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/data/daxblock.h"

namespace Goldbox {
namespace Gfx {

Pic *Pic::read(DaxBlockPic *daxBlock) {
	int width = daxBlock->width;
	int height = daxBlock->height;
	Pic *pic = new Pic(width, height);
	// Decode the pixel data
	const uint8 *data = daxBlock->_data.begin();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x += 2) {
			uint8 byte = *data++;
			// Extract high nibble and set the corresponding pixel
			pic->setPixel(x, y, (byte & 0xF0) >> 4);
			// Extract low nibble and set the corresponding pixel
			pic->setPixel(x + 1, y, byte & 0x0F);
		}
	}
	return pic;
}

Pic *Pic::clone() const {
	Pic *copy = new Pic(w, h);
	copy->blitFrom(*this);

	return copy;
}

} // namespace Gfx
} // namespace Goldbox
