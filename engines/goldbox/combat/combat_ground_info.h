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

namespace Combat {

class CombatantTable;
class BattlefieldMap;

/**
 * Special tile ID values returned by getGroundInfo.
 *
 * kTileIdNone (0)     = no tile: OOB or edge of map — caller offers Flee
 * kTileIdHazard (0x1E) = hazard terrain (water, fire, etc.)
 * kTileIdDefault (0x17) = neutral walkable fallback
 *
 * Note: tile property passable==0 is a separate concept (impassable terrain).
 * The caller checks tileId==0 first (flee), then move < tileProps[tileId].passable (blocked).
 */
static const uint8 kTileIdNone    = 0x00;
static const uint8 kTileIdHazard  = 0x1E;
static const uint8 kTileIdDefault = 0x17;

/**
 * Query ground tile and occupant in a given direction from a combatant.
 *
 * Scans all tiles occupied by the character's icon footprint one step
 * in the specified direction. Reports the "best" tile using priority
 * rules and any occupant found at the destination.
 *
 * Caller logic (from original):
 *   occupant != 0                    → destination occupied
 *   occupant == 0, tileId == 0       → OOB: offer Flee dialog
 *   occupant == 0, tileId != 0
 *     move < tileProps[id].passable  → "Blocked" message
 *     else                           → advance/move
 *
 * Used by: placement, AI movement, player step validation, etc.
 *
 * @param charIdx     Combatant index in the CombatantTable
 * @param direction   Direction index (0-7), or 8 for stationary check
 * @param map         Battlefield map for raw tile queries
 * @param table       Combatant table for position/occupancy queries
 * @param outTile     Result: 0=impassable/OOB, 0x1E=hazard, other=terrain
 * @param outOccupant Result: occupant index if found, else 0
 */
void getGroundInfo(int charIdx, uint8 direction,
                   const BattlefieldMap &map,
                   const CombatantTable &table,
                   uint8 &outTile, uint8 &outOccupant);

/**
 * Get icon footprint offset for a given slot.
 *
 * Maps combat footprint code and slot index (0-3) to col/row deltas within
 * the character's multi-tile footprint.

 * IMPORTANT: This footprint code comes from combat icon data and is used only
 * for tactical placement/occupancy. It is distinct from gfx::IconSize
 * (ICON_SIZE_SMALL / ICON_SIZE_LARGE), which selects sprite ranges for
 * rendering.
 *
 * Size classes:
 *   0 = Invalid (no icon)
 *   1 = 1x1  — slot 0
 *   2 = 1x2 tall — slots 0, 2 (col 0 row 0; col 0 row 1)
 *   3 = 2x1 wide — slots 0, 1 (col 0 row 0; col 1 row 0)
 *   4 = 2x2 — slots 0, 1, 2, 3
 *
 * Slots use {-1, -1} as sentinel for unused entries.
 *
 * @param iconSize  Combat footprint code (from combatant data, masked to 0-7)
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
