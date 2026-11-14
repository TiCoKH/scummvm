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

#include "wizardry/views/town/equip.h"
#include "wizardry/game/equip.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace Town {

enum {
	kMsgYes = 254,
	kMsgNo = 255
};

namespace {

class EngineDice : public Dice {
public:
	uint roll(uint count) override {
		return g_engine->getRandomNumber(count - 1);
	}
};

} // End of anonymous namespace

void EquipView::open(uint rosterIndex) {
	EquipView *view = static_cast<EquipView *>(g_events->findView("Town.Equip"));
	view->_rosterIndex = rosterIndex;
	view->_anyApplied = false;
	view->_kind = -1;

	Equip::begin(view->character(), _G(items), true);
	view->addView();
	view->nextKind();
}

Character &EquipView::character() {
	return _G(party).rosterEntry(_rosterIndex);
}

void EquipView::drawBackground(TownScreens &screens) {
	if (g_events->isPresent("Maze"))
		screens.drawCampBackground(false);
	else
		screens.drawBackground(false);
}

void EquipView::drawTown(TownScreens &screens) {
	const Character &c = character();
	screens.drawSheet(c);

	switch (_state) {
	case kChoose:
		screens.drawEquipChoices(c, _kind, _slots, true);
		break;
	case kCursed:
		screens.drawEquipCursed();
		break;
	case kPower:
		screens.drawInvokePower(c, _powerSlot);
		break;
	}
}

void EquipView::nextKind() {
	const Character &c = character();

	while (++_kind < Equip::kKindCount) {
		_slots = Equip::candidates(c, Equip::KINDS[_kind], _G(items));
		if (_slots.empty())
			continue;

		// With a cursed item among them there is no choice
		bool cursed = false;
		for (uint i = 0; i < _slots.size(); ++i)
			cursed |= c.items[_slots[i]].cursed;

		if (cursed) {
			afterChoice();
		} else {
			_state = kChoose;
			redraw();
		}
		return;
	}

	_powerSlot = -1;
	nextPower();
}

void EquipView::afterChoice() {
	const Character &c = character();

	// The last cursed item of the kind, if any, puts itself on. This also
	// happens to a cursed item that was just chosen, which the original
	// then counts twice.
	for (uint i = 0; i < _slots.size(); ++i) {
		if (c.items[_slots[i]].cursed) {
			_state = kCursed;
			delayFrames(kPauseFrames);
			redraw();
			return;
		}
	}

	nextKind();
}

void EquipView::timeout() {
	if (_state != kCursed) {
		TownView::timeout();
		return;
	}

	Character &c = character();
	for (int i = _slots.size() - 1; i >= 0; --i) {
		if (c.items[_slots[i]].cursed) {
			c.items[_slots[i]].equipped = true;
			Equip::applyItem(c, _slots[i], _G(items));
			_anyApplied = true;
			break;
		}
	}

	nextKind();
}

void EquipView::nextPower() {
	const Character &c = character();

	while (++_powerSlot < c.itemCount) {
		if (Equip::hasPower(c, _powerSlot, _G(items))) {
			_state = kPower;
			redraw();
			return;
		}
	}

	finish();
}

void EquipView::finish() {
	Equip::end(character(), _anyApplied);
	close();
}

void EquipView::choose(int result, uint16 ascii) {
	Character &c = character();
	const MessageFile &messages = _G(messages);

	switch (_state) {
	case kChoose:
		// RETURN or 0: none of this kind
		if (ascii == kKeyReturn || ascii == '0') {
			afterChoice();
			return;
		}
		if (ascii < '1' || ascii > '0' + (int)_slots.size())
			return;

		c.items[_slots[ascii - '1']].equipped = true;
		Equip::applyItem(c, _slots[ascii - '1'], _G(items));
		_anyApplied = true;
		afterChoice();
		return;

	case kPower:
		if (ascii == (byte)messages.get(kMsgYes)[0]) {
			EngineDice dice;
			const int member = _G(party).memberIndexOf(_rosterIndex);
			if (member >= 0)
				Equip::invokePower(_G(party), member, _powerSlot, _G(items), dice);
		} else if (ascii != (byte)messages.get(kMsgNo)[0]) {
			return;
		}
		nextPower();
		return;

	default:
		// Nothing to do while the curse is announced
		return;
	}
}

} // End of namespace Town
} // End of namespace Wizardry
