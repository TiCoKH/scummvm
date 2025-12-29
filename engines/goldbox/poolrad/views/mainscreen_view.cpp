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

#include "goldbox/poolrad/views/mainscreen_view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

void MainScreenView::drawBaseWindows() {
	// Draw the three fixed windows that appear in all main screen states
	// Coordinates match Amiga SCREEN_DrawMainWindows implementation:
	// SCREEN_DrawWindow(1,1,38,22,0,15,&DAT_00240c2c);
	drawWindow(1, 1, 38, 22);

	// SCREEN_DrawWindow(1,17,38,22,0,15,&DAT_00240c2e);
	drawWindow(1, 17, 38, 22);

	// SCREEN_DrawWindow(1,1,15,15,0,15,&DAT_00240c30);
	drawWindow(1, 1, 15, 15);

	// Conditionally draw the small window when param_1 != 0
	// if (param_1 != 0) {
	//     SCREEN_DrawWindow(3,3,13,13,0,15,&DAT_00240c32);
	// }
	if (_showMiniWindow) {
		drawWindow(3, 3, 13, 13);
	}
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
