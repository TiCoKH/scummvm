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

#ifndef WIZARDRY_RESOURCES_CHARSET_H
#define WIZARDRY_RESOURCES_CHARSET_H

#include "wizardry/profile.h"
#include "wizardry/resources/cell_image.h"

namespace Wizardry {

/**
 * The character set file of a graphics set: text glyphs, window borders,
 * the wall pieces of the maze view and, on the Japanese machines, kana.
 * Each glyph fills one screen cell.
 */
class CharSet {
private:
	uint8 _cellW = 0, _cellH = 0;
	uint _count = 0;
	Common::Array<byte> _pixels;

public:
	bool open(const GfxSet &gfx);

	uint8 cellWidth() const { return _cellW; }
	uint8 cellHeight() const { return _cellH; }

	/**
	 * Number of glyphs: 512, or 256 in IBM-PC Wizardry IV
	 */
	uint size() const { return _count; }

	/**
	 * Pixels of a glyph, cellWidth() bytes per row. The colour numbers are
	 * 0-3 on CGA four colours and 0-1 elsewhere.
	 */
	const byte *glyph(uint index) const;
};

} // End of namespace Wizardry

#endif
