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

#ifndef WIZARDRY_GFX_WIDE_FONT_H
#define WIZARDRY_GFX_WIDE_FONT_H

#include "common/scummsys.h"

namespace Wizardry {

/**
 * The font of the two-byte characters that the PC-98 takes from its kanji
 * ROM rather than from the game's character set: kanji and full-width
 * symbols. Each takes one tile.
 */
class WideFont {
public:
	virtual ~WideFont() {}

	/**
	 * Draws a character into a tile, one byte a pixel: 1 for the ink,
	 * 0 for the rest
	 * @param code  Shift-JIS, lead byte above
	 * @returns false if the font has no such character
	 */
	virtual bool drawChar(uint16 code, byte *dst, uint pitch, uint width, uint height) const = 0;
};

#ifdef USE_SJIS
/**
 * Creates the font from what ScummVM can find for the PC-98: FONT.BMP,
 * FONT.ROM or its own SJIS.FNT
 * @returns nullptr if there is none
 */
WideFont *createPC98WideFont();
#endif

} // End of namespace Wizardry

#endif
