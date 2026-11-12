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

#include "wizardry/gfx/wizardry_font.h"
#include "wizardry/data/globals.h"
#include "wizardry/libs/file.h"

namespace Wizardry {

void WizardryFont::drawChar(Graphics::Surface *dst, uint32 chr, int x, int y, uint32 color) const {
	assert(x >= 0 && y >= 0 && (x + 8) <= dst->w && (y + 8) <= dst->h);
	assert(chr >= 32 && chr < (32 + 64));
	const byte *charData = _data + (chr - 32) * 8;

	for (int yp = 0; yp < 8; ++yp, ++y) {
		byte *lineP = (byte *)dst->getBasePtr(x, y);
		byte row = charData[yp];

		for (int xp = 0; xp < 8; ++xp, ++lineP) {
			if (row & (1 << xp))
				*lineP = color;
		}
	}
}

/*------------------------------------------------------------------------*/

WizardryFontV1::WizardryFontV1(const byte *data) : WizardryFont(_data) {
	Common::copy(data, data + 64 * 8, _data);
}

WizardryFontV1::WizardryFontV1(Common::SeekableReadStream &src) : WizardryFont(_data) {
	src.read(_data, 64 * 8);
}


} // namespace Wizardry
