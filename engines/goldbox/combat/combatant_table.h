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
#include "goldbox/core/tile_pos.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

/**
 * Runtime combatant position and state table for tactical combat.
 *
 * Mirrors original parallel arrays:
 *   gbCombatPosition_ARRAY[] (id, icon_size, tile_col, tile_row)
 *   C_CH_PTR_TABLE[]         (character pointers)
 *   C_FIELD_PLACEMENT_MAP[]  (50x25 occupancy grid)
 *   BYTE_ARRAY_COL_DIST[]    (viewport-relative column position)
 *   BYTE_ARRAY_ROW_DIST[]    (viewport-relative row position)
 *
 * Modernized: single Entry struct as authoritative source, occupancy
 * grid and viewport-relative positions are derived state rebuilt
 * lazily on access.
 *
 * Uses 0-based indexing internally; the original 1-based convention
 * is handled at the API boundary (occupancy stores index+1).
 */
class CombatantTable {
public:
    static const int MAX_COMBATANTS = 72;
    static const uint8 TILE_DOWNED_MEMBER = 0x1F;

    enum Side {
        SIDE_PARTY = 0,
        SIDE_ENEMY = 1,
        SIDE_COUNT = 2
    };

    /**
     * Record for downed/sleeping combatants placed as terrain triggers.
     * Mirrors original ARRAY_MEMBER_DOWN_ICONS[].
     */
    struct DownedMemberRecord {
        Data::PlayerCharacter *character;
        TilePos pos;
        uint8 savedTile;

        DownedMemberRecord()
            : character(nullptr), pos(), savedTile(0) {}
    };

    CombatantTable();

    void clear();

    /** Total placed combatants. */
    int getCount() const { return _count; }

    /** Add a combatant entry. Returns slot index or -1 on failure. */
    int addCombatant(Data::PlayerCharacter *ch, uint8 size);

    /** Roll back the most recently added combatant entry. */
    bool rollbackLastAdd(int idx);

    /** Remove combatant at index (zeros the slot). */
    void removeCombatant(int idx);

    // --- Index lookup ---

    /** Find the table index for a given character pointer. Returns -1 if not found. */
    int findIndex(const Data::PlayerCharacter *ch) const;

    // --- Position access by index ---

    Data::PlayerCharacter *getCharacter(int idx) const;
    uint8 getTileCol(int idx) const;
    uint8 getTileRow(int idx) const;
    uint8 getSize(int idx) const;

    /** Set position and mark occupancy dirty. */
    void setPosition(int idx, TilePos pos);

    /** Set size and mark occupancy dirty. */
    void setSize(int idx, uint8 size);

    // --- Position access by character pointer ---

    uint8 getCharacterCol(const Data::PlayerCharacter *ch) const;
    uint8 getCharacterRow(const Data::PlayerCharacter *ch) const;
    uint8 getCharacterSize(const Data::PlayerCharacter *ch) const;

    // --- Trigger records ---

    void addDownedMember(Data::PlayerCharacter *ch, TilePos pos, uint8 savedTile);
    const Common::Array<DownedMemberRecord> &getDownedMembers() const { return _downedMembers; }

    // --- Occupancy grid (lazy rebuild) ---

    uint8 getOccupant(TilePos pos) const;
    // Raw int overload for arithmetic call sites
    uint8 getOccupant(int col, int row) const { return getOccupant(TilePos((uint8)col, (uint8)row)); }

    /**
     * Force an immediate occupancy rebuild.
     * Normally not needed — getOccupant() self-heals. Provided for
     * callers that need to guarantee the grid is fresh before a batch
     * of raw grid reads (e.g. rendering loops).
     */
    void ensureOccupancy() const;

    /**
     * Explicitly mark occupancy as dirty.
     * Called automatically by setPosition/setSize/add/remove, but
     * exposed for edge cases (e.g. external tile map changes).
     */
    void invalidateOccupancy() { _occupancyDirty = true; }

    // --- Viewport-relative position cache (lazy rebuild) ---

    /**
     * Set the viewport origin for position calculations.
     *
     * Mirrors original BYTE_ARRAY_COL_DIST / BYTE_ARRAY_ROW_DIST which
     * store each combatant's tile position relative to the viewport
     * top-left corner (PTR_COMBAT_FIELD->viewport_startX/Y).
     *
     * Used by the renderer to compute screen-space tile coordinates
     * and by COMBAT_IsCharacterInBounds to check viewport visibility.
     *
     * @param vpCol Viewport top-left column (CombatViewport::getTopLeftCol)
     * @param vpRow Viewport top-left row (CombatViewport::getTopLeftRow)
     */
    void setViewportOrigin(TilePos origin);

    /** Column offset from viewport origin for combatant idx. */
    int8 getColDist(int idx) const;

    /** Row offset from viewport origin for combatant idx. */
    int8 getRowDist(int idx) const;

    /** Manhattan distance from viewport origin to combatant. */
    int getManhattanDist(int idx) const;

    // --- Side counts ---

    int getFriendsCount() const { return _friendsCount; }
    int getFoesCount() const { return _foesCount; }
    void countSides(int partyCount);

    // --- Legacy compatibility ---

    /**
     * Explicit rebuild calls for code that mirrors original call sites.
     * These just mark dirty and optionally force immediate rebuild.
     * Prefer letting lazy rebuild handle it instead.
     */
    void rebuildOccupancy() { _occupancyDirty = true; ensureOccupancy(); }

    /** @deprecated Use setViewportOrigin instead. */
    void setDistanceOrigin(TilePos origin) { setViewportOrigin(origin); }
    void rebuildDistances(TilePos origin) { setViewportOrigin(origin); }

private:
    struct Entry {
        Data::PlayerCharacter *character;
        TilePos pos;
        uint8 size;

        Entry() : character(nullptr), pos(), size(0) {}
    };

    Entry _entries[MAX_COMBATANTS];
    int _count;
    int _friendsCount;
    int _foesCount;

    // --- Lazy occupancy grid (50x25, stores 1-based index, 0=empty) ---
    mutable uint8 _occupancy[25][50];
    mutable bool _occupancyDirty;

    void doRebuildOccupancy() const;

    // --- Lazy viewport-relative position cache ---
    mutable int8 _colDist[MAX_COMBATANTS];
    mutable int8 _rowDist[MAX_COMBATANTS];
    mutable bool _vpPosDirty;
    mutable int _vpOriginCol;
    mutable int _vpOriginRow;

    void doRebuildViewportPositions() const;

    Common::Array<DownedMemberRecord> _downedMembers;
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBATANT_TABLE_H
