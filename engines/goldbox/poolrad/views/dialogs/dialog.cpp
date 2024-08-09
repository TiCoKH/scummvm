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

#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

void Dialog::drawFrame(const Common::Rect &r) {
	Surface s = getSurface();

	s.writeSymbol(20, r.left, r.top);
	for (int x = r.left + 1; x < r.right; ++x)
		s.writeSymbol(18);
	s.writeSymbol(15);

	for (int y = r.top + 1; y < r.bottom; ++y) {
		s.writeChar(13, r.left, y);
		s.writeChar(19, r.right, y);
	}

	s.writeChar(16, r.left, r.bottom);
	for (int x = r.left + 1; x < r.right; ++x)
		s.writeChar(18);
	s.writeChar(17);
}

void Dialog::drawWindow(uint8 left, uint8 top, uint8 right, uint8 bottom){
	Surface s = getSurface();
	s.writeSymbol(20, left-1, top-1);
	for (int x = left; x <= right; x++) {
		s.writeSymbol(22);
	}
	s.writeSymbol(20);
	for (int y = top; y <= bottom+1; y++) {
		s.writeSymbol(21, left-1, y);
		s.writeSymbol(21, right+1, y);
	}
	s.writeSymbol(20, left - 1, bottom+1);
	for (int x = left; x <= right; x++)
		s.writeSymbol(22);
	s.writeSymbol(20);
	//drawFrame(Common::Rect(left-1, top-1, left + x_size, top + y_size));
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
