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

#include "common/system.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Shared {
namespace Gfx {

#define BACK_COLOR 0
#define FONT_COLOR 10

Surface::Surface() : Graphics::ManagedSurface() {
	_currentFont = g_engine->_font;
}

Surface::Surface(ManagedSurface &surf, const Common::Rect &bounds) :
	Graphics::ManagedSurface(surf, bounds) {
	_currentFont = g_engine->_font;
}

void Surface::setupPalette() {
	Graphics::Palette ega = Graphics::Palette::createEGAPalette();
	g_system->getPaletteManager()->setPalette(ega);

	uint32 white = 0xffffffff;
	g_system->getPaletteManager()->setPalette((const byte *)&white, 255, 1);
}

unsigned char Surface::mapCharToIndex(unsigned char chr) const {
    if (chr >= 'a' && chr <= 'z') {
        chr = chr - ('a' - 'A'); // Convert lowercase to uppercase
    }
    if (chr >= 0x41 && chr <= 0x5A) {
        return chr - 0x40; // A-Z mapped to 0x01-0x1A
    } else if (chr >= 0x20 && chr <= 0x40) {
		return (chr == 0x24 || chr == 0x25) ? 0xFF : chr; // Map '$' and '%' to 0xFF
	} else {
        return 0xFF; // An invalid index that signifies no drawing
    }
}

void Surface::setToText() {
	_currentFont = g_engine->_font;
}

void Surface::setToSymbols() {
	_currentFont = g_engine->_symbols;
}

void Surface::writeString(const Common::String &str) {
/*	StringArray lines;
	lines.split(str, '\n', this->w / FONT_W);

	for (uint lineNum = 0; lineNum < lines.size(); ++lineNum) {
		if (lineNum > 0) {
			_textX = 0;
			++_textY;
		}
*/
	Common::String idString;

    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char mappedIndex = mapCharToIndex(str[i]);
        idString += mappedIndex; // Append the converted character
    }

	_currentFont->drawString(this, idString, _textX * FONT_W, _textY * FONT_H,
		this->w - (_textX * FONT_W), _textColor);
		//_textX += lines[lineNum].size();
}

void Surface::writeStringC(const Common::String &str, int color){
	setTextColor(color);
	writeString(str);
}

void Surface::writeString(const Common::String &str, int x, int y) {
	setTextPos(x, y);
	writeString(str);
}

void Surface::writeStringC(const Common::String &str, int color, int line, int column) {
	setTextPos(column, line);
	setTextColor(color);
	writeString(str);
}

void Surface::writeString(const unsigned char *str, int x, int y) {
	setTextPos(x, y);
	Common::String s((const char *)str);
	writeString(s);
}

void Surface::writeStringC(const unsigned char *str, int color, int line, int column) {
	setTextPos(column, line);
	setTextColor(color);
	Common::String s((const char *)str);
	writeString(s);
}

void Surface::writeCenteredString(const Common::String &str, int y) {
	int x = ((this->w / FONT_W) - str.size()) / 2;
	writeString(str, x, y);
}

void Surface::writeChar(unsigned char c) {
	unsigned char f_id = mapCharToIndex(c);
	setToText();
	_currentFont->drawChar(this, f_id, _textX * FONT_W, _textY * FONT_H, _textColor);
	++_textX;
}

void Surface::writeGlyph(unsigned char c) {
	setToText();
	_currentFont->drawChar(this, c, _textX * FONT_W, _textY * FONT_H, _textColor);
	++_textX;
}

void Surface::writeCharC(unsigned char c, int color) {
	setTextColor(color);
	writeChar(c);
}

void Surface::writeChar(unsigned char c, int x, int y) {
	setTextPos(x, y);
	writeChar(c);
}

void Surface::writeCharC(unsigned char c, int color, int x, int y) {
	setTextPos(x, y);
	setTextColor(color);
	writeChar(c);
}

void Surface::writeGlyphC(unsigned char c, int color, int x, int y) {
	setTextPos(x, y);
	setTextColor(color);
	writeGlyph(c);
}

void Surface::writeSymbol(unsigned char c) {
	setToSymbols();
	_currentFont->drawChar(this, c, _textX * FONT_W, _textY * FONT_H, _textColor);
	++_textX;
}

void Surface::writeSymbol(unsigned char c, int x, int y) {
	setTextPos(x, y);
	writeSymbol(c);
}

void Surface::setTextPos(int x, int y) {
	_textX = x;
	_textY = y;
}

void Surface::setTextColor(int color) {
	_textColor = color;
}

void Surface::clearBox(int start_x, int start_y, int end_x, int end_y, uint32 color) {
	// Convert character positions to pixel positions
	Common::Rect rect(start_x * FONT_W, start_y * FONT_H, (end_x + 1) * FONT_W, (end_y + 1) * FONT_H);

	// Fill the rectangle with the given color
	fillRect(rect, color);
}

} // namespace Gfx
} // namespace Shared
} // namespace Goldbox
