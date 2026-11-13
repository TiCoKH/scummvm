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

#include "wizardry/views/town/sheet.h"
#include "wizardry/data/globals.h"
#include "wizardry/views/town/equip.h"
#include "wizardry/views/town/spell_books.h"

namespace Wizardry {
namespace Town {

enum {
	kMsgReturnToLeave = 600,
	kMsgDropWhich = 636,
	kMsgCursed = 637,
	kMsgEquipped = 638,
	kMsgDropped = 639,
	kMsgTradeGold = 640,
	kMsgArrow = 641,
	kMsgBadAmount = 642,
	kMsgTooMuch = 643,
	kMsgTradeItem = 644,
	kMsgFull = 645,
	kMsgTradeCursed = 646,
	kMsgTradeEquipped = 647,
	kMsgTradeWithWhom = 648,
	kMsgMenuTavern = 650,
	kMsgMenuTraining = 651,

	kMaxAmount = 12
};

// Results of "R)EAD/E)QUIP/T)RADE/D)ROP/P)OOL GOLD/I)DENTIFY/l)EAVE"
enum {
	kRead, kEquip, kTrade, kDrop, kPoolGold, kIdentify, kLeaveSheet
};

void SheetView::open(uint rosterIndex, Mode mode) {
	SheetView *view = static_cast<SheetView *>(g_events->findView("Town.Sheet"));
	view->_rosterIndex = rosterIndex;
	view->_mode = mode;
	view->_state = kMenu;
	view->addView();
}

Character &SheetView::character() {
	return _G(party).rosterEntry(_rosterIndex);
}

void SheetView::drawBackground(TownScreens &screens) {
	// The sheet takes the place of the party
	if (_mode == kFromTraining)
		screens.drawTrainingBackground();
	else
		screens.drawBackground(false);
}

void SheetView::drawTown(TownScreens &screens) {
	const MessageFile &messages = _G(messages);
	Common::Array<Common::String> lines;

	screens.drawSheet(character());

	switch (_state) {
	case kMenu:
		screens.drawSheetMenu(_mode == kFromTraining ? kMsgMenuTraining : kMsgMenuTavern, _menu);
		break;

	case kDropWhich:
		lines.push_back(messages.get(kMsgDropWhich));
		lines.push_back(messages.get(kMsgReturnToLeave));
		screens.drawSheetPopup(lines, nullptr);
		break;

	case kTradeWho:
		// The party shows again while one of it is chosen
		screens.drawParty();
		lines.push_back("");
		lines.push_back(messages.get(kMsgTradeWithWhom));
		lines.push_back(messages.get(kMsgReturnToLeave));
		screens.drawSheetPopup(lines, nullptr);
		break;

	case kTradeGold: {
		lines.push_back(messages.get(kMsgTradeGold));
		lines.push_back(messages.get(kMsgReturnToLeave));
		const Common::String input = messages.get(kMsgArrow) + _input;
		screens.drawSheetPopup(lines, &input);
		break;
	}

	case kTradeItem:
		lines.push_back(messages.get(kMsgTradeItem));
		lines.push_back(messages.get(kMsgReturnToLeave));
		screens.drawSheetPopup(lines, nullptr);
		break;

	case kSaid:
		screens.drawSheetNotice(_said);
		break;
	}
}

void SheetView::say(uint message, State next) {
	_said = _G(messages).get(message);
	_afterSaid = next;
	_state = kSaid;
	delayFrames(kPauseFrames);
	redraw();
}

void SheetView::timeout() {
	if (_state == kSaid) {
		_state = _afterSaid;
		redraw();
	} else {
		TownView::timeout();
	}
}

void SheetView::choose(int result, uint16 ascii) {
	Party &party = _G(party);
	Character &c = character();

	switch (_state) {
	case kMenu:
		if (_mode == kFromTraining) {
			// "R)EAD/l)EAVE"
			if (result == 0)
				SpellBooks::open(_rosterIndex, true);
			else if (result == 1)
				close();
			return;
		}

		switch (result) {
		case kRead:
			SpellBooks::open(_rosterIndex, false);
			return;
		case kEquip:
			if (c.itemCount > 0)
				EquipView::open(_rosterIndex);
			return;
		case kIdentify:
			notice("NOT IN THIS VERSION YET");       // TODO
			return;
		case kTrade:
			// There must be someone else to trade with
			if (party.size() < 2)
				return;
			_state = kTradeWho;
			break;
		case kDrop:
			if (c.itemCount == 0)
				return;
			_state = kDropWhich;
			break;
		case kPoolGold: {
			const int member = party.memberIndexOf(_rosterIndex);
			if (member >= 0)
				party.poolGold(member);
			break;
		}
		case kLeaveSheet:
			close();
			return;
		default:
			return;
		}
		break;

	case kDropWhich:
		if (ascii == kKeyReturn || ascii == '0') {
			_state = kMenu;
			break;
		}
		if (ascii < '1' || ascii > '0' + c.itemCount)
			return;

		switch (Party::dropItem(c, ascii - '1')) {
		case Party::kTradeCursed:
			say(kMsgCursed, kMenu);
			return;
		case Party::kTradeEquipped:
			say(kMsgEquipped, kMenu);
			return;
		default:
			say(kMsgDropped, kMenu);
			return;
		}

	case kTradeWho: {
		if (ascii == kKeyReturn || ascii == '0') {
			_state = kMenu;
			break;
		}
		if (ascii < '1' || ascii > '0' + (int)party.size())
			return;
		// Not with oneself
		const uint member = ascii - '1';
		if ((int)member == party.memberIndexOf(_rosterIndex))
			return;

		_receiver = member;
		_input.clear();
		_state = kTradeGold;
		break;
	}

	case kTradeGold:
		if (ascii == kKeyBackspace) {
			if (!_input.empty())
				_input.deleteLastChar();
			break;
		}
		if (ascii == kKeyEscape) {
			_input.clear();
			break;
		}
		if (ascii != kKeyReturn) {
			if (ascii >= ' ' && ascii < 127 && _input.size() < kMaxAmount)
				_input += (char)ascii;
			break;
		}

		// Nothing typed: no gold, on to the items
		switch (Party::giveGold(c, party.member(_receiver), _input)) {
		case Party::kTradeBadAmount:
			say(kMsgBadAmount, kMenu);
			return;
		case Party::kTradeTooMuch:
			say(kMsgTooMuch, kMenu);
			return;
		default:
			break;
		}
		_state = kTradeItem;
		break;

	case kTradeItem:
		if (ascii == kKeyReturn || ascii == '0') {
			_state = kMenu;
			break;
		}
		if (ascii < '1' || ascii > '0' + c.itemCount)
			return;

		switch (Party::giveItem(c, party.member(_receiver), ascii - '1')) {
		case Party::kTradeFull:
			say(kMsgFull, kMenu);
			return;
		case Party::kTradeCursed:
			say(kMsgTradeCursed, kMenu);
			return;
		case Party::kTradeEquipped:
			say(kMsgTradeEquipped, kMenu);
			return;
		default:
			break;
		}
		// The next item is asked for until RETURN
		break;

	default:
		return;
	}

	redraw();
}

} // End of namespace Town
} // End of namespace Wizardry
