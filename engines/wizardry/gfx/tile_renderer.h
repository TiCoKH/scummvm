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
#include "wizardry/gfx/tile_screen.h"
#include "wizardry/gfx/wide_font.h"
#include "wizardry/profile.h"
#include "wizardry/resources/charset.h"

namespace Wizardry {

/**
 * Turns the tile screen into pixels. This is the only place that knows the
 * display of a release:
 *
 *   IBM-PC "200" set    tile  8x8    320x200, four colours
 *   IBM-PC "400" set    tile 16x8    640x200, two colours, shown as 640x400
 *   PC-98 "200" set     tile 16x8    640x200, eight colours, shown as 640x400
 *   PC-98 "400" set     tile 16x16   640x400, eight colours
 *
 * The 640x200 screens are drawn with every row doubled, so that all modes
 * come out at the proportions of a 4:3 monitor.
 */
class TileRenderer {
private:
	CharSet _charset;
	uint8 _tileW = 0, _tileH = 0;
	uint8 _rowScale = 1;
	uint8 _layout = kPixelsNone;
	const byte *_cgaPalette = nullptr;
	const WideFont *_wideFont = nullptr;

	void drawTile(const TileScreen &screen, int col, int row, byte *dst, uint pitch) const;

public:
	/**
	 * Loads the character set of the graphics set and takes its geometry
	 */
	bool open(const GfxSet &gfx);

	/** Size of a tile in the output */
	uint tileWidth() const { return _tileW; }
	uint tileHeight() const { return _tileH * _rowScale; }

	/** Size of the output: 320x200 or 640x400 */
	uint width() const { return TileScreen::COLS * tileWidth(); }
	uint height() const { return TileScreen::ROWS * tileHeight(); }

	/** Size of a tile in the files; images shown must have tiles of this size */
	uint8 dataTileWidth() const { return _tileW; }
	uint8 dataTileHeight() const { return _tileH; }

	const CharSet &charset() const { return _charset; }

	/**
	 * The font of kanji and full-width symbols, or nullptr: then they show
	 * as '?'. The caller keeps it.
	 */
	void setWideFont(const WideFont *font) { _wideFont = font; }

	/**
	 * Chooses the colours of the four-colour CGA set: a CgaPalette.
	 * The original's palettes are all at high intensity.
	 */
	void setCgaPalette(int which);

	/** Colours as red, green, blue bytes */
	const byte *palette() const;
	uint paletteSize() const;

	/**
	 * Draws the tiles that changed since the last call.
	 * @param dst    Top left of a buffer of width() x height() bytes
	 * @param area   Receives the part of the buffer that was drawn to
	 * @returns false if nothing had changed
	 */
	bool render(TileScreen &screen, byte *dst, uint pitch, Common::Rect &area) const;
};

} // End of namespace Wizardry

#endif
