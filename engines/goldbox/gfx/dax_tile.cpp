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
#include "goldbox/gfx/dax_tile.h"

namespace Goldbox {
namespace Gfx {

DaxTile::DaxTile(Data::DaxBlock8x8D *daxBlock) : _daxBlock(daxBlock) {
    load();
}

void DaxTile::load() {
    assert(_daxBlock != nullptr);
    _chars.resize(_daxBlock->item_count);

    const uint8 *data = _daxBlock->_data.data(); // Pointer to the start of the DaxBlock data

    for (int i = 0; i < _daxBlock->item_count; ++i) {
        Graphics::ManagedSurface &s = _chars[i];
        s.create(FONT_W, FONT_H);

        for (int y = 0; y < FONT_H; y++) {
            for (int x = 0; x < FONT_W; x += 2) {
                uint8 byte = *data++;
                // Extract high nibble and set the corresponding pixel
                s.setPixel(x, y, (byte & 0xF0) >> 4);
                // Extract low nibble and set the corresponding pixel
                s.setPixel(x + 1, y, byte & 0x0F);
            }
        }
    }
}

void DaxTile::drawChar(Graphics::Surface *dst, uint32 chr, int x, int y, uint32 color) const {
    assert(chr >= 0 && chr < _chars.size());
    const Graphics::ManagedSurface &charImage = _chars[chr];
	dst->copyRectToSurface(charImage, x, y, Common::Rect(0, 0, charImage.w, charImage.h));
}

} // namespace Gfx
} // namespace Goldbox
