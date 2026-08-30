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
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/core/direction.h"

namespace Goldbox {
namespace Combat {

/**
 * Combat footprint offset table: gbIconOffsetPair[footprint_code][slot]
 *
 * Each entry is {col_offset, row_offset}.
 * Invalid slots use col_offset = -1 as sentinel.
 *
 * Size 0: Invalid (no icon)
 * Size 1: 1x1 — slot 0 only
 * Size 2: 1x2 (tall) — slots 0, 2 (top, bottom)
 * Size 3: 2x1 (wide) — slots 0, 1 (left, right)
 * Size 4: 2x2 — slots 0, 1, 2, 3
 * Size 5+: 3x3 — all 4 slots (approximate)
 */
struct FootprintOffsetPair {
    int8 col;
    int8 row;
};

static const FootprintOffsetPair kFootprintOffsets[5][4] = {
    // size 0: invalid
    { {-1, 0}, {-1, 0}, {-1, 0}, {-1, 0} },
    // size 1: 1x1
    { { 0, 0}, {-1, 0}, {-1, 0}, {-1, 0} },
    // size 2: 1x2 (tall)
    { { 0, 0}, {-1, 0}, { 0, 1}, {-1, 0} },
    // size 3: 2x1 (wide)
    { { 0, 0}, { 1, 0}, {-1, 0}, {-1, 0} },
    // size 4: 2x2
    { { 0, 0}, { 1, 0}, { 0, 1}, { 1, 1} }
};

bool getIconOffsetBySize(uint8 iconSize, uint8 slot,
                         int8 &outColDelta, int8 &outRowDelta) {
    if (iconSize == 0)
        return false;
    if (iconSize > 4)
        iconSize = 4;
    if (slot > 3)
        return false;

    outColDelta = kFootprintOffsets[iconSize][slot].col;
    outRowDelta = kFootprintOffsets[iconSize][slot].row;

    // Sentinel: col < 0 means slot is unused
    if (outColDelta < 0)
        return false;

    return true;
}

/**
 * Look up tile priority from the game-specific tile property table.
 *
 * Original table base includes a dummy entry at index 0 (for raw tile 0 = empty).
 * Our kTable[0] corresponds to raw tile 1. So index = rawTile - 1.
 * Priority is byte 0 (passable field): 0xFF=impassable(highest), 0x01=walkable.
 * Unsigned comparison: if priority >= best_priority, update.
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

void getGroundInfo(int charIdx, uint8 direction,
                   const BattlefieldMap &map,
                   const CombatantTable &table,
                   uint8 &outTile, uint8 &outOccupant) {
    outOccupant = 0;
    outTile = kTileNeutralDefault;
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

        if (checkCol < 0 || checkCol >= BattlefieldMap::kPlayfieldCols ||
            checkRow < 0 || checkRow >= BattlefieldMap::kPlayfieldRows) {
            foundTile = 0;
            foundOccupant = 0;
        } else {
            foundTile = map.getRawTile(TilePos((uint8)checkCol, (uint8)checkRow));
            foundOccupant = table.getOccupant(checkCol, checkRow);
        }

        // Ignore self-reference (1-based occupant index)
        if (foundOccupant == (uint8)(charIdx + 1))
            foundOccupant = 0;

        // Update occupant if found
        if (foundOccupant != 0)
            outOccupant = foundOccupant;

        // Tile priority rules
        if (foundTile == kTileImpassable || outTile == kTileImpassable) {
            // Impassable always propagates
            outTile = kTileImpassable;
        } else if (foundTile == kTileHazard || outTile == kTileHazard) {
            // Hazard always propagates over normal tiles
            outTile = kTileHazard;
        } else {
            uint8 priority = getTilePriority(foundTile, tileProps);
            if (bestPriority <= priority) {
                bestPriority = priority;
                outTile = foundTile;
            }
        }
    }
}

} // namespace Combat
} // namespace Goldbox
