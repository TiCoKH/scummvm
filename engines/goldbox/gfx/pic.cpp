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

Pic::~Pic() {
	if (_transparencyMask) {
		delete _transparencyMask;
		_transparencyMask = nullptr;
	}
}

Pic *Pic::read(Data::DaxBlockPic *daxBlock) {
	int width = daxBlock->width;
	int height = daxBlock->height;
	Pic *pic = new Pic(width, height);
	pic->setTransparentIndex(0);
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

Pic *Pic::readWithRemapping(Data::DaxBlockPic *daxBlock, uint8 sourceColor, uint8 targetColor) {
	int width = daxBlock->width;
	int height = daxBlock->height;
	Pic *pic = new Pic(width, height);
	// Use sentinel colorkey 255 for original zeros
	pic->setTransparentIndex(255);

	// Pass 1: Decode pixel data and create transparency mask
	const uint8 *data = daxBlock->_data.begin();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x += 2) {
			uint8 byte = *data++;

			// High nibble
			uint8 pixelHigh = (byte & 0xF0) >> 4;
			pic->setPixel(x, y, (pixelHigh == 0) ? 255 : pixelHigh);

			// Low nibble
			uint8 pixelLow = byte & 0x0F;
			pic->setPixel(x + 1, y, (pixelLow == 0) ? 255 : pixelLow);
		}
	}

	// Pass 2: Apply color remapping
	if (sourceColor != targetColor) {
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				uint8 pixel = pic->getPixel(x, y);
				if (pixel == sourceColor) {
					pic->setPixel(x, y, targetColor);
				}
			}
		}
	}

	return pic;
}

bool Pic::isPixelTransparent(int x, int y) const {
	if (!_transparencyMask || x < 0 || y < 0 || x >= w || y >= h) {
		return false;
	}
	int maskIndex = y * w + x;
	return _transparencyMask->get(maskIndex);
}

Pic *Pic::clone() const {
	Pic *copy = new Pic(w, h);
	copy->blitFrom(*this);
	copy->setTransparentIndex(_transparentIndex);

	return copy;
}

Common::BitArray *Pic::ensureTransparencyMask() {
	if (!_transparencyMask) {
		_transparencyMask = new Common::BitArray(w * h);
	}
	return _transparencyMask;
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

void Pic::drawWithMask(Graphics::ManagedSurface *dst, int x, int y) const {
	// Draw using transparency mask if available
	if (_transparencyMask) {
		for (int py = 0; py < h; ++py) {
			const byte *srcRow = (const byte *)getBasePtr(0, py);
			byte *dstRow = (byte *)dst->getBasePtr(x, y + py);

			for (int px = 0; px < w; ++px) {
				// Skip if pixel is marked as transparent in mask
				if (!isPixelTransparent(px, py)) {
					byte pixel = srcRow[px];
					dstRow[px] = pixel;
				}
			}
		}
	} else {
		// No mask: draw normally
		draw(dst, x, y);
	}
}

} // namespace Gfx
} // namespace Goldbox
