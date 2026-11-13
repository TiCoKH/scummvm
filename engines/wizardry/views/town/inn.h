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

#ifndef WIZARDRY_VIEWS_TOWN_INN_H
#define WIZARDRY_VIEWS_TOWN_INN_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * The Adventurer's Inn: a member rests week by week until healed, out of
 * gold or a key is pressed, and then hears whether a new level was made.
 */
class InnView : public TownView {
private:
	enum State {
		kWho,       ///< asking which member stays
		kRooms,     ///< choosing a room
		kNapping,   ///< the stables: a short pause
		kResting,   ///< a week goes by with every tick of the timer
		kResult     ///< what came of it, until RETURN
	};

	State _state = kWho;
	uint _member = 0;
	int _room = 0;
	bool _birthday = false;
	Common::Array<Common::String> _lines;

	void startRest(int room);
	void restStep();
	void finishRest();

protected:
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	InnView() : TownView("Town.Inn") {}

	bool msgFocus(const FocusMessage &msg) override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
