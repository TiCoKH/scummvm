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

#ifndef WIZARDRY_VIEWS_TOWN_SPELL_BOOKS_H
#define WIZARDRY_VIEWS_TOWN_SPELL_BOOKS_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * The spell books of a character: how many spells of each level can be
 * cast, and the names of the mage and priest spells known
 */
class SpellBooks : public TownView {
private:
	enum State {
		kMenu,
		kMageList,
		kPriestList
	};

	State _state = kMenu;
	uint _rosterIndex = 0;
	bool _inTraining = false;

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;

public:
	SpellBooks() : TownView("Town.SpellBooks") {}

	/**
	 * Opens the books of a character of the roster over the current view
	 * @param inTraining  Whether this is the training grounds, where no
	 *                    castle heading shows
	 */
	static void open(uint rosterIndex, bool inTraining);
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
