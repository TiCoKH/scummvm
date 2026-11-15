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
	void initScreen() override;
	void setupPalette() override;
	Graphics::Font *createFont(Data::DaxFileManager &dax) override;
	void loadArtwork() override;
	uint8 mapChar(uint8 chr) const override;
	bool drawFrame(Graphics::ManagedSurface &dst, int startX, int startY,
		int endX, int endY) const override;
	bool hasTitlePictures() const override { return false; }

private:
	AmigaBorders _borders;
};

} // namespace Gfx
} // namespace Goldbox

#endif
