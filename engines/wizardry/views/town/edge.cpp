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

#include "wizardry/views/town/edge.h"
#include "wizardry/views/maze/maze_view.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace Town {

// Results of "T)RAINING/M)AZE/C)ASTLE/U)TILITIES/L)EAVE GAME"
enum EdgeChoice {
	kTraining,
	kMaze,
	kCastle,
	kUtilities,
	kLeaveGame
};

bool Edge::msgFocus(const FocusMessage &msg) {
	_entering = false;
	return TownView::msgFocus(msg);
}

void Edge::drawTown(TownScreens &screens) {
	if (_entering)
		screens.drawEnteringMaze();
	else
		screens.drawEdgeMenu(_menu);
}

void Edge::choose(int result, uint16 ascii) {
	Party &party = _G(party);

	switch (result) {
	case kTraining:
		party.disband();
		replaceView("Town.Training");
		break;

	case kMaze:
		// Nobody to send in: the menu just stays, as in the original
		if (!party.size())
			break;

		InMaze::MazeView::enter();
		break;

	case kCastle:
		close();
		break;

	case kUtilities:
		notice("NOT IN THIS VERSION YET");   // TODO
		break;

	case kLeaveGame:
		party.disband();
		g_engine->quitGame();
		break;

	default:
		break;
	}
}

void Edge::noticeDone() {
	_entering = false;
}

} // End of namespace Town
} // End of namespace Wizardry
