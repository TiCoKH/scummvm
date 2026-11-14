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

#include "wizardry/views/maze/camp_view.h"
#include "wizardry/data/globals.h"
#include "wizardry/game/equip.h"
#include "wizardry/views/town/equip.h"
#include "wizardry/views/town/sheet.h"

namespace Wizardry {
namespace InMaze {

// Results of "#)INSPECT/R)EORDER/E)QUIP/l)EAVE"
enum {
	kInspectFirst = 0,
	kInspectLast = 5,
	kReorderParty = 6,
	kEquipParty = 7,
	kLeaveCamp = 8
};

static uint rosterIndexOf(uint member) {
	const Party &party = _G(party);
	for (uint i = 0; i < party.rosterSize(); ++i) {
		if (party.memberIndexOf(i) == (int)member)
			return i;
	}
	return 0;
}

bool CampView::msgFocus(const FocusMessage &msg) {
	_state = kMenu;

	// Coming back from equipping one member while equipping all: the
	// next one, but not from in here, where the views are being changed
	if (!(msg._priorView && msg._priorView == g_events->findView("Town.Equip")))
		_equipNext = -1;

	return TownView::msgFocus(msg);
}

bool CampView::tick() {
	if (_equipNext >= 0 && isFocused()) {
		const int member = _equipNext;
		_equipNext = (member + 1 < (int)_G(party).size()) ? member + 1 : -1;
		Town::EquipView::open(rosterIndexOf(member));
		return true;
	}

	return TownView::tick();
}

void CampView::drawBackground(TownScreens &screens) {
	screens.drawCampBackground();
}

void CampView::drawTown(TownScreens &screens) {
	if (_state == kReorder)
		screens.drawReorder(_chosen, true);
	else
		screens.drawCampMenu(_menu);
}

void CampView::choose(int result, uint16 ascii) {
	Party &party = _G(party);

	if (_state == kReorder) {
		if (ascii < '1' || ascii > '0' + (int)party.size())
			return;

		// Nobody twice
		const uint member = ascii - '1';
		for (uint i = 0; i < _chosen.size(); ++i) {
			if (_chosen[i] == member)
				return;
		}
		_chosen.push_back(member);

		// The last place needs no asking
		if (_chosen.size() + 1 == party.size()) {
			for (uint m = 0; m < party.size(); ++m) {
				bool taken = false;
				for (uint i = 0; i < _chosen.size(); ++i)
					taken |= _chosen[i] == m;
				if (!taken)
					_chosen.push_back(m);
			}
			party.reorder(_chosen);
			_state = kMenu;
		}
		redraw();
		return;
	}

	if (result >= kInspectFirst && result <= kInspectLast) {
		Town::SheetView::open(rosterIndexOf(result), Town::SheetView::kFromCamp);
	} else if (result == kReorderParty) {
		if (party.size() < 2)
			return;
		_chosen.clear();
		_state = kReorder;
		redraw();
	} else if (result == kEquipParty) {
		// Everyone in turn; see tick()
		_equipNext = 0;
	} else if (result == kLeaveCamp) {
		// Breaking camp, everyone's figures are worked out afresh
		for (uint i = 0; i < party.size(); ++i)
			Equip::recalc(party.member(i), _G(items));
		close();
	}
}

} // End of namespace InMaze
} // End of namespace Wizardry
