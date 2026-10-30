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

#include "ultima/shared/gfx/towns_screen.h"
#include "common/endian.h"
#include "common/config-manager.h"
#include "common/rect.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

TownsScreen::TownsScreen(Screen *scr) : _width(0), _height(0), _pitch(0), _pixelFormat(scr->format), _scrollOffset(0), _scrollRemainder(0), _numDirtyRects(0) {
	_system = g_system;
	Graphics::Surface *s = scr->surfacePtr();
	_width = s->w;
	_height = s->h;
	_pitch = s->pitch;
	_writePage = &_page[0];

	setupLayer(_width, _height, 1, 1, 16);
}

TownsScreen::~TownsScreen() {
	_page[0].layer.free();
	_page[1].layer.free();
}

void TownsScreen::setupLayer(int width, int height, int scaleW, int scaleH, int numCol) {
	if (width <= 0 || height <= 0 || width > 32767 || height > 32767 ||
			scaleW < 1 || scaleW > 2 || scaleH < 1 || scaleH > 2 ||
			(numCol != 16 && numCol != 256 && numCol != 32768))
		error("TownsScreen::setupLayer(): Invalid width/height/number of colors setting.");

	_writePage->bpp = ((numCol - 1) & 0xff00) ? 2 : 1;
	_writePage->layer.create(width, height, _writePage->bpp == 2 ?
		Graphics::PixelFormat(2, 5, 5, 5, 0, 5, 10, 0, 0) :
		Graphics::PixelFormat::createFormatCLUT8());
	_writePage->scaleW = scaleW;
	_writePage->scaleH = scaleH;
	_writePage->numCol = numCol;
	_writePage->hScroll = 0;
	_writePage->foreColor = 0;
	_writePage->backColor = 0;
	_writePage->fillColor = 0;
	_writePage->transColor = 0;
	_writePage->enabled = true;
	_page[0].onTop = true;
	_page[1].onTop = !_page[0].enabled;
	_writePage->ready = true;
}

void TownsScreen::setPalette(const byte *colors) {
	_writePage->layer.setPalette(colors, 0, _writePage->numCol);
}

void TownsScreen::setForegroundColor(uint16 colorID) {
	if (colorID >= _writePage->numCol)
		return;
	_writePage->foreColor = colorID;
}

void TownsScreen::setBackgroundColor(uint16 colorID) {
	if (colorID >= _writePage->numCol)
		return;
	_writePage->backColor = colorID;
}

void TownsScreen::setSurfaceFillColor(uint16 colorID) {
	if (colorID >= _writePage->numCol)
		return;
	_writePage->fillColor = colorID;
	}

void TownsScreen::setTransparentColor(uint16 colorID) {
	if (colorID >= _writePage->numCol)
		return;
	_writePage->transColor = colorID;
}

void TownsScreen::drawLine(int x1, int y1, int x2, int y2) {
	_writePage->layer.drawLine(x1, y1, x2, y2, _writePage->foreColor);
}

/// @brief
/// @param page
void TownsScreen::setWritePage(int pageID){
	if (pageID < 0 || pageID > 1)
		return;
	_writePage = &_page[pageID];
}


void TownsScreen::drawWindow(Common::String name) {

}

void TownsScreen::drawCornerPoints(int xc, int yc, int x, int y, byte corner) {
	switch (corner) {
		case 1: // down-right
			_writePage->layer.setPixel(xc+x, yc+y, _writePage->foreColor);
			_writePage->layer.setPixel(xc+y, yc+x, _writePage->foreColor);
			break;
		case 2: // down-left
			_writePage->layer.setPixel(xc-x, yc+y, _writePage->foreColor);
			_writePage->layer.setPixel(xc-y, yc+x, _writePage->foreColor);
			break;
		case 3: // up-left
			_writePage->layer.setPixel(xc-x, yc-y, _writePage->foreColor);
			_writePage->layer.setPixel(xc-y, yc-x, _writePage->foreColor);
			break;
		case 4: // up-right
			_writePage->layer.setPixel(xc+x, yc-y, _writePage->foreColor);
			_writePage->layer.setPixel(xc+y, yc-x, _writePage->foreColor);
			break;
	}
}

void TownsScreen::drawCirclePoints(int xc, int yc, int x, int y) {
	_writePage->layer.setPixel(xc+x, yc+y, _writePage->foreColor);
	_writePage->layer.setPixel(xc+y, yc+x, _writePage->foreColor);
	_writePage->layer.setPixel(xc-x, yc+y, _writePage->foreColor);
	_writePage->layer.setPixel(xc-y, yc+x, _writePage->foreColor);
	_writePage->layer.setPixel(xc+x, yc-y, _writePage->foreColor);
	_writePage->layer.setPixel(xc+y, yc-x, _writePage->foreColor);
	_writePage->layer.setPixel(xc-x, yc-y, _writePage->foreColor);
	_writePage->layer.setPixel(xc-y, yc-x, _writePage->foreColor);
}

void TownsScreen::drawArc(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius) {
	const int startDx = int(start_x) - center_x;
	const int startDy = int(start_y) - center_y;
	const int endDx = int(end_x) - center_x;
	const int endDy = int(end_y) - center_y;
	const bool horizontalStart = startDy == 0 && ABS(startDx) == radius;
	const bool verticalStart = startDx == 0 && ABS(startDy) == radius;
	const bool horizontalEnd = endDy == 0 && ABS(endDx) == radius;
	const bool verticalEnd = endDx == 0 && ABS(endDy) == radius;
	if (radius <= 0 || !((horizontalStart && verticalEnd) || (verticalStart && horizontalEnd))) {
		warning("TownsScreen::drawArc(): Expected perpendicular quarter-circle endpoints");
		return;
	}

	const int dx = horizontalStart ? startDx : endDx;
	const int dy = verticalStart ? startDy : endDy;
	const int left = center_x + MIN(dx, 0);
	const int top = center_y + MIN(dy, 0);
	const int right = center_x + MAX(dx, 0);
	const int bottom = center_y + MAX(dy, 0);
	if (!_writePage->ready || left < 0 || top < 0 ||
			right >= _writePage->layer.w || bottom >= _writePage->layer.h) {
		warning("TownsScreen::drawArc(): Quarter circle outside the active layer");
		return;
	}

	const byte corner = dy > 0 ? (dx > 0 ? 1 : 2) : (dx < 0 ? 3 : 4);
	int x = 0, y = radius;
	int d = 3 - 2 * radius;
	while (x <= y) {
		drawCornerPoints(center_x, center_y, x, y, corner);
		if (d > 0) {
			d = d + 4 * (x - y) + 10;
			--y;
		} else {
			d = d + 4 * x + 6;
		}
		++x;
	}
	addDirtyRect(left, top, right - left + 1, bottom - top + 1);
}

void TownsScreen::clearLayer(int layer) {
	if (layer < 0 || layer > 1)
		return;

	TownsScreenLayer *l = &_page[layer];
	if (!l->ready)
		return;

	l->layer.clear(l->backColor);
	_dirtyRects.push_back(Common::Rect(_width, _height));
	_numDirtyRects = kFullRedraw;
}

void TownsScreen::drawRectangle(Rect rect) {
	drawRectangleC(rect, _writePage->fillColor);
}

void TownsScreen::drawRectangleC(Rect rect, int color) {
	_writePage->layer.fillRect(rect, color);
}

void TownsScreen::drawRectangleS(Point ps, int w, int h) {
	drawRectangleSC(ps, w, h, _writePage->fillColor);
}

void TownsScreen::drawRectangleSC(Point ps, int w, int h, int color) {
	drawRectangleC(Rect(ps.x, ps.y, ps.x+w, ps.y+h), color);
}

uint8 *TownsScreen::getLayerPixels(int layer, int x, int y) const {
	if (layer < 0 || layer > 1)
		return nullptr;

	const TownsScreenLayer &page = _page[layer];
	if (!page.ready)
		return nullptr;
	if (x < 0 || y < 0 || x >= page.layer.w || y >= page.layer.h) {
		warning("TownsScreen::getLayerPixels(): Coordinates outside the requested layer");
		return nullptr;
	}
	return const_cast<uint8 *>(static_cast<const uint8 *>(page.layer.getBasePtr(x, y)));
}

void TownsScreen::addDirtyRect(int x, int y, int w, int h) {
	if (w <= 0 || h <= 0 || _numDirtyRects > kDirtyRectsMax)
		return;

	if (_numDirtyRects == kDirtyRectsMax) {
		// full redraw
		_dirtyRects.clear();
		_dirtyRects.push_back(Common::Rect(_width, _height));
		_numDirtyRects++;
		return;
	}

	int x2 = x + w;
	int y2 = y + h;

	assert(x >= 0 && y >= 0 && x2 <= _width && y2 <= _height);

	bool skip = false;
	for (Common::List<Common::Rect>::iterator r = _dirtyRects.begin(); r != _dirtyRects.end(); ++r) {
		// Try to merge new rect with an existing rect (only once, since trying to merge
		// more than one overlapping rect would be causing more overhead than doing any good).
		if (x > r->left && x < r->right && y > r->top && y < r->bottom) {
			x = r->left;
			y = r->top;
			skip = true;
		}

		if (x2 > r->left && x2 < r->right && y > r->top && y < r->bottom) {
			x2 = r->right;
			y = r->top;
			skip = true;
		}

		if (x2 > r->left && x2 < r->right && y2 > r->top && y2 < r->bottom) {
			x2 = r->right;
			y2 = r->bottom;
			skip = true;
		}

		if (x > r->left && x < r->right && y2 > r->top && y2 < r->bottom) {
			x = r->left;
			y2 = r->bottom;
			skip = true;
		}

		if (skip) {
			r->left = x;
			r->top = y;
			r->right = x2;
			r->bottom = y2;
			break;
		}
	}

	if (!skip) {
		_dirtyRects.push_back(Common::Rect(x, y, x2, y2));
		_numDirtyRects++;
	}
}

void TownsScreen::displayPage(int flags) {
	if (flags < 0 || flags > 3)
		return;

	_page[0].enabled = (flags & 1) ? true : false;
	_page[0].onTop = true;
	_page[1].enabled = (flags & 2) ? true : false;
	_page[1].onTop = !_page[0].enabled;

	_dirtyRects.clear();
	_dirtyRects.push_back(Common::Rect(_width, _height));
	_numDirtyRects = kFullRedraw;

	update();

	_system->updateScreen();
}

void TownsScreen::update() {
	if (_dirtyRects.empty())
		return;

	const Graphics::PixelFormat format = _system->getScreenFormat();
	Graphics::Palette palette;
	for (int i = 0; i < 2; ++i) {
		const TownsScreenLayer &page = _page[i];
		if (!page.ready || !page.enabled)
			continue;
		if (page.layer.format.isCLUT8()) {
			const Graphics::Palette *colors = page.layer.grabPalette();
			if (!colors || colors->size() < page.numCol)
				error("TownsScreen::update(): No palette for page %d", i);
			if (format.isCLUT8()) {
				if (!palette.empty() && !palette.equals(*colors))
					error("TownsScreen::update(): Different page palettes require an RGB screen");
				palette = *colors;
			}
		} else if (format.isCLUT8()) {
			error("TownsScreen::update(): Packed-color pages require an RGB screen");
		}
	}
	if (!palette.empty())
		_system->getPaletteManager()->setPalette(palette.data(), 0, palette.size());

	Graphics::Surface *dest = _system->lockScreen();
	if (!dest)
		error("TownsScreen::update(): Unable to lock the backend screen");
	for (const Common::Rect &dirty : _dirtyRects) {
		Common::Rect rect = dirty;
		rect.clip(Common::Rect(MIN(_width, int(dest->w)), MIN(_height, int(dest->h))));
		for (int y = rect.top; y < rect.bottom; ++y) {
			for (int x = rect.left; x < rect.right; ++x) {
				uint32 color = format.isCLUT8() ? 0 : format.RGBToColor(0, 0, 0);
				// Page 0 is the foreground when both pages are enabled.
				for (int i = 1; i >= 0; --i) {
					const TownsScreenLayer &page = _page[i];
					if (!page.ready || !page.enabled || x >= page.layer.w * page.scaleW ||
							y >= page.layer.h * page.scaleH)
						continue;
					const int sx = (x / page.scaleW + page.hScroll) % page.layer.w;
					const int sy = y / page.scaleH;
					const uint32 pixel = page.layer.rawSurface().getPixel(sx, sy);
					if (page.onTop && page.layer.format.isCLUT8() && pixel == page.transColor)
						continue;
					if (format.isCLUT8()) {
						color = pixel;
					} else {
						byte r, g, b;
						if (page.layer.format.isCLUT8()) {
							const Graphics::Palette *colors = page.layer.grabPalette();
							if (pixel >= colors->size())
								error("TownsScreen::update(): Palette index outside page %d palette", i);
							colors->get(pixel, r, g, b);
						} else {
							page.layer.format.colorToRGB(pixel, r, g, b);
						}
						color = format.RGBToColor(r, g, b);
					}
				}
				dest->setPixel(x, y, color);
			}
		}
	}
	_system->unlockScreen();
	_dirtyRects.clear();
	_numDirtyRects = 0;
}

void TownsScreen::scrollLayers(int flags, int offset, bool fast) {
	// This actually supports layer 0 only, since this is all we need.
	_scrollRemainder += offset;
	if (!_scrollRemainder)
		return;

	int step = (_scrollRemainder > 0) ? -1 : 1;

	_scrollRemainder += step;
	_scrollOffset = (_scrollOffset + step + _page[0].layer.w) % _page[0].layer.w;

	_dirtyRects.clear();
	_dirtyRects.push_back(Common::Rect(_width, _height));
	_numDirtyRects = kFullRedraw;

	for (int i = 0; i < 2; ++i) {
		if (!(flags & (1 << i)))
			continue;
		TownsScreenLayer *l = &_page[i];
		if (l->ready)
			l->hScroll = _scrollOffset % l->layer.w;
	}
}

} // End of namespace Gfx
} // End of namespace Shared
} // End of namespace Ultima
