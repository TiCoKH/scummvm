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

#ifndef WIZARDRY_VIEWS_TOWN_UTILITIES_H
#define WIZARDRY_VIEWS_TOWN_UTILITIES_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * The Wizardry utilities, from the edge of town: changing a name, moving
 * characters to another scenario, and restarting a party left in the maze.
 * The party is sent home on the way here.
 */
class Utilities : public TownView {
private:
	enum State {
		kMenu,
		kRestart,      ///< choosing whom to restart with
		kRenameWho,    ///< choosing whose name to change
		kRenameTyping, ///< typing the new name
		kRenameSaid    ///< what came of it, for a moment
	};

	State _state = kMenu;
	Common::Array<uint> _out;
	Common::Array<uint> _named;   ///< the characters whose names can be changed
	uint _renamed = 0;            ///< roster place of the one chosen
	Common::String _typed, _said;

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	Utilities() : TownView("Town.Utilities") {}

	bool msgFocus(const FocusMessage &msg) override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
