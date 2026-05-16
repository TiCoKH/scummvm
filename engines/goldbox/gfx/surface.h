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

#ifndef GOLDBOX_GFX_SURFACE_H
#define GOLDBOX_GFX_SURFACE_H

#include "common/rect.h"
#include "graphics/font.h"
#include "graphics/managed_surface.h"

namespace Goldbox {
namespace Shared {
namespace Gfx {

#define TEXT_W 40
#define TEXT_H 25
#define FONT_W 8
#define FONT_H 8

struct Window : public Common::Rect {
	int _xOffset = 0;
	int _yOffset = 0;

	Window() : Common::Rect() {
	}
	Window(int x1, int y1, int x2, int y2, int xOffset = 0, int yOffset = 0) :
		Common::Rect(x1, y1, x2, y2), _xOffset(xOffset), _yOffset(yOffset) {
	}
	Window(const Common::Rect &r, int xOffset = 0, int yOffset = 0) :
		Common::Rect(r), _xOffset(xOffset), _yOffset(yOffset) {
	}
};

struct Position : public Common::Point {
	int _xOffset = 0;
	int _yOffset = 0;

	Position() : Common::Point() {
	}
	Position(int x, int y) : Common::Point(x, y) {
	}
	Position(int x, int y, int xOffset, int yOffset) : Common::Point(x, y),
		_xOffset(xOffset), _yOffset(yOffset) {
	}
};

/**
 * Implements the surface class views use when they call getSurface
 */
class Surface : public Graphics::ManagedSurface {
private:
	int _textX = 0, _textY = 0, _textColor = 10;
	Graphics::Font *_currentFont;

	unsigned char mapCharToIndex(unsigned char) const;

public:
	Surface();
	Surface(ManagedSurface &surf, const Common::Rect &bounds);

	static void setupPalette();
	void setToText();
	void setToSymbols();
	void writeString(const Common::String &str);
	void writeString(const unsigned char *str);
	void writeString(int x, int y, const Common::String &str);
	void writeString(int x, int y, const unsigned char *str);
	void writeStringC(const Common::String &str, int color);
	void writeStringC(const unsigned char *str, int color);
	void writeStringC(int x, int y, int color, const Common::String &str);
	void writeStringC(int x, int y, int color, const unsigned char *str);
	void writeCenteredString(const Common::String &str, int y);
	void writeChar(unsigned char c);
	void writeChar(int x, int y, unsigned char c);
	void writeGlyph(unsigned char c);
	void writeSymbol(unsigned char s_id);
	void writeSymbol(int x, int y, unsigned char s_id);

	/**
	 * Draws an 8x8 tile from the tile cache at the given character cell position.
	 * Mirrors x86 GFX_DrawTile8x8 (without-transparency path).
	 * @param charX Character column (pixel x = charX * 8)
	 * @param charY Character row    (pixel y = charY * 8)
	 * @param globalTileId Global tile ID as used by the x86 GFX_DrawTile8x8
	 * @param bgColor Optional replacement color for tpColorIndex
	 * @param tpColorIndex Optional color index to replace with bgColor
	 */
	void writeTile(int charX, int charY, uint16 globalTileId,
			uint32 bgColor = 0, uint32 tpColorIndex = 0);

	void writeCharC(unsigned char c, int color);
	void writeCharC(int x, int y, int color, unsigned char c);
	void writeGlyphC(int x, int y, int color, unsigned char g_id);
	void setTextPos(int x, int y);
	void setTextColor(int color);

	/**
	 * Clears an area from character position (start_x, start_y) to (end_x, end_y) using the specified color
	 */
	void clearBox(int start_x, int start_y, int end_x, int end_y, uint32 color);

	/**
	 * Draws a frame on the surface around the given area from (start_x, start_y)
	 * to (end_x, end_y) using explicit tile IDs.
	 *
	 * This is the generic helper that game-specific views should prefer when
	 * their border style differs.
	 */
	void drawFrameTiles(int start_x, int start_y, int end_x, int end_y,
			uint16 cornerTileId, uint16 sideTileId, uint16 edgeTileId,
			uint32 bgColor = 0, uint32 tpColorIndex = 0);

	/**
	 * Draws a frame on the surface around the given area from (start_x, start_y)
	 * to (end_x, end_y).
	 *
	 * Default style is Poolrad-compatible; game-specific code should call
	 * drawFrameTiles() with its own tile IDs when needed.
	 *
	 * @param bgColor Optional replacement color for tpColorIndex in frame tiles
	 * @param tpColorIndex Optional tile color index to replace
	 */
	void drawFrame(int start_x, int start_y, int end_x, int end_y,
			uint32 bgColor = 0, uint32 tpColorIndex = 0);

	/**
	 * Clears an area with the given color and draws a frame around it
	 */
	void drawWindow(int start_x, int start_y, int end_x, int end_y, uint32 color);
};

} // namespace Gfx
} // namespace Shared

using Shared::Gfx::Window;
using Shared::Gfx::Surface;

} // namespace Goldbox

#endif
