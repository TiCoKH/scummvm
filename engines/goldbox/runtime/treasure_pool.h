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

#ifndef GOLDBOX_RUNTIME_TREASURE_POOL_H
#define GOLDBOX_RUNTIME_TREASURE_POOL_H

#include "common/array.h"
#include "common/scummsys.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Data {
namespace Items {
struct CharacterItem;
} // namespace Items
} // namespace Data

/**
 * Shared runtime treasure pool.
 *
 * Populated by the Host layer (opcode 0x27 TREASURE / setupTreasure),
 * consumed by the UI layer (TreasureDialog, StoreDialog, TempleDialog).
 * Acts as the engine equivalent of PTR_ITEM_LIST + MONEY_POOL_SLOTS globals
 * from the original binary.
 *
 * Item storage uses a pointer to avoid requiring CharacterItem to be
 * a complete type in this header (breaks circular include via engine.h).
 */
class TreasurePool {
public:
    TreasurePool();
    ~TreasurePool();

    void clear();

    // --- Coin access (uses existing ValuableType enum) ---
    uint16 coin(Data::ValuableType type) const { return _coins[type]; }
    void setCoin(Data::ValuableType type, uint16 amount) {
        _coins[type] = amount;
    }

    const Data::ValuableItems &coins() const { return _coins; }
    Data::ValuableItems &coins() { return _coins; }

    bool hasAnyCoin() const { return _coins.getTotalWeight() > 0; }

    // --- Item access (defined out-of-line in .cpp) ---
    const Common::Array<Data::Items::CharacterItem> &items() const;
    Common::Array<Data::Items::CharacterItem> &items();
    void addItem(const Data::Items::CharacterItem &item);
    bool hasItems() const;
    bool isEmpty() const;

private:
    Data::ValuableItems _coins;
    Common::Array<Data::Items::CharacterItem> *_items;
};

} // namespace Goldbox

#endif // GOLDBOX_RUNTIME_TREASURE_POOL_H
