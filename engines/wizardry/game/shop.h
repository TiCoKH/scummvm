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

#ifndef WIZARDRY_GAME_SHOP_H
#define WIZARDRY_GAME_SHOP_H

#include "wizardry/game/items.h"

namespace Wizardry {

/**
 * Rules of Boltac's Trading Post of Wizardry I
 */
class Shop {
public:
	enum {
		kCatalogLines = 6
	};

	enum Service {
		kSell,
		kUncurse,
		kIdentify
	};

	enum Answer {
		kDone,
		kSoldOut,          ///< "you bought the last one"
		kPackFull,
		kCannotAfford,
		kUnusable,         ///< buying: ask whether to buy it all the same
		kNotInterested,    ///< selling something worth nothing, or not identified
		kNoCursedItems,    ///< selling a cursed item
		kEquipped,         ///< selling: ask whether to sell an equipped item
		kNotCursed,
		kAlreadyIdentified
	};

private:
	ItemList &_items;

	/** Next item on sale after or before an item; the list goes round */
	int step(int id, int direction) const;

	void unlockNextItem(int id);

public:
	/** The item records shown in the six catalogue lines */
	int _lines[kCatalogLines];

	Shop(ItemList &items);

	bool isOnSale(int id) const;

	/**
	 * Fills the catalogue. Going forward the new page starts with the
	 * last line of the old one; going back it ends with the first line
	 * of the old one. This is how the original pages.
	 */
	void pageStart();
	void pageForward();
	void pageBack();

	/**
	 * Checks a purchase. kUnusable means everything else is in order
	 * and the player has to confirm.
	 */
	Answer checkPurchase(const Character &c, int id) const;
	void purchase(Character &c, int id);

	/**
	 * What a service costs, or what is paid for a sale: half the price.
	 * Nothing is paid for an item that is not identified; uncursing or
	 * identifying something without a price costs 150000.
	 */
	int64 servicePrice(Service service, const ItemSlot &slot) const;

	/**
	 * Checks a service on an item of the pack. kEquipped means
	 * everything else is in order and the player has to confirm.
	 */
	Answer checkService(Service service, const Character &c, uint slot) const;
	void perform(Service service, Character &c, uint slot);
};

} // End of namespace Wizardry

#endif
