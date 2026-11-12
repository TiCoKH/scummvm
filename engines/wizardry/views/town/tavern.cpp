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

#include "wizardry/views/town/tavern.h"
#include "wizardry/data/globals.h"

namespace Wizardry {
namespace Town {

// Results of the menu "A)DD/R)EMOVE/#)INSPECT/D)IVVY GOLD/l)EAVE"
enum TavernChoice {
	kAddMember = 0,
	kRemoveMember = 1,
	kInspectFirst = 2,
	kInspectLast = 7,
	kDivvyGold = 8,
	kLeave = 9
};

enum {
	kMsgBadAlignment = 409,
	kTaken = TownScreens::kNobody
};

bool Tavern::msgFocus(const FocusMessage &msg) {
	_state = kMenu;
	return TownView::msgFocus(msg);
}

void Tavern::drawTown(TownScreens &screens) {
	screens.drawTavernMenu(_menu);

	if (_state == kAdd) {
		// Names already taken leave a gap, as in the original
		screens.drawTavernAdd(_available);
	} else if (_state == kRemove) {
		screens.drawTavernRemove();
	}
}

void Tavern::choose(int result, uint16 ascii) {
	Party &party = _G(party);

	switch (_state) {
	case kAdd:
		if (ascii == 13)
			_state = kMenu;
		else if (ascii >= 'A' && ascii < 'A' + (int)_available.size())
			addChosen(ascii - 'A');
		redraw();
		return;

	case kRemove:
		// RETURN or 0 leaves
		if (ascii >= '1' && ascii <= '0' + (int)party.size())
			party.remove(ascii - '1');
		else if (ascii != 13 && ascii != '0')
			return;
		_state = kMenu;
		redraw();
		return;

	default:
		break;
	}

	if (result == kAddMember) {
		if (!party.isFull()) {
			_available = party.available();
			_state = kAdd;
		}
	} else if (result == kRemoveMember) {
		if (party.size())
			_state = kRemove;
	} else if (result >= kInspectFirst && result <= kInspectLast) {
		if (party.size())
			notice("NOT IN THIS VERSION YET");   // TODO: the character sheet
	} else if (result == kDivvyGold) {
		party.divvyGold();
	} else if (result == kLeave) {
		close();
		return;
	}

	redraw();
}

void Tavern::addChosen(uint letter) {
	Party &party = _G(party);
	const uint rosterIndex = _available[letter];
	if (rosterIndex == kTaken)
		return;

	// The party may have changed alignment since the list was made
	if (party.alignmentClashes(rosterIndex)) {
		notice(_G(messages).get(kMsgBadAlignment));
		return;
	}

	// TODO: ask for the character's password, if it has one

	party.add(rosterIndex);
	_available[letter] = kTaken;
	if (party.isFull())
		_state = kMenu;
}

void Tavern::noticeDone() {
	// The original goes back to the tavern menu after "bad alignment"
	_state = kMenu;
}

} // End of namespace Town
} // End of namespace Wizardry
