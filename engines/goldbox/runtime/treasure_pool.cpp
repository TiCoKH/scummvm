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
#include "common/debug.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/player_character.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Runtime {

namespace {

// Base carrying capacity (Money.CarryingCapacity in money.pas). Characters can
// hold 1500 units plus their Strength-based capacity modifier.
const uint32 kBaseCarryingCapacity = 1500;

uint32 carryingCapacity(const Data::ADnDCharacter &character) {
    // getCapacityModifier() is signed; add in a wide signed intermediate and
    // clamp so an unusually large negative modifier cannot underflow.
    const int32 cap = (int32)kBaseCarryingCapacity +
        character.getCapacityModifier();
    return (cap < 0) ? 0u : (uint32)cap;
}

// money.pas CheckOverload: an add of `want` overloads when
// encumbrance + want exceeds capacity. `takable` receives how much still fits
// (capacity - encumbrance, or 0 when already at/over the limit).
bool checkOverload(const Data::ADnDCharacter &character, uint32 want,
        uint32 &takable) {
    const uint32 capacity = carryingCapacity(character);
    const uint32 encumbrance = character.encumbrance;
    if (encumbrance + want > capacity) {
        takable = (encumbrance >= capacity) ? 0u : capacity - encumbrance;
        return true;
    }
    takable = 0;
    return false;
}

// money.pas eligibility: PoolMoney/CountPoolers pool from true PCs (npc == 0)
// and animated PCs (npc == ANIMATED_PC); other NPCs are excluded.
bool isPooler(const Data::ADnDCharacter &character) {
    const uint8 npc = (uint8)character.npc;
    return npc == 0 || npc == TreasurePool::kAnimatedPc;
}

// ShareMoney pass 1 receives shares for any member whose NPC flag is clear
// (npc < 0x80), which is broader than the pooler test.
bool isShareReceiver(const Data::ADnDCharacter &character) {
    return (uint8)character.npc < TreasurePool::kNpcFlag;
}

Data::ADnDCharacter *asAdnd(Data::PlayerCharacter *pc) {
    return dynamic_cast<Data::ADnDCharacter *>(pc);
}

} // namespace

TreasurePool::TreasurePool() {
    for (int i = 0; i < Data::VALUABLE_COUNT; ++i)
        _coins[i] = 0;
    _items = new Common::Array<Data::Items::CharacterItem>();
}

TreasurePool::~TreasurePool() {
    delete _items;
}

void TreasurePool::clear() {
    for (int i = 0; i < Data::VALUABLE_COUNT; ++i)
        _coins[i] = 0;
    _items->clear();
}

uint16 TreasurePool::coin(Data::ValuableType type) const {
    if (type < Data::VAL_COPPER || type >= Data::VALUABLE_COUNT)
        return 0;
    const uint32 v = _coins[type];
    return (v > 0xFFFF) ? (uint16)0xFFFF : (uint16)v;
}

uint32 TreasurePool::coinWide(Data::ValuableType type) const {
    if (type < Data::VAL_COPPER || type >= Data::VALUABLE_COUNT)
        return 0;
    return _coins[type];
}

void TreasurePool::setCoin(Data::ValuableType type, uint16 amount) {
    if (type < Data::VAL_COPPER || type >= Data::VALUABLE_COUNT)
        return;
    _coins[type] = amount;
}

void TreasurePool::setCoinWide(Data::ValuableType type, uint32 amount) {
    if (type < Data::VAL_COPPER || type >= Data::VALUABLE_COUNT)
        return;
    _coins[type] = amount;
}

void TreasurePool::addCoinWide(Data::ValuableType type, uint32 amount) {
    if (type < Data::VAL_COPPER || type >= Data::VALUABLE_COUNT)
        return;
    // Non-saturating add in the 32-bit domain. A pool balance exceeding the
    // 32-bit range is not reachable from the game's money values; diagnose it
    // rather than silently wrapping (overflow/narrowing edge cases stay OPEN
    // and trace-gated per GB13-027).
    const uint32 before = _coins[type];
    _coins[type] = before + amount;
    if (_coins[type] < before)
        warning("TreasurePool::addCoinWide: 32-bit pool overflow (type %d)",
            (int)type);
}

bool TreasurePool::hasAnyCoin() const {
    for (int i = 0; i < Data::VALUABLE_COUNT; ++i)
        if (_coins[i] != 0)
            return true;
    return false;
}

uint32 TreasurePool::poolGoldValue() const {
    // CoinValueInGold (money.pas:80): sum copper..platinum weighted by the
    // copper valuation, then (total + 100) / goldRate. Computed in uint32.
    uint32 sum = 0;
    for (int i = 0; i < Data::ValuableItems::kCoinSlotCount; ++i)
        sum += _coins[i] * Data::ValuableItems::kExchangeRates[i];
    return (sum + 100) / Data::ValuableItems::kGoldRate;
}

void TreasurePool::setPoolFromGoldValue(uint32 goldValue) {
    // SetTreasureCoinsInGold (money.pas:107): zero the coin slots, store bulk
    // as platinum (gp / 5) and the remainder as gold (gp mod 5). Gems and
    // jewelry are untouched.
    for (int i = 0; i < Data::ValuableItems::kCoinSlotCount; ++i)
        _coins[i] = 0;
    _coins[Data::VAL_PLATINUM] = goldValue / 5;
    _coins[Data::VAL_GOLD] = goldValue % 5;
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

void TreasurePool::syncPool(Common::Serializer &s) {
    // 32-bit per denomination: the widened pool balance round-trips with no
    // narrowing. See the header TODO(GB13-033) about wiring this into the
    // native-save schema.
    for (int i = 0; i < Data::VALUABLE_COUNT; ++i)
        s.syncAsUint32LE(_coins[i]);
}

TreasurePool::MoneyTransfer TreasurePool::takeMoney(
        Data::ADnDCharacter &character, Data::ValuableType type,
        uint32 amount) {
    MoneyTransfer result;
    result.requested = amount;

    if (type < Data::VAL_COPPER || type >= Data::VALUABLE_COUNT) {
        warning("TreasurePool::takeMoney: invalid valuable type %d", type);
        result.status = MoneyTransfer::TRANSFER_INVALID_TYPE;
        return result;
    }

    character.recalcCombatStats();

    // money.pas TakeMoney: CheckOverload is run against the requested amount.
    // When it overloads, the original rejects the whole request (prints
    // "Overloaded") and moves nothing.
    uint32 takable = 0;
    if (checkOverload(character, amount, takable)) {
        result.status = MoneyTransfer::TRANSFER_OVERLOADED;
        result.transferred = 0;
        result.remainder = _coins[type];
        return result;
    }

    // Clamp to what the pool actually holds and move exactly that much. The
    // character field is Word; the move is bounded by carrying capacity above,
    // which keeps coin counts well inside the Word range in practice.
    const uint32 moved = MIN<uint32>(amount, _coins[type]);
    _coins[type] -= moved;

    const uint32 balance = (uint32)character.valuableItems[type] + moved;
    character.valuableItems[type] = (balance > 0xFFFF)
        ? (uint16)0xFFFF : (uint16)balance;
    if (balance > 0xFFFF)
        warning("TreasurePool::takeMoney: character Word overflow (type %d)",
            (int)type);

    character.recalcCombatStats();

    result.transferred = moved;
    result.remainder = _coins[type];
    result.status = MoneyTransfer::TRANSFER_OK;
    return result;
}

bool TreasurePool::givePlatinum(Data::ADnDCharacter &character,
        uint32 amount) {
    character.recalcCombatStats();

    uint32 takable = 0;
    const bool overloaded = checkOverload(character, amount, takable);
    const uint32 given = overloaded ? takable : amount;
    if (overloaded)
        _coins[Data::VAL_PLATINUM] += amount - takable;

    const uint32 balance =
        (uint32)character.valuableItems[Data::VAL_PLATINUM] + given;
    character.valuableItems[Data::VAL_PLATINUM] = (balance > 0xFFFF)
        ? (uint16)0xFFFF : (uint16)balance;
    character.recalcCombatStats();
    return overloaded;
}

bool TreasurePool::poolMoneyFromParty(
        Common::List<Data::PlayerCharacter *> &party) {
    for (Common::List<Data::PlayerCharacter *>::const_iterator it =
            party.begin(); it != party.end(); ++it) {
        Data::ADnDCharacter *ch = asAdnd(*it);
        if (!ch || !isPooler(*ch))
            continue;

        for (int t = 0; t < Data::VALUABLE_COUNT; ++t) {
            addCoinWide((Data::ValuableType)t, ch->valuableItems.values[t]);
            ch->valuableItems.values[t] = 0;
        }
        ch->recalcCombatStats();
    }
    return hasAnyCoin();
}

bool TreasurePool::shareMoneyToParty(
        Common::List<Data::PlayerCharacter *> &party) {
    // CountPoolers: number of eligible members to divide shares among.
    uint32 cnt = 0;
    for (Common::List<Data::PlayerCharacter *>::const_iterator it =
            party.begin(); it != party.end(); ++it) {
        Data::ADnDCharacter *ch = asAdnd(*it);
        if (ch && isPooler(*ch))
            ++cnt;
    }
    if (cnt == 0)
        return hasAnyCoin();

    uint32 share[Data::VALUABLE_COUNT];
    uint32 rem[Data::VALUABLE_COUNT];
    for (int i = 0; i < Data::VALUABLE_COUNT; ++i) {
        if (_coins[i] > 0) {
            share[i] = _coins[i] / cnt;
            rem[i] = _coins[i] % cnt;
        } else {
            share[i] = 0;
            rem[i] = 0;
        }
    }

    // Pass 1: hand each share-receiver its per-denomination share plus, when
    // it still fits, one remainder coin. JEWELRY..COPPER order matches the
    // original. A share that overloads only partially credits what fits and
    // pushes the rest back into the remainder.
    for (Common::List<Data::PlayerCharacter *>::const_iterator it =
            party.begin(); it != party.end(); ++it) {
        Data::ADnDCharacter *ch = asAdnd(*it);
        if (!ch || !isShareReceiver(*ch))
            continue;

        ch->recalcCombatStats();
        for (int i = Data::VAL_JEWELRY; i >= Data::VAL_COPPER; --i) {
            uint32 takable = 0;
            if (!checkOverload(*ch, share[i], takable)) {
                ch->valuableItems.values[i] =
                    (uint16)(ch->valuableItems.values[i] + share[i]);
                ch->recalcCombatStats();
                if (rem[i] > 0) {
                    uint32 takable2 = 0;
                    if (!checkOverload(*ch, 1, takable2)) {
                        ch->valuableItems.values[i] =
                            (uint16)(ch->valuableItems.values[i] + 1);
                        ch->recalcCombatStats();
                        --rem[i];
                    }
                }
            } else {
                ch->valuableItems.values[i] =
                    (uint16)(ch->valuableItems.values[i] + takable);
                rem[i] = rem[i] + share[i] - takable;
                ch->recalcCombatStats();
            }
        }
    }

    // Pass 2: distribute leftover remainder across the whole list, limited by
    // each member's remaining carrying capacity. JEWELRY..COPPER order.
    for (int i = Data::VAL_JEWELRY; i >= Data::VAL_COPPER; --i) {
        if (rem[i] == 0)
            continue;
        for (Common::List<Data::PlayerCharacter *>::const_iterator it =
                party.begin(); it != party.end(); ++it) {
            Data::ADnDCharacter *ch = asAdnd(*it);
            if (!ch)
                continue;
            ch->recalcCombatStats();
            const uint32 capacity = carryingCapacity(*ch);
            const uint32 encumbrance = ch->encumbrance;
            const uint32 takable = (encumbrance >= capacity)
                ? 0u : capacity - encumbrance;
            if (takable == 0)
                continue;
            const uint32 give = MIN<uint32>(rem[i], takable);
            ch->valuableItems.values[i] =
                (uint16)(ch->valuableItems.values[i] + give);
            rem[i] -= give;
            ch->recalcCombatStats();
            if (rem[i] == 0)
                break;
        }
    }

    // Return whatever did not fit to the pool.
    for (int i = 0; i < Data::VALUABLE_COUNT; ++i)
        _coins[i] = rem[i];

    return hasAnyCoin();
}

} // namespace Runtime
} // namespace Goldbox
