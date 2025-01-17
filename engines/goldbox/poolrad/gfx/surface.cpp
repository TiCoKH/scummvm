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

#include "goldbox/poolrad/gfx/surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Gfx {

void Surface::drawFrame(const Common::Rect &r) {
    writeSymbol(20, r.left, r.top);
    for (int x = r.left + 1; x <= r.right - 1; x++) {
        writeSymbol(22);
    }
    writeSymbol(20);
    for (int y = r.top + 1; y <= r.bottom - 1; y++) {
        writeSymbol(21, r.left, y);
        writeSymbol(21, r.right, y);
    }
    writeSymbol(20, r.left, r.bottom);
    for (int x = r.left + 1; x <= r.right - 1; x++) {
        writeSymbol(22);
    }
    writeSymbol(20);
}

void Surface::drawWindow(uint8 left, uint8 top, uint8 right, uint8 bottom) {
    drawFrame(Common::Rect(left - 1, top - 1, right + 1, bottom + 1));
    clearBox(left, top, right, bottom, 0);
}

} // namespace Gfx
} // namespace Poolrad
} // namespace Goldbox
