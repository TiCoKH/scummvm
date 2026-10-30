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
#include "common/str.h"

#include "graphics/managed_surface.h"
#include "graphics/screen.h"
#include "ultima/shared/core/rect.h"

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
	TownsScreen(Graphics::Screen *scr);
	~TownsScreen();

	void setupLayer(int width, int height, int scaleW, int scaleH, int numCol);
	/** Initialize a page without changing the selected write page. */
	void setupPage(int page, int width, int height, int scaleW, int scaleH, int numCol);
	int getWritePage() const { return _writePage == &_page[1] ? 1 : 0; }
	/** Visibility mask: bit 0 = page 0, bit 1 = page 1. Page 0 has priority. Does not present. */
	void setVisiblePages(uint8 mask);
	/**
	 * Map an original GFX_setDisplayPage mode to an EGB_displayPage visibility mask.
	 * The original wrapper calls EGB_displayPage(work, 0, mode + 1), with mode 3
	 * mapped to 0: 0 = page 0, 1 = page 1, 2 = both (page 0 on top), 3 = none.
	 * Returns -1 for an unknown mode.
	 */
	static int legacyDisplayPageMask(int mode);
	/** Apply a GFX_setDisplayPage mode. Does not present. */
	bool setLegacyDisplayPage(int mode);
	uint8 getVisiblePages() const;
	void clearLayer(int layer);
	void setPalette(const byte *colors);
	void setPaletteForPage(int page, const byte *rgb, uint16 start, uint16 count);
	void setPaletteEntry(int page, uint16 index, byte red, byte green, byte blue);
	/** Update entries 1..15, preserving entry 0. Input is 16 RGB triplets. */
	void applyDefaultPalette4bit(const byte *rgb);
	void setForegroundColor(uint16 colorID);
	void setBackgroundColor(uint16 colorID);
	void setSurfaceFillColor(uint16 colorID);
	void setTransparentColor(uint16 colorID);
	void setTransparentColorEnabled(int page, bool enabled);
	/** Nonzero raster operations are recorded but rejected when drawing. */
	void setWriteMode(uint16 mode) { _writeMode = mode; }
	uint16 getWriteMode() const { return _writeMode; }
	void setFontStyle(uint16 style) { _fontStyle = style; }
	uint16 getFontStyle() const { return _fontStyle; }
	enum EGBPaintMode {
		kPaintBorderSolid = 0x0002,
		kPaintFillSolid = 0x0020
	};
	void setPaintMode(uint16 mode) { _paintMode = mode; }
	uint16 getPaintMode() const { return _paintMode; }
	/** Endpoints must be on the active page; offscreen line clipping is not implemented. */
	void drawLine(int x1, int y1, int x2, int y2);
	/** Inclusive endpoints; temporarily enables solid fill. */
	void drawFilledRectangle(int x1, int y1, int x2, int y2);
	/** Sets foreground/fill colors and temporarily uses write mode zero. */
	void drawColoredRectangle(int x1, int y1, int x2, int y2, uint16 color);
	/**
	 * Tightly packed rows: low nibble first at 4bpp, little-endian at 16bpp.
	 * Packed words retain bit 15; it is not interpreted as conventional alpha.
	 */
	bool putPixelBlock(const byte *src, uint32 srcSize, int x, int y,
		int width, int height, uint8 sourceBitsPerPixel);
	/** Raw move within an inclusive, surface-clipped region; no EGB viewport. */
	void scrollRegion(int x1, int y1, int x2, int y2,
		int deltaX, int deltaY, bool fillExposed);
	Graphics::ManagedSurface &getWriteSurface();
	/** Invalidate active-page coordinates, accounting for page scaling/scroll. */
	void invalidateRect(int x, int y, int width, int height);
	void drawRectangle(Rect rect);
	void drawRectangleC(Rect rect, int color);
	void drawRectangleS(Point ps, int w, int h);
	void drawRectangleSC(Point ps, int w, int h, int color);
	void setWritePage(int page);
	void drawWindow(Common::String name);
	/**
	 * EGB_arc parameter block {cx, cy, sx, sy, ex, ey, r}: start/end are vectors
	 * relative to the center. The arc sweeps from start to end in increasing
	 * screen angle (clockwise with y down). Equal directions draw a full circle.
	 * Pixel-exact EGB rasterization is not verified; a midpoint circle is used.
	 */
	void drawArc(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius);
	/**
	 * EGB_fan with the same parameter block as drawArc. kPaintFillSolid fills the
	 * sector with the fill color; kPaintBorderSolid draws the arc and both radii in
	 * the foreground color. Other paint bits are ignored with a warning.
	 */
	void drawFan(int16 center_x, int16 center_y, int16 start_x, int16 start_y, int16 end_x, int16 end_y, int16 radius);
	void addDirtyRect(int x, int y, int w, int h);
	void displayPage(int flags);
	void scrollLayers(int flags, int offset, bool fast);
	/** Compose dirty regions of the enabled pages into the backend screen. */
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
		uint16 bpp = 0;
		uint16 numCol = 0;
		uint16 hScroll = 0;
		uint16 foreColor = 0;
		uint16 backColor = 0;
		uint16 fillColor = 0;
		uint16 transColor = 0;
		uint8 scaleW = 1;
		uint8 scaleH = 1;
		bool onTop = false;
		bool enabled = false;
		bool ready = false;
		bool transparentEnabled = false;
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
	uint16 _paintMode;
	uint16 _writeMode;
	uint16 _fontStyle;
	Common::List<Common::Rect> _dirtyRects;
	TownsScreenLayer *_writePage;
	OSystem *_system;

	void drawCirclePoints(int xc, int yc, int x, int y);
	void drawRectangleWithMode(int x1, int y1, int x2, int y2, uint16 mode);
	void markFullDirty();
	bool canDraw(const char *operation) const;
	void plotPixelClipped(int x, int y, uint16 color);
	void plotLineClipped(int x1, int y1, int x2, int y2, uint16 color);
	void plotArcOutline(int xc, int yc, int sx, int sy, int ex, int ey, int radius, uint16 color);
	static bool isInArcSweep(int px, int py, int sx, int sy, int ex, int ey);
};

} // End of namespace Gfx
} // End of namespace Shared
} // End of namespace Ultima
#endif
