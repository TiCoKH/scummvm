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
	// r is the outer rect (corners at r.left/r.top/r.right/r.bottom).
	// The base drawFrame takes interior bounds, so shift inward by 1.
	Shared::Gfx::Surface::drawFrame(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1);
}

void Surface::drawWindow(uint8 left, uint8 top, uint8 right, uint8 bottom) {
	// Delegates to the base implementation: drawFrame + clearBox.
	Shared::Gfx::Surface::drawWindow(left, top, right, bottom, 0);
}

} // namespace Gfx
} // namespace Poolrad
} // namespace Goldbox
