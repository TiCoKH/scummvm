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

#ifndef WIZARDRY_PROFILE_H
#define WIZARDRY_PROFILE_H

#include "common/platform.h"
#include "wizardry/detection.h"

namespace Wizardry {

enum PixelLayout {
	kPixelsNone,
	kPixelsCGA2bpp,    ///< 2 bits per pixel, 8 pixels per cell row
	kPixelsMono,       ///< 1 bit per pixel, 16 pixels per cell row
	kPixelsPlanarBRG   ///< glyphs 1 bit; picture cells are three planes: blue, red, green
};

enum PicturePacking {
	kPicturesPlain,             ///< fixed-size records, record = picture id - 1
	kPicturesPackedIndexed,     ///< zero-suppressed; block table inside the interpreter file
	kPicturesPackedSequential   ///< zero-suppressed; pictures follow each other on block boundaries
};

/**
 * One set of graphics files. The originals call them the "200" and the
 * "400" set; the number is part of the file name, not a line count.
 */
struct GfxSet {
	const char *charset;    ///< nullptr when the release has no such set
	const char *pictures;   ///< nullptr when the set has no picture file
	const char *title;      ///< nullptr when the set has no title picture
	uint8 cellW, cellH;     ///< size of a screen cell in pixels
	uint8 layout;           ///< PixelLayout
	uint16 screenW, screenH;

	bool isPresent() const { return charset != nullptr; }
};

/**
 * Everything that differs between releases and is not game logic.
 */
struct Profile {
	uint8 game;                ///< GameType
	Common::Platform platform;
	const char *codeFile;      ///< p-code file; not executed, kept for reference
	const char *interpreter;   ///< native interpreter; holds the picture table of PC-98 Wizardry V
	const char *scenario;      ///< SCENARIO.DATA, or DRAGON.DATA in Wizardry V
	const char *asciiText;     ///< message files, nullptr when absent
	const char *kanaText;
	const char *kanjiText;
	const char *huffman;       ///< decoding table for Wizardry IV and V text, else nullptr
	GfxSet low;                ///< the "200" set
	GfxSet high;               ///< the "400" set
	uint8 pictureCols, pictureRows;   ///< picture size in cells
	uint8 picturePacking;      ///< PicturePacking
};

/**
 * Returns the profile of a release, or nullptr if there is none.
 */
extern const Profile *findProfile(GameType game, Common::Platform platform);

} // End of namespace Wizardry

#endif // WIZARDRY_PROFILE_H
