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
#include "common/list.h"
#include "common/scummsys.h"
#include "common/serializer.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Data {
class ADnDCharacter;
class PlayerCharacter;
namespace Items {
struct CharacterItem;
} // namespace Items
} // namespace Data

namespace Runtime {

/**
 * Shared runtime treasure pool.
 *
 * Populated by the Host layer (opcode 0x27 TREASURE / setupTreasure),
 * consumed by the UI layer (TreasureDialog, StoreDialog, TempleDialog).
 * Acts as the engine equivalent of PTR_ITEM_LIST + MONEY_POOL_SLOTS globals
 * from the original binary.
 *
 * Width policy (GB13-027):
 *   The original (money.pas / TMoney) stores each pooled denomination as a
 *   signed 32-bit LongInt, while character coin fields are 16-bit Word. We
 *   mirror that: the pool balance is kept in a `uint32` per denomination so
 *   that aggregating several near-Word-limit party balances cannot saturate,
 *   while character coin slots stay Word in their codecs. Pooled balances are
 *   non-negative by construction (only additions and clamped subtractions),
 *   so `uint32` covers the LongInt positive range used in practice. All
 *   per-denomination arithmetic is done in `uint32`; narrowing to Word only
 *   happens at explicit, named boundaries (character coin fields).
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

    // Legacy/display accessor: clamps the 32-bit pool balance into a Word.
    // Use coinWide() for the authoritative balance and for any arithmetic.
    uint16 coin(Data::ValuableType type) const;

    // Authoritative 32-bit balance for a denomination (the LongInt width).
    uint32 coinWide(Data::ValuableType type) const;

    // Set a denomination directly. setCoin keeps the Word-sized setter that
    // TREASURE / setupTreasure uses; setCoinWide covers the full 32-bit range.
    void setCoin(Data::ValuableType type, uint16 amount);
    void setCoinWide(Data::ValuableType type, uint32 amount);

    // Add to a denomination without saturating (overflow-returning path for
    // e.g. appraisal remainder). Computed in a wider intermediate.
    void addCoinWide(Data::ValuableType type, uint32 amount);

    bool hasAnyCoin() const;

    // Gold-value helpers for the payment/appraisal paths. Conversion uses the
    // original copper-based valuation (200 copper per gp); see rules_types.h.
    uint32 poolGoldValue() const;
    void setPoolFromGoldValue(uint32 goldValue);

    // --- Item access (defined out-of-line in .cpp) ---
    const Common::Array<Data::Items::CharacterItem> &items() const;
    Common::Array<Data::Items::CharacterItem> &items();
    void addItem(const Data::Items::CharacterItem &item);
    bool hasItems() const;
    bool isEmpty() const;

    // --- Money operations (Pool/Share/Take) ---

    /**
     * Result of a capacity-aware transfer. `requested` is the amount asked
     * for, `transferred` the amount actually moved (never more than the pool
     * balance or the recipient's remaining carrying capacity), and
     * `remainder` the pool balance left in that denomination afterwards.
     * The original TakeMoney rejects the whole request when the requested
     * amount would overload the recipient (it does not do a partial move),
     * so an overloaded transfer reports `transferred == 0` with the balance
     * unchanged in `remainder`.
     */
    struct MoneyTransfer {
        enum Status {
            TRANSFER_OK,
            TRANSFER_OVERLOADED,
            TRANSFER_INVALID_TYPE
        };

        uint32 requested;
        uint32 transferred;
        uint32 remainder;
        Status status;

        MoneyTransfer()
            : requested(0), transferred(0), remainder(0),
              status(TRANSFER_OK) {}
    };

    /**
     * CHARACTER_addCoin / money.pas TakeMoney: move `amount` of a denomination
     * from the pool into a character. Carrying capacity is checked against the
     * full requested amount before any mutation; if it would overload the
     * recipient the request is rejected with no transfer (matching the
     * original "Overloaded" early-out). Otherwise the amount is clamped to the
     * pool balance, moved, and the source decreased by exactly the amount
     * moved. Recalculates character stats (and therefore encumbrance) after a
     * committed transfer. `amount` is taken as 32-bit so pool balances above
     * the Word range can be requested; the character field stays Word.
     */
    MoneyTransfer takeMoney(Data::ADnDCharacter &character,
        Data::ValuableType type, uint32 amount);

    /**
     * ACTION_PoolMoney: strip coins from every eligible party member into the
     * pool. Eligible members are true PCs (npc == 0) and animated PCs
     * (npc == ANIMATED_PC); other NPCs keep their coins. All seven
     * denominations are pooled, the member's encumbrance is reduced, and the
     * member's coin slots are zeroed.
     * @return true if the pool holds coins afterwards.
     */
    bool poolMoneyFromParty(Common::List<Data::PlayerCharacter *> &party);

    /**
     * ACTION_ShareMoney: distribute pooled coins across the party following
     * money.pas ShareMoney. Each denomination is split share = total / cnt
     * with remainder = total mod cnt, where cnt counts the poolers
     * (CountPoolers). Pass 1 (JEWELRY..COPPER, every member with npc < NPC
     * flag) gives each member its share subject to carrying capacity, hands a
     * single remainder coin to members that can still carry one, and pushes
     * any share that did not fit back into the remainder. Pass 2 distributes
     * leftover remainder capacity-aware across the whole list. Whatever still
     * does not fit is returned to the pool.
     * @return true if the pool still holds coins after sharing.
     */
    bool shareMoneyToParty(Common::List<Data::PlayerCharacter *> &party);

    // Eligibility helpers (exposed for the money operations / tests).
    // ANIMATED_PC is the original C_Npc.ANIMATED_PC value (0xB3).
    static const uint8 kNpcFlag = 0x80;
    static const uint8 kAnimatedPc = 0xB3;

    /**
     * (De)serialize the pooled money balances. Each denomination is written
     * as a full 32-bit value so the widened pool state round-trips without
     * truncation (the whole point of GB13-027). Items are NOT handled here;
     * they are owned/serialized elsewhere.
     *
     * TODO(GB13-033): the native-save schema does not yet call this seam
     * (Engine::syncGame currently writes only a placeholder). When GB13-033
     * defines the native save schema it must sync the pool through this
     * 32-bit-wide path and version the schema; do not narrow to Word.
     */
    void syncPool(Common::Serializer &s);

private:
    // Per-denomination pool balance, 32-bit (original LongInt width).
    uint32 _coins[Data::VALUABLE_COUNT];
    Common::Array<Data::Items::CharacterItem> *_items;
};

} // namespace Runtime
} // namespace Goldbox

#endif // GOLDBOX_RUNTIME_TREASURE_POOL_H
