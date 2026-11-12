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

#include "wizardry/gfx/cell_renderer.h"

namespace Wizardry {

// CGA palette 1 at high intensity
static const byte CGA_PALETTE[4 * 3] = {
	0x00, 0x00, 0x00,  0x55, 0xff, 0xff,  0xff, 0x55, 0xff,  0xff, 0xff, 0xff
};

static const byte MONO_PALETTE[2 * 3] = {
	0x00, 0x00, 0x00,  0xff, 0xff, 0xff
};

// The colour number of a pixel is blue + 2 * red + 4 * green. The PC-98
// driver sets the palette so that colours 1 and 7 change places: text, which
// is drawn in the blue plane alone, comes out white.
static const byte PLANAR_PALETTE[8 * 3] = {
	0x00, 0x00, 0x00,  0xff, 0xff, 0xff,  0xff, 0x00, 0x00,  0xff, 0x00, 0xff,
	0x00, 0xff, 0x00,  0x00, 0xff, 0xff,  0xff, 0xff, 0x00,  0x00, 0x00, 0xff
};

bool CellRenderer::open(const GfxSet &gfx) {
	if (!_charset.open(gfx))
		return false;

	_cellW = gfx.cellW;
	_cellH = gfx.cellH;
	_layout = gfx.layout;
	_rowScale = (gfx.screenW == 640 && gfx.screenH == 200) ? 2 : 1;
	return true;
}

const byte *CellRenderer::palette() const {
	switch (_layout) {
	case kPixelsCGA2bpp:
		return CGA_PALETTE;
	case kPixelsPlanarBRG:
		return PLANAR_PALETTE;
	default:
		return MONO_PALETTE;
	}
}

uint CellRenderer::paletteSize() const {
	switch (_layout) {
	case kPixelsCGA2bpp:
		return 4;
	case kPixelsPlanarBRG:
		return 8;
	default:
		return 2;
	}
}

void CellRenderer::drawCell(const CellScreen &screen, int col, int row, byte *dst, uint pitch) const {
	const CellScreen::Cell &cell = screen.cell(col, row);
	const byte *src = nullptr;
	uint srcPitch = _cellW;
	byte fill = 0;

	switch (cell.type) {
	case CellScreen::kGlyph:
		src = _charset.glyph(cell.value);
		break;

	case CellScreen::kSolid:
		// All bits set: the last colour of the CGA, the text colour elsewhere
		fill = (_layout == kPixelsCGA2bpp) ? 3 : 1;
		break;

	case CellScreen::kImage: {
		const CellImage &image = screen.image(cell.slot);
		if (image.cellW == _cellW && image.cellH == _cellH && cell.value < (uint)image.cols * image.rows) {
			src = image.getBasePtr((cell.value % image.cols) * _cellW, (cell.value / image.cols) * _cellH);
			srcPitch = image.width();
		}
		break;
	}

	default:
		break;
	}

	dst += row * cellHeight() * pitch + col * _cellW;

	for (uint y = 0; y < _cellH; ++y) {
		for (uint repeat = 0; repeat < _rowScale; ++repeat, dst += pitch) {
			for (uint x = 0; x < _cellW; ++x)
				dst[x] = src ? src[y * srcPitch + x] : fill;
		}
	}
}

bool CellRenderer::render(CellScreen &screen, byte *dst, uint pitch, Common::Rect &area) const {
	int left = CellScreen::COLS, top = CellScreen::ROWS, right = -1, bottom = -1;

	for (int row = 0; row < CellScreen::ROWS; ++row) {
		for (int col = 0; col < CellScreen::COLS; ++col) {
			if (!screen.isDirty(col, row))
				continue;

			drawCell(screen, col, row, dst, pitch);
			left = MIN(left, col);
			top = MIN(top, row);
			right = MAX(right, col);
			bottom = MAX(bottom, row);
		}
	}

	screen.clearDirty();
	if (right < 0)
		return false;

	area = Common::Rect(left * cellWidth(), top * cellHeight(),
		(right + 1) * cellWidth(), (bottom + 1) * cellHeight());
	return true;
}

} // End of namespace Wizardry
