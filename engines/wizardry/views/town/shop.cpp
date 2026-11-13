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

#include "wizardry/views/town/shop.h"
#include "wizardry/data/globals.h"

namespace Wizardry {
namespace Town {

enum {
	kMsgPurchaseWhich = 551,
	kMsgPerhapsAnother = 552,
	kMsgBoughtLastOne = 553,
	kMsgCannotCarry = 554,
	kMsgCannotAfford = 555,
	kMsgUnusableBuyIt = 556,
	kMsgYes = 557,
	kMsgNo = 558,
	kMsgMistakes = 559,
	kMsgJustWhatYouNeeded = 560,
	kMsgNotInterested = 563,
	kMsgNoCursedItems = 564,
	kMsgNotCursed = 565,
	kMsgAlreadyIdentified = 566,
	kMsgCannotAffordFee = 567,
	kMsgAnythingElse = 568,
	kMsgEquippedSellIt = 580,
	kMsgEquippedKeys = 581
};

// Results of "B)UY/S)ELL/U)NCURSE/I)DENTIFY/P)OOL GOLD/l)EAVE"
enum {
	kBuy, kSellItems, kUncurseItems, kIdentifyItems, kPoolGold, kLeaveShop
};

// Results of "P)URCHASE/F)ORWARD/B)ACKWARD/S)TART/l)EAVE"
enum {
	kPurchase, kForward, kBackward, kStart, kLeaveCatalog
};

// Rows in the window of the trading post
enum {
	kRowBuyPrompt = 7,
	kRowBuyConfirm = 8,
	kRowBuySaid = 9,
	kRowSellConfirm = 8,
	kRowServiceSaid = 11
};

ShopView::ShopView() : TownView("Town.Shop"), _shop(_G(items)) {
}

bool ShopView::msgFocus(const FocusMessage &msg) {
	_state = kWho;
	return TownView::msgFocus(msg);
}

void ShopView::drawTown(TownScreens &screens) {
	const MessageFile &messages = _G(messages);
	const State shown = (_state == kSaid) ? _afterSaid : _state;

	switch (shown) {
	case kWho:
		screens.drawShopWho();
		break;

	case kMenu:
		screens.drawShopMenu(_member, _menu);
		break;

	case kCatalog:
		// While something is being said the menu is not there
		screens.drawShopCatalog(_member, _shop, _state == kSaid ? nullptr : &_menu);
		break;

	case kBuyWhich:
	case kBuyConfirm:
		screens.drawShopCatalog(_member, _shop, nullptr);
		screens.drawShopLine(kRowBuyPrompt, messages.get(kMsgPurchaseWhich), false, false);
		if (shown == kBuyConfirm)
			screens.drawShopLine(kRowBuyConfirm, messages.get(kMsgUnusableBuyIt), false, false);
		break;

	case kPack:
	case kSellConfirm:
		screens.drawShopPack(_member, _shop, _service, _state != kSaid);
		if (shown == kSellConfirm)
			screens.drawShopLine(kRowSellConfirm, messages.get(kMsgEquippedSellIt), false, false);
		break;

	default:
		break;
	}

	if (_state == kSaid) {
		const bool inCatalog = (_afterSaid == kCatalog);
		screens.drawShopLine(inCatalog ? kRowBuySaid : kRowServiceSaid, _said, true, _saidStars);
	}
}

void ShopView::say(uint message, State next, bool stars) {
	_said = _G(messages).get(message);
	_saidStars = stars;
	_afterSaid = next;
	_state = kSaid;
	delayFrames(kPauseFrames);
	redraw();
}

void ShopView::timeout() {
	if (_state != kSaid) {
		TownView::timeout();
		return;
	}

	_state = _afterSaid;

	// Whatever came of it, the original turns the page after each
	// attempt to buy
	if (_state == kCatalog)
		_shop.pageForward();
	// With nothing left in the pack there is nothing more to offer
	if (_state == kPack && _G(party).member(_member).itemCount == 0)
		_state = kMenu;

	redraw();
}

void ShopView::openPack(Shop::Service service) {
	if (_G(party).member(_member).itemCount == 0)
		return;

	_service = service;
	_state = kPack;
}

void ShopView::choose(int result, uint16 ascii) {
	Party &party = _G(party);
	const MessageFile &messages = _G(messages);

	switch (_state) {
	case kWho:
		if (ascii == 13 || ascii == '0') {
			close();
			return;
		}
		if (ascii < '1' || ascii > '0' + (int)party.size())
			return;
		_member = ascii - '1';
		_state = kMenu;
		break;

	case kMenu:
		switch (result) {
		case kBuy:
			_shop.pageStart();
			_state = kCatalog;
			break;
		case kSellItems:
			openPack(Shop::kSell);
			break;
		case kUncurseItems:
			openPack(Shop::kUncurse);
			break;
		case kIdentifyItems:
			openPack(Shop::kIdentify);
			break;
		case kPoolGold:
			party.poolGold(_member);
			break;
		case kLeaveShop:
			_state = kWho;
			break;
		default:
			return;
		}
		break;

	case kCatalog:
		switch (result) {
		case kPurchase:
			_state = kBuyWhich;
			break;
		case kForward:
			_shop.pageForward();
			break;
		case kBackward:
			_shop.pageBack();
			break;
		case kStart:
			_shop.pageStart();
			break;
		case kLeaveCatalog:
			_state = kMenu;
			break;
		default:
			return;
		}
		break;

	case kBuyWhich: {
		if (ascii == 13 || ascii == '0') {
			say(kMsgPerhapsAnother, kCatalog);
			return;
		}
		if (ascii < '1' || ascii > '0' + Shop::kCatalogLines)
			return;

		_chosen = _shop._lines[ascii - '1'];
		switch (_shop.checkPurchase(party.member(_member), _chosen)) {
		case Shop::kSoldOut:
			say(kMsgBoughtLastOne, kCatalog);
			return;
		case Shop::kPackFull:
			say(kMsgCannotCarry, kCatalog);
			return;
		case Shop::kCannotAfford:
			say(kMsgCannotAfford, kCatalog);
			return;
		case Shop::kUnusable:
			_state = kBuyConfirm;
			break;
		default:
			buy();
			return;
		}
		break;
	}

	case kBuyConfirm:
		if (ascii == (byte)messages.get(kMsgYes)[0])
			buy();
		else if (ascii == (byte)messages.get(kMsgNo)[0])
			say(kMsgMistakes, kCatalog);
		return;

	case kPack: {
		if (ascii == 13 || ascii == '0') {
			_state = kMenu;
			break;
		}
		const Character &c = party.member(_member);
		if (ascii < '1' || ascii > '0' + c.itemCount)
			return;

		_chosen = ascii - '1';
		switch (_shop.checkService(_service, c, _chosen)) {
		case Shop::kNotInterested:
			say(kMsgNotInterested, kPack);
			return;
		case Shop::kNoCursedItems:
			say(kMsgNoCursedItems, kPack);
			return;
		case Shop::kNotCursed:
			say(kMsgNotCursed, kPack);
			return;
		case Shop::kAlreadyIdentified:
			say(kMsgAlreadyIdentified, kPack);
			return;
		case Shop::kCannotAfford:
			say(kMsgCannotAffordFee, kPack);
			return;
		case Shop::kEquipped:
			_state = kSellConfirm;
			break;
		default:
			doService();
			return;
		}
		break;
	}

	case kSellConfirm: {
		const Common::String &keys = messages.get(kMsgEquippedKeys);
		if (keys.size() >= 2 && ascii == (byte)keys[0])
			doService();
		else if (keys.size() >= 2 && ascii == (byte)keys[1])
			say(kMsgMistakes, kPack);
		return;
	}

	default:
		return;
	}

	redraw();
}

void ShopView::buy() {
	_shop.purchase(_G(party).member(_member), _chosen);
	say(kMsgJustWhatYouNeeded, kCatalog);
}

void ShopView::doService() {
	_shop.perform(_service, _G(party).member(_member), _chosen);
	// This message has its asterisks already
	say(kMsgAnythingElse, kPack, false);
}

} // End of namespace Town
} // End of namespace Wizardry
