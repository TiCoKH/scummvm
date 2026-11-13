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

#ifndef WIZARDRY_VIEWS_TOWN_EDGE_H
#define WIZARDRY_VIEWS_TOWN_EDGE_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * The edge of town: the way to the training grounds, the maze and out
 * of the game. Going anywhere but back to the castle or into the maze
 * sends the party home.
 */
class Edge : public TownView {
private:
	bool _entering = false;

protected:
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void noticeDone() override;

public:
	Edge() : TownView("Town.Edge") {}

	bool msgFocus(const FocusMessage &msg) override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
