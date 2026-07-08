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
    : _currentSide(0), _isDungeon(false), _mapCenterX(0), _mapCenterY(0),
      _tilemap(nullptr), _table(nullptr) {
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
                               Gfx::BattlefieldTilemap &tilemap,
                               bool isDungeon,
                               bool combatTriggerActive,
                               CombatantTable &table) {
    _tilemap = &tilemap;
    _isDungeon = isDungeon;
    _mapCenterX = tilemap.getCenterX();
    _mapCenterY = tilemap.getCenterY();
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
            const bool notInTeam = ch->combatState && ch->combatState->notInTeam;
            if (!ch->enabled && !combatTriggerActive && !notInTeam) {
                // Disabled team member: hide icon, save tile, stamp downed-member tile
                table.setSize(idx, 0);
                uint8 col = table.getTileCol(idx);
                uint8 row = table.getTileRow(idx);
                uint8 savedTile = _tilemap->getRawTile(col, row);
                _tilemap->setRawTile(col, row, CombatantTable::TILE_DOWNED_MEMBER);
                table.addDownedMember(ch, col, row, savedTile);
            }
            table.rebuildOccupancy();
        } else {
            const bool notInTeam = ch->combatState && ch->combatState->notInTeam;
            if (notInTeam) {
                // Failed not-in-team placement should not consume a combatant slot.
                table.rollbackLastAdd(idx);
                roster[i] = nullptr;
                table.rebuildOccupancy();
            } else {
                table.setSize(idx, 0);
            }
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
    int state = 1;
    int ring = 0;
    int formSet = 0;
    bool isFirstRing = true;
    int8 originCol = _originX[_currentSide];
    int8 originRow = _originY[_currentSide];
    uint8 dirIdx = _teamDir[_currentSide];

    int8 anchorCol = 0, anchorRow = 0;
    int8 candCol = 0, candRow = 0;
    int stepsTaken = 0;
    int armLen = 0;

    for (int iterations = 0; iterations < 2000; iterations++) {
        // Compute dir_index from primary direction table
        uint8 dirIndex = (kDirPrimary[dirIdx][formSet] >> 1) & 3;

        switch (state) {
        case 1: {
            // State 1: Initialize ring anchor
            // Direction = C_BF_DIRECTION_TABLE[(dir_base + 2) % 4]
            uint8 advDir = kHalfDirToIso[(dirIndex + 2) % 4];
            int8 dx = kDirDeltaX[advDir];
            int8 dy = kDirDeltaY[advDir];

            int baseIdx = (formSet != 0) ? 1 : 0;
            anchorCol = kBaseX[baseIdx * 4 + dirIndex] + (int8)(dx * ring);
            anchorRow = kBaseY[baseIdx * 4 + dirIndex] + (int8)(dy * ring);

            candCol = anchorCol;
            candRow = anchorRow;
            stepsTaken = 1;
            armLen = 1;
            state = 2;
            break;
        }
        case 2: {
            // State 2: Extend positive arm from anchor
            // Direction = C_BF_DIRECTION_TABLE[(dir_base + 1) % 4]
            uint8 axisADir = kHalfDirToIso[(dirIndex + 1) % 4];
            int8 dx = kDirDeltaX[axisADir];
            int8 dy = kDirDeltaY[axisADir];

            candCol = anchorCol + (int8)(dx * armLen);
            candRow = anchorRow + (int8)(dy * armLen);
            stepsTaken++;
            state = 3;
            break;
        }
        case 3: {
            // State 3: Extend negative arm (opposite side)
            // Direction = C_BF_DIRECTION_TABLE[(dir_base + 3) % 4]
            uint8 axisBDir = kHalfDirToIso[(dirIndex + 3) % 4];
            int8 dx = kDirDeltaX[axisBDir];
            int8 dy = kDirDeltaY[axisBDir];

            candCol = anchorCol + (int8)(dx * armLen);
            candRow = anchorRow + (int8)(dy * armLen);
            armLen++;
            stepsTaken++;
            state = 2;
            break;
        }
        default:
            break;
        }

        // Bounds check
        bool outOfBounds = (candCol < 0 || candRow < 0 ||
                            candCol > 10 || candRow > 5);

        // Ring advance check
        if (state > 1) {
            bool needAdvance = false;

            if (outOfBounds && !isOutOfFormation(candCol, candRow))
                needAdvance = true;
            if (isFirstRing && stepsTaken >= _halfCount[_currentSide])
                needAdvance = true;
            if (!isFirstRing && stepsTaken > 11)
                needAdvance = true;

            if (needAdvance) {
                ring++;

                // Diagonal terrain check: if party facing diagonal,
                // primary form_set, just entered ring 1
                if (_currentSide == CombatantTable::SIDE_PARTY &&
                    (dirIdx % 2 != 0) && formSet == 0 && ring == 1) {
                    bool anyPassable = false;
                    for (int d = 1; d < 4; d++) {
                        uint8 checkDir = kDirFallback[dirIdx][d];
                        if (checkDir >= 8)
                            continue;
                        if (!_isDungeon ||
                            _tilemap->checkOpenPassage(
                                _mapCenterX, _mapCenterY, checkDir) != 1) {
                            anyPassable = true;
                        }
                    }
                    if (anyPassable)
                        ring++;
                }

                state = 1;
                isFirstRing = false;
            }
        }

        // Formation exhaustion / fallback
        if (outOfBounds && isOutOfFormation(candCol, candRow)) {
            state = 0;

            while (formSet < 3 && state != 1) {
                formSet++;
                uint8 testDir = kDirFallback[dirIdx][formSet];
                if (testDir >= 8)
                    continue;

                // In wilderness (mapType > 1), always passable.
                // In dungeon, check bidirectional passability != 1.
                if (!_isDungeon ||
                    _tilemap->checkOpenPassage(
                        _mapCenterX, _mapCenterY, testDir) != 1) {
                    originCol = _originX[_currentSide] + kDirDeltaX[testDir];
                    originRow = _originY[_currentSide] + kDirDeltaY[testDir];
                    ring = 0;
                    state = 1;
                }
            }

            if (state != 1)
                return false;
            continue;
        }

        // Attempt placement at valid in-bounds position
        if (!outOfBounds) {
            if (tryPlaceAt(charIdx, candCol, candRow,
                           originCol, originRow, formSet))
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

    // Scan destination using shared ground info utility
    uint8 occupant = 0;
    uint8 groundTile = 0;
    getGroundInfo(charIdx, 8, *_tilemap, *_table, groundTile, occupant);

    if (occupant != 0)
        return false;
    if (groundTile == 0)
        return false;

    // Check passability via tile property provider
    const Combat::TilePropertyProvider *tileProps = _tilemap->getTilePropertyProvider();
    if (tileProps && tileProps->isImpassable(groundTile))
        return false;

    // Commit: mark formation cell as used
    _formationValid[_currentSide][slot][formRow][formCol] = 0;
    return true;
}

bool CombatPlacement::isOutOfFormation(int col, int row) {
    // A position is "out of formation" if it doesn't fall within
    // any valid formation cell for the current side's active slot.
    // This is used to detect when the spiral has completely left
    // the formation area (triggering fallback to next form_set).
    if (col < 0 || col >= FORMATION_COLS || row < 0 || row >= FORMATION_ROWS)
        return true;

    // Check all slots — if valid in any slot, it's still in formation
    for (int slot = 0; slot < FORMATION_SLOTS; slot++) {
        if (_formationValid[_currentSide][slot][row][col])
            return false;
    }
    return true;
}

} // namespace Combat
} // namespace Goldbox
