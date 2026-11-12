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

#ifndef WIZARDRY_GFX_CELL_RENDERER_H
#define WIZARDRY_GFX_CELL_RENDERER_H

#include "common/rect.h"
#include "wizardry/gfx/cell_screen.h"
#include "wizardry/profile.h"
#include "wizardry/resources/charset.h"

namespace Wizardry {

/**
 * Turns the cell screen into pixels. This is the only place that knows the
 * display of a release:
 *
 *   IBM-PC "200" set    cell  8x8    320x200, four colours
 *   IBM-PC "400" set    cell 16x8    640x200, two colours, shown as 640x400
 *   PC-98 "200" set     cell 16x8    640x200, eight colours, shown as 640x400
 *   PC-98 "400" set     cell 16x16   640x400, eight colours
 *
 * The 640x200 screens are drawn with every row doubled, so that all modes
 * come out at the proportions of a 4:3 monitor.
 */
class CellRenderer {
private:
	CharSet _charset;
	uint8 _cellW = 0, _cellH = 0;
	uint8 _rowScale = 1;
	uint8 _layout = kPixelsNone;

	void drawCell(const CellScreen &screen, int col, int row, byte *dst, uint pitch) const;

public:
	/**
	 * Loads the character set of the graphics set and takes its geometry
	 */
	bool open(const GfxSet &gfx);

	/** Size of a cell in the output */
	uint cellWidth() const { return _cellW; }
	uint cellHeight() const { return _cellH * _rowScale; }

	/** Size of the output: 320x200 or 640x400 */
	uint width() const { return CellScreen::COLS * cellWidth(); }
	uint height() const { return CellScreen::ROWS * cellHeight(); }

	/** Size of a cell in the files; images shown must have cells of this size */
	uint8 dataCellWidth() const { return _cellW; }
	uint8 dataCellHeight() const { return _cellH; }

	const CharSet &charset() const { return _charset; }

	/** Colours as red, green, blue bytes */
	const byte *palette() const;
	uint paletteSize() const;

	/**
	 * Draws the cells that changed since the last call.
	 * @param dst    Top left of a buffer of width() x height() bytes
	 * @param area   Receives the part of the buffer that was drawn to
	 * @returns false if nothing had changed
	 */
	bool render(CellScreen &screen, byte *dst, uint pitch, Common::Rect &area) const;
};

} // End of namespace Wizardry

#endif
