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
#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/data/player_character.h"
#include "goldbox/core/direction.h"
#include <string.h>

namespace Goldbox {
namespace Combat {

// HalfDirToIso[4] — converts half-direction to 8-way isometric facing
const uint8 CombatPlacement::kHalfDirToIso[4] = { 7, 2, 3, 6 };

// DIR_PRIMARY[4][4] — primary placement directions (full 8-way values)
const uint8 CombatPlacement::kDirPrimary[4][4] = {
    { 0, 0, 2, 6 },   // party facing N
    { 2, 2, 0, 4 },   // party facing E
    { 4, 4, 2, 6 },   // party facing S
    { 6, 6, 4, 0 },   // party facing W
};

// DIR_FALLBACK[4][4] — fallback approach directions
const uint8 CombatPlacement::kDirFallback[4][4] = {
    { 8, 4, 6, 2 },   // party facing N
    { 8, 6, 4, 0 },   // party facing E
    { 8, 0, 6, 2 },   // party facing S
    { 8, 2, 0, 4 },   // party facing W
};

// BASE_X[8] and BASE_Y[8] — spiral base offsets
// First 4: primary direction offsets, Last 4: fallback direction offsets
const int8 CombatPlacement::kBaseX[8] = { 5, 4, 5, 6, 3, 8, 7, 2 };
const int8 CombatPlacement::kBaseY[8] = { 3, 2, 2, 3, 0, 2, 5, 3 };

// FORMATION_RANGE[5][6][2] — valid column bounds per direction and row
// [direction_index][row][0=minCol, 1=maxCol]
// Rows where min > max have no valid cells.
const int8 CombatPlacement::kFormationRange[5][6][2] = {
    // dir 0
    { {1, 0}, {1, 0}, {1, 0}, {2, 9}, {3, 10}, {4, 10} },
    // dir 1
    { {0, 2}, {0, 3}, {1, 4}, {2, 5}, {3, 6}, {4, 7} },
    // dir 2
    { {0, 6}, {0, 7}, {1, 8}, {1, 0}, {1, 0}, {1, 0} },
    // dir 3
    { {3, 6}, {4, 7}, {5, 8}, {6, 9}, {7, 10}, {8, 10} },
    // dir 4
    { {0, 6}, {0, 7}, {1, 8}, {2, 9}, {3, 10}, {4, 10} },
};

CombatPlacement::CombatPlacement()
    : _currentSide(0), _isDungeon(false), _tilemap(nullptr), _table(nullptr) {
    memset(_formationValid, 0, sizeof(_formationValid));
    memset(_originX, 0, sizeof(_originX));
    memset(_originY, 0, sizeof(_originY));
    memset(_teamDir, 0, sizeof(_teamDir));
    memset(_halfCount, 0, sizeof(_halfCount));
}

void CombatPlacement::placeAll(Common::Array<Data::PlayerCharacter *> &roster,
                               int partyCount,
                               uint8 mapDirection,
                               int encounterDist,
                               const Gfx::BattlefieldTilemap &tilemap,
                               bool isDungeon,
                               CombatantTable &table) {
    _tilemap = &tilemap;
    _isDungeon = isDungeon;
    _table = &table;

    table.clear();

    // Compute team origins and directions
    _originX[CombatantTable::SIDE_PARTY] = 0;
    _originY[CombatantTable::SIDE_PARTY] = 0;
    _teamDir[CombatantTable::SIDE_PARTY] = (mapDirection / 2) & 3;

    _originX[CombatantTable::SIDE_ENEMY] = (int8)(encounterDist * kDirDeltaX[mapDirection]);
    _originY[CombatantTable::SIDE_ENEMY] = (int8)(encounterDist * kDirDeltaY[mapDirection]);
    _teamDir[CombatantTable::SIDE_ENEMY] = (((mapDirection + 4) % 8) / 2) & 3;

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

    // Build formation validity masks
    buildFormationMasks();

    // Place each combatant
    for (int i = 0; i < (int)roster.size(); i++) {
        Data::PlayerCharacter *ch = roster[i];
        if (!ch)
            continue;

        _currentSide = (i < partyCount) ? CombatantTable::SIDE_PARTY
                                        : CombatantTable::SIDE_ENEMY;

        uint8 size = ch->iconData.iconSize & 7;
        int idx = table.addCombatant(ch, size);
        if (idx < 0)
            break;

        if (placeCombatantSpiral(idx)) {
            if (!ch->enabled) {
                // Inactive combatant: mark as trigger
                table.setSize(idx, 0);
                uint8 col = table.getTileCol(idx);
                uint8 row = table.getTileRow(idx);
                uint8 savedTile = _tilemap->getRawTile(col, row);
                table.addTrigger(ch, col, row, savedTile);
            }
            table.rebuildOccupancy();
        } else {
            table.setSize(idx, 0);
        }
    }

    table.countSides(partyCount);
}

void CombatPlacement::buildFormationMasks() {
    memset(_formationValid, 0, sizeof(_formationValid));

    for (int side = 0; side < CombatantTable::SIDE_COUNT; side++) {
        for (int slot = 0; slot < FORMATION_SLOTS; slot++) {
            int dirIdx;
            if (slot == 1)
                dirIdx = 4;  // slot 1 always uses direction index 4
            else
                dirIdx = _teamDir[side];

            for (int row = 0; row < FORMATION_ROWS; row++) {
                int8 minCol = kFormationRange[dirIdx][row][0];
                int8 maxCol = kFormationRange[dirIdx][row][1];

                for (int col = 0; col < FORMATION_COLS; col++) {
                    if (col >= minCol && col <= maxCol)
                        _formationValid[side][slot][row][col] = 1;
                    else
                        _formationValid[side][slot][row][col] = 0;
                }
            }
        }
    }
}

bool CombatPlacement::placeCombatantSpiral(int charIdx) {
    enum SpiralState { STATE_START = 0, STATE_RIGHT = 1, STATE_LEFT = 2 };

    SpiralState state = STATE_START;
    int rowDepth = 0;
    int dirStep = 0;
    bool firstRow = true;
    int8 originCol = _originX[_currentSide];
    int8 originRow = _originY[_currentSide];

    int8 baseX = 0, baseY = 0;
    int8 curX = 0, curY = 0;
    int lateralScale = 1;
    int lateralCount = 1;

    for (int iterations = 0; iterations < 2000; iterations++) {
        uint8 halfDir = (kDirPrimary[_teamDir[_currentSide]][dirStep] / 2) & 3;

        switch (state) {
        case STATE_START: {
            int baseIdx = (dirStep > 0 ? 4 : 0) + halfDir;
            uint8 backwardDir = kHalfDirToIso[((int)halfDir + 2) % 4];
            baseX = kBaseX[baseIdx] + (int8)(rowDepth * kDirDeltaX[backwardDir]);
            baseY = kBaseY[baseIdx] + (int8)(rowDepth * kDirDeltaY[backwardDir]);
            curX = baseX;
            curY = baseY;
            lateralScale = 1;
            lateralCount = 1;
            state = STATE_RIGHT;
            break;
        }
        case STATE_RIGHT: {
            uint8 rightDir = kHalfDirToIso[((int)halfDir + 1) % 4];
            curX = baseX + (int8)(kDirDeltaX[rightDir] * lateralScale);
            curY = baseY + (int8)(kDirDeltaY[rightDir] * lateralScale);
            lateralCount++;
            state = STATE_LEFT;
            break;
        }
        case STATE_LEFT: {
            uint8 leftDir = kHalfDirToIso[((int)halfDir + 3) % 4];
            curX = baseX + (int8)(kDirDeltaX[leftDir] * lateralScale);
            curY = baseY + (int8)(kDirDeltaY[leftDir] * lateralScale);
            lateralScale++;
            lateralCount++;
            state = STATE_RIGHT;
            break;
        }
        }

        // Bounds check
        bool softOob = (curX < 0 || curY < 0 || curX > 10 || curY > 5);
        bool hardOob = bothAxesOutOfRange(curY, curX);

        // Row advance check
        if (state != STATE_START) {
            bool needAdvance = false;

            if (softOob && !hardOob)
                needAdvance = true;
            if (firstRow && lateralCount >= _halfCount[_currentSide])
                needAdvance = true;
            if (!firstRow && lateralCount > 11)
                needAdvance = true;

            if (needAdvance) {
                rowDepth++;
                state = STATE_START;
                firstRow = false;
            }
        }

        // Hard out-of-bounds: try next approach direction
        if (softOob && hardOob) {
            bool found = false;
            while (dirStep < 3) {
                dirStep++;
                uint8 testDir = kDirFallback[_teamDir[_currentSide]][dirStep];
                if (testDir == 8)
                    continue;

                // In wilderness, all directions are passable
                if (!_isDungeon) {
                    originCol = _originX[_currentSide] + kDirDeltaX[testDir];
                    originRow = _originY[_currentSide] + kDirDeltaY[testDir];
                    rowDepth = 0;
                    state = STATE_START;
                    found = true;
                    break;
                }

                // TODO: dungeon wall_passable check
                originCol = _originX[_currentSide] + kDirDeltaX[testDir];
                originRow = _originY[_currentSide] + kDirDeltaY[testDir];
                rowDepth = 0;
                state = STATE_START;
                found = true;
                break;
            }

            if (!found)
                return false;
        }

        // Attempt placement at valid position
        if (!softOob) {
            if (tryPlaceAt(charIdx, curX, curY, originCol, originRow, dirStep))
                return true;
        }
    }

    return false;
}

bool CombatPlacement::tryPlaceAt(int charIdx, int formCol, int formRow,
                                 int8 originCol, int8 originRow, int slot) {
    if (formCol < 0 || formCol > 10 || formRow < 0 || formRow > 5)
        return false;

    // Formation mask check
    if (_formationValid[_currentSide][slot][formRow][formCol] == 0)
        return false;

    // Compute absolute tile position (spec formula)
    int tileCol = formCol + (originCol * 6) + (originRow * 5) + 22;
    int tileRow = formRow + (originRow * 5) + 10;

    // Map bounds check
    if (tileCol < 0 || tileCol >= Gfx::BattlefieldTilemap::kPlayfieldCols)
        return false;
    if (tileRow < 0 || tileRow >= Gfx::BattlefieldTilemap::kPlayfieldRows)
        return false;

    // Write position temporarily
    _table->setPosition(charIdx, (uint8)tileCol, (uint8)tileRow);

    // Scan destination
    uint8 occupant = 0;
    uint8 groundTile = 0;
    scanDestination(charIdx, 8, occupant, groundTile);

    if (occupant != 0)
        return false;
    if (groundTile == 0)
        return false;

    // Check passability via tile property table
    if (groundTile > 0 && groundTile <= Gfx::BattlefieldTilemap::kTilePropTableCount) {
        if (Gfx::BattlefieldTilemap::kTilePropTable[groundTile - 1].passable == -1)
            return false;
    }

    // Commit: mark formation cell as used
    _formationValid[_currentSide][slot][formRow][formCol] = 0;
    return true;
}

void CombatPlacement::scanDestination(int charIdx, uint8 direction,
                                      uint8 &outOccupant, uint8 &outTile) const {
    outOccupant = 0;
    outTile = 23;  // default passable (grass)

    uint8 baseCol = _table->getTileCol(charIdx);
    uint8 baseRow = _table->getTileRow(charIdx);
    uint8 size = _table->getSize(charIdx) & 7;

    // Direction 8 = stationary (no delta)
    int8 dx = (direction < 8) ? kDirDeltaX[direction] : 0;
    int8 dy = (direction < 8) ? kDirDeltaY[direction] : 0;

    // Size footprint
    int w = (size >= 2 && size != 3) ? 2 : 1;
    int h = (size >= 3) ? 2 : 1;

    for (int fy = 0; fy < h; fy++) {
        for (int fx = 0; fx < w; fx++) {
            int checkCol = (int)baseCol + fx + dx;
            int checkRow = (int)baseRow + fy + dy;

            if (checkCol < 0 || checkCol >= Gfx::BattlefieldTilemap::kPlayfieldCols ||
                checkRow < 0 || checkRow >= Gfx::BattlefieldTilemap::kPlayfieldRows) {
                outTile = 0;
                continue;
            }

            // Check occupant
            uint8 occ = _table->getOccupant(checkCol, checkRow);
            if (occ != 0 && occ != (uint8)(charIdx + 1))
                outOccupant = occ;

            // Check tile
            uint8 raw = _tilemap->getRawTile(checkCol, checkRow);
            if (raw == 0) {
                outTile = 0;
            } else if (outTile != 0) {
                outTile = raw;
            }
        }
    }
}

bool CombatPlacement::bothAxesOutOfRange(int row, int col) {
    bool colInRange = (col >= 0 && col <= 10);
    bool rowInRange = (row >= 0 && row <= 5);
    return !colInRange && !rowInRange;
}

} // namespace Combat
} // namespace Goldbox
