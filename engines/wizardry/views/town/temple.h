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

#ifndef WIZARDRY_VIEWS_TOWN_TEMPLE_H
#define WIZARDRY_VIEWS_TOWN_TEMPLE_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * The Temple of Cant: the paralysed, the stoned and the dead are brought
 * back for a donation, if the ritual works.
 */
class TempleView : public TownView {
private:
	enum State {
		kChoose,     ///< which patient
		kDonation,   ///< who pays
		kRitual,     ///< murmur, chant, pray, invoke, a pause after each
		kSay         ///< a line said, then back to the start
	};

	State _state = kChoose;
	Common::Array<uint> _patients;
	uint _patient = 0;      ///< roster number
	uint _steps = 0;
	uint _sayMessage = 0;
	Common::String _sayName;
	bool _afterRitual = false;

	void say(uint message, const Common::String &name);

protected:
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	TempleView() : TownView("Town.Temple") {}

	bool msgFocus(const FocusMessage &msg) override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
