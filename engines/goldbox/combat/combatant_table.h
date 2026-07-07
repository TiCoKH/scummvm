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

#ifndef GOLDBOX_COMBAT_COMBATANT_TABLE_H
#define GOLDBOX_COMBAT_COMBATANT_TABLE_H

#include "common/scummsys.h"
#include "common/array.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

/**
 * Runtime combatant position and state table for tactical combat.
 *
 * Mirrors original 1-based combatant arrays:
 *   combatant_ptr[], combatant_tile_col[], combatant_tile_row[],
 *   combatant_size[]
 *
 * Uses 0-based indexing internally; the original 1-based convention
 * is handled at the API boundary where needed.
 */
class CombatantTable {
public:
    static const int MAX_COMBATANTS = 72;
    static const uint8 TILE_TRIGGER = 0x1F;

    enum Side {
        SIDE_PARTY = 0,
        SIDE_ENEMY = 1,
        SIDE_COUNT = 2
    };

    /**
     * Record for downed/sleeping combatants placed as terrain triggers.
     */
    struct TriggerRecord {
        Data::PlayerCharacter *character;
        uint8 tileCol;
        uint8 tileRow;
        uint8 savedTile;

        TriggerRecord()
            : character(nullptr), tileCol(0), tileRow(0), savedTile(0) {}
    };

    CombatantTable();

    void clear();

    /** Total placed combatants. */
    int getCount() const { return _count; }

    /** Add a combatant entry. Returns slot index or -1 on failure. */
    int addCombatant(Data::PlayerCharacter *ch, uint8 size);

    /** Remove combatant at index (shifts nothing; zeros the slot). */
    void removeCombatant(int idx);

    // --- Position access ---

    Data::PlayerCharacter *getCharacter(int idx) const;
    uint8 getTileCol(int idx) const;
    uint8 getTileRow(int idx) const;
    uint8 getSize(int idx) const;

    void setPosition(int idx, uint8 col, uint8 row);
    void setSize(int idx, uint8 size);

    // --- Trigger records ---

    void addTrigger(Data::PlayerCharacter *ch, uint8 col, uint8 row, uint8 savedTile);
    const Common::Array<TriggerRecord> &getTriggers() const { return _triggers; }

    // --- Occupancy grid ---

    /** Rebuild occupancy from current positions. */
    void rebuildOccupancy();

    /** Get occupant index at tile (0 = empty, 1-based combatant index). */
    uint8 getOccupant(int col, int row) const;

    // --- Distance cache ---

    /**
     * Recompute distance arrays relative to a cursor/origin position.
     * After calling, getColDist(i) and getRowDist(i) return the signed
     * offset from (cursorCol, cursorRow) to combatant i's position.
     */
    void rebuildDistances(int cursorCol, int cursorRow);

    int8 getColDist(int idx) const;
    int8 getRowDist(int idx) const;

    /** Manhattan distance from cursor to combatant. */
    int getManhattanDist(int idx) const;

    // --- Side counts ---

    int getFriendsCount() const { return _friendsCount; }
    int getFoesCount() const { return _foesCount; }
    void countSides(int partyCount);

private:
    struct Entry {
        Data::PlayerCharacter *character;
        uint8 tileCol;
        uint8 tileRow;
        uint8 size;  // 0 = not placed/removed

        Entry() : character(nullptr), tileCol(0), tileRow(0), size(0) {}
    };

    Entry _entries[MAX_COMBATANTS];
    int _count;
    int _friendsCount;
    int _foesCount;

    // 50x25 occupancy grid: stores 1-based combatant index (0 = empty)
    uint8 _occupancy[25][50];

    // Distance cache: signed offset from cursor to each combatant
    int8 _colDist[MAX_COMBATANTS];
    int8 _rowDist[MAX_COMBATANTS];

    Common::Array<TriggerRecord> _triggers;
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBATANT_TABLE_H
