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

#include "wizardry/views/title.h"
#include "wizardry/resources/title_image.h"
#include "wizardry/wizardry.h"

namespace Wizardry {

void Title::drawCells(TextWindow &window) {
	_G(cells).clear();

	// The IBM-PC driver puts the picture at cell (2, 2), which centres its
	// 35 columns. The Japanese titles are 40 columns wide. Their row is
	// assumed to be the same; it has not been read from those drivers.
	CellImage image;
	if (loadTitleImage(*g_engine->getProfile(), g_engine->getGfxSet(), image))
		window.showImage(image, (window.width() - image.cols) / 2, 2);

	// TODO: the text of the original title screen
	window.printCentered(window.height() - 3, "PRESS ANY KEY");
}

void Title::leave() {
	if (g_engine->hasTown())
		replaceView("Town.Castle");
	else
		replaceView("Gallery");
}

bool Title::msgKeypress(const KeypressMessage &msg) {
	leave();
	return true;
}

bool Title::msgMouseDown(const MouseDownMessage &msg) {
	leave();
	return true;
}

} // End of namespace Wizardry
