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
#include "common/math.h"
#include "math/angle.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

TownsScreen::TownsScreen(Screen *scr) : _width(0), _height(0), _pitch(0), _pixelFormat(scr->format), _scrollOffset(0), _scrollRemainder(0), _numDirtyRects(0) {
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
	if (numCol >> 15)
		error("TownsScreen::setupLayer(): No more than 32767 colors supported.");

	if (width <= 0 || height <= 0 || numCol < 16)
		error("TownsScreen::setupLayer(): Invalid width/height/number of colors setting.");

	_writePage->layer.w = width;
	_writePage->layer.h = height;
	_writePage->bpp = ((numCol - 1) & 0xff00) ? 2 : 1;
	_writePage->layer.pitch = width * _writePage->bpp;
	_writePage->layer.create(width,height);
	_writePage->scaleW = scaleW;
	_writePage->scaleH = scaleH;
	_writePage->numCol = numCol;
	_writePage->hScroll = 0;
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

void TownsScreen::drawArc(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius) {
	Math::Angle startAngle, endAngle;
	startAngle.arcTangent2(start_y, start_x);
	endAngle.arcTangent2(end_y, end_x);

	if (endAngle < startAngle) {
		endAngle += 2 * M_PI;
	}
	// Calculate the angle span
	float angleSpan = endAngle.getRadians() - startAngle.getRadians();
	// Choose a factor to control the number of points
	// Adjust this factor to control the density of points
	float factor = 0.06;

	// Calculate the number of points based on the angle span and factor
	int numPoints = static_cast<int>(angleSpan / factor);

	for (int i = 0; i <= numPoints; ++i) {
		Math::Angle t = startAngle + i * angleSpan / numPoints;
		int x = static_cast<int>(center_x + radius * t.getCosine() + 0.5);
		int y = static_cast<int>(center_y + radius * t.getSine() + 0.5);
		_writePage->layer.setPixel(x, y, _writePage->foreColor);
	}
}

void TownsScreen::drawFan(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius) {
	Math::Angle startAngle, endAngle;
	startAngle.arcTangent2(start_y, start_x);
	endAngle.arcTangent2(end_y, end_x);

	if (endAngle < startAngle) {
		endAngle += 2 * M_PI;
	}
	// Calculate the angle span
	float angleSpan = endAngle.getRadians() - startAngle.getRadians();
	// Choose a factor to control the number of points
	// Adjust this factor to control the density of points
	float factor = 0.06;

	// Calculate the number of points based on the angle span and factor
	int numPoints = static_cast<int>(angleSpan / factor);

	for (int i = 0; i <= numPoints; ++i) {
		Math::Angle t = startAngle + i * angleSpan / numPoints;
		int x = static_cast<int>(center_x + radius * t.getCosine() + 0.5);
		int y = static_cast<int>(center_y + radius * t.getSine() + 0.5);
		_writePage->layer.drawLine(x, y, center_x, center_y, _writePage->foreColor);
	}
}

void TownsScreen::clearLayer(int layer) {
	if (layer < 0 || layer > 1)
		return;

	TownsScreenLayer *l = &_page[layer];
	if (!l->ready)
		return;

	_dirtyRects.push_back(Common::Rect(_width - 1, _height - 1));
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

	if (!_writePage->ready)
		return nullptr;
}

void TownsScreen::addDirtyRect(int x, int y, int w, int h) {
	if (w <= 0 || h <= 0 || _numDirtyRects > kDirtyRectsMax)
		return;

	if (_numDirtyRects == kDirtyRectsMax) {
		// full redraw
		_dirtyRects.clear();
		_dirtyRects.push_back(Common::Rect(_width - 1, _height - 1));
		_numDirtyRects++;
		return;
	}

	int x2 = x + w - 1;
	int y2 = y + h - 1;

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
	_dirtyRects.push_back(Common::Rect(_width - 1, _height - 1));
	_numDirtyRects = kFullRedraw;

	Graphics::Surface *s = _system->lockScreen();
	assert(s);
	memset(s->getPixels(), 0, _pitch * _height);
	_system->unlockScreen();
	update();

	_system->updateScreen();
}

void TownsScreen::scrollLayers(int flags, int offset, bool fast) {
	// This actually supports layer 0 only, since this is all we need.
	_scrollRemainder += offset;
	if (!_scrollRemainder)
		return;

	int step = (_scrollRemainder > 0) ? -1 : 1;

	_scrollRemainder += step;
	_scrollOffset = (_scrollOffset + step) % _page[0].layer.w;

	_dirtyRects.clear();
	_dirtyRects.push_back(Common::Rect(_width - 1, _height - 1));
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
