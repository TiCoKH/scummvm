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

#include "wizardry/resources/charset.h"
#include "wizardry/resources/files.h"

namespace Wizardry {

bool CharSet::open(const GfxSet &gfx) {
	Common::Array<byte> data;
	_count = 0;
	_pixels.clear();

	if (!gfx.charset || !readFile(gfx.charset, data))
		return false;

	_cellW = gfx.cellW;
	_cellH = gfx.cellH;

	// Glyphs have one plane on every machine
	const uint glyphBytes = cellDataSize(_cellH, 1);
	const uint glyphPixels = _cellW * _cellH;
	_count = data.size() / glyphBytes;
	_pixels.resize(_count * glyphPixels);

	for (uint i = 0; i < _count; ++i)
		decodeCell(&data[i * glyphBytes], glyphBytes, &_pixels[i * glyphPixels],
			_cellW, _cellW, _cellH, 1);

	return _count != 0;
}

const byte *CharSet::glyph(uint index) const {
	if (index >= _count)
		index = 0;
	return &_pixels[index * _cellW * _cellH];
}

} // End of namespace Wizardry
