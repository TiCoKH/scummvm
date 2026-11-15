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

#include "goldbox/gfx/backend/amiga_gfx_backend.h"
#include "common/file.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "engines/util.h"
#include "graphics/fonts/amigafont.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"

namespace Goldbox {
namespace Gfx {

// Screen palette of the v1.3 program (PAL_ARRAY, loaded by GFX_SetScreen), 0RGB.
static const uint16 kAmigaPalette[32] = {
	0x000, 0xdb9, 0xb97, 0x964, 0x521, 0x742, 0xa84, 0xcb6,
	0xff9, 0x007, 0x44b, 0x444, 0x555, 0x777, 0xaaa, 0xfff,
	0x444, 0x999, 0xba0, 0xdc0, 0xf1b, 0xf1b, 0x666, 0x888,
	0x800, 0xc00, 0xb86, 0xeb9, 0x070, 0x0a0, 0xc70, 0xfa4
};

void AmigaGfxBackend::initScreen() {
	initGraphics(320, 200);
}

void AmigaGfxBackend::setupPalette() {
	Graphics::Palette pal(ARRAYSIZE(kAmigaPalette));
	for (uint i = 0; i < ARRAYSIZE(kAmigaPalette); ++i) {
		const uint16 c = kAmigaPalette[i];
		pal.set(i, ((c >> 8) & 0xF) * 17, ((c >> 4) & 0xF) * 17, (c & 0xF) * 17);
	}
	g_system->getPaletteManager()->setPalette(pal);
}

Graphics::Font *AmigaGfxBackend::createFont(Data::DaxFileManager &dax) {
	Common::File fontFile;
	if (!fontFile.open(Common::Path("fonts/poolrad/8")))
		error("Failed to open Amiga font fonts/poolrad/8");
	return new Graphics::AmigaFont(&fontFile);
}

void AmigaGfxBackend::loadArtwork() {
	if (!_borders.load(Common::Path("poolradamigagfx.dat")))
		warning("poolradamigagfx.dat not found; using the common frame tiles");
}

const GfxBackend::ArtColors &AmigaGfxBackend::artColors() const {
	// Walls see through color 0. Combat screens are cleared to color 0, the
	// palette has no combat swap and icons are loaded unchanged. Icon keys
	// are the table read by CHARACTER_IconLoad (v1.3 program at 0x285043).
	static const ArtColors kColors = {
		0, 0, -1, -1, -1,
		{ 0x19, 0x2A, 0x3B, 0x76, 0x85, 0xEF }
	};
	return kColors;
}

uint8 AmigaGfxBackend::mapChar(uint8 chr) const {
	// The disk font is indexed by ASCII.
	return (chr >= 0x20 && chr < 0x7F) ? chr : ' ';
}

uint32 AmigaGfxBackend::textPen(uint32 color) {
	// Text pens are pairs in the upper half of the palette. The C port
	// replaced the common DOS text colors at the call sites (matched calls
	// of DisplayString, MenuHorizontal etc.); any other color below 16 is
	// converted by DisplayString itself to 16 + (color & ~1).
	switch (color) {
	case 10: // LightGreen: normal text
		return 28;
	case 13: // LightMagenta: prompts
		return 26;
	case 14: // Yellow
		return 24;
	case 15: // White: highlights and hotkeys
		return 16;
	default:
		return (color < 16) ? 16 + (color & ~1u) : color;
	}
}

void AmigaGfxBackend::drawText(Graphics::ManagedSurface &dst,
		const Graphics::Font &font, const Common::String &glyphs, int x, int y,
		int maxWidth, uint32 color) const {
	// DisplayString draws the text in the pen, then again one pixel to the
	// right in the next pen.
	const uint32 pen = textPen(color);
	font.drawString(&dst, glyphs, x, y, maxWidth, pen);
	if (pen >= 16 && pen < 31)
		font.drawString(&dst, glyphs, x + 1, y, maxWidth, pen + 1);
}

void AmigaGfxBackend::drawGlyph(Graphics::ManagedSurface &dst,
		const Graphics::Font &font, uint32 glyph, int x, int y,
		uint32 color) const {
	const uint32 pen = textPen(color);
	font.drawChar(&dst, glyph, x, y, pen);
	if (pen >= 16 && pen < 31)
		font.drawChar(&dst, glyph, x + 1, y, pen + 1);
}

bool AmigaGfxBackend::drawFrame(Graphics::ManagedSurface &dst, int startX,
		int startY, int endX, int endY) const {
	if (!_borders.isLoaded())
		return false;
	_borders.drawFrame(dst, startX, startY, endX, endY);
	return true;
}

} // namespace Gfx
} // namespace Goldbox
