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

#ifndef GOLDBOX_COMBAT_COMBAT_PLACEMENT_H
#define GOLDBOX_COMBAT_COMBAT_PLACEMENT_H

#include "common/scummsys.h"
#include "common/array.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_globals.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

class BattlefieldMap;

/**
 * Combat placement engine implementing the spiral placement algorithm.
 *
 * Places all combatants onto the battlefield tilemap using formation
 * masks and an expanding spiral search pattern. Operates independently
 * of the VM — receives roster and terrain, produces positioned combatants.
 *
 * Reference: BattleSetup_Analysis.md "PlaceCombatants" and "Spiral Placement"
 */
class CombatPlacement {
public:
    static const int FORMATION_COLS = 11;
    static const int FORMATION_ROWS = 6;
    static const int FORMATION_SLOTS = 4;

    CombatPlacement();

    /**
     * Place all combatants onto the battlefield.
     *
     * @param roster       All characters entering combat
     * @param partyCount   Number of player-controlled characters
     * @param mapDirection Approach direction (0-7 wire format)
     * @param encounterDist Distance between party and enemy origins
    * @param map          Battlefield map data (for passability checks)
     * @param table        Output: combatant positions written here
     */
    void placeAll(Common::Array<Data::PlayerCharacter *> &roster,
                  int partyCount,
                  uint8 mapDirection,
                  int encounterDist,
                  BattlefieldMap &map,
                  bool combatTriggerActive,
                  CombatantTable &table,
                  CombatGlobals &globals);

private:
    /**
     * Per-side placement state.
     * Mirrors the original CombatSideData struct:
     *   origin_x/y   = ORIGIN_X/Y[side]
     *   dir_idx      = PARTY_DIR_IDX[side]  (way_flag derived half-direction 0-3)
     *   side_dir     = SIDE_DIR_TABLE[side] = (hostility+1)>>1
     *   valid_mask   = ARRAY_FORMATION_VALID_MASK[side]
     */
    struct CombatSideData {
        int8  origin_x;
        int8  origin_y;
        uint8 dir_idx;
        uint8 side_dir;
        // Formation mask: reset per-combatant, tracks cells available in
        // the current spiral pass (consumed on success).
        uint8 valid_mask[FORMATION_SLOTS][FORMATION_ROWS][FORMATION_COLS];
        // Terrain-blocked playfield bitmap: set permanently when a tile is
        // found impassable, so future combatants skip it immediately.
        // 50*25 = 1250 bits, 157 bytes.
        uint8 blocked[25][50];

        CombatSideData();
    };

    CombatSideData _sides[CombatantTable::SIDE_COUNT];
    int _halfCount[CombatantTable::SIDE_COUNT];

    // Current placement context
    int _currentSide;
    bool _isDungeon;
    int8 _mapCenterX;
    int8 _mapCenterY;
    uint8 _mapDirection;
    BattlefieldMap *_map;
    CombatantTable *_table;
    // Table index range [start, end) for each side — set in placeAll.
    int _sideStart[CombatantTable::SIDE_COUNT];
    int _sideEnd[CombatantTable::SIDE_COUNT];

    // --- Direction tables (from spec) ---

    static const uint8 kDirAxisDirection[4][4];  // ARRAY_SPIRAL_AXIS_DIRECTION
    static const uint8 kDirFormFallback[4][4];   // ARRAY_SPIRAL_FORM_FALLBACK
    static const int8 kBaseX[8];
    static const int8 kBaseY[8];
    static const uint8 kHalfDirToIso[4];
    static const int8 kFormationRange[5][6][2];

    // --- Internal methods ---

    void buildFormationMasks();
    void rebuildFormationMask(int side);  // reset mask for one side before each placement

    /** Spiral-place one combatant. Returns true on success. */
    bool placeCombatantSpiral(int charIdx);

    /** Full playfield scan fallback when spiral exhausts all formSets. */
    bool placeCombatantFullScan(int charIdx);

    /** Try placing at a formation cell. Returns true if valid and unoccupied. */
    bool tryPlaceAt(int charIdx, int formCol, int formRow,
                    int8 originCol, int8 originRow, int slot);

    /** Check if position is outside all formation masks for current side. */
    bool isOutOfFormation(int col, int row);
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_PLACEMENT_H
