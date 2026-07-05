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

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Gfx {
class BattlefieldTilemap;
}

namespace Combat {

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
     * @param tilemap      Battlefield terrain (for passability checks)
     * @param isDungeon    True if dungeon terrain
     * @param table        Output: combatant positions written here
     */
    void placeAll(Common::Array<Data::PlayerCharacter *> &roster,
                  int partyCount,
                  uint8 mapDirection,
                  int encounterDist,
                  const Gfx::BattlefieldTilemap &tilemap,
                  bool isDungeon,
                  CombatantTable &table);

private:
    // Formation validity mask: [side][slot][row][col]
    uint8 _formationValid[CombatantTable::SIDE_COUNT][FORMATION_SLOTS][FORMATION_ROWS][FORMATION_COLS];

    // Team state
    int8 _originX[CombatantTable::SIDE_COUNT];
    int8 _originY[CombatantTable::SIDE_COUNT];
    uint8 _teamDir[CombatantTable::SIDE_COUNT];  // half-direction (0-3)
    int _halfCount[CombatantTable::SIDE_COUNT];

    // Current placement context
    int _currentSide;
    bool _isDungeon;
    const Gfx::BattlefieldTilemap *_tilemap;
    CombatantTable *_table;

    // --- Direction tables (from spec) ---

    static const uint8 kDirPrimary[4][4];
    static const uint8 kDirFallback[4][4];
    static const int8 kBaseX[8];
    static const int8 kBaseY[8];
    static const uint8 kHalfDirToIso[4];
    static const int8 kFormationRange[5][6][2];

    // --- Internal methods ---

    void buildFormationMasks();

    /** Spiral-place one combatant. Returns true on success. */
    bool placeCombatantSpiral(int charIdx);

    /** Try placing at a formation cell. Returns true if valid and unoccupied. */
    bool tryPlaceAt(int charIdx, int formCol, int formRow,
                    int8 originCol, int8 originRow, int slot);

    /** Scan destination tiles for occupant/passability. */
    void scanDestination(int charIdx, uint8 direction,
                         uint8 &outOccupant, uint8 &outTile) const;

    /** Check if both axes are simultaneously out of formation range. */
    static bool bothAxesOutOfRange(int row, int col);
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_PLACEMENT_H
