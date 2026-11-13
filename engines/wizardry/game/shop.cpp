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

#include "wizardry/game/shop.h"

namespace Wizardry {

Shop::Shop(ItemList &items) : _items(items) {
	for (int i = 0; i < kCatalogLines; ++i)
		_lines[i] = 1;
}

bool Shop::isOnSale(int id) const {
	const Item &item = _items[id];
	return item.stock != 0 && item.stock >= Item::kUnlimited && !item.cursed;
}

int Shop::step(int id, int direction) const {
	const int count = _items.size();

	// Item 0 is never listed. Give up after once round, which the
	// original does not need to: its item list always has things on sale.
	for (int tries = 0; tries < count; ++tries) {
		id += direction;
		if (id >= count)
			id = 1;
		else if (id < 1)
			id = count - 1;

		if (isOnSale(id))
			break;
	}

	return id;
}

void Shop::pageStart() {
	_lines[kCatalogLines - 1] = 1;
	pageForward();
}

void Shop::pageForward() {
	int id = _lines[kCatalogLines - 1] - 1;
	for (int i = 0; i < kCatalogLines; ++i)
		_lines[i] = id = step(id, 1);
}

void Shop::pageBack() {
	int id = _lines[0] + 1;
	for (int i = kCatalogLines - 1; i >= 0; --i)
		_lines[i] = id = step(id, -1);
}

Shop::Answer Shop::checkPurchase(const Character &c, int id) const {
	const Item &item = _items[id];

	if (item.stock == 0)
		return kSoldOut;
	if (c.itemCount >= Character::kMaxItems)
		return kPackFull;
	if (c.gold < item.price)
		return kCannotAfford;
	if (!item.usableBy(c))
		return kUnusable;
	return kDone;
}

void Shop::purchase(Character &c, int id) {
	Item &item = _items[id];

	c.gold -= item.price;
	c.addItem(id, true);

	if (item.stock > 0 && --item.stock == 0)
		unlockNextItem(id);
}

void Shop::unlockNextItem(int id) {
	// Going round the list from the item that sold out, the first one
	// held back (stock below -1) goes on sale with that many pieces
	const int count = _items.size();

	for (int next = (id + 1) % count; next != id; next = (next + 1) % count) {
		if (_items[next].stock < Item::kUnlimited) {
			_items[next].stock = -_items[next].stock;
			return;
		}
	}
}

int64 Shop::servicePrice(Service service, const ItemSlot &slot) const {
	int64 price = _items[slot.id].price / 2;

	if (service == kSell) {
		if (!slot.identified)
			price = 0;
	} else if (price == 0) {
		price = 150000;
	}

	return price;
}

Shop::Answer Shop::checkService(Service service, const Character &c, uint index) const {
	const ItemSlot &slot = c.items[index];
	const int64 price = servicePrice(service, slot);

	if (service == kSell) {
		if (price == 0)
			return kNotInterested;
		if (slot.cursed)
			return kNoCursedItems;
		return slot.equipped ? kEquipped : kDone;
	}

	if (service == kUncurse && !slot.cursed)
		return kNotCursed;
	if (service == kIdentify && slot.identified)
		return kAlreadyIdentified;
	if (c.gold < price)
		return kCannotAfford;
	return kDone;
}

void Shop::perform(Service service, Character &c, uint index) {
	const ItemSlot slot = c.items[index];
	const int64 price = servicePrice(service, slot);

	if (service == kSell)
		c.gold = MIN<int64>(c.gold + price, 999999999999LL);
	else
		c.gold -= price;

	if (service == kIdentify) {
		c.items[index].identified = true;
		return;
	}

	// Sold, or destroyed with its curse
	c.removeItem(index);

	if (service == kSell) {
		Item &item = _items[slot.id];
		if (item.stock < Item::kUnlimited)
			item.stock = 1 - item.stock;   // as the original does it
		else if (item.stock >= 0)
			++item.stock;
	}
}

} // End of namespace Wizardry
