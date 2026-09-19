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

#include "goldbox/combat/combat_ground_info.h"
#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/core/direction.h"
#include "common/debug.h"

namespace Goldbox {
namespace Combat {

struct FootprintOffsetPair {
    int8 col;
    int8 row;
};

static const FootprintOffsetPair kFootprintOffsets[5][4] = {
    // size 0: invalid
    { {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1} },
    // size 1: 1x1
    { { 0,  0}, {-1, -1}, {-1, -1}, {-1, -1} },
    // size 2: 1x2 (tall)
    { { 0,  0}, { 0,  1}, {-1, -1}, {-1, -1} },
    // size 3: 2x1 (wide)
    { { 0,  0}, { 1,  0}, {-1, -1}, {-1, -1} },
    // size 4: 2x2
    { { 0,  0}, { 1,  0}, { 0,  1}, { 1,  1} }
};

bool getIconOffsetBySize(uint8 iconSize, uint8 slot,
                         int8 &outColDelta, int8 &outRowDelta) {
    if (iconSize == 0 || iconSize > 4 || slot > 3)
        return false;
    outColDelta = kFootprintOffsets[iconSize][slot].col;
    outRowDelta = kFootprintOffsets[iconSize][slot].row;
    return outColDelta >= 0;
}

/**
 * Look up tile passable cost from the tile property table.
 * rawTile==0 means no tile; returns 0 (caller treats tileId==0 as OOB/none).
 * index = rawTile - 1 because the property table has no entry for tileId 0.
 */
static uint8 getTilePriority(uint8 rawTile,
                             const TilePropertyProvider *provider) {
    if (rawTile == 0 || !provider)
        return 0;
    const TileProp *prop = provider->getTileProp(rawTile - 1);
    if (!prop)
        return 0;
    return (uint8)prop->passable;
}

void getGroundInfo(Data::PlayerCharacter *ch, uint8 direction,
                   int *outPlayerIndex, uint8 *outTile) {
    assert(g_combatSession);
    const CombatantTable &table = g_combatSession->getTable();
    const BattlefieldMap &map   = g_combatSession->getBattlefieldMap();

    int charIdx = table.findIndex(ch);
    if (charIdx < 0) {
        if (outTile)        *outTile        = kTileIdNone;
        if (outPlayerIndex) *outPlayerIndex = 0;
        return;
    }

    uint8 outOccupant = 0;
    uint8 outTileVal  = kTileIdDefault;
    uint8 bestPriority = 1;

    uint8 baseCol = table.getTileCol(charIdx);
    uint8 baseRow = table.getTileRow(charIdx);
    uint8 footprintCode = table.getSize(charIdx) & 7;
    const TilePropertyProvider *tileProps = map.getTilePropertyProvider();

    int8 dx = (direction < 8) ? kDirDeltaX[direction] : 0;
    int8 dy = (direction < 8) ? kDirDeltaY[direction] : 0;

    for (uint8 slot = 0; slot < 4; slot++) {
        int8 colDelta, rowDelta;
        if (!getIconOffsetBySize(footprintCode, slot, colDelta, rowDelta))
            continue;

        int checkCol = (int)baseCol + colDelta + dx;
        int checkRow = (int)baseRow + rowDelta + dy;

        uint8 foundTile = 0;
        uint8 foundOccupant = 0;

        if (checkCol >= 0 && checkCol < BattlefieldMap::kPlayfieldCols &&
            checkRow >= 0 && checkRow < BattlefieldMap::kPlayfieldRows) {
            foundTile     = map.getRawTile(TilePos((uint8)checkCol, (uint8)checkRow));
            foundOccupant = table.getOccupant(checkCol, checkRow);
        }

        if (foundOccupant == (uint8)(charIdx + 1))
            foundOccupant = 0;
        if (foundOccupant != 0)
            outOccupant = foundOccupant;

        if (foundTile == kTileIdNone || outTileVal == kTileIdNone) {
            outTileVal = kTileIdNone;
        } else if (foundTile == kTileIdHazard || outTileVal == kTileIdHazard) {
            outTileVal = kTileIdHazard;
        } else {
            uint8 priority = getTilePriority(foundTile, tileProps);
            if (bestPriority <= priority) {
                bestPriority = priority;
                outTileVal = foundTile;
            }
        }
    }

    if (outTile)        *outTile        = outTileVal;
    if (outPlayerIndex) *outPlayerIndex = (int)outOccupant;
}

} // namespace Combat
} // namespace Goldbox
