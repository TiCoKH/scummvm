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

#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_viewport.h"
#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/core/direction.h"
#include "goldbox/data/player_character.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

CombatContext::CombatContext(CombatGlobals &globals_,
                             CombatParams &params_,
                             CombatantTable &table_,
                             BattlefieldMap &map_,
                             CombatViewport &viewport_)
    : globals(globals_), params(params_), table(table_),
      map(map_), viewport(viewport_), clouds(globals_.clouds) {}

void CombatContext::updateSideCount() {
    globals.updateSideCount(params.roster);
}

void CombatContext::rebuildPlacementMap() {
    table.rebuildOccupancy();
}

void CombatContext::rebuildDistances() {
    // Viewport-relative positions are now computed on demand via
    // CombatViewport::getCharacterViewportPosition() — no cache to rebuild.
}

static const FootprintOffsetPair kFootprintOffsets[5][4] = {
    { {-1,-1}, {-1,-1}, {-1,-1}, {-1,-1} }, // invalid
    { { 0, 0}, {-1,-1}, {-1,-1}, {-1,-1} }, // 1x1
    { { 0, 0}, { 0, 1}, {-1,-1}, {-1,-1} }, // 1x2
    { { 0, 0}, { 1, 0}, {-1,-1}, {-1,-1} }, // 2x1
    { { 0, 0}, { 1, 0}, { 0, 1}, { 1, 1} }  // 2x2
};

bool CombatContext::getFootprintOffset(uint8 iconSize, uint8 slot, FootprintOffsetPair &off) {
    if (iconSize == 0 || iconSize > 4 || slot > 3)
        return false;
    off = kFootprintOffsets[iconSize][slot];
    return off.col >= 0;
}

void CombatContext::getGroundInfo(Data::PlayerCharacter *ch, uint8 direction,
                                  int *outPlayerIndex, uint8 *outTile) const {
    static const uint8 kTileIdNone    = 0x00;
    static const uint8 kTileIdHazard  = 0x1E;
    static const uint8 kTileIdDefault = 0x17;

    int charIdx = table.findIndex(ch);
    if (charIdx < 0) {
        if (outTile)        *outTile        = kTileIdNone;
        if (outPlayerIndex) *outPlayerIndex = 0;
        return;
    }

    uint8 outOccupant  = 0;
    uint8 outTileVal   = kTileIdDefault;
    uint8 bestPriority = 1;

    const TilePos basePos      = table.getTilePos(charIdx);
    const uint8 footprintCode = table.getSize(charIdx) & 7;
    const TilePropertyProvider *tileProps = map.getTilePropertyProvider();

    const int8 dx = (direction < 8) ? kDirDeltaX[direction] : 0;
    const int8 dy = (direction < 8) ? kDirDeltaY[direction] : 0;

    for (uint8 slot = 0; slot < 4; slot++) {
        FootprintOffsetPair off;
        if (!getFootprintOffset(footprintCode, slot, off))
            continue;

        const int checkCol = (int)basePos.col + off.col + dx;
        const int checkRow = (int)basePos.row + off.row + dy;

        uint8 foundTile     = 0;
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
        } else if (tileProps) {
            const TileProp *prop = tileProps->getTileProp(foundTile - 1);
            uint8 priority = prop ? (uint8)prop->passable : 0;
            if (bestPriority <= priority) {
                bestPriority = priority;
                outTileVal   = foundTile;
            }
        }
    }

    if (outTile)        *outTile        = outTileVal;
    if (outPlayerIndex) *outPlayerIndex = (int)outOccupant;
}

void CombatContext::buildTargetList(const Data::PlayerCharacter *attacker,
                                    uint8 maxRange, TargetList &result) const {
    result.targetOrder.clear();
    if (!attacker)
        return;

    const int attackerIdx = table.findIndex(attacker);
    if (attackerIdx < 0)
        return;

    const TilePos attackerPos = table.getTilePos(attackerIdx);
    const Data::CombatSide attackerSide = attacker->combatSide;

    struct Candidate { uint8 idx; uint8 dist; };
    Common::Array<Candidate> candidates;

    for (int i = 0; i < table.getCount(); i++) {
        Data::PlayerCharacter *ch = table.getCharacter(i);
        if (!ch || !ch->enabled || table.getSize(i) == 0 || i == attackerIdx)
            continue;
        if (ch->combatSide == attackerSide)
            continue;

        const TilePos pos = table.getTilePos(i);
        const int dx = ABS((int)pos.col - (int)attackerPos.col);
        const int dy = ABS((int)pos.row - (int)attackerPos.row);
        const uint8 dist = (uint8)(dx > dy ? dx : dy);
        if (dist > maxRange)
            continue;

        Candidate c = { (uint8)i, dist };
        candidates.push_back(c);
    }

    for (uint i = 1; i < candidates.size(); i++) {
        Candidate key = candidates[i];
        int j = (int)i - 1;
        while (j >= 0 && candidates[j].dist > key.dist) {
            candidates[j + 1] = candidates[j--];
        }
        candidates[j + 1] = key;
    }

    for (uint i = 0; i < candidates.size(); i++)
        result.targetOrder.push_back(candidates[i].idx);
}

void CombatContext::initCharacterTurnState(Data::PlayerCharacter *ch) {
    ::Goldbox::Combat::initCharacterTurnState(
        ch, params.effectRuntime, &globals,
        params.eclMemory, params.vmGlobalLayout);
}

void CombatContext::initAllTurnStates() {
    for (uint i = 0; i < params.roster.size(); i++)
        initCharacterTurnState(params.roster[i]);
}

Data::PlayerCharacter *CombatContext::selectNextActor() const {
    return ::Goldbox::Combat::selectNextActor(params.roster, globals);
}

void CombatContext::removeMember(Data::PlayerCharacter *ch, bool keepPartyCount, bool freeIconSlot) {
    if (!ch)
        return;

    if (freeIconSlot)
        ch->iconData.iconSlotId = 0;

    // Remove from roster
    for (uint i = 0; i < params.roster.size(); i++) {
        if (params.roster[i] == ch) {
            params.roster.remove_at(i);
            break;
        }
    }

    if (!keepPartyCount && params.partyCount > 0)
        params.partyCount--;

    updateSideCount();
}

bool CombatContext::isTargetInArc(Direction direction,
                                   TilePos attackerPos, TilePos targetPos) {
    if (attackerPos.col > 49 || targetPos.col > 49 ||
        attackerPos.row > 24 || targetPos.row > 24)
        return false;

    if (direction == DIR_NONE)
        return true;

    // Cell immediately in front of the target is always in-arc.
    const uint8 adjCol = (uint8)(targetPos.col + kDirDeltaX[direction]);
    const uint8 adjRow = (uint8)(targetPos.row + kDirDeltaY[direction]);
    if ((attackerPos.col == targetPos.col  && attackerPos.row == targetPos.row) ||
        (attackerPos.col == adjCol         && attackerPos.row == adjRow))
        return true;

    const int dx = (int)attackerPos.col - (int)targetPos.col; // +east
    const int dy = (int)attackerPos.row - (int)targetPos.row; // +south

    switch (direction) {
    case DIR_N:  return dy < 0 && ABS(dx) <= -dy;
    case DIR_NE: return dx > 0 && dy < 0 && dx >= -dy;
    case DIR_E:  return dx > 0 && ABS(dy) <= dx;
    case DIR_SE: return dx > 0 && dy > 0 && dy >= dx;
    case DIR_S:  return dy > 0 && ABS(dx) <= dy;
    case DIR_SW: return dx < 0 && dy > 0 && dy >= -dx;
    case DIR_W:  return dx < 0 && ABS(dy) <= -dx;
    case DIR_NW: return dx < 0 && dy < 0 && (-dy) >= (-dx);
    default:     return false;
    }
}

} // namespace Combat
} // namespace Goldbox
