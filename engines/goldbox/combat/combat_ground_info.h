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

#ifndef GOLDBOX_COMBAT_COMBAT_GROUND_INFO_H
#define GOLDBOX_COMBAT_COMBAT_GROUND_INFO_H

#include "common/scummsys.h"

namespace Goldbox {

namespace Gfx {
class BattlefieldTilemap;
}

namespace Combat {

class CombatantTable;

/**
 * Special tile values returned by getGroundInfo.
 */
static const uint8 kTileImpassable = 0x00;
static const uint8 kTileHazard = 0x1E;
static const uint8 kTileNeutralDefault = 0x17;

/**
 * Query ground tile and occupant in a given direction from a combatant.
 *
 * Scans all tiles occupied by the character's icon footprint one step
 * in the specified direction. Reports the "best" tile using priority
 * rules and any occupant found at the destination.
 *
 * Tile priority system:
 *   0x00 = Impassable/invalid (highest priority, always propagates)
 *   0x1E = Special hazard/water (always propagates over normal tiles)
 *   1-0x1D = Walkable terrain (selected by priority table value)
 *
 * Used by: placement, AI movement, player step validation, etc.
 *
 * @param charIdx     Combatant index in the CombatantTable
 * @param direction   Direction index (0-7), or 8 for stationary check
 * @param tilemap     Battlefield tilemap for raw tile queries
 * @param table       Combatant table for position/occupancy queries
 * @param outTile     Result: best tile found (0=blocked, 0x1E=hazard)
 * @param outOccupant Result: occupant index if found, else 0
 */
void getGroundInfo(int charIdx, uint8 direction,
                   const Gfx::BattlefieldTilemap &tilemap,
                   const CombatantTable &table,
                   uint8 &outTile, uint8 &outOccupant);

/**
 * Get icon footprint offset for a given slot.
 *
 * Maps icon_size and slot index (0-3) to col/row deltas within
 * the character's multi-tile footprint.
 *
 * Size classes:
 *   0 = Invalid (no icon)
 *   1 = 1x1 (slot 0 only)
 *   2 = 1x2 tall (slots 0, 2)
 *   3 = 2x1 wide (slots 0, 1)
 *   4 = 2x2 (slots 0, 1, 2, 3)
 *   5+ = 3x3 (all 4 corner slots)
 *
 * Invalid slots are marked with col_offset = -1 in the lookup table.
 *
 * @param iconSize  Icon size value (from combatant data, masked to 0-7)
 * @param slot      Slot index (0-3)
 * @param outColDelta  Column offset for this slot
 * @param outRowDelta  Row offset for this slot
 * @return true if slot is valid for this icon size
 */
bool getIconOffsetBySize(uint8 iconSize, uint8 slot,
                         int8 &outColDelta, int8 &outRowDelta);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_GROUND_INFO_H
