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

#ifndef GOLDBOX_COMBAT_COMBAT_TARGETING_H
#define GOLDBOX_COMBAT_COMBAT_TARGETING_H

#include "common/scummsys.h"
#include "common/array.h"
#include "goldbox/core/tile_pos.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

class CombatantTable;
class CombatViewport;
class BattlefieldMap;

/**
 * A single entry in the raw target candidate list.
 *
 * Mirrors original gbCombatTarget:
 *   placementIndex  — 1-based index into the combatant table
 *   pos             — tile position of the target
 *   distance        — Chebyshev distance from attacker
 */
struct CombatTarget {
    uint8 placementIndex; // 1-based combatant table index
    TilePos pos;
    uint8 distance;

    CombatTarget() : placementIndex(0), pos(), distance(0) {}
};

/**
 * Result of buildTargetList: filtered, ordered target indices.
 *
 * targetOrder[i] is a 0-based combatant table index.
 * Mirrors original ARRAY_TARGET_ORDER[] (1-based in original; 0-based here).
 */
struct TargetList {
    Common::Array<uint8> targetOrder; // 0-based combatant table indices
};

/**
 * Build the list of valid targets for an attacker.
 *
 * Mirrors COMBAT_BuildTargetList:
 *   1. Enumerate all combatants within maxRange of the attacker's footprint
 *      (Chebyshev distance, full 360° arc).
 *   2. Filter to only those on the opposite combat side.
 *   3. Write ordered 0-based indices into result.targetOrder.
 *
 * @param attacker   The attacking character
 * @param maxRange   Maximum Chebyshev range (tiles)
 * @param table      Combatant table (positions + occupancy)
 * @param result     Output: ordered list of valid target indices
 */
void buildTargetList(const Data::PlayerCharacter *attacker,
                     uint8 maxRange,
                     const CombatantTable &table,
                     TargetList &result);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_TARGETING_H
