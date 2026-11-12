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

#ifndef WIZARDRY_VIEWS_TITLE_H
#define WIZARDRY_VIEWS_TITLE_H

#include "wizardry/views/cell_view.h"

namespace Wizardry {

/**
 * Shows the title picture of the release, if it has one
 */
class Title : public CellView {
protected:
	void drawCells(TextWindow &window) override;

public:
	Title() : CellView("Title", false) {}

	/**
	 * Goes on to the game, or to the gallery for releases whose game
	 * screens do not exist yet
	 */
	void leave();

	bool msgKeypress(const KeypressMessage &msg) override;
	bool msgMouseDown(const MouseDownMessage &msg) override;
};

} // End of namespace Wizardry

#endif
