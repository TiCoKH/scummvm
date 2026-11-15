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

#ifndef GOLDBOX_GFX_BACKEND_PC98_GFX_BACKEND_H
#define GOLDBOX_GFX_BACKEND_PC98_GFX_BACKEND_H

#include "goldbox/gfx/backend/gfx_backend.h"

namespace Goldbox {
namespace Gfx {

/**
 * PC-98: 640x400 with three bitplanes (8 digital colors). Japanese text is
 * drawn through the text VRAM and is not implemented yet.
 */
class Pc98GfxBackend : public GfxBackend {
public:
	void initScreen() override;
	void setupPalette() override;
	Graphics::Font *createFont(Data::DaxFileManager &dax) override;
	uint8 mapChar(uint8 chr) const override { return chr; }
	bool hasTitlePictures() const override { return false; }
};

} // namespace Gfx
} // namespace Goldbox

#endif
