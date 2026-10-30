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
#include "common/textconsole.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

namespace {

Common::Rect clipInclusiveRect(int64 x1, int64 y1, int64 x2, int64 y2, int width, int height) {
	const int left = int(MAX<int64>(0, x1));
	const int top = int(MAX<int64>(0, y1));
	const int right = int(MIN<int64>(width, x2 + 1));
	const int bottom = int(MIN<int64>(height, y2 + 1));
	if (left >= right || top >= bottom)
		return Common::Rect();
	return Common::Rect(left, top, right, bottom);
}

} // End of anonymous namespace

TownsScreen::TownsScreen(Graphics::Screen *scr) : _height(0), _width(0), _pitch(0),
		_scrollOffset(0), _scrollRemainder(0), _pixelFormat(scr->format),
		_numDirtyRects(0), _paintMode(kPaintBorderSolid | kPaintFillSolid),
		_writeMode(0), _fontStyle(0) {
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
	_writePage->transparentEnabled = getWritePage() == 0;
	_writePage->layer.clear(0);
	markFullDirty();
}

void TownsScreen::setupPage(int page, int width, int height, int scaleW, int scaleH, int numCol) {
	if (page < 0 || page > 1) {
		warning("TownsScreen::setupPage(): Invalid page %d", page);
		return;
	}
	const int previousPage = getWritePage();
	setWritePage(page);
	setupLayer(width, height, scaleW, scaleH, numCol);
	setWritePage(previousPage);
}

void TownsScreen::markFullDirty() {
	_dirtyRects.clear();
	_dirtyRects.push_back(Common::Rect(_width, _height));
	_numDirtyRects = kFullRedraw;
}

void TownsScreen::setVisiblePages(uint8 mask) {
	if (mask > 3) {
		warning("TownsScreen::setVisiblePages(): Invalid mask %u", mask);
		return;
	}
	_page[0].enabled = (mask & 1) != 0;
	_page[1].enabled = (mask & 2) != 0;
	_page[0].onTop = true;
	_page[1].onTop = !_page[0].enabled;
	markFullDirty();
}

int TownsScreen::legacyDisplayPageMask(int mode) {
	// DARKNESS GFX_setDisplayPage: EGB_displayPage(work, 0, mode == 3 ? 0 : mode + 1)
	if (mode < 0 || mode > 3)
		return -1;
	return mode == 3 ? 0 : mode + 1;
}

bool TownsScreen::setLegacyDisplayPage(int mode) {
	const int mask = legacyDisplayPageMask(mode);
	if (mask < 0) {
		warning("TownsScreen::setLegacyDisplayPage(): Invalid mode %d", mode);
		return false;
	}
	setVisiblePages((uint8)mask);
	return true;
}

uint8 TownsScreen::getVisiblePages() const {
	return (_page[0].enabled ? 1 : 0) | (_page[1].enabled ? 2 : 0);
}

void TownsScreen::setPalette(const byte *colors) {
	setPaletteForPage(getWritePage(), colors, 0, _writePage->numCol);
}

void TownsScreen::setPaletteForPage(int page, const byte *rgb, uint16 start, uint16 count) {
	if (page < 0 || page > 1 || !rgb || !count || !_page[page].ready ||
			!_page[page].layer.format.isCLUT8() ||
			uint32(start) + count > _page[page].numCol) {
		warning("TownsScreen::setPaletteForPage(): Invalid indexed palette range");
		return;
	}
	_page[page].layer.setPalette(rgb, start, count);
	markFullDirty();
}

void TownsScreen::setPaletteEntry(int page, uint16 index, byte red, byte green, byte blue) {
	const byte rgb[] = { red, green, blue };
	setPaletteForPage(page, rgb, index, 1);
}

void TownsScreen::applyDefaultPalette4bit(const byte *rgb) {
	if (!rgb || !_writePage->ready || !_writePage->layer.format.isCLUT8() ||
			_writePage->numCol < 16) {
		warning("TownsScreen::applyDefaultPalette4bit(): Requires an indexed page and palette");
		return;
	}
	setPaletteForPage(getWritePage(), rgb + 3, 1, 15);
}

void TownsScreen::setForegroundColor(uint16 colorID) {
	if (colorID >= _writePage->numCol) {
		warning("TownsScreen::setForegroundColor(): Invalid color %u", colorID);
		return;
	}
	_writePage->foreColor = colorID;
}

void TownsScreen::setBackgroundColor(uint16 colorID) {
	if (colorID >= _writePage->numCol) {
		warning("TownsScreen::setBackgroundColor(): Invalid color %u", colorID);
		return;
	}
	_writePage->backColor = colorID;
}

void TownsScreen::setSurfaceFillColor(uint16 colorID) {
	if (colorID >= _writePage->numCol) {
		warning("TownsScreen::setSurfaceFillColor(): Invalid color %u", colorID);
		return;
	}
	_writePage->fillColor = colorID;
}

void TownsScreen::setTransparentColor(uint16 colorID) {
	if (colorID >= _writePage->numCol) {
		warning("TownsScreen::setTransparentColor(): Invalid color %u", colorID);
		return;
	}
	_writePage->transColor = colorID;
	markFullDirty();
}

void TownsScreen::setTransparentColorEnabled(int page, bool enabled) {
	if (page < 0 || page > 1) {
		warning("TownsScreen::setTransparentColorEnabled(): Invalid page %d", page);
		return;
	}
	_page[page].transparentEnabled = enabled;
	markFullDirty();
}

bool TownsScreen::canDraw(const char *operation) const {
	if (!_writePage->ready || _writeMode != 0) {
		warning("TownsScreen::%s(): Page not initialized or unsupported write mode %u", operation, _writeMode);
		return false;
	}
	return true;
}

void TownsScreen::drawLine(int x1, int y1, int x2, int y2) {
	if (!canDraw("drawLine"))
		return;
	if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 ||
			x1 >= _writePage->layer.w || x2 >= _writePage->layer.w ||
			y1 >= _writePage->layer.h || y2 >= _writePage->layer.h) {
		warning("TownsScreen::drawLine(): Offscreen endpoints are not supported");
		return;
	}
	_writePage->layer.drawLine(x1, y1, x2, y2, _writePage->foreColor);
	invalidateRect(MIN(x1, x2), MIN(y1, y2), ABS(x2 - x1) + 1, ABS(y2 - y1) + 1);
}

void TownsScreen::drawRectangleWithMode(int x1, int y1, int x2, int y2, uint16 mode) {
	if (!canDraw("drawRectangleWithMode"))
		return;
	if (x2 < x1 || y2 < y1 || (mode & ~(kPaintFillSolid | kPaintBorderSolid))) {
		warning("TownsScreen::drawRectangleWithMode(): Invalid bounds or unsupported paint mode %u", mode);
		return;
	}
	const Common::Rect rect = clipInclusiveRect(x1, y1, x2, y2, _writePage->layer.w, _writePage->layer.h);
	if (rect.isEmpty())
		return;
	if (mode & kPaintFillSolid)
		_writePage->layer.fillRect(rect, _writePage->fillColor);
	if (mode & kPaintBorderSolid) {
		// Clip the original edges, rather than inventing borders at clipped bounds.
		Graphics::Surface &surface = *_writePage->layer.surfacePtr();
		surface.hLine(rect.left, y1, rect.right - 1, _writePage->foreColor);
		surface.hLine(rect.left, y2, rect.right - 1, _writePage->foreColor);
		surface.vLine(x1, rect.top, rect.bottom - 1, _writePage->foreColor);
		surface.vLine(x2, rect.top, rect.bottom - 1, _writePage->foreColor);
	}
	invalidateRect(rect.left, rect.top, rect.width(), rect.height());
}

void TownsScreen::drawFilledRectangle(int x1, int y1, int x2, int y2) {
	drawRectangleWithMode(x1, y1, x2, y2, _paintMode | kPaintFillSolid);
}

void TownsScreen::drawColoredRectangle(int x1, int y1, int x2, int y2, uint16 color) {
	if (!_writePage->ready || color >= _writePage->numCol) {
		warning("TownsScreen::drawColoredRectangle(): Invalid page or color %u", color);
		return;
	}
	const uint16 previousMode = _writeMode;
	_writeMode = 0;
	setForegroundColor(color);
	setSurfaceFillColor(color);
	drawRectangleWithMode(x1, y1, x2, y2, _paintMode);
	_writeMode = previousMode;
}

bool TownsScreen::putPixelBlock(const byte *src, uint32 srcSize, int x, int y,
		int width, int height, uint8 sourceBitsPerPixel) {
	if (!canDraw("putPixelBlock"))
		return false;
	if (!src || width <= 0 || height <= 0 ||
			(sourceBitsPerPixel != 4 && sourceBitsPerPixel != 8 && sourceBitsPerPixel != 16) ||
			((sourceBitsPerPixel == 16) != (_writePage->bpp == 2))) {
		warning("TownsScreen::putPixelBlock(): Invalid raster or incompatible pixel format");
		return false;
	}
	const uint64 rowBytes = (uint64(width) * sourceBitsPerPixel + 7) / 8;
	if (rowBytes * uint64(height) > srcSize) {
		warning("TownsScreen::putPixelBlock(): Truncated raster");
		return false;
	}
	const Common::Rect rect = clipInclusiveRect(x, y, int64(x) + width - 1,
			int64(y) + height - 1, _writePage->layer.w, _writePage->layer.h);
	if (rect.isEmpty())
		return true;
	if (sourceBitsPerPixel == 8 && _writePage->numCol == 16) {
		for (int dy = rect.top; dy < rect.bottom; ++dy) {
			const byte *row = src + uint64(int64(dy) - y) * rowBytes;
			for (int dx = rect.left; dx < rect.right; ++dx) {
				if (row[int64(dx) - x] >= 16) {
					warning("TownsScreen::putPixelBlock(): Palette index outside the 16-color page");
					return false;
				}
			}
		}
	}
	for (int dy = rect.top; dy < rect.bottom; ++dy) {
		const byte *row = src + uint64(int64(dy) - y) * rowBytes;
		for (int dx = rect.left; dx < rect.right; ++dx) {
			const uint32 sx = uint32(int64(dx) - x);
			const uint32 pixel = sourceBitsPerPixel == 4 ?
					((sx & 1) ? row[sx / 2] >> 4 : row[sx / 2] & 15) :
					(sourceBitsPerPixel == 8 ? row[sx] : READ_LE_UINT16(row + sx * 2));
			_writePage->layer.setPixel(dx, dy, pixel);
		}
	}
	invalidateRect(rect.left, rect.top, rect.width(), rect.height());
	return true;
}

void TownsScreen::scrollRegion(int x1, int y1, int x2, int y2,
		int deltaX, int deltaY, bool fillExposed) {
	if (!_writePage->ready || x2 < x1 || y2 < y1) {
		warning("TownsScreen::scrollRegion(): Invalid page or region");
		return;
	}
	const Common::Rect rect = clipInclusiveRect(x1, y1, x2, y2, _writePage->layer.w, _writePage->layer.h);
	if (rect.isEmpty() || (!deltaX && !deltaY))
		return;
	const int yBegin = deltaY > 0 ? rect.bottom - 1 : rect.top;
	const int yEnd = deltaY > 0 ? rect.top - 1 : rect.bottom;
	const int yStep = deltaY > 0 ? -1 : 1;
	const int xBegin = deltaX > 0 ? rect.right - 1 : rect.left;
	const int xEnd = deltaX > 0 ? rect.left - 1 : rect.right;
	const int xStep = deltaX > 0 ? -1 : 1;
	for (int yy = yBegin; yy != yEnd; yy += yStep) {
		for (int xx = xBegin; xx != xEnd; xx += xStep) {
			const int64 sx = int64(xx) - deltaX;
			const int64 sy = int64(yy) - deltaY;
			if (sx >= rect.left && sx < rect.right && sy >= rect.top && sy < rect.bottom)
				_writePage->layer.setPixel(xx, yy, _writePage->layer.rawSurface().getPixel(int(sx), int(sy)));
			else if (fillExposed)
				_writePage->layer.setPixel(xx, yy, _writePage->backColor);
		}
	}
	invalidateRect(rect.left, rect.top, rect.width(), rect.height());
}

Graphics::ManagedSurface &TownsScreen::getWriteSurface() {
	if (!_writePage->ready)
		error("TownsScreen::getWriteSurface(): Page not initialized");
	return _writePage->layer;
}

void TownsScreen::invalidateRect(int x, int y, int width, int height) {
	if (!_writePage->ready || width < 0 || height < 0) {
		warning("TownsScreen::invalidateRect(): Invalid page or size");
		return;
	}
	if (!width || !height)
		return;
	const Common::Rect rect = clipInclusiveRect(x, y, int64(x) + width - 1,
			int64(y) + height - 1, _writePage->layer.w, _writePage->layer.h);
	if (rect.isEmpty())
		return;
	if (_writePage->hScroll) {
		markFullDirty();
		return;
	}
	addDirtyRect(rect.left * _writePage->scaleW, rect.top * _writePage->scaleH,
			rect.width() * _writePage->scaleW, rect.height() * _writePage->scaleH);
}

/// @brief
/// @param page
void TownsScreen::setWritePage(int pageID){
	if (pageID < 0 || pageID > 1) {
		warning("TownsScreen::setWritePage(): Invalid page %d", pageID);
		return;
	}
	_writePage = &_page[pageID];
}


void TownsScreen::drawWindow(Common::String name) {

}

void TownsScreen::plotPixelClipped(int x, int y, uint16 color) {
	if (x >= 0 && y >= 0 && x < _writePage->layer.w && y < _writePage->layer.h)
		_writePage->layer.setPixel(x, y, color);
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

bool TownsScreen::isInArcSweep(int px, int py, int sx, int sy, int ex, int ey) {
	// Equal start/end directions select the whole circle (the Darkness frame's
	// upper-left corner passes identical (-16,-16) vectors).
	if (int64(sx) * ey - int64(sy) * ex == 0 && int64(sx) * ex + int64(sy) * ey >= 0)
		return true;
	if (px == 0 && py == 0)
		return true;
	const double twoPi = 2.0 * M_PI;
	const double start = atan2((double)sy, (double)sx);
	double span = atan2((double)ey, (double)ex) - start;
	while (span < 0)
		span += twoPi;
	double rel = atan2((double)py, (double)px) - start;
	while (rel < 0)
		rel += twoPi;
	while (rel >= twoPi)
		rel -= twoPi;
	return rel <= span + 1e-9;
}

void TownsScreen::plotArcOutline(int xc, int yc, int sx, int sy, int ex, int ey, int radius, uint16 color) {
	int x = 0, y = radius;
	int d = 3 - 2 * radius;
	while (x <= y) {
		const int pts[8][2] = {
			{ x, y }, { y, x }, { -x, y }, { -y, x },
			{ x, -y }, { y, -x }, { -x, -y }, { -y, -x }
		};
		for (int i = 0; i < 8; ++i) {
			if (isInArcSweep(pts[i][0], pts[i][1], sx, sy, ex, ey))
				plotPixelClipped(xc + pts[i][0], yc + pts[i][1], color);
		}
		if (d > 0) {
			d = d + 4 * (x - y) + 10;
			--y;
		} else {
			d = d + 4 * x + 6;
		}
		++x;
	}
}

void TownsScreen::plotLineClipped(int x1, int y1, int x2, int y2, uint16 color) {
	const int dx = ABS(x2 - x1), sx = x1 < x2 ? 1 : -1;
	const int dy = -ABS(y2 - y1), sy = y1 < y2 ? 1 : -1;
	int err = dx + dy;
	for (;;) {
		plotPixelClipped(x1, y1, color);
		if (x1 == x2 && y1 == y2)
			break;
		const int e2 = 2 * err;
		if (e2 >= dy) {
			err += dy;
			x1 += sx;
		}
		if (e2 <= dx) {
			err += dx;
			y1 += sy;
		}
	}
}

void TownsScreen::drawArc(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius) {
	if (!canDraw("drawArc"))
		return;
	if (radius <= 0) {
		warning("TownsScreen::drawArc(): Invalid radius %d", radius);
		return;
	}
	plotArcOutline(center_x, center_y, start_x, start_y, end_x, end_y, radius, _writePage->foreColor);
	invalidateRect(center_x - radius, center_y - radius, 2 * radius + 1, 2 * radius + 1);
}

void TownsScreen::drawFan(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius) {
	if (!canDraw("drawFan"))
		return;
	if (radius <= 0) {
		warning("TownsScreen::drawFan(): Invalid radius %d", radius);
		return;
	}
	if (_paintMode & ~(kPaintFillSolid | kPaintBorderSolid))
		warning("TownsScreen::drawFan(): Unsupported paint mode bits 0x%x ignored",
			_paintMode & ~(kPaintFillSolid | kPaintBorderSolid));

	if (_paintMode & kPaintFillSolid) {
		const int64 r2 = int64(radius) * radius;
		for (int dy = -radius; dy <= radius; ++dy) {
			for (int dx = -radius; dx <= radius; ++dx) {
				if (int64(dx) * dx + int64(dy) * dy <= r2 &&
						isInArcSweep(dx, dy, start_x, start_y, end_x, end_y))
					plotPixelClipped(center_x + dx, center_y + dy, _writePage->fillColor);
			}
		}
	}

	if (_paintMode & kPaintBorderSolid) {
		const uint16 color = _writePage->foreColor;
		plotArcOutline(center_x, center_y, start_x, start_y, end_x, end_y, radius, color);
		const bool fullCircle = int64(start_x) * end_y - int64(start_y) * end_x == 0 &&
			int64(start_x) * end_x + int64(start_y) * end_y >= 0;
		if (!fullCircle) {
			const int16 vectors[2][2] = { { start_x, start_y }, { end_x, end_y } };
			for (int i = 0; i < 2; ++i) {
				const double len = sqrt((double)vectors[i][0] * vectors[i][0] +
					(double)vectors[i][1] * vectors[i][1]);
				if (len == 0)
					continue;
				const int px = center_x + (int)floor(vectors[i][0] * radius / len + 0.5);
				const int py = center_y + (int)floor(vectors[i][1] * radius / len + 0.5);
				plotLineClipped(center_x, center_y, px, py, color);
			}
		}
	}
	invalidateRect(center_x - radius, center_y - radius, 2 * radius + 1, 2 * radius + 1);
}
void TownsScreen::clearLayer(int layer) {
	if (layer < 0 || layer > 1 || !_page[layer].ready) {
		warning("TownsScreen::clearLayer(): Invalid or uninitialized layer %d", layer);
		return;
	}

	TownsScreenLayer *l = &_page[layer];
	l->layer.clear(l->backColor);
	markFullDirty();
}

void TownsScreen::drawRectangle(Rect rect) {
	drawRectangleC(rect, _writePage->fillColor);
}

void TownsScreen::drawRectangleC(Rect rect, int color) {
	if (!canDraw("drawRectangleC"))
		return;
	if (color < 0 || color >= _writePage->numCol) {
		warning("TownsScreen::drawRectangleC(): Invalid color %d", color);
		return;
	}
	rect.clip(Common::Rect(_writePage->layer.w, _writePage->layer.h));
	if (!rect.isEmpty()) {
		_writePage->layer.fillRect(rect, color);
		invalidateRect(rect.left, rect.top, rect.width(), rect.height());
	}
}

void TownsScreen::drawRectangleS(Point ps, int w, int h) {
	drawRectangleSC(ps, w, h, _writePage->fillColor);
}

void TownsScreen::drawRectangleSC(Point ps, int w, int h, int color) {
	if (w <= 0 || h <= 0) {
		warning("TownsScreen::drawRectangleSC(): Invalid size");
		return;
	}
	drawRectangleC(clipInclusiveRect(ps.x, ps.y, int64(ps.x) + w - 1,
			int64(ps.y) + h - 1, _writePage->layer.w, _writePage->layer.h), color);
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

	const Common::Rect rect = clipInclusiveRect(x, y, int64(x) + w - 1,
			int64(y) + h - 1, _width, _height);
	if (rect.isEmpty())
		return;
	if (_numDirtyRects == kDirtyRectsMax) {
		markFullDirty();
		return;
	}

	for (Common::Rect &dirty : _dirtyRects) {
		if (dirty.intersects(rect)) {
			dirty.extend(rect);
			return;
		}
	}
	_dirtyRects.push_back(rect);
	++_numDirtyRects;
}

void TownsScreen::displayPage(int flags) {
	if (flags < 0 || flags > 3) {
		warning("TownsScreen::displayPage(): Invalid page mask %d", flags);
		return;
	}
	setVisiblePages(flags);
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
					if (page.onTop && page.transparentEnabled && page.layer.format.isCLUT8() &&
							pixel == page.transColor)
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

	markFullDirty();

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
