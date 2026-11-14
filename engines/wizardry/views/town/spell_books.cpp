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

#include "wizardry/views/town/spell_books.h"
#include "wizardry/data/globals.h"

namespace Wizardry {
namespace Town {

// Results of "M)AGE SPELL BOOKS/P)RIEST SPELL BOOKS/l)EAVE"
enum {
	kMageBooks, kPriestBooks, kLeaveBooks
};

void SpellBooks::open(uint rosterIndex, bool inTraining) {
	SpellBooks *view = static_cast<SpellBooks *>(g_events->findView("Town.SpellBooks"));
	view->_rosterIndex = rosterIndex;
	view->_inTraining = inTraining;
	view->_state = kMenu;
	view->addView();
}

void SpellBooks::drawBackground(TownScreens &screens) {
	if (_inTraining)
		screens.drawTrainingBackground();
	else if (g_events->isPresent("Maze"))
		screens.drawCampBackground(false);
	else
		screens.drawBackground(false);
}

void SpellBooks::drawTown(TownScreens &screens) {
	const Character &c = _G(party).rosterEntry(_rosterIndex);

	if (_state == kMenu)
		screens.drawSpellBooks(c, _menu);
	else
		screens.drawSpellList(c, _state == kMageList);
}

void SpellBooks::choose(int result, uint16 ascii) {
	if (_state != kMenu) {
		if (ascii != kKeyReturn)
			return;
		_state = kMenu;
	} else if (result == kMageBooks) {
		_state = kMageList;
	} else if (result == kPriestBooks) {
		_state = kPriestList;
	} else if (result == kLeaveBooks) {
		close();
		return;
	} else {
		return;
	}

	redraw();
}

} // End of namespace Town
} // End of namespace Wizardry
