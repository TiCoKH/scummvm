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

#include "wizardry/views/town/utilities.h"
#include "wizardry/views/maze/maze_view.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace Town {

// Results of "C)HANGE NAME/M)OVE CHARACTERS/R)ESTART AN "OUT" PARTY/l)EAVE UTILITIES"
enum {
	kChangeName, kMoveCharacters, kRestartParty, kLeaveUtilities
};

bool Utilities::msgFocus(const FocusMessage &msg) {
	_state = kMenu;
	return TownView::msgFocus(msg);
}

void Utilities::drawBackground(TownScreens &screens) {
	// Each screen draws all of itself
}

void Utilities::drawTown(TownScreens &screens) {
	if (_state == kRestart)
		screens.drawRestart(_out);
	else
		screens.drawUtilities(_menu);
}

void Utilities::choose(int result, uint16 ascii) {
	Party &party = _G(party);

	if (_state == kRestart) {
		if (ascii == kKeyReturn) {
			_state = kMenu;
			redraw();
			return;
		}
		if (ascii < 'A' || ascii >= 'A' + (int)_out.size())
			return;

		MazeState start;
		Maze::restartParty(party, _out[ascii - 'A'], start);
		InMaze::MazeView::enter(&start);
		return;
	}

	switch (result) {
	case kChangeName:
	case kMoveCharacters:
		notice("NOT IN THIS VERSION YET");   // TODO
		return;

	case kRestartParty:
		_out = Maze::outCharacters(party);
		_state = kRestart;
		break;

	case kLeaveUtilities:
		replaceView("Town.Castle");
		return;

	default:
		return;
	}

	redraw();
}

} // End of namespace Town
} // End of namespace Wizardry
