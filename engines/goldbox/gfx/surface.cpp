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
#include "goldbox/gfx/amiga_borders.h"
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

// Amiga v1.3 screen palette (PAL_ARRAY, loaded by GFX_SetScreen), 0RGB.
static const uint16 kAmigaPalette[32] = {
	0x000, 0xdb9, 0xb97, 0x964, 0x521, 0x742, 0xa84, 0xcb6,
	0xff9, 0x007, 0x44b, 0x444, 0x555, 0x777, 0xaaa, 0xfff,
	0x444, 0x999, 0xba0, 0xdc0, 0xf1b, 0xf1b, 0x666, 0x888,
	0x800, 0xc00, 0xb86, 0xeb9, 0x070, 0x0a0, 0xc70, 0xfa4
};

void Surface::setupPalette() {
	if (g_engine->getPlatform() == Common::kPlatformAmiga) {
		Graphics::Palette pal(ARRAYSIZE(kAmigaPalette));
		for (uint i = 0; i < ARRAYSIZE(kAmigaPalette); ++i) {
			const uint16 c = kAmigaPalette[i];
			pal.set(i, ((c >> 8) & 0xF) * 17, ((c >> 4) & 0xF) * 17,
				(c & 0xF) * 17);
		}
		g_system->getPaletteManager()->setPalette(pal);
	} else {
		const Graphics::Palette &ega = Graphics::Palette::createEGAPalette();
		g_system->getPaletteManager()->setPalette(ega);
	}

	uint32 white = 0xffffffff;
	g_system->getPaletteManager()->setPalette((const byte *)&white, 255, 1);
}

unsigned char Surface::mapCharToIndex(unsigned char chr) const {
	// The Amiga disk font is indexed by ASCII.
	if (g_engine->getPlatform() == Common::kPlatformAmiga)
		return (chr >= 0x20 && chr < 0x7F) ? chr : ' ';

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

void Surface::writeString(const Common::String &str) {
	// TODO: Handle multiple lines
	Common::String idString;

    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char mappedIndex = mapCharToIndex(str[i]);
        idString += mappedIndex; // Append the converted character
    }

	_currentFont->drawString(this, idString, _textX * FONT_W, _textY * FONT_H,
		this->w - (_textX * FONT_W), _textColor);
	_textX+=str.size();
}

void Surface::writeString(const unsigned char *str) {
	if (!str)
		return;

	writeString(Common::String((const char *)str));
}

void Surface::writeStringC(const Common::String &str, int color){
	setTextColor(color);
	writeString(str);
}

void Surface::writeString(int x, int y, const Common::String &str) {
	setTextPos(x, y);
	writeString(str);
}

void Surface::writeStringC(int x, int y, int color, const Common::String &str) {
	setTextPos(x, y);
	setTextColor(color);
	writeString(str);
}

void Surface::writeStringC(const unsigned char *str, int color) {
	setTextColor(color);
	Common::String s((const char *)str);
	writeString(s);
}

void Surface::writeString(int x, int y, const unsigned char *str) {
	setTextPos(x, y);
	Common::String s((const char *)str);
	writeString(s);
}

void Surface::writeStringC(int x, int y, int color, const unsigned char *str) {
	setTextPos(x, y);
	setTextColor(color);
	Common::String s((const char *)str);
	writeString(s);
}

void Surface::writeCenteredString(const Common::String &str, int y) {
	int x = ((this->w / FONT_W) - str.size()) / 2;
	writeString(x, y, str);
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

void Surface::writeChar(int x, int y, unsigned char c) {
	setTextPos(x, y);
	writeChar(c);
}

void Surface::writeCharC(int x, int y, int color, unsigned char c) {
	setTextPos(x, y);
	setTextColor(color);
	writeChar(c);
}

void Surface::writeGlyphC(int x, int y, int color, unsigned char c) {
	setTextPos(x, y);
	setTextColor(color);
	writeGlyph(c);
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

void Surface::writeTile(int charX, int charY, uint16 globalTileId,
		uint32 bgColor, uint32 tpColorIndex) {
	// Resolves globalTileId through the engine tile cache and blits the
	// 8×8 tile at (charX*8, charY*8).
	// blitFrom is used so dirty-rect tracking matches every other draw path.
	const Graphics::ManagedSurface *tile =
		g_engine->getTileCache().tileSurface(globalTileId);
	if (!tile)
		return;

	const int dstX = charX * FONT_W;
	const int dstY = charY * FONT_H;

	if (bgColor == 0 && tpColorIndex == 0) {
		blitFrom(*tile, Common::Point(dstX, dstY));
		return;
	}

	for (int py = 0; py < FONT_H; ++py) {
		for (int px = 0; px < FONT_W; ++px) {
			const uint32 srcPixel = tile->getPixel(px, py);
			const uint32 outPixel = (srcPixel == tpColorIndex) ? bgColor : srcPixel;
			setPixel(dstX + px, dstY + py, outPixel);
		}
	}

	addDirtyRect(Common::Rect(dstX, dstY, dstX + FONT_W, dstY + FONT_H));
}

void Surface::drawFrameTiles(int startX, int startY, int endX, int endY,
		uint16 cornerTileId, uint16 sideTileId, uint16 edgeTileId,
		uint32 bgColor, uint32 tpColorIndex) {
	// Generic tile-frame renderer.
	// startX/startY/endX/endY are the INTERIOR bounds; the frame is drawn
	// one character cell outside in every direction, matching the x86 source:
	//   x_pos    = start_x - 1
	//   y_pos    = start_y - 1
	//   x_pos_00 = end_x   + 1
	//   y_pos_00 = end_y   + 1

	// The Amiga version blits its border bitmaps for the standard frame.
	if (cornerTileId == kFrameCornerTile && sideTileId == kFrameSideTile
			&& edgeTileId == kFrameEdgeTile && g_engine->_amigaBorders
			&& g_engine->_amigaBorders->isLoaded()) {
		g_engine->_amigaBorders->drawFrame(*this, startX, startY, endX, endY);
		return;
	}

	const int x0 = startX - 1;  // x_pos
	const int y0 = startY - 1;  // y_pos
	const int x1 = endX   + 1;  // x_pos_00
	const int y1 = endY   + 1;  // y_pos_00

	// Four corners
	writeTile(x0, y0, cornerTileId, bgColor, tpColorIndex);
	writeTile(x1, y0, cornerTileId, bgColor, tpColorIndex);
	writeTile(x0, y1, cornerTileId, bgColor, tpColorIndex);
	writeTile(x1, y1, cornerTileId, bgColor, tpColorIndex);

	// Top and bottom horizontal edges (start_x..end_x)
	for (int x = startX; x <= endX; ++x) {
		writeTile(x, y0, edgeTileId, bgColor, tpColorIndex);
		writeTile(x, y1, edgeTileId, bgColor, tpColorIndex);
	}

	// Left and right vertical edges (start_y..end_y)
	for (int y = startY; y <= endY; ++y) {
		writeTile(x0, y, sideTileId, bgColor, tpColorIndex);
		writeTile(x1, y, sideTileId, bgColor, tpColorIndex);
	}
}

void Surface::drawFrame(int startX, int startY, int endX, int endY,
		uint32 bgColor, uint32 tpColorIndex) {
	drawFrameTiles(startX, startY, endX, endY,
		kFrameCornerTile, kFrameSideTile, kFrameEdgeTile, bgColor, tpColorIndex);
}

void Surface::drawWindow(int startX, int startY, int endX, int endY, uint32 color) {
	clearBox(startX, startY, endX, endY, color);
	drawFrame(startX, startY, endX, endY, color);
}

void Surface::drawWindow(int startX, int startY, int endX, int endY,
		uint32 color, int titleColor, const Common::String &title) {
	clearBox(startX, startY, endX, endY, color);
	drawFrame(startX, startY, endX, endY, color);
	if (!title.empty()) {
		int x = (endX + startX - (int)title.size()) / 2;
		writeStringC(x, startY, titleColor, title);
	}
}

} // namespace Gfx
} // namespace Shared
} // namespace Goldbox
