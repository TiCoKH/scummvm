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

#include "goldbox/gfx/backend/pc98_gfx_backend.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "engines/util.h"
#include "graphics/palette.h"
#include "graphics/paletteman.h"

namespace Goldbox {
namespace Gfx {

void Pc98GfxBackend::initScreen() {
	initGraphics(640, 400);
}

void Pc98GfxBackend::setupPalette() {
	// Digital palette: bit 0 = blue plane, bit 1 = red, bit 2 = green.
	Graphics::Palette pal(8);
	for (uint i = 0; i < 8; ++i)
		pal.set(i, (i & 2) ? 0xFF : 0, (i & 4) ? 0xFF : 0, (i & 1) ? 0xFF : 0);
	g_system->getPaletteManager()->setPalette(pal);
}

Graphics::Font *Pc98GfxBackend::createFont(Data::DaxFileManager &dax) {
	error("PC-98 text output is not implemented yet");
	return nullptr;
}

} // namespace Gfx
} // namespace Goldbox
