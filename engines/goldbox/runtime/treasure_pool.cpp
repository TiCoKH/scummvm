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
#include "goldbox/data/player_character.h"
#include "goldbox/poolrad/data/poolrad_character.h"

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

bool TreasurePool::poolMoneyFromParty(
        Common::List<Data::PlayerCharacter *> &party) {
    for (Common::List<Data::PlayerCharacter *>::const_iterator it = party.begin(); it != party.end(); ++it) {
        Poolrad::Data::PoolradCharacter *ch =
            dynamic_cast<Poolrad::Data::PoolradCharacter *>(*it);
        if (!ch || ch->isNpc())
            continue;
        for (int t = 0; t < Data::VALUABLE_COUNT; ++t) {
            uint32 v = (uint32)_coins.values[t] + ch->valuableItems.values[t];
            _coins.values[t] = (v > 0xFFFF) ? (uint16)0xFFFF : (uint16)v;
            ch->valuableItems.values[t] = 0;
        }
    }
    return hasAnyCoin();
}

bool TreasurePool::shareMoneyToParty(
        Common::List<Data::PlayerCharacter *> &party) {
    // Count eligible party members
    int count = 0;
    for (Common::List<Data::PlayerCharacter *>::const_iterator it = party.begin(); it != party.end(); ++it) {
        Poolrad::Data::PoolradCharacter *ch =
            dynamic_cast<Poolrad::Data::PoolradCharacter *>(*it);
        if (ch && !ch->isNpc())
            ++count;
    }
    if (count == 0)
        return hasAnyCoin();

    // Divide each coin type evenly; remainder stays in pool
    for (int t = 0; t < Data::VALUABLE_COUNT; ++t) {
        uint16 total = _coins.values[t];
        if (total == 0)
            continue;
        uint16 share = total / (uint16)count;
        uint16 remainder = total - share * (uint16)count;

        for (Common::List<Data::PlayerCharacter *>::const_iterator it = party.begin(); it != party.end(); ++it) {
            Poolrad::Data::PoolradCharacter *ch =
                dynamic_cast<Poolrad::Data::PoolradCharacter *>(*it);
            if (!ch || ch->isNpc())
                continue;
            uint32 v = (uint32)ch->valuableItems.values[t] + share;
            ch->valuableItems.values[t] =
                (v > 0xFFFF) ? (uint16)0xFFFF : (uint16)v;
        }
        _coins.values[t] = remainder;
    }
    return hasAnyCoin();
}

} // namespace Goldbox
