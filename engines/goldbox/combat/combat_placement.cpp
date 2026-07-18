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

#include "goldbox/combat/combat_placement.h"
#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/combat_ground_info.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/data/player_character.h"
#include "goldbox/core/direction.h"
#include <string.h>


namespace Goldbox {
namespace Combat {

// HalfDirToIso[4] — converts half-direction to 8-way isometric facing
const uint8 CombatPlacement::kHalfDirToIso[4] = { 7, 2, 3, 6 };

// ARRAY_SPIRAL_FORM_FALLBACK[4][4] — form_set direction table (drives dirIndex for anchor/arm axes)
const uint8 CombatPlacement::kDirAxisDirection[4][4] = {
    { 0, 0, 2, 6 },
    { 2, 2, 0, 4 },
    { 4, 4, 2, 6 },
    { 6, 6, 4, 0 },
};

// ARRAY_SPIRAL_AXIS_DIRECTION[4][4] — fallback approach directions (form_set advance + diagonal check)
const uint8 CombatPlacement::kDirFormFallback[4][4] = {
    { 8, 4, 6, 2 },
    { 8, 6, 4, 0 },
    { 8, 0, 6, 2 },
    { 8, 2, 0, 4 },
};

// BASE_X[8] and BASE_Y[8] — spiral base offsets
// First 4: primary direction offsets, Last 4: fallback direction offsets
const int8 CombatPlacement::kBaseX[8] = { 5, 4, 5, 6, 3, 8, 7, 2 };
const int8 CombatPlacement::kBaseY[8] = { 3, 2, 2, 3, 0, 2, 5, 3 };

// FORMATION_RANGE[5][6][2] — valid column bounds per direction and row
// [direction_index][row][0=minCol, 1=maxCol]
// Rows where min > max have no valid cells.
const int8 CombatPlacement::kFormationRange[5][6][2] = {
    // dir N
    { {1, 0}, {1, 0}, {1, 0}, {2, 9}, {3, 10}, {4, 10} },
    // dir E
    { {0, 2}, {0, 3}, {1, 4}, {2, 5}, {3, 6}, {4, 7} },
    // dir S
    { {0, 6}, {0, 7}, {1, 8}, {1, 0}, {1, 0}, {1, 0} },
    // dir W
    { {3, 6}, {4, 7}, {5, 8}, {6, 9}, {7, 10}, {8, 10} },
    // dir alaways slot 1
    { {0, 6}, {0, 7}, {1, 8}, {2, 9}, {3, 10}, {4, 10} },
};

CombatPlacement::CombatSideData::CombatSideData()
    : origin_x(0), origin_y(0), dir_idx(0), side_dir(0) {
    memset(valid_mask, 0, sizeof(valid_mask));
}

CombatPlacement::CombatPlacement()
        : _currentSide(0), _isDungeon(false), _mapCenterX(0), _mapCenterY(0),
            _mapDirection(0), _map(nullptr), _table(nullptr) {
    memset(_halfCount, 0, sizeof(_halfCount));
    memset(_sideStart, 0, sizeof(_sideStart));
    memset(_sideEnd, 0, sizeof(_sideEnd));
}

void CombatPlacement::placeAll(Common::Array<Data::PlayerCharacter *> &roster,
                               int partyCount,
                               uint8 mapDirection,
                               int encounterDist,
                               BattlefieldMap &map,
                               bool combatTriggerActive,
                               CombatantTable &table,
                               CombatGlobals &globals) {
    _map = &map;
    _isDungeon = map.isDungeon();
    _mapCenterX = map.getCenterX();
    _mapCenterY = map.getCenterY();
    _mapDirection = mapDirection;
    _table = &table;

    globals.updateSideCount(roster);

    table.clear();

    // Compute team origins and directions
    CombatSideData &party = _sides[CombatantTable::SIDE_PARTY];
    CombatSideData &enemy = _sides[CombatantTable::SIDE_ENEMY];

    party.origin_x = 0;
    party.origin_y = 0;
    party.dir_idx  = (mapDirection / 2) & 3;
    party.side_dir = (globals.sideCount[CombatantTable::SIDE_PARTY] + 1) >> 1;

    // Enemy origin uses the full 8-way approach direction from the reference
    // placement logic; team_direction is reserved for facing/formation tables.
    enemy.origin_x = (int8)(encounterDist * kDirDeltaX[mapDirection]);
    enemy.origin_y = (int8)(encounterDist * kDirDeltaY[mapDirection]);
    enemy.dir_idx  = (((mapDirection + 4) % 8) / 2) & 3;
    enemy.side_dir = (globals.sideCount[CombatantTable::SIDE_ENEMY] + 1) >> 1;

    // Count sides
    int friendsCount = 0;
    int foesCount = 0;
    for (int i = 0; i < (int)roster.size(); i++) {
        if (!roster[i])
            continue;
        if (i < partyCount)
            friendsCount++;
        else
            foesCount++;
    }
    _halfCount[CombatantTable::SIDE_PARTY] = (friendsCount + 1) / 2;
    _halfCount[CombatantTable::SIDE_ENEMY] = (foesCount + 1) / 2;

    // Record per-side table index ranges for full-scan centroid calculation.
    // Count non-null party and enemy entries to get table index boundaries.
    int tablePartyCount = 0;
    for (int i = 0; i < partyCount && i < (int)roster.size(); i++)
        if (roster[i]) tablePartyCount++;
    _sideStart[CombatantTable::SIDE_PARTY] = 0;
    _sideEnd[CombatantTable::SIDE_PARTY]   = tablePartyCount;
    _sideStart[CombatantTable::SIDE_ENEMY] = tablePartyCount;
    _sideEnd[CombatantTable::SIDE_ENEMY]   = tablePartyCount + foesCount;

    // Build formation validity masks
    buildFormationMasks();

    // Place each combatant
    for (int i = 0; i < (int)roster.size(); i++) {
        Data::PlayerCharacter *ch = roster[i];
        if (!ch)
            continue;

        _currentSide = (i < partyCount) ? CombatantTable::SIDE_PARTY
                                        : CombatantTable::SIDE_ENEMY;

        uint8 size = ch->iconDimension & 7;
        if (size == 0)
            size = 1; // default to 1x1 if not set
        int idx = table.addCombatant(ch, size);
        if (idx < 0)
            break;

        if (placeCombatantSpiral(idx)) {
            const bool notInTeam = ch->combatState && ch->combatState->notInTeam;
            if (!ch->enabled && !combatTriggerActive && !notInTeam) {
                table.setSize(idx, 0);
                uint8 col = table.getTileCol(idx);
                uint8 row = table.getTileRow(idx);
                uint8 savedTile = _map->getRawTile(col, row);
                _map->setRawTile(col, row, CombatantTable::TILE_DOWNED_MEMBER);
                table.addDownedMember(ch, col, row, savedTile);
                globals.membersOnGround++;
            }
            table.rebuildOccupancy();
        } else {
            const bool notInTeam = ch->combatState && ch->combatState->notInTeam;
            if (notInTeam) {
                table.rollbackLastAdd(idx);
                roster[i] = nullptr;
            } else {
                table.setSize(idx, 0);
            }
        }
    }

    table.countSides(partyCount);
}

void CombatPlacement::buildFormationMasks() {
    for (int side = 0; side < CombatantTable::SIDE_COUNT; side++) {
        CombatSideData &sd = _sides[side];
        memset(sd.valid_mask, 0, sizeof(sd.valid_mask));

        for (int slot = 0; slot < FORMATION_SLOTS; slot++) {
            int dirIdx = (slot == 1) ? 4 : sd.dir_idx;

            for (int row = 0; row < FORMATION_ROWS; row++) {
                int8 minCol = kFormationRange[dirIdx][row][0];
                int8 maxCol = kFormationRange[dirIdx][row][1];

                for (int col = 0; col < FORMATION_COLS; col++)
                    sd.valid_mask[slot][row][col] = (col >= minCol && col <= maxCol) ? 1 : 0;
            }
        }
    }
}

bool CombatPlacement::placeCombatantSpiral(int charIdx) {
    int state = 1;
    int rowScale = 0;
    int var14 = 0;
    bool firstRow = true;
    bool totalFail = false;

    const CombatSideData &sd = _sides[_currentSide];
    int8 teamX = sd.origin_x;
    int8 teamY = sd.origin_y;

    int8 baseCol = 0;
    int8 baseRow = 0;
    int8 candCol = 0;
    int8 candRow = 0;
    int colScale = 0;
    int stepsTaken = 0;

    bool placed = false;
    do {
        const uint8 halfDir = (uint8)(kDirAxisDirection[sd.dir_idx][var14] / 2);

        if (state == 1) {
            const uint8 backwardIso = kHalfDirToIso[(halfDir + 2) % 4];
            const int8 dx = kDirDeltaX[backwardIso];
            const int8 dy = kDirDeltaY[backwardIso];

            const int tableIdx = (var14 > 0 ? 4 : 0) + halfDir;
            baseCol = kBaseX[tableIdx] + (int8)(rowScale * dx);
            baseRow = kBaseY[tableIdx] + (int8)(rowScale * dy);

            candCol = baseCol;
            candRow = baseRow;
            colScale = 1;
            state = 2;
            stepsTaken = 1;
        } else if (state == 2) {
            const uint8 rightIso = kHalfDirToIso[(halfDir + 1) % 4];
            const int8 dx = kDirDeltaX[rightIso];
            const int8 dy = kDirDeltaY[rightIso];

            candCol = baseCol + (int8)(dx * colScale);
            candRow = baseRow + (int8)(dy * colScale);
            state = 3;
            stepsTaken++;
        } else if (state == 3) {
            const uint8 leftIso = kHalfDirToIso[(halfDir + 3) % 4];
            const int8 dx = kDirDeltaX[leftIso];
            const int8 dy = kDirDeltaY[leftIso];

            candCol = baseCol + (int8)(dx * colScale);
            candRow = baseRow + (int8)(dy * colScale);
            state = 2;
            colScale++;
            stepsTaken++;
        }

        const bool colOk = (candCol >= 0 && candCol <= 10);
        const bool rowOk = (candRow >= 0 && candRow <= 5);
        const bool softOob = !(colOk && rowOk);
        const bool hardOob = !colOk && !rowOk;

        if (state > 1) {
            bool needAdvance = false;

            if (softOob && !hardOob)
                needAdvance = true;
            if (firstRow && stepsTaken >= _halfCount[_currentSide])
                needAdvance = true;
            if (!firstRow && stepsTaken > 11)
                needAdvance = true;

            if (needAdvance) {
                rowScale++;

                if (_currentSide == CombatantTable::SIDE_PARTY &&
                    (sd.dir_idx & 1) == 1 && var14 == 0 && rowScale == 1) {
                    bool anyPassable = false;
                    const int checkX = _mapCenterX + sd.origin_x;
                    const int checkY = _mapCenterY + sd.origin_y;

                    for (int d = 1; d <= 3; d++) {
                        const uint8 testDir = kDirFormFallback[sd.dir_idx][d];
                        if (testDir >= 8)
                            continue;
                        if (!_isDungeon || _map->checkOpenPassage(checkX, checkY, testDir) != 1)
                            anyPassable = true;
                    }

                    if (anyPassable)
                        rowScale++;
                }

                state = 1;
                firstRow = false;
            }
        }

        if (hardOob) {
            state = 0;

            while (var14 < 3 && state != 1) {
                var14++;

                const uint8 testDir = kDirFormFallback[sd.dir_idx][var14];
                if (testDir >= 8)
                    continue;

                const int checkX = _mapCenterX + sd.origin_x;
                const int checkY = _mapCenterY + sd.origin_y;
                if (!_isDungeon || _map->checkOpenPassage(checkX, checkY, testDir) != 1) {
                    teamX = sd.origin_x + kDirDeltaX[testDir];
                    teamY = sd.origin_y + kDirDeltaY[testDir];
                    rowScale = 0;
                    state = 1;
                }
            }

            if (state != 1)
                return false;

            continue;
        }

        if (!softOob) {
            placed = tryPlaceAt(charIdx, candCol, candRow, teamX, teamY, var14);
            if (placed)
                return true;
        }
    } while (!placed && !totalFail);

    return false;
}

bool CombatPlacement::tryPlaceAt(int charIdx, int formCol, int formRow,
                                 int8 originCol, int8 originRow, int slot) {
    if (formCol < 0 || formCol > 10 || formRow < 0 || formRow > 5)
        return false;

    // Formation mask check
    if (_sides[_currentSide].valid_mask[slot][formRow][formCol] == 0)
        return false;

    // Compute absolute tile position (spec formula)
    int tileCol = formCol + (originCol * 6) + (originRow * 5) + 22;
    int tileRow = formRow + (originRow * 5) + 10;

    // Map bounds check
    if (tileCol < 0 || tileCol >= BattlefieldMap::kPlayfieldCols)
        return false;
    if (tileRow < 0 || tileRow >= BattlefieldMap::kPlayfieldRows)
        return false;

    // Prevent duplicate base-tile placement before the temporary write. This
    // avoids silent overwrites in the dump while still leaving the broader
    // footprint/terrain validation to the original getGroundInfo path.
    _table->ensureOccupancy();
    if (_table->getOccupant(tileCol, tileRow) != 0)
        return false;

    _table->setPosition(charIdx, (uint8)tileCol, (uint8)tileRow);

    uint8 occupant = 0;
    uint8 groundTile = 0;
    getGroundInfo(charIdx, 8, *_map, *_table, groundTile, occupant);

    if (occupant != 0)
        return false;

    if (groundTile == 0 || (_map->getTilePropertyProvider() &&
            _map->getTilePropertyProvider()->isImpassable(groundTile))) {
        _sides[_currentSide].valid_mask[slot][formRow][formCol] = 0;
        return false;
    }

    // Commit: mark formation cell as used by this combatant
    _sides[_currentSide].valid_mask[slot][formRow][formCol] = 0;
    return true;
}

bool CombatPlacement::placeCombatantFullScan(int charIdx) {
    // Scan the entire playfield for any passable unoccupied tile, starting
    // from the centroid of already-placed same-side combatants and expanding
    // outward. This keeps overflow enemies near their side's existing cluster.
    //
    // Check occupancy BEFORE setPosition so the current combatant is not
    // yet registered at the candidate tile when we query getOccupant().
    const TilePropertyProvider *tileProps = _map->getTilePropertyProvider();
    _table->ensureOccupancy();

    // Compute centroid of already-placed same-side combatants.
    int sumCol = 0, sumRow = 0, count = 0;
    for (int i = _sideStart[_currentSide]; i < charIdx; i++) {
        if (_table->getSize(i) == 0)
            continue;
        sumCol += _table->getTileCol(i);
        sumRow += _table->getTileRow(i);
        count++;
    }

    int centerCol, centerRow;
    if (count > 0) {
        centerCol = sumCol / count;
        centerRow = sumRow / count;
    } else {
        // No same-side combatants placed yet — fall back to origin formula.
        const CombatSideData &sd = _sides[_currentSide];
        centerCol = (sd.origin_x * 6) + (sd.origin_y * 5) + 22;
        centerRow = (sd.origin_y * 5) + 10;
        // Clamp to playfield
        if (centerCol < 0) centerCol = 0;
        if (centerCol >= BattlefieldMap::kPlayfieldCols) centerCol = BattlefieldMap::kPlayfieldCols - 1;
        if (centerRow < 0) centerRow = 0;
        if (centerRow >= BattlefieldMap::kPlayfieldRows) centerRow = BattlefieldMap::kPlayfieldRows - 1;
    }
    const int maxRadius = BattlefieldMap::kPlayfieldCols + BattlefieldMap::kPlayfieldRows;

    for (int radius = 0; radius <= maxRadius; radius++) {
        int rowMin = centerRow - radius;
        int rowMax = centerRow + radius;
        int colMin = centerCol - radius;
        int colMax = centerCol + radius;

        // Clamp to playfield bounds
        if (rowMin < 0) rowMin = 0;
        if (rowMax >= BattlefieldMap::kPlayfieldRows) rowMax = BattlefieldMap::kPlayfieldRows - 1;
        if (colMin < 0) colMin = 0;
        if (colMax >= BattlefieldMap::kPlayfieldCols) colMax = BattlefieldMap::kPlayfieldCols - 1;

        // Scan only the perimeter ring at this radius
        for (int row = rowMin; row <= rowMax; row++) {
            for (int col = colMin; col <= colMax; col++) {
                // Skip interior cells (already checked at smaller radius)
                if (radius > 0 &&
                    row > rowMin && row < rowMax &&
                    col > colMin && col < colMax)
                    continue;

                // Skip tiles closer to the opposing side's origin than ours.
                // Use Manhattan distance to keep overflow on the correct side.
                const int oppSide = 1 - _currentSide;
                const CombatSideData &oppSd = _sides[oppSide];
                const int oppCol = (oppSd.origin_x * 6) + (oppSd.origin_y * 5) + 22;
                const int oppRow = (oppSd.origin_y * 5) + 10;
                int distToUs   = ABS(col - centerCol) + ABS(row - centerRow);
                int distToThem = ABS(col - oppCol)    + ABS(row - oppRow);
                if (distToUs > distToThem)
                    continue;

                uint8 raw = _map->getRawTile(col, row);
                if (raw == 0)
                    continue;
                if (tileProps && tileProps->isImpassable(raw))
                    continue;
                if (_table->getOccupant(col, row) != 0)
                    continue;

                _table->setPosition(charIdx, (uint8)col, (uint8)row);
                return true;
            }
        }
    }
    return false;
}

bool CombatPlacement::isOutOfFormation(int col, int row) {
    // A position is "out of formation" if it doesn't fall within
    // any valid formation cell for the current side's active slot.
    // This is used to detect when the spiral has completely left
    // the formation area (triggering fallback to next form_set).
    if (col < 0 || col >= FORMATION_COLS || row < 0 || row >= FORMATION_ROWS)
        return true;

    const CombatSideData &sd = _sides[_currentSide];
    for (int slot = 0; slot < FORMATION_SLOTS; slot++) {
        if (sd.valid_mask[slot][row][col])
            return false;
    }
    return true;
}

} // namespace Combat
} // namespace Goldbox
