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

#ifndef WIZARDRY_VIEWS_MAZE_MAZE_VIEW_H
#define WIZARDRY_VIEWS_MAZE_MAZE_VIEW_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace InMaze {

/**
 * Walking in the maze.
 *
 * So far: the view of the walls, moving and turning, stairs, chutes,
 * transfers, spinners, pits, rock, and leaving the expedition. Still to
 * come: fights, special squares, elevators, the camp and the marks on
 * floor and ceiling.
 */
class MazeView : public Town::TownView {
private:
	enum State {
		kWalking,
		kSaid,        ///< a box with a message for a moment, then on
		kStairs,      ///< stairs: take them, yes or no
		kQuit         ///< leave the expedition, yes or no
	};

	State _state = kWalking;
	Common::String _said, _said2;
	bool _windows = true;       ///< the line of commands and the party show
	bool _partyShown = true;
	bool _leaveAfterSaid = false;
	int _encounter = -1;        ///< monster of a fight about to start

	void say(const Common::String &text);
	void loadLevel();
	void enterSquare();
	void transfer(uint index);
	void backToTown();

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	MazeView() : TownView("Maze") {}

	bool msgFocus(const FocusMessage &msg) override;

	/**
	 * Sends the party into the maze: down the stairs to the first level
	 */
	static void enter();
};

} // End of namespace InMaze
} // End of namespace Wizardry

#endif
