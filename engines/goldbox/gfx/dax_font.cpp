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

#include "goldbox/data/daxblock.h"
#include "goldbox/gfx/dax_font.h"

namespace Goldbox {
namespace Gfx {

DaxFont::DaxFont(Data::DaxBlock8x8D *daxBlock, int glyphSize) :
        _daxBlock(daxBlock), _glyphCount(0), _glyphSize(glyphSize) {
    load();
}

void DaxFont::load() {
    assert(_daxBlock != nullptr);
    _glyphCount = _daxBlock->_data.size() / (_glyphSize * _glyphSize / 8);
}

void DaxFont::drawChar(Graphics::Surface *dst, uint32 index, int x, int y, uint32 color) const {
    if (index >= _glyphCount)
        return; // if index is out of bounds, do nothing
    const int rowBytes = _glyphSize / 8;
    const byte *src = &_daxBlock->_data[index * _glyphSize * rowBytes];

    // One bit per pixel, rows of whole bytes, high bit first.
    for (int yc = 0; yc < _glyphSize; ++yc, ++y, src += rowBytes) {
        byte *dest = (byte *)dst->getBasePtr(x, y);
        for (int xc = 0; xc < _glyphSize; ++xc, ++dest) {
            if (src[xc / 8] & (0x80 >> (xc & 7)))
                *dest = color;
        }
    }
}

} // namespace Gfx
} // namespace Goldbox
