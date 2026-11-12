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

#ifndef WIZARDRY_GFX_CELL_SCREEN_H
#define WIZARDRY_GFX_CELL_SCREEN_H

#include "common/array.h"
#include "wizardry/resources/cell_image.h"

namespace Wizardry {

/**
 * The screen as the game sees it: 40 x 25 cells on every machine. Text,
 * window frames, the maze view and pictures are all made of cells, as in
 * the original display driver. Nothing here knows how large a cell is in
 * pixels; that is the business of CellRenderer.
 */
class CellScreen {
public:
	enum {
		COLS = 40,
		ROWS = 25
	};

	enum CellType {
		kBlank,
		kGlyph,   ///< value is a glyph of the character set
		kSolid,   ///< filled cell
		kImage    ///< value is a cell of the image in the given slot, counted row by row
	};

	struct Cell {
		uint8 type;
		uint8 slot;
		uint16 value;

		bool operator==(const Cell &c) const {
			return type == c.type && slot == c.slot && value == c.value;
		}
	};

private:
	struct Image {
		CellImage image;
		int16 col, row;
		bool used;
	};

	Cell _cells[ROWS][COLS];
	bool _dirty[ROWS][COLS];
	Common::Array<Image> _images;

public:
	CellScreen();

	static bool contains(int col, int row) {
		return col >= 0 && col < COLS && row >= 0 && row < ROWS;
	}

	const Cell &cell(int col, int row) const { return _cells[row][col]; }

	/**
	 * Changes a cell. Positions outside the screen are ignored.
	 */
	void setCell(int col, int row, CellType type, uint16 value = 0, uint8 slot = 0);

	void setBlank(int col, int row) { setCell(col, row, kBlank); }

	/**
	 * Puts a glyph. Glyphs 0 and 32 are blank whatever the character set
	 * holds, as in the original driver.
	 */
	void setGlyph(int col, int row, uint16 glyph);

	void clear();

	/**
	 * Shows an image with its top left cell at the given position. The
	 * screen keeps a copy. Cells outside the screen are left out.
	 * @returns the slot, for removeImage()
	 */
	int addImage(const CellImage &image, int col, int row);

	/**
	 * Takes an image off the screen. Its cells that still show it turn blank.
	 */
	void removeImage(int slot);

	const CellImage &image(uint slot) const { return _images[slot].image; }

	bool isDirty(int col, int row) const { return _dirty[row][col]; }
	void clearDirty();
	void markAllDirty();
};

} // End of namespace Wizardry

#endif
