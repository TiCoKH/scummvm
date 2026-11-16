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

#ifndef GOLDBOX_GFX_BACKEND_AMIGA_INTRO_H
#define GOLDBOX_GFX_BACKEND_AMIGA_INTRO_H

#include "common/array.h"
#include "common/path.h"
#include "graphics/managed_surface.h"

namespace Goldbox {
namespace Gfx {

/**
 * The pictures of the Amiga "intro" program. The program is crunched with
 * PowerPacker inside a loader hunk; unpacked, its chip data hunk holds a
 * 320x200 painting, a 320x600 sheet of logos and credits (4 bitplanes
 * each) and the two 16-color palettes the intro fades.
 */
class AmigaIntro {
public:
	static const int kWidth = 320;
	static const int kPaintingHeight = 200;
	static const int kSheetHeight = 600;
	static const int kColors = 16;

	/** Read and unpack the intro program; false if it is missing or unknown. */
	bool load(const Common::Path &path);

	const Graphics::ManagedSurface &painting() const { return _painting; }
	const Graphics::ManagedSurface &sheet() const { return _sheet; }

	/** Palette 0 or 1, as 0RGB words. */
	const uint16 *palette(int index) const { return _palettes[index]; }

	/** One hunk of an Amiga program, without relocation. */
	struct Hunk {
		uint32 type;
		Common::Array<uint8> data;
	};

	/**
	 * Read the hunks of an AmigaDOS program (CODE, DATA, BSS; relocation,
	 * symbol and debug blocks are skipped).
	 */
	static bool readHunks(const Common::Array<uint8> &file, Common::Array<Hunk> &hunks);

private:
	static bool decrunch(const Common::Array<uint8> &packed, const uint8 efficiency[4],
		Common::Array<uint8> &out);
	static void decodePlanar(const uint8 *src, int height, int planeStride,
		Graphics::ManagedSurface &dst);

	Graphics::ManagedSurface _painting;
	Graphics::ManagedSurface _sheet;
	uint16 _palettes[2][kColors] = {};
};

} // namespace Gfx
} // namespace Goldbox

#endif
