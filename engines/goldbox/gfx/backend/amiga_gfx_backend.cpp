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

// Colors 0-15 of each screen kind, loaded into the ColorMap by the routines
// misnamed Mouse_SetSprite1..5/SND_Dat (SPRITE_ARRAY_1..5 at 0x2534EC).
// Normal is PAL_ARRAY: DisplayNoncombatInterface, DisplayPortrait.
static const uint16 kAmigaSubPalettes[5][16] = {
	// kPaletteNormal (SPRITE_ARRAY_3)
	{ 0x000, 0xdb9, 0xb97, 0x964, 0x521, 0x742, 0xa84, 0xcb6,
	  0xff9, 0x007, 0x44b, 0x444, 0x555, 0x777, 0xaaa, 0xfff },
	// kPaletteCombat (SPRITE_ARRAY_1): DisplayCombatInterface, ViewCharMenu
	{ 0x222, 0x06b, 0x096, 0x0aa, 0x000, 0x940, 0xb44, 0xfaa,
	  0xfe9, 0x59f, 0x0d8, 0x5ff, 0x555, 0x777, 0xaaa, 0xfff },
	// kPaletteCharacter (SPRITE_ARRAY_2): CreateCharacterMenu, CustomizeIcon
	{ 0x000, 0x06b, 0x096, 0x0aa, 0x000, 0x940, 0xb44, 0xfaa,
	  0xfe9, 0x59f, 0x0d8, 0x5ff, 0x555, 0x777, 0xaaa, 0xfff },
	// kPaletteView3D (SPRITE_ARRAY_4): Display3DMapView, DisplayMapView
	{ 0x000, 0x06b, 0x096, 0x0aa, 0x000, 0x940, 0xb44, 0xfaa,
	  0xfe9, 0x59f, 0x0d8, 0x5ff, 0x555, 0x777, 0xaaa, 0xfff },
	// kPalettePicture (SPRITE_ARRAY_5): EclPicture
	{ 0x000, 0x124, 0x953, 0xb97, 0xecb, 0x710, 0xa42, 0xdb0,
	  0xff5, 0x074, 0x053, 0x369, 0x555, 0x777, 0xaaa, 0xfff }
};

static void setAmigaColors(const uint16 *colors, uint start, uint count) {
	Graphics::Palette pal(count);
	for (uint i = 0; i < count; ++i) {
		const uint16 c = colors[i];
		pal.set(i, ((c >> 8) & 0xF) * 17, ((c >> 4) & 0xF) * 17, (c & 0xF) * 17);
	}
	g_system->getPaletteManager()->setPalette(pal, start);
}

AmigaGfxBackend::~AmigaGfxBackend() {
	delete _runeFont;
}

void AmigaGfxBackend::initScreen() {
	initGraphics(320, 200);
}

void AmigaGfxBackend::setupPalette() {
	setAmigaColors(kAmigaPalette, 0, ARRAYSIZE(kAmigaPalette));
	_paletteMode = kPaletteNormal;
}

void AmigaGfxBackend::setPaletteMode(PaletteMode mode) {
	if (mode == _paletteMode)
		return;
	setAmigaColors(kAmigaSubPalettes[mode], 0, 16);
	_paletteMode = mode;
}

Graphics::Font *AmigaGfxBackend::createFont(Data::DaxFileManager &dax) {
	Common::File fontFile;
	if (!fontFile.open(Common::Path("fonts/poolrad/8")))
		error("Failed to open Amiga font fonts/poolrad/8");

	// TextRend.PrintSpaces switches to the "protect" font for runes.
	Common::File runeFile;
	delete _runeFont;
	_runeFont = nullptr;
	if (runeFile.open(Common::Path("fonts/protect/8")))
		_runeFont = new Graphics::AmigaFont(&runeFile);
	else
		warning("Amiga font fonts/protect/8 not found; runes use the game font");
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
	// 3D colors are View3D palette entries: ARRAY_COLOR_LOOKUP (0x2327A7),
	// DisplayMapView_CeilColor/FloorColor/WallColor (0x230A62..0x230AE2) and
	// SetView3DColors(11, 4, 14, 5) before GB_EngineMain.
	static const ArtColors kColors = {
		0, 0, -1, -1, -1,
		{ 0x19, 0x2A, 0x3B, 0x76, 0x85, 0xEF },
		{
			{ 0x04, 0x0F, 0x06, 0x0B, 0x07, 0x0A, 0x09, 0x08,
			  0x04, 0x0F, 0x06, 0x0B, 0x07, 0x0A, 0x09, 0x08 },
			0x04, 0x0F, 0x0C, 0x0E, 0x0C, 0x05, 0x01,
			{ 0x0B, 0x04, 0x0E, 0x05 }
		}
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

void AmigaGfxBackend::drawSelectedName(Graphics::ManagedSurface &dst,
		const Graphics::Font &font, const Common::String &glyphs, int x, int y,
		int maxWidth) const {
	// CharHlp.DisplayParty (0x23DEA6): leading spaces are skipped, a bar
	// in pen 28 (green) is filled under the name, then the name is drawn in
	// pen 14 and again one pixel right in pen 15 (white), both JAM1.
	uint skip = 0;
	while (skip < glyphs.size() && glyphs[skip] == ' ')
		++skip;
	const Common::String name = glyphs.substr(skip);
	const int nameX = x + (int)skip * 8;
	const int width = MIN<int>((int)name.size() * 8 + 1, x + maxWidth - nameX);
	if (width <= 0)
		return;
	dst.fillRect(Common::Rect(nameX, y, nameX + width, y + 8), 28);
	font.drawString(&dst, name, nameX, y, width, 14);
	font.drawString(&dst, name, nameX + 1, y, width - 1, 15);
}

const GfxBackend::PartyColors &AmigaGfxBackend::partyColors() const {
	// DisplayCreatureName: disabled 24, enemy 18, friendly 28.
	// PrintArmorClass: 28. PrintHitPoints passes DOS colors 10 (full) and 8
	// (injured), which DisplayString turns into pens 26 and 24.
	static const PartyColors kColors = { 24, 18, 28, 28, 26, 24 };
	return kColors;
}

void AmigaGfxBackend::drawRune(Graphics::ManagedSurface &dst,
		const Graphics::Font &font, uint32 glyph, int x, int y,
		uint32 color) const {
	if (!_runeFont) {
		drawGlyph(dst, font, glyph, x, y, color);
		return;
	}
	// One pass, no shadow. The C port passes its pens directly: the code
	// wheel draws in 30 (DOS LightRed), PROTECTION text in 28 (White);
	// other colors follow PrintSpaces' 16 - color / 2 + color.
	uint32 pen;
	if (color == 12)
		pen = 30;
	else if (color == 15)
		pen = 28;
	else
		pen = color < 16 ? 16 - color / 2 + color : color;
	_runeFont->drawChar(&dst, glyph, x, y, pen);
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
