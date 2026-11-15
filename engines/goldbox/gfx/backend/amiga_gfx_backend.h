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

#ifndef GOLDBOX_GFX_BACKEND_AMIGA_GFX_BACKEND_H
#define GOLDBOX_GFX_BACKEND_AMIGA_GFX_BACKEND_H

#include "goldbox/gfx/backend/gfx_backend.h"
#include "goldbox/gfx/backend/amiga_borders.h"

namespace Goldbox {
namespace Gfx {

/**
 * Amiga: 320x200 custom screen with a 32-color palette, the "poolrad"
 * disk font, and window borders blitted from bitmaps (poolradamigagfx.dat).
 */
class AmigaGfxBackend : public GfxBackend {
public:
	~AmigaGfxBackend() override;

	void initScreen() override;
	void setupPalette() override;
	void setPaletteMode(PaletteMode mode) override;
	PaletteMode paletteMode() const override {
		return _paletteMode < 0 ? kPaletteNormal : (PaletteMode)_paletteMode;
	}
	Graphics::Font *createFont(Data::DaxFileManager &dax) override;
	void loadArtwork() override;
	uint8 mapChar(uint8 chr) const override;
	void drawText(Graphics::ManagedSurface &dst, const Graphics::Font &font,
		const Common::String &glyphs, int x, int y, int maxWidth,
		uint32 color) const override;
	void drawGlyph(Graphics::ManagedSurface &dst, const Graphics::Font &font,
		uint32 glyph, int x, int y, uint32 color) const override;
	void drawRune(Graphics::ManagedSurface &dst, const Graphics::Font &font,
		uint32 glyph, int x, int y, uint32 color) const override;
	bool drawFrame(Graphics::ManagedSurface &dst, int startX, int startY,
		int endX, int endY) const override;
	/** CustomizeIcon_ShowIconFrames draws the ready cursor frame twice. */
	uint8 iconEditorActionCursorFrame() const override { return 0; }
	const ArtColors &artColors() const override;

private:
	/** Text pen for a DOS text color; the shadow pass uses the next pen. */
	static uint32 textPen(uint32 color);

	AmigaBorders _borders;
	int _paletteMode = -1;
	/** The "protect" disk font: runes at the DOS game font indices. */
	Graphics::Font *_runeFont = nullptr;
};

} // namespace Gfx
} // namespace Goldbox

#endif
