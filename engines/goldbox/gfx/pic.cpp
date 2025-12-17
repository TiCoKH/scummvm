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

Pic *Pic::read(Data::DaxBlockPic *daxBlock) {
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

void Pic::draw(Graphics::ManagedSurface *dst, int x, int y) const {
	dst->copyRectToSurface(*this, x, y, Common::Rect(0, 0, w, h));
}

void Pic::trDraw(Graphics::ManagedSurface *dst, int x, int y, uint32 tpColorIndex) const {
	// Manually copy pixels, skipping the transparent color
	for (int py = 0; py < h; ++py) {
		const byte *srcRow = (const byte *)getBasePtr(0, py);
		byte *dstRow = (byte *)dst->getBasePtr(x, y + py);

		for (int px = 0; px < w; ++px) {
			byte pixel = srcRow[px];
			if (pixel != tpColorIndex) {
				dstRow[px] = pixel;
			}
		}
	}
}

void Pic::drawAtCharPos(Graphics::ManagedSurface *dst, int charX, int charY) const {
	draw(dst, charX * 8, charY * 8);
}

void Pic::trDrawAtCharPos(Graphics::ManagedSurface *dst, int charX, int charY, uint32 tpColorIndex) const {
	trDraw(dst, charX * 8, charY * 8, tpColorIndex);
}

void Pic::drawAtIconPos(Graphics::ManagedSurface *dst, int iconX, int iconY) const {
	// Gold Box tile grid: 3x3 pixels per tile, with +1 pixel offset
	const int px = iconX * 3 + 1;
	const int py = iconY * 3 + 1;
	draw(dst, px, py);
}

void Pic::trDrawAtIconPos(Graphics::ManagedSurface *dst, int iconX, int iconY, uint32 tpColorIndex) const {
	// Gold Box tile grid: 3x3 pixels per tile, with +1 pixel offset
	const int px = iconX * 3 + 1;
	const int py = iconY * 3 + 1;
	trDraw(dst, px, py, tpColorIndex);
}

} // namespace Gfx
} // namespace Goldbox
