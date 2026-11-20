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

#ifndef WIZARDRY_GFX_TILE_SCREEN_H
#define WIZARDRY_GFX_TILE_SCREEN_H

#include "common/array.h"
#include "common/str.h"
#include "wizardry/resources/tile_image.h"

namespace Wizardry {

/**
 * The screen as the game sees it: 40 x 25 tiles on every machine. Text,
 * window frames, the maze view and pictures are all made of tiles, as in
 * the original display driver. Nothing here knows how large a tile is in
 * pixels; that is the business of TileRenderer.
 */
class TileScreen {
public:
	enum {
		COLS = 40,
		ROWS = 25
	};

	enum TileType {
		kBlank,
		kGlyph,   ///< value is a glyph of the character set
		kSolid,   ///< filled tile
		kImage    ///< value is a tile of the image in the given slot, counted row by row
	};

	struct Tile {
		uint8 type;
		uint8 slot;
		uint16 value;

		bool operator==(const Tile &c) const {
			return type == c.type && slot == c.slot && value == c.value;
		}
	};

private:
	struct Image {
		TileImage image;
		int16 col, row;
		bool used;
	};

	Tile _tiles[ROWS][COLS];
	bool _twoByte = false;
	bool _dirty[ROWS][COLS];
	Common::Array<Image> _images;

public:
	TileScreen();

	static bool contains(int col, int row) {
		return col >= 0 && col < COLS && row >= 0 && row < ROWS;
	}

	const Tile &tile(int col, int row) const { return _tiles[row][col]; }

	/**
	 * Whether text holds two-byte (Shift-JIS) characters, as in Japanese
	 * Wizardry I. A byte 0x81 to 0x9F starts one, which takes one tile.
	 */
	void setTwoByteText(bool twoByte) { _twoByte = twoByte; }
	bool twoByteText() const { return _twoByte; }

	/** Tiles a string takes: a two-byte character counts once */
	int textWidth(const Common::String &str) const;

	/**
	 * The glyph of a two-byte character, as the PC-98 driver finds it
	 * (offset 0x10AA): hiragana from 0x81, katakana from 0xD4. Kanji, which
	 * the PC-98 takes from its kanji ROM, show as '?'.
	 */
	static uint16 twoByteGlyph(byte lead, byte trail);

	/**
	 * Changes a tile. Positions outside the screen are ignored.
	 */
	void setTile(int col, int row, TileType type, uint16 value = 0, uint8 slot = 0);

	void setBlank(int col, int row) { setTile(col, row, kBlank); }

	/**
	 * Puts a glyph. Glyphs 0 and 32 are blank whatever the character set
	 * holds, as in the original driver.
	 */
	void setGlyph(int col, int row, uint16 glyph);

	void clear();

	/**
	 * Shows an image with its top left tile at the given position. The
	 * screen keeps a copy. Tiles outside the screen are left out.
	 * @param skipBlank  Leave the tiles below the image's empty ones as they
	 *                   are: an image in a bit plane of its own, as the
	 *                   PC-98 title, lets them show through
	 * @returns the slot, for removeImage()
	 */
	int addImage(const TileImage &image, int col, int row, bool skipBlank = false);

	/**
	 * Takes an image off the screen. Its tiles that still show it turn blank.
	 */
	void removeImage(int slot);

	const TileImage &image(uint slot) const { return _images[slot].image; }

	bool isDirty(int col, int row) const { return _dirty[row][col]; }
	void clearDirty();
	void markAllDirty();
};

} // End of namespace Wizardry

#endif
