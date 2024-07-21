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

void DaxTile::load() {
    assert(_daxBlock != nullptr);
    _chars.resize(_daxBlock->item_count);

    int i;
    int b;
    int y;
    int bit, pixel;
    const byte *data = _daxBlock->_data.data(); // Pointer to the start of the DaxBlock data

    for (i = 0; i < _daxBlock->item_count; ++i) {
        Graphics::ManagedSurface &s = _chars[i];
        s.create(FONT_W, FONT_H);

        for (bit = 0; bit < 4; ++bit) {
            for (y = 0; y < 8; ++y) {
                b = *data++; // Read the next byte from the DaxBlock data

                for (pixel = 0; pixel < 8; ++pixel) {
                    s.setPixel(pixel, y, s.getPixel(pixel, y)
                        | (((b >> (7 - pixel)) & 1) << bit));
                }
            }
        }
    }
}

void DaxTile::drawChar(Graphics::Surface *dst, uint32 chr, int x, int y, uint32 color) const {
    assert(chr >= 0 && chr < _chars.size());
    const Graphics::ManagedSurface &charImage = _chars[chr];

    Graphics::ManagedSurface dest(dst, DisposeAfterUse::NO);
    dest.blitFrom(charImage, Common::Point(x, y));
}

} // namespace Gfx
} // namespace Goldbox
