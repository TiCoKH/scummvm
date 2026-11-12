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

#ifndef WIZARDRY_VIEWS_TOWN_TAVERN_H
#define WIZARDRY_VIEWS_TOWN_TAVERN_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * Gilgamesh's Tavern: putting the party together
 */
class Tavern : public TownView {
private:
	enum State {
		kMenu,
		kAdd,
		kRemove
	};

	State _state = kMenu;
	Common::Array<uint> _available;   ///< roster numbers behind the letters; ~0 once taken

	void addChosen(uint letter);

protected:
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void noticeDone() override;

public:
	Tavern() : TownView("Town.Tavern") {}

	bool msgFocus(const FocusMessage &msg) override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
