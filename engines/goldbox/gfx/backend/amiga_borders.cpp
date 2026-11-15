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

#include "goldbox/gfx/backend/amiga_borders.h"
#include "common/file.h"
#include "common/textconsole.h"

namespace Goldbox {
namespace Gfx {

static const uint32 kAmigaGfxMagic = MKTAG('G', 'B', 'G', 'X');
static const uint16 kAmigaGfxVersion = 1;

bool AmigaBorders::load(const Common::Path &fileName) {
	_loaded = false;

	Common::File f;
	if (!f.open(fileName))
		return false;

	if (f.readUint32BE() != kAmigaGfxMagic || f.readUint16LE() != kAmigaGfxVersion) {
		warning("AmigaBorders: %s is not a supported graphics file",
			fileName.toString().c_str());
		return false;
	}

	const uint16 count = f.readUint16LE();
	bool found[kImageCount] = {};
	for (uint16 i = 0; i < count; ++i) {
		const uint16 id = f.readUint16LE();
		const uint16 width = f.readUint16LE();
		const uint16 height = f.readUint16LE();
		if (f.eos() || width == 0 || height == 0) {
			warning("AmigaBorders: %s is truncated", fileName.toString().c_str());
			return false;
		}
		if (id >= kImageCount) {
			f.skip((uint32)width * height);
			continue;
		}

		Graphics::ManagedSurface &image = _images[id];
		image.create(width, height);
		for (uint16 y = 0; y < height; ++y)
			f.read(image.getBasePtr(0, y), width);
		found[id] = true;
	}

	if (f.err() || f.eos()) {
		warning("AmigaBorders: %s is truncated", fileName.toString().c_str());
		return false;
	}
	for (int id = 0; id < kImageCount; ++id) {
		if (!found[id]) {
			warning("AmigaBorders: %s lacks image %d", fileName.toString().c_str(), id);
			return false;
		}
	}

	_loaded = true;
	return true;
}

void AmigaBorders::drawFrame(Graphics::ManagedSurface &dst, int startX,
		int startY, int endX, int endY) const {
	if (!_loaded)
		return;

	const int x0 = startX - 1;
	const int y0 = startY - 1;
	const int x1 = endX + 1;
	const int y1 = endY + 1;
	const int innerW = (x1 - x0 - 1) * 8;
	const int innerH = (y1 - y0 - 1) * 8;

	blit(dst, kTop, (x0 + 1) * 8, y0 * 8, innerW, 8);
	blit(dst, kBottom, (x0 + 1) * 8, y1 * 8, innerW, 8);
	blit(dst, kLeftEdge, x0 * 8, (y0 + 1) * 8, 8, innerH);
	blit(dst, kRightEdge, x1 * 8, (y0 + 1) * 8, 8, innerH);
	blit(dst, kCorner, x0 * 8, y0 * 8, 8, 8);
	blit(dst, kCorner, x1 * 8, y0 * 8, 8, 8);
	blit(dst, kCorner, x0 * 8, y1 * 8, 8, 8);
	blit(dst, kCorner, x1 * 8, y1 * 8, 8, 8);
}

void AmigaBorders::blit(Graphics::ManagedSurface &dst, ImageId id, int x,
		int y, int w, int h) const {
	const Graphics::ManagedSurface &src = _images[id];
	Common::Rect srcRect(0, 0, MIN<int>(w, src.w), MIN<int>(h, src.h));
	if (srcRect.isEmpty())
		return;
	dst.blitFrom(src, srcRect, Common::Point(x, y));
}

} // namespace Gfx
} // namespace Goldbox
