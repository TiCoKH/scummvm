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

#ifndef WIZARDRY_VIEWS_TOWN_EQUIP_H
#define WIZARDRY_VIEWS_TOWN_EQUIP_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * Equipping a character: everything comes off, then one item of each kind
 * may be put on, and last the special powers of the items are offered.
 */
class EquipView : public TownView {
private:
	enum State {
		kChoose,   ///< which item of the kind
		kCursed,   ///< a cursed item puts itself on: a notice and a pause
		kPower     ///< invoke the power of an item, yes or no
	};

	State _state = kChoose;
	uint _rosterIndex = 0;
	int _kind = 0;                 ///< index into Equip::KINDS
	Common::Array<uint> _slots;    ///< the items of the kind
	int _powerSlot = 0;            ///< place in the pack whose power is offered
	bool _anyApplied = false;

	Character &character();

	/** Goes on to the next kind with something to choose, then to the powers */
	void nextKind();
	/** After a choice: a cursed item among those of the kind puts itself on */
	void afterChoice();
	void nextPower();
	void finish();

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	EquipView() : TownView("Town.Equip") {}

	/**
	 * Equips a character of the roster, over the current view
	 */
	static void open(uint rosterIndex);
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
