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

#include "wizardry/gfx/cell_screen.h"

namespace Wizardry {

CellScreen::CellScreen() {
	for (int row = 0; row < ROWS; ++row) {
		for (int col = 0; col < COLS; ++col) {
			Cell c = { kBlank, 0, 0 };
			_cells[row][col] = c;
			_dirty[row][col] = true;
		}
	}
}

void CellScreen::setCell(int col, int row, CellType type, uint16 value, uint8 slot) {
	if (!contains(col, row))
		return;

	const Cell c = { (uint8)type, slot, value };
	if (!(_cells[row][col] == c)) {
		_cells[row][col] = c;
		_dirty[row][col] = true;
	}
}

void CellScreen::setGlyph(int col, int row, uint16 glyph) {
	if (glyph == 0 || glyph == 32)
		setCell(col, row, kBlank);
	else
		setCell(col, row, kGlyph, glyph);
}

void CellScreen::clear() {
	for (int row = 0; row < ROWS; ++row) {
		for (int col = 0; col < COLS; ++col)
			setCell(col, row, kBlank);
	}

	for (uint i = 0; i < _images.size(); ++i)
		_images[i].used = false;
}

int CellScreen::addImage(const CellImage &image, int col, int row) {
	uint slot = 0;
	while (slot < _images.size() && _images[slot].used)
		++slot;
	if (slot == _images.size())
		_images.push_back(Image());

	Image &entry = _images[slot];
	entry.image = image;
	entry.col = col;
	entry.row = row;
	entry.used = true;

	for (int y = 0; y < image.rows; ++y) {
		for (int x = 0; x < image.cols; ++x) {
			if (contains(col + x, row + y)) {
				setCell(col + x, row + y, kImage, y * image.cols + x, slot);
				// The slot may have held another image before
				_dirty[row + y][col + x] = true;
			}
		}
	}

	return slot;
}

void CellScreen::removeImage(int slot) {
	if (slot < 0 || slot >= (int)_images.size() || !_images[slot].used)
		return;

	const Image &entry = _images[slot];
	for (int y = 0; y < entry.image.rows; ++y) {
		for (int x = 0; x < entry.image.cols; ++x) {
			const int col = entry.col + x, row = entry.row + y;
			if (contains(col, row) && _cells[row][col].type == kImage && _cells[row][col].slot == slot)
				setCell(col, row, kBlank);
		}
	}

	_images[slot].used = false;
}

void CellScreen::clearDirty() {
	for (int row = 0; row < ROWS; ++row) {
		for (int col = 0; col < COLS; ++col)
			_dirty[row][col] = false;
	}
}

void CellScreen::markAllDirty() {
	for (int row = 0; row < ROWS; ++row) {
		for (int col = 0; col < COLS; ++col)
			_dirty[row][col] = true;
	}
}

} // End of namespace Wizardry
