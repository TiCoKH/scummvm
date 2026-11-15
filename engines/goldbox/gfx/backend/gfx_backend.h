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

#ifndef GOLDBOX_GFX_BACKEND_GFX_BACKEND_H
#define GOLDBOX_GFX_BACKEND_GFX_BACKEND_H

#include "common/platform.h"
#include "graphics/font.h"
#include "graphics/managed_surface.h"

namespace Goldbox {

namespace Data {
class DaxFileManager;
}

namespace Gfx {

class Pic;

/**
 * Platform side of the graphics: screen mode, palette, text font and the
 * platform's own artwork. Common rendering code asks the backend instead of
 * checking the platform; each platform backend stands alone.
 */
class GfxBackend {
public:
	/** Color conventions of the platform's DAX artwork. */
	struct ArtColors {
		uint8 wallTransparent;  ///< See-through color of 3D wall tiles
		uint8 combatBackground; ///< Background of combat-style screens
		int combatBlack;        ///< Art color shown as black in combat, -1 for none
		int combatPicRemapFrom; ///< CPIC color recolored at load, -1 for none
		int combatPicRemapTo;
		/**
		 * Key colors of the six character icon color bytes: the low nibble is
		 * replaced by the byte's low color, the high nibble by its high color.
		 */
		uint8 iconColorKeys[6];
	};

	/** What the screen shows, for platforms that change palette by screen. */
	enum PaletteMode {
		kPaletteNormal,    ///< Menus, portraits, character sheets
		kPaletteCombat,    ///< Combat screen
		kPaletteCharacter, ///< Character creation and icon editor
		kPaletteView3D,    ///< 3D view, area and wilderness maps
		kPalettePicture    ///< Encounter and event pictures
	};

	virtual ~GfxBackend() {}

	/** Create the backend of a platform; DOS for any other platform. */
	static GfxBackend *create(Common::Platform platform);

	/** Switch to the platform's screen mode. */
	virtual void initScreen() = 0;

	/** Load the platform palette (entries 0-254). */
	virtual void setupPalette() = 0;

	/** Switch to the palette of a kind of screen; most platforms keep one. */
	virtual void setPaletteMode(PaletteMode mode) {}
	virtual PaletteMode paletteMode() const { return kPaletteNormal; }

	/**
	 * Create the game font from the loaded DAX data or platform files.
	 * The caller owns the font.
	 */
	virtual Graphics::Font *createFont(Data::DaxFileManager &dax) = 0;

	/** Load platform artwork that isn't in the DAX files. */
	virtual void loadArtwork() {}

	/** Glyph of the game font for a character of a game string, 0xFF for none. */
	virtual uint8 mapChar(uint8 chr) const = 0;

	/**
	 * Draw glyphs in a game text color (a DOS EGA color number) with the
	 * platform's pens and text style.
	 */
	virtual void drawText(Graphics::ManagedSurface &dst, const Graphics::Font &font,
		const Common::String &glyphs, int x, int y, int maxWidth,
		uint32 color) const {
		font.drawString(&dst, glyphs, x, y, maxWidth, color);
	}

	/** Draw one glyph in a game text color, see drawText(). */
	virtual void drawGlyph(Graphics::ManagedSurface &dst, const Graphics::Font &font,
		uint32 glyph, int x, int y, uint32 color) const {
		font.drawChar(&dst, glyph, x, y, color);
	}

	/**
	 * Draw a rune (code wheel, PROTECTION text). The glyph is the DOS game
	 * font index (rune + 0x40 on the code wheel); DOS and PC-98 keep the
	 * runes in the game font.
	 */
	virtual void drawRune(Graphics::ManagedSurface &dst, const Graphics::Font &font,
		uint32 glyph, int x, int y, uint32 color) const {
		drawGlyph(dst, font, glyph, x, y, color);
	}

	/**
	 * Draw the standard window frame around the interior character cells
	 * startX..endX, startY..endY.
	 * @return false to draw it with the common frame tiles.
	 */
	virtual bool drawFrame(Graphics::ManagedSurface &dst, int startX, int startY,
		int endX, int endY) const {
		return false;
	}

	/** Whether the title screens are pictures in the title DAX family. */
	/**
	 * Load title picture 1 or 2, or nullptr when the port keeps its title
	 * outside the game data (the Amiga "intro" program).
	 */
	virtual Pic *loadTitlePicture(Data::DaxFileManager &dax, int index) const {
		return nullptr;
	}

	/** Pixel size of a text cell; the PC-98 draws at double resolution. */
	virtual int cellSize() const { return 8; }

	/**
	 * Cursor frame under the icon editor's action icon (ShowIconFrames):
	 * DOS uses the action frame, other ports may reuse the ready frame.
	 */
	virtual uint8 iconEditorActionCursorFrame() const { return 1; }

	/** Color conventions of the platform's artwork. */
	virtual const ArtColors &artColors() const = 0;
};

/** Background color of combat-style screens on the running platform. */
uint8 combatBackgroundColor();

} // namespace Gfx
} // namespace Goldbox

#endif
