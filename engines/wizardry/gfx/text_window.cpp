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

#include "wizardry/gfx/text_window.h"

namespace Wizardry {

enum FrameGlyph {
	kFrameTopLeft = 1,
	kFrameTop,
	kFrameTopRight,
	kFrameLeft,
	kFrameRight,
	kFrameBottomLeft,
	kFrameBottom,
	kFrameBottomRight
};

TextWindow::TextWindow(CellScreen &screen, const Common::Rect &bounds, bool framed) :
		_screen(screen), _bounds(bounds), _inner(bounds) {
	if (framed)
		_inner.grow(-1);
}

void TextWindow::clear() {
	drawFrame();
	CLRRECT(0, 0, width(), height());
	MVCURSOR(0, 0);
}

void TextWindow::drawFrame() {
	if (_inner == _bounds)
		return;

	const int left = _bounds.left, right = _bounds.right - 1;
	const int top = _bounds.top, bottom = _bounds.bottom - 1;

	for (int x = left + 1; x < right; ++x) {
		_screen.setGlyph(x, top, kFrameTop);
		_screen.setGlyph(x, bottom, kFrameBottom);
	}
	for (int y = top + 1; y < bottom; ++y) {
		_screen.setGlyph(left, y, kFrameLeft);
		_screen.setGlyph(right, y, kFrameRight);
	}

	_screen.setGlyph(left, top, kFrameTopLeft);
	_screen.setGlyph(right, top, kFrameTopRight);
	_screen.setGlyph(left, bottom, kFrameBottomLeft);
	_screen.setGlyph(right, bottom, kFrameBottomRight);
}

void TextWindow::PRINTCHR(uint16 glyph) {
	if (_cursor.x >= 0 && _cursor.x < width() && _cursor.y >= 0 && _cursor.y < height())
		_screen.setGlyph(_inner.left + _cursor.x, _inner.top + _cursor.y, glyph);
	++_cursor.x;
}

void TextWindow::PRINTSTR(const Common::String &str) {
	for (uint i = 0; i < str.size(); ++i)
		PRINTCHR((byte)str[i]);
}

void TextWindow::printCentered(int y, const Common::String &str) {
	PRINTSTR((width() - (int)str.size()) / 2, y, str);
}

void TextWindow::newLine() {
	_cursor.x = 0;
	if (++_cursor.y >= height())
		_cursor.y = 0;
}

void TextWindow::printCenteredLine(const Common::String &str) {
	const int row = _cursor.y;
	PRINTSTR(width() / 2 - (1 + (int)str.size()) / 2, row, str);
	MVCURSOR(0, row + 1);
}

void TextWindow::printInt(int value, int fieldWidth) {
	const Common::String str = Common::String::format("%d", value);

	if ((int)str.size() > fieldWidth) {
		if (value < 0) {
			PRINTCHR('-');
			--fieldWidth;
		}
		for (; fieldWidth > 0; --fieldWidth)
			PRINTCHR('*');
	} else {
		for (int i = str.size(); i < fieldWidth; ++i)
			PRINTCHR(' ');
		PRINTSTR(str);
	}
}

Common::Rect TextWindow::centeredBounds(int textWidth, int y, int height) {
	const int half = (textWidth + 2) / 2;
	return Common::Rect(19 - half, y, 19 - half + half * 2 + 2, y + height);
}

void TextWindow::CLRRECT(int x1, int y1, int x2, int y2) {
	for (int y = MAX(y1, 0); y < MIN(y2, height()); ++y) {
		for (int x = MAX(x1, 0); x < MIN(x2, width()); ++x)
			_screen.setBlank(_inner.left + x, _inner.top + y);
	}
}

int TextWindow::showImage(const CellImage &image, int x, int y) {
	return _screen.addImage(image, _inner.left + x, _inner.top + y);
}

} // End of namespace Wizardry
