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

#ifndef ULTIMA_SHARED_GFX_TOWNS_SCREEN_H
#define ULTIMA_SHARED_GFX_TOWNS_SCREEN_H

#include "common/system.h"
#include "common/list.h"
#include "common/hashmap.h"
#include "common/scummsys.h"

#include "graphics/managed_surface.h"
#include "ultima/shared/core/rect.h"
#include "ultima/shared/core/towns_window.h"
#include "ultima/shared/gfx/visual_surface.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

// Helper class for FM-Towns output (required for specific hardware effects like switching graphics layers on and off).

class TownsScreen {
public:
	enum {
		kDirtyRectsMax = 20,
		kFullRedraw = (kDirtyRectsMax + 1)
	};

public:
	TownsScreen(Screen *scr);
	~TownsScreen();

	void setupLayer(int width, int height, int scaleW, int scaleH, int numCol);
	void clearLayer(int layer);
	void setPalette(const byte *colors);
	void setForegroundColor(uint16 colorID);
	void setBackgroundColor(uint16 colorID);
	void setSurfaceFillColor(uint16 colorID);
	void setTransparentColor(uint16 colorID);
	void drawLine(int x1, int y1, int x2, int y2);
	void drawRectangle(Rect rect);
	void drawRectangleC(Rect rect, int color);
	void drawRectangleS(Point ps, int w, int h);
	void drawRectangleSC(Point ps, int w, int h, int color);
	void setWritePage(int page); 
	void drawWindow(Common::String name);
	void drawArc(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius);
	void drawFan(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius);
	void addDirtyRect(int x, int y, int w, int h);
	void displayPage(int flags);
	void scrollLayers(int flags, int offset, bool fast);
	void update();
	bool isScrolling(int direction, int threshold = 0) const { return (direction == 0) ? _scrollRemainder != threshold : (direction == 1 ? _scrollRemainder > threshold : _scrollRemainder < threshold); }

	uint8 *getLayerPixels(int layer, int x, int y) const;
	int getLayerPitch(int layer) const { return (layer >= 0 && layer < 2) ? _page[layer].layer.pitch : 0; }
	int getLayerWidth(int layer) const { return (layer >= 0 && layer < 2) ? _page[layer].layer.w : 0; }
	int getLayerHeight(int layer) const { return (layer >= 0 && layer < 2) ? _page[layer].layer.h : 0; }
	int getLayerBpp(int layer) const { return (layer >= 0 && layer < 2) ? _page[layer].bpp : 0; }
	int getLayerScaleW(int layer) const { return (layer >= 0 && layer < 2) ? _page[layer].scaleW : 0; }
	int getLayerScaleH(int layer) const { return (layer >= 0 && layer < 2) ? _page[layer].scaleH : 0; }

private:
	struct TownsScreenLayer {
		Graphics::ManagedSurface layer;
		uint16 bpp;
		uint16 numCol;
		uint16 hScroll;
		uint16 foreColor;
		uint16 backColor;
		uint16 fillColor;
		uint16 transColor;
		uint8 scaleW;
		uint8 scaleH;
		bool onTop;
		bool enabled;
		bool ready;
	} _page[2];

#ifdef USE_RGB_COLOR
	void update16BitPalette();
	uint16 calc16BitColor(const uint8 *palEntry);
#endif

	int _height;
	int _width;
	int _pitch;
	uint16 _scrollOffset;
	int _scrollRemainder;
	Graphics::PixelFormat _pixelFormat;

	int _numDirtyRects;
	Common::List<Common::Rect> _dirtyRects;
	TownsScreenLayer *_writePage;
	OSystem *_system;
};

} // End of namespace Gfx
} // End of namespace Shared
} // End of namespace Ultima
#endif
