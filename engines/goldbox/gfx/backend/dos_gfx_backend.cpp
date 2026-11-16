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

#include "goldbox/gfx/backend/dos_gfx_backend.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"
#include "goldbox/data/daxfilemanager.h"
#include "goldbox/gfx/dax_font.h"
#include "goldbox/gfx/pic.h"

namespace Goldbox {
namespace Gfx {

static const uint8 kDosFontBlock = 201;

void DosGfxBackend::initScreen() {
	initGraphics(320, 200);
}

void DosGfxBackend::setupPalette() {
	g_system->getPaletteManager()->setPalette(Graphics::Palette::createEGAPalette());
}

Graphics::Font *DosGfxBackend::createFont(Data::DaxFileManager &dax) {
	Data::DaxBlock8x8D *block = dynamic_cast<Data::DaxBlock8x8D *>(
		dax.get8x8d().getBlockById(kDosFontBlock));
	if (!block)
		error("Failed to load font block %d from 8x8d container", kDosFontBlock);
	return new DaxFont(block);
}

const GfxBackend::ArtColors &DosGfxBackend::artColors() const {
	// Walls see through LightMagenta. SetCombatPalette swaps EGA registers
	// 0 and 8: the cleared background shows DarkGray and art color 8 shows
	// as black. LoadIconAnim turns CPIC LightMagenta into DarkGray. Icon keys
	// are GDConsts.defIconCol and +8. 3D colors: DisplayMapView (54c1:016d)
	// and SetView3DColors(6, 7, 0, 0xB) in FixMain.Init (EGA colors).
	static const ArtColors kColors = {
		13, 8, 8, 13, 8,
		{ 0x91, 0xA2, 0xB3, 0xC4, 0xE6, 0xF7 },
		{
			{ 0x00, 0x0F, 0x0C, 0x0B, 0x0D, 0x0A, 0x09, 0x0E,
			  0x00, 0x0F, 0x0C, 0x0B, 0x0D, 0x0A, 0x09, 0x0E },
			0x00, 0x0F, 0x08, 0x07, 0x08, 0x06, 0x01,
			{ 0x0B, 0x00, 0x07, 0x06 }
		}
	};
	return kColors;
}

Pic *DosGfxBackend::loadTitlePicture(Data::DaxFileManager &dax, int index) const {
	// TITLE.DAX blocks 1 and 2.
	Data::DaxBlockPic *block = dynamic_cast<Data::DaxBlockPic *>(
		dax.getTitle().getBlockById(index));
	if (!block)
		error("Failed to load title block %d from title container", index);
	return Pic::read(block);
}

uint8 DosGfxBackend::fontIndex(uint8 chr) {
	// The DOS font holds A-Z at 0x01-0x1A and punctuation and digits at
	// their ASCII codes; '$' and '%' have no glyph.
	if (chr >= 'a' && chr <= 'z')
		chr -= 'a' - 'A';
	if (chr >= 'A' && chr <= 'Z')
		return chr - 0x40;
	if (chr >= 0x20 && chr <= 0x40)
		return (chr == '$' || chr == '%') ? 0xFF : chr;
	return 0xFF;
}

} // namespace Gfx
} // namespace Goldbox
