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

#include "wizardry/views/town/inn.h"
#include "wizardry/game/inn.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace Town {

enum {
	kMsgRoomKeys = 441,   // "ABCDEPL": five rooms, pool gold, leave

	// The original waits 16 timer ticks of the PC (18.2 a second) for a week
	kWeekFrames = FRAME_RATE * 16 / 18,
	kNapFrames = FRAME_RATE * 2
};

namespace {

class EngineDice : public Dice {
public:
	uint roll(uint count) override {
		return g_engine->getRandomNumber(count - 1);
	}
};

} // End of anonymous namespace

bool InnView::msgFocus(const FocusMessage &msg) {
	_state = kWho;
	return TownView::msgFocus(msg);
}

void InnView::drawTown(TownScreens &screens) {
	switch (_state) {
	case kWho:
		screens.drawInnWho();
		break;
	case kRooms:
		screens.drawInnRooms(_member);
		break;
	case kNapping:
		screens.drawInnNapping(_member);
		break;
	case kResting:
		screens.drawInnResting(_member);
		break;
	case kResult:
		screens.drawInnResult(_lines);
		break;
	}
}

void InnView::choose(int result, uint16 ascii) {
	Party &party = _G(party);

	switch (_state) {
	case kWho:
		if (ascii == 13 || ascii == '0') {
			close();
			return;
		}
		// Only someone in good health can take a room
		if (ascii >= '1' && ascii <= '0' + (int)party.size() &&
				party.member(ascii - '1').status == kStatusOk) {
			_member = ascii - '1';
			_state = kRooms;
		}
		break;

	case kRooms: {
		const Common::String &keys = _G(messages).get(kMsgRoomKeys);
		int choice = (ascii == 13) ? Inn::kRoomCount + 1 : -1;
		for (uint i = 0; i < keys.size(); ++i) {
			if (keys[i] == ascii)
				choice = i;
		}

		if (choice < 0)
			return;
		if (choice < Inn::kRoomCount) {
			startRest(choice);
			return;
		}
		if (choice == Inn::kRoomCount)
			party.poolGold(_member);
		else
			_state = kWho;
		break;
	}

	case kResting:
		// Any key ends the stay
		cancelDelay();
		finishRest();
		return;

	case kResult:
		if (ascii != 13)
			return;
		// Someone who died of old age cannot take another room
		_state = (party.member(_member).status == kStatusOk) ? kRooms : kWho;
		break;

	default:
		return;
	}

	redraw();
}

void InnView::startRest(int room) {
	_room = room;
	_birthday = false;

	if (Inn::ROOMS[room].cost == 0) {
		_state = kNapping;
		delayFrames(kNapFrames);
		redraw();
	} else {
		_state = kResting;
		restStep();
	}
}

void InnView::restStep() {
	Character &c = _G(party).member(_member);

	if (!Inn::canRest(c, Inn::ROOMS[_room])) {
		finishRest();
		return;
	}

	_birthday |= Inn::restOneWeek(c, Inn::ROOMS[_room]);
	delayFrames(kWeekFrames);
	redraw();
}

void InnView::finishRest() {
	EngineDice dice;
	Inn inn(_G(scenario), _G(messages), dice);

	_lines.clear();
	inn.checkLevel(_G(party).member(_member), _birthday, _lines);
	_state = kResult;
	redraw();
}

void InnView::timeout() {
	if (_state == kResting)
		restStep();
	else if (_state == kNapping)
		finishRest();
	else
		TownView::timeout();
}

} // End of namespace Town
} // End of namespace Wizardry
