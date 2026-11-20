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

#ifndef WIZARDRY_GFX_TEXT_WINDOW_H
#define WIZARDRY_GFX_TEXT_WINDOW_H

#include "common/rect.h"
#include "common/str.h"
#include "wizardry/gfx/tile_screen.h"

namespace Wizardry {

/**
 * A rectangle of the tile screen to print into, like a window of the
 * original program. All positions are in tiles and count from the top left
 * of the inside of the window; what falls outside is not drawn.
 *
 * The frame uses glyphs 1 to 8 of the character set: top left corner, top
 * edge, top right corner, left edge, right edge, bottom left corner,
 * bottom edge, bottom right corner.
 */
class TextWindow {
private:
	TileScreen &_screen;
	Common::Rect _bounds;   ///< whole window, in screen tiles
	Common::Rect _inner;    ///< the part inside the frame
	Common::Point _cursor;

public:
	TextWindow(TileScreen &screen, const Common::Rect &bounds, bool framed);

	int width() const { return _inner.width(); }
	int height() const { return _inner.height(); }

	/**
	 * Draws the frame, if the window has one, and blanks the inside
	 */
	void clear();
	void drawFrame();

	void MVCURSOR(int x, int y) {
		_cursor.x = x;
		_cursor.y = y;
	}

	/**
	 * Prints a glyph of the character set at the cursor
	 */
	void PRINTCHR(uint16 glyph);

	/** Prints a two-byte character of the kanji font at the cursor */
	void printWide(uint16 code);

	/**
	 * Prints a string; each byte is a glyph number
	 */
	void PRINTSTR(const Common::String &str);
	void PRINTSTR(int x, int y, const Common::String &str) {
		MVCURSOR(x, y);
		PRINTSTR(str);
	}
	void printCentered(int y, const Common::String &str);

	int cursorX() const { return _cursor.x; }
	int cursorY() const { return _cursor.y; }

	/** Screen tile of the top left of the inside of the window */
	int innerLeft() const { return _inner.left; }
	int innerTop() const { return _inner.top; }

	/**
	 * Moves to the start of the next row; after the last row comes the first
	 */
	void newLine();

	/**
	 * Prints a string centred on the cursor's row and goes to the next row
	 */
	void printCenteredLine(const Common::String &str);

	/**
	 * Prints a number right-justified in a field. A number that does not
	 * fit is shown as asterisks.
	 */
	void printInt(int value, int fieldWidth);

	/**
	 * Bounds of a framed window centred on the screen whose inside holds
	 * a text of the given width, as the original program places them
	 */
	static Common::Rect centeredBounds(int textWidth, int y, int height);

	void CLRRECT(int x1, int y1, int x2, int y2);

	/**
	 * Shows an image with its top left tile at the given position
	 * @param skipBlank  See TileScreen::addImage()
	 * @returns the slot, for TileScreen::removeImage()
	 */
	int showImage(const TileImage &image, int x, int y, bool skipBlank = false);
};

} // End of namespace Wizardry

#endif
