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

#include "wizardry/views/town/temple.h"
#include "wizardry/game/temple.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace Town {

enum {
	kMsgCheapApostates = 522,
	kMsgWillBeBuried = 523,
	kMsgNeedsKadorto = 524,
	kMsgHasBeenHealed = 529
};

namespace {

class EngineDice : public Dice {
public:
	uint roll(uint count) override {
		return g_engine->getRandomNumber(count - 1);
	}
};

} // End of anonymous namespace

bool TempleView::msgFocus(const FocusMessage &msg) {
	_state = kChoose;
	_patients = Temple::patients(_G(party));
	return TownView::msgFocus(msg);
}

void TempleView::drawTown(TownScreens &screens) {
	switch (_state) {
	case kChoose:
		screens.drawTempleChoose(_patients);
		break;
	case kDonation:
		screens.drawTempleDonation(Temple::donation(_G(party).rosterEntry(_patient)));
		break;
	case kRitual:
		screens.drawTempleRitual(_steps);
		break;
	case kSay:
		if (_afterRitual)
			screens.drawTempleRitual(4);
		else
			screens.drawTempleDonation(Temple::donation(_G(party).rosterEntry(_patient)));
		screens.drawTempleSay(_sayMessage, _sayName);
		break;
	}
}

void TempleView::choose(int result, uint16 ascii) {
	Party &party = _G(party);

	switch (_state) {
	case kChoose:
		if (ascii == 13) {
			close();
			return;
		}
		if (ascii < 'A' || ascii >= 'A' + (int)_patients.size())
			return;
		_patient = _patients[ascii - 'A'];
		_state = kDonation;
		break;

	case kDonation: {
		// RETURN or 0 leaves
		if (ascii == 13 || ascii == '0') {
			_state = kChoose;
			break;
		}
		if (ascii < '1' || ascii > '0' + (int)party.size())
			return;

		_afterRitual = false;
		if (!Temple::collect(party, ascii - '1', Temple::donation(party.rosterEntry(_patient)))) {
			say(kMsgCheapApostates, "");
			return;
		}

		_state = kRitual;
		_steps = 1;
		delayFrames(kPauseFrames);
		break;
	}

	default:
		// Nothing can be done while the priests are at work
		return;
	}

	redraw();
}

void TempleView::say(uint message, const Common::String &name) {
	_sayMessage = message;
	_sayName = name;
	_state = kSay;
	delayFrames(kPauseFrames);
	redraw();
}

void TempleView::timeout() {
	if (_state == kRitual) {
		if (++_steps < 4) {
			delayFrames(kPauseFrames);
			redraw();
			return;
		}

		// "Invoke!" and the outcome come together
		EngineDice dice;
		Character &c = _G(party).rosterEntry(_patient);
		_afterRitual = true;

		switch (Temple::ritual(c, dice)) {
		case Temple::kNowLost:
			say(kMsgWillBeBuried, c.name);
			break;
		case Temple::kNowAshes:
			say(kMsgNeedsKadorto, c.name);
			break;
		default:
			say(kMsgHasBeenHealed, c.name);
			break;
		}
	} else if (_state == kSay) {
		_patients = Temple::patients(_G(party));
		_state = kChoose;
		redraw();
	} else {
		TownView::timeout();
	}
}

} // End of namespace Town
} // End of namespace Wizardry
