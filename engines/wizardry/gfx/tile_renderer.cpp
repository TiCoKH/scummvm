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

#include "wizardry/gfx/tile_renderer.h"

namespace Wizardry {

// The CGA palettes at high intensity. Colour 0 is the background, black.
static const byte CGA_PALETTE_0[4 * 3] = {
	0x00, 0x00, 0x00,  0x55, 0xff, 0x55,  0xff, 0x55, 0x55,  0xff, 0xff, 0x55
};

// Video mode 5: the colour-burst bit off gives cyan, red and white on an
// RGB monitor
static const byte CGA_PALETTE_3[4 * 3] = {
	0x00, 0x00, 0x00,  0x55, 0xff, 0xff,  0xff, 0x55, 0x55,  0xff, 0xff, 0xff
};

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

bool TileRenderer::open(const GfxSet &gfx) {
	if (!_charset.open(gfx))
		return false;

	_tileW = gfx.tileW;
	_tileH = gfx.tileH;
	_layout = gfx.layout;
	_rowScale = (gfx.screenW == 640 && gfx.screenH == 200) ? 2 : 1;
	return true;
}

void TileRenderer::setCgaPalette(int which) {
	switch (which) {
	case kCgaPalette0:
		_cgaPalette = CGA_PALETTE_0;
		break;
	case kCgaPalette1:
		_cgaPalette = CGA_PALETTE;
		break;
	default:
		// The original's own choice
		_cgaPalette = CGA_PALETTE_3;
		break;
	}
}

const byte *TileRenderer::palette() const {
	switch (_layout) {
	case kPixelsCGA2bpp:
		return _cgaPalette ? _cgaPalette : CGA_PALETTE_3;
	case kPixelsPlanarBRG:
		return PLANAR_PALETTE;
	default:
		return MONO_PALETTE;
	}
}

uint TileRenderer::paletteSize() const {
	switch (_layout) {
	case kPixelsCGA2bpp:
		return 4;
	case kPixelsPlanarBRG:
		return 8;
	default:
		return 2;
	}
}

void TileRenderer::drawTile(const TileScreen &screen, int col, int row, byte *dst, uint pitch) const {
	const TileScreen::Tile &tile = screen.tile(col, row);
	const byte *src = nullptr;
	uint srcPitch = _tileW;
	byte fill = 0;
	byte wide[32 * 32];

	switch (tile.type) {
	case TileScreen::kGlyph:
		src = _charset.glyph(tile.value);
		break;

	case TileScreen::kWide:
		// From the kanji font, in the colour of text; '?' without one
		if (_wideFont && _tileW * _tileH <= sizeof(wide) && _wideFont->drawChar(tile.value, wide, _tileW, _tileW, _tileH))
			src = wide;
		else
			src = _charset.glyph('?');
		break;

	case TileScreen::kSolid:
		// All bits set: the last colour of the CGA, the text colour elsewhere
		fill = (_layout == kPixelsCGA2bpp) ? 3 : 1;
		break;

	case TileScreen::kImage: {
		const TileImage &image = screen.image(tile.slot);
		if (image.tileW == _tileW && image.tileH == _tileH && tile.value < (uint)image.cols * image.rows) {
			src = image.getBasePtr((tile.value % image.cols) * _tileW, (tile.value / image.cols) * _tileH);
			srcPitch = image.width();
		}
		break;
	}

	default:
		break;
	}

	dst += row * tileHeight() * pitch + col * _tileW;

	for (uint y = 0; y < _tileH; ++y) {
		for (uint repeat = 0; repeat < _rowScale; ++repeat, dst += pitch) {
			for (uint x = 0; x < _tileW; ++x)
				dst[x] = src ? src[y * srcPitch + x] : fill;
		}
	}
}

bool TileRenderer::render(TileScreen &screen, byte *dst, uint pitch, Common::Rect &area) const {
	int left = TileScreen::COLS, top = TileScreen::ROWS, right = -1, bottom = -1;

	for (int row = 0; row < TileScreen::ROWS; ++row) {
		for (int col = 0; col < TileScreen::COLS; ++col) {
			if (!screen.isDirty(col, row))
				continue;

			drawTile(screen, col, row, dst, pitch);
			left = MIN(left, col);
			top = MIN(top, row);
			right = MAX(right, col);
			bottom = MAX(bottom, row);
		}
	}

	screen.clearDirty();
	if (right < 0)
		return false;

	area = Common::Rect(left * tileWidth(), top * tileHeight(),
		(right + 1) * tileWidth(), (bottom + 1) * tileHeight());
	return true;
}

} // End of namespace Wizardry
