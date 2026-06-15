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

#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/data/items/character_item.h"

namespace Goldbox {

TreasurePool::TreasurePool() {
    _items = new Common::Array<Data::Items::CharacterItem>();
}

TreasurePool::~TreasurePool() {
    delete _items;
}

void TreasurePool::clear() {
    for (int i = 0; i < Data::VALUABLE_COUNT; ++i)
        _coins[static_cast<Data::ValuableType>(i)] = 0;
    _items->clear();
}

const Common::Array<Data::Items::CharacterItem> &TreasurePool::items() const {
    return *_items;
}

Common::Array<Data::Items::CharacterItem> &TreasurePool::items() {
    return *_items;
}

void TreasurePool::addItem(const Data::Items::CharacterItem &item) {
    _items->push_back(item);
}

bool TreasurePool::hasItems() const {
    return !_items->empty();
}

bool TreasurePool::isEmpty() const {
    return !hasAnyCoin() && !hasItems();
}

} // namespace Goldbox
