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

#ifndef WIZARDRY_VIEWS_MAZE_CAMP_VIEW_H
#define WIZARDRY_VIEWS_MAZE_CAMP_VIEW_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace InMaze {

/**
 * The camp in the maze: looking at a member, putting the party in a new
 * order, and equipping everyone in turn
 */
class CampView : public Town::TownView {
private:
	enum State {
		kMenu,
		kReorder     ///< picking who goes to each place, front first
	};

	State _state = kMenu;
	Common::Array<uint> _chosen;
	int _equipNext = -1;     ///< member to equip next, while equipping everyone

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;

public:
	CampView() : TownView("Camp") {}

	bool msgFocus(const FocusMessage &msg) override;
	bool tick() override;
};

} // End of namespace InMaze
} // End of namespace Wizardry

#endif
