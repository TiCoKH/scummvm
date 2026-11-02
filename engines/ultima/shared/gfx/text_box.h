/* ScummVM - Graphic Adventure Engine
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef ULTIMA_SHARED_GFX_TEXT_BOX_H
#define ULTIMA_SHARED_GFX_TEXT_BOX_H

#include "common/scummsys.h"

namespace Ultima {
namespace Shared {
namespace Gfx {

class TownsScreen;

/**
 * Draws the exact Shift-JIS bytes on the active TownsScreen write page.
 * baselineY is the glyph's lower-left anchor; TextBox invalidates the region.
 */
class SjisTextRasterizer {
public:
	virtual ~SjisTextRasterizer() {}
	virtual void drawSjisBytes(TownsScreen &screen, int16 x, int16 baselineY,
			const byte *sjis, uint16 byteLength) = 0;
};

struct TextBoxState {
	int32 x;
	int32 y;
	int32 width;
	int32 height;
	int32 maxColumns;
	int32 maxRows;
	int32 cursorColumn;
	int32 cursorRow;
	int32 unknownState;
	uint16 fontColor;
	uint16 fontStyle;
	uint16 borderColor;
	uint16 backgroundColor;
	int32 textBackgroundMode;
	bool needsRedraw;
};

struct TextBoxConfig {
	int32 x;
	int32 y;
	int32 width;
	int32 height;
	int32 requestedColumns;
	int32 requestedRows;
	uint16 fontColor;
	uint16 borderColor;
	uint16 backgroundColor;
	int32 textBackgroundMode;

	TextBoxConfig() :
			x(0), y(0), width(0), height(0), requestedColumns(0),
			requestedRows(0), fontColor(15), borderColor(15),
			backgroundColor(0), textBackgroundMode(-1) {}
};

class TextBox {
public:
	TextBox(TownsScreen &screen, SjisTextRasterizer &rasterizer);

	bool initialize(const TextBoxConfig &config);
	bool isInitialized() const { return _initialized; }
	const TextBoxState &state() const { return _state; }

	void drawFrame();
	void prepare();
	void applyFont();
	void printSjis(const char *text);
	void advanceLine();
	void scrollUpOneLine();
	void setCursor(int32 column, int32 row);
	/**
	 * GFX_drawBoxBorder: if needsRedraw is false (box is visible), erases the
	 * full box rectangle with the given color, resets textBackgroundMode to -1
	 * and marks needsRedraw true. No-op when the box is already hidden.
	 */
	void hide(uint16 eraseColor);
	/**
	 * SCR_fillBoxArea: fill a text-cell region with the box background color.
	 * Coordinates are in cell units; the pixel rect is
	 *   x1 = box.x + 4 + xCell*8,  y1 = box.y + 2 + yCell*16
	 *   x2 = box.x + 11 + xEnd*8,  y2 = box.y + 17 + yEnd*16
	 */
	void fillArea(int32 xCell, int32 yCell, int32 xEnd, int32 yEnd);
	/**
	 * SCR_clearBoxArea: apply the selection/highlight raster operation
	 * (writeMode=4, paintMode=0x422, fg=15, fill=15) over a cell region,
	 * then restore the box font/fill colors and the global write/paint modes.
	 * Same pixel mapping as fillArea.
	 */
	void highlightArea(int32 xCell, int32 yCell, int32 xEnd, int32 yEnd);
	/**
	 * TEXT_printMappedAscii: convert each ASCII byte through the DARKNESS
	 * extended-glyph table and render the resulting 2-byte SJIS code at the
	 * current cursor position, advancing by 2 columns per glyph.
	 * Unmapped characters (table entry == 0) are silently skipped.
	 * Wraps and scrolls identically to printSjis.
	 */
	void printMappedAscii(const char *text);
	/**
	 * GFX_putCharDataIntoBox: render a single 16x16 glyph supplied as raw
	 * pixel data at the current cursor position without advancing the cursor.
	 */
	void putCharData(const byte *glyphData);

private:
	static bool isLeadByte(byte ch);
	static bool isTrailByte(byte ch);
	void renderSegment(const byte *text, uint16 length);

	TownsScreen &_screen;
	SjisTextRasterizer &_rasterizer;
	TextBoxState _state;
	bool _initialized;
};

} // namespace Gfx
} // namespace Shared
} // namespace Ultima

#endif
