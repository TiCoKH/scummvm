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

#include "goldbox/combat/combat_targeting.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_ground_info.h"
#include "goldbox/data/player_character.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

/**
 * Chebyshev distance between two tile positions.
 * Mirrors the original range check: max(|dx|, |dy|).
 */
static uint8 chebyshevDist(TilePos a, TilePos b) {
    int dx = ABS((int)a.col - (int)b.col);
    int dy = ABS((int)a.row - (int)b.row);
    return (uint8)(dx > dy ? dx : dy);
}

void buildTargetList(const Data::PlayerCharacter *attacker,
                     uint8 maxRange,
                     const CombatantTable &table,
                     TargetList &result) {
    result.targetOrder.clear();

    if (!attacker)
        return;

    const int attackerIdx = table.findIndex(attacker);
    if (attackerIdx < 0)
        return;

    const uint8 attackerCol = table.getTileCol(attackerIdx);
    const uint8 attackerRow = table.getTileRow(attackerIdx);
    const TilePos attackerPos(attackerCol, attackerRow);
    const Data::CombatSide attackerSide = attacker->combatSide;

    // Step 1+2: scan all combatants, collect those in range on the opposite side.
    // Mirrors COMBAT_BuildTargetListCore + side filter in one pass.
    Common::Array<CombatTarget> candidates;

    for (int i = 0; i < table.getCount(); i++) {
        Data::PlayerCharacter *ch = table.getCharacter(i);
        if (!ch || !ch->enabled || table.getSize(i) == 0)
            continue;
        if (i == attackerIdx)
            continue;

        // Must be on the opposite side (attacker targets enemies).
        if (ch->combatSide == attackerSide)
            continue;

        TilePos targetPos(table.getTileCol(i), table.getTileRow(i));
        uint8 dist = chebyshevDist(attackerPos, targetPos);
        if (dist > maxRange)
            continue;

        CombatTarget t;
        t.placementIndex = (uint8)(i + 1); // 1-based, mirrors original
        t.pos = targetPos;
        t.distance = dist;
        candidates.push_back(t);
    }

    // Step 3: sort candidates by distance (insertion sort — list is small),
    // then write 0-based indices into targetOrder.
    for (uint i = 1; i < candidates.size(); i++) {
        CombatTarget key = candidates[i];
        int j = (int)i - 1;
        while (j >= 0 && candidates[j].distance > key.distance) {
            candidates[j + 1] = candidates[j];
            j--;
        }
        candidates[j + 1] = key;
    }

    for (uint i = 0; i < candidates.size(); i++)
        result.targetOrder.push_back((uint8)(candidates[i].placementIndex - 1));
}

} // namespace Combat
} // namespace Goldbox
