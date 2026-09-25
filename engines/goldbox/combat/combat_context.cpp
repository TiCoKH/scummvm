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
#include "goldbox/combat/combat_damage.h"
#include "goldbox/core/direction.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

CombatContext::CombatContext(CombatGlobals &globals_,
                             CombatParams &params_,
                             CombatantTable &table_,
                             BattlefieldMap &map_,
                             CombatViewport &viewport_,
                             TargetList &targetList_)
    : globals(globals_), params(params_), table(table_),
      map(map_), viewport(viewport_), clouds(globals_.clouds),
      targetList(targetList_) {}

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

CombatContext::CombatCell CombatContext::getTileAndOccupantAt(TilePos pos) const {
    CombatCell cell{};
    if (!Goldbox::isValidTilePos(pos))
        return cell;
    cell.tileId     = map.getRawTile(pos);
    cell.occupantId = table.getOccupant(pos);
    return cell;
}

void CombatContext::buildTargetListCore(TilePos pos, uint8 iconSize,
                                        Direction facing, uint8 maxRange) {
    TargetList &result = targetList;
    result.clear();

    TilePos sourceTiles[4];
    for (uint8 slot = 0; slot < 4; ++slot) {
        FootprintOffsetPair off;
        if (getFootprintOffset(iconSize, slot, off)) {
            sourceTiles[slot] = TilePos(
                (uint8)(pos.col + off.col),
                (uint8)(pos.row + off.row));
        } else {
            sourceTiles[slot] = TilePos(0xFF, 0xFF);
        }
    }

    for (int i = 0; i < table.getCount(); i++) {
        Data::PlayerCharacter *ch = table.getCharacter(i);
        if (!ch || !ch->enabled || table.getSize(i) == 0)
            continue;

        const TilePos candidatePos = table.getTilePos(i);
        const uint8 candidateSize = table.getSize(i) & 7;

        TilePos candidateTiles[4];
        for (uint8 slot = 0; slot < 4; ++slot) {
            FootprintOffsetPair off;
            if (getFootprintOffset(candidateSize, slot, off)) {
                candidateTiles[slot] = TilePos(
                    (uint8)(candidatePos.col + off.col),
                    (uint8)(candidatePos.row + off.row));
            } else {
                candidateTiles[slot] = TilePos(0xFF, 0xFF);
            }
        }

        bool foundTarget = false;
        uint16 nearestRange = 0xFF;
        uint8 bestAttackerSlot = 0;
        uint8 bestTargetSlot = 0;

        for (uint8 cSlot = 0; cSlot < 4; ++cSlot) {
            if (candidateTiles[cSlot].col == 0xFF)
                continue;
            for (uint8 sSlot = 0; sSlot < 4; ++sSlot) {
                if (sourceTiles[sSlot].col == 0xFF)
                    continue;
                if (!isTargetInArc(facing, sourceTiles[sSlot], candidateTiles[cSlot]))
                    continue;

                uint16 range = maxRange;
                if (!lineOfSightCheck(sourceTiles[sSlot], candidateTiles[cSlot], range))
                    continue;

                foundTarget = true;
                if (range < nearestRange) {
                    nearestRange = range;
                    bestTargetSlot = sSlot;
                    bestAttackerSlot = cSlot;
                }
            }
        }

        if (!foundTarget)
            continue;

        Direction setFacing = DIR_N;
        if (facing < DIR_NONE) {
            setFacing = facing;
        } else {
            while (!isTargetInArc(setFacing,
                                   sourceTiles[bestTargetSlot],
                                   candidateTiles[bestAttackerSlot])) {
                setFacing = static_cast<Direction>(setFacing + 1);
            }
        }

        result.entries.push_back(TargetEntry((uint8)i, (uint8)nearestRange, setFacing));
    }

    // Sort: primary key = range ascending; tie-break = lower idx wins when
    // (candidate.idx & 1) <= (current.idx & 1). Mirrors COMBAT_SortTargetList.
    for (uint i = 0; i < result.entries.size(); ++i) {
        for (uint j = i + 1; j < result.entries.size(); ++j) {
            const TargetEntry &cur  = result.entries[i];
            const TargetEntry &cand = result.entries[j];
            bool swap = cand.range < cur.range;
            if (!swap && cand.range == cur.range &&
                cand.idx < cur.idx &&
                (cand.idx & 1) <= (cur.idx & 1))
                swap = true;
            if (swap)
                SWAP(result.entries[i], result.entries[j]);
        }
    }
}

void CombatContext::buildTargetList(const Data::PlayerCharacter *attacker,
                                    uint8 maxRange) {
    targetList.clear();
    if (!attacker)
        return;

    const int attackerIdx = table.findIndex(attacker);
    if (attackerIdx < 0)
        return;

    const TilePos attackerPos = table.getTilePos(attackerIdx);
    const uint8 attackerSize = table.getSize(attackerIdx) & 7;

    buildTargetListCore(attackerPos, attackerSize, DIR_NONE, maxRange);

    TargetList &result = targetList;
    const Data::CombatSide attackerSide = attacker->combatSide;
    uint j = 0;
    for (uint i = 0; i < result.entries.size(); i++) {
        if ((int)result.entries[i].idx == attackerIdx)
            continue;
        const Data::PlayerCharacter *ch = table.getCharacter(result.entries[i].idx);
        if (!ch || ch->combatSide == attackerSide)
            continue;
        result.entries[j++] = result.entries[i];
    }
    result.entries.resize(j);

    for (uint i = 0; i < result.entries.size(); i++)
        result.targetOrder.push_back(result.entries[i].idx);
}

bool CombatContext::lineOfSightCheck(TilePos source, TilePos target,
                                     uint16 &range, TilePos *blockedAt) const {
    const uint16 initialRange = range;
    const TilePropertyProvider *tileProps = map.getTilePropertyProvider();

    FieldPath linePath;
    linePath.start = source;
    linePath.endCol = (int16)target.col;
    linePath.endRow = (int16)target.row;
    Goldbox::initBresenham(linePath);

    const uint8 startRaw = map.getRawTile(source);
    uint8 terrainLevel = 0;
    if (startRaw > 0 && tileProps) {
        const TileProp *prop = tileProps->getTileProp(startRaw - 1);
        if (prop)
            terrainLevel = prop->terrainHeight;
    }

    FieldPath heightPath;
    heightPath.start = TilePos(0, terrainLevel);
    heightPath.endCol = (int16)MAX(linePath.deltaX, linePath.deltaY);
    heightPath.endRow = (int16)terrainLevel;
    Goldbox::initBresenham(heightPath);

    for (;;) {
        const uint8 raw = map.getRawTile(linePath.current);
        uint8 tileHeight = 0;
        if (raw > 0 && tileProps) {
            const TileProp *prop = tileProps->getTileProp(raw - 1);
            if (prop)
                tileHeight = prop->terrainHeight;
        }

        const bool passable = map.getIgnoreWalls() ||
                              tileHeight <= (uint8)heightPath.current.row;
        const bool inRange  = linePath.moveCost <= (int16)(initialRange * 2 + 1);

        if (!passable || !inRange)
            break;

        Goldbox::stepBresenham(heightPath);

        if (!Goldbox::stepBresenham(linePath)) {
            range = (uint16)linePath.moveCost;
            return true;
        }
    }

    if (blockedAt)
        *blockedAt = linePath.current;
    range = (uint16)linePath.moveCost;
    return false;
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
    const int ax = (int)attackerPos.col;
    const int ay = (int)attackerPos.row;
    const int tx = (int)targetPos.col;
    const int ty = (int)targetPos.row;

    if (ax < 0 || ax > 49 || ay < 0 || ay > 24 ||
        tx < 0 || tx > 49 || ty < 0 || ty > 24)
        return false;

    // Wire value 0xFF (~DIR_N) is the original sentinel for "no facing".
    if (direction == static_cast<Direction>(~DIR_N))
        direction = DIR_NONE;

    const int arcX = tx + kDirDeltaX[direction];
    const int arcY = ty + kDirDeltaY[direction];

    if ((tx == ax && ty == ay) || (arcX == ax && arcY == ay))
        return true;

    switch (direction) {
    case DIR_N:
        return !(
            (ax < arcX || arcX - ax + arcY < ay) &&
            (arcX < ax || ax - arcX + arcY < ay)
        );
    case DIR_NE:
        return !(
            (ax < arcX || arcX - ax + arcY < ay) &&
            (ax < arcX + arcY - ay || arcY < ay)
        );
    case DIR_E:
        return !(
            (ax < arcX + arcY - ay || arcY < ay) &&
            (ax < arcX + ay - arcY || ay < arcY)
        );
    case DIR_SE:
        return !(
            (ax < arcX + ay - arcY || ay < arcY) &&
            (ax < arcX || ay < ax - arcX + arcY)
        );
    case DIR_S:
        return !(
            (ax < arcX || ay < ax - arcX + arcY) &&
            (arcX < ax || ay < arcX - ax + arcY)
        );
    case DIR_SW:
        return !(
            (arcX < ax || ay < arcX - ax + arcY) &&
            (arcX + arcY - ay < ax || ay < arcY)
        );
    case DIR_W:
        return !(
            (arcX + arcY - ay < ax || ay < arcY) &&
            (arcX + ay - arcY < ax || arcY < ay)
        );
    case DIR_NW:
        return !(
            (arcX + ay - arcY < ax || arcY < ay) &&
            (arcX < ax || ax - arcX + arcY < ay)
        );
    case DIR_NONE:
        return true;
    default:
        return false;
    }
}

// ---------------------------------------------------------------------------
// checkAdvanceEngagement helpers
// ---------------------------------------------------------------------------

Direction CombatContext::getFacingToward(const Data::PlayerCharacter *target,
                                         const Data::PlayerCharacter *attacker) const {
    const TilePos targetPos   = table.getCharacterPos(target);
    const TilePos attackerPos = table.getCharacterPos(attacker);

    Direction facing = DIR_N;
    while (!isTargetInArc(facing, attackerPos, targetPos))
        facing = static_cast<Direction>(facing + 1);
    return facing;
}

bool CombatContext::isCharacterInBounds(const Data::PlayerCharacter *ch,
                                        bool requireAllCells) const {
    const int idx = table.findIndex(ch);
    if (idx < 0)
        return false;

    const uint8 iconSize = table.getSize(idx) & 7;
    if (iconSize == 0)
        return false;

    const ViewportPos base = viewport.getCharacterViewportPosition(table, idx);

    for (uint8 slot = 0; slot < 4; ++slot) {
        FootprintOffsetPair off;
        if (!getFootprintOffset(iconSize, slot, off))
            continue;

        ViewportPos cell(base.column + off.col, base.row + off.row);
        const bool inside = cell.isVisible(CombatViewport::VIEW_COLS,
                                           CombatViewport::VIEW_ROWS);

        if (requireAllCells && !inside)
            return false;
        if (!requireAllCells && inside)
            return true;
    }

    return requireAllCells;
}

// ---------------------------------------------------------------------------
// checkAdvanceEngagement helpers
// ---------------------------------------------------------------------------

/**
 * Mirrors COMBAT_CanEngageTarget.
 * Runs ES_DEFENSIVE_PASSIVE on target and ES_ATTACKER_OFFENSE on attacker
 * to let effects set globals.targetUnavailable.
 */
static bool canEngageTarget(Data::PlayerCharacter *attacker,
                             Data::PlayerCharacter *target,
                             CombatContext &ctx) {
    if (!target)
        return false;
    if (attacker == target)
        return true;

    ctx.globals.targetUnavailable = false;

    if (ctx.params.effectRuntime && target->getEffects())
        ctx.params.effectRuntime->checkEffectSet(
            Data::Effects::ES_DEFENSIVE_PASSIVE,
            *target->getEffects(), *target, &ctx.globals);

    if (!ctx.globals.targetUnavailable && ctx.params.effectRuntime && attacker->getEffects()) {
        Data::PlayerCharacter *oldTarget = attacker->combatState ? attacker->combatState->target : nullptr;
        if (attacker->combatState)
            attacker->combatState->target = target;

        ctx.params.effectRuntime->checkEffectSet(
            Data::Effects::ES_ATTACKER_OFFENSE,
            *attacker->getEffects(), *attacker, &ctx.globals);

        if (attacker->combatState)
            attacker->combatState->target = oldTarget;
    }

    return !ctx.globals.targetUnavailable;
}

/**
 * Mirrors SelectReactionAttack.
 * Initial choice: slot 1 if primary has attacks, else slot 2.
 * Then scan slots 1-2: last available slot wins (so slot 2 beats slot 1
 * when both are available).
 * Ensures the chosen slot has at least 1 attack marked available.
 */
static uint8 selectEngagementAttack(Data::PlayerCharacter *enemy) {
    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(enemy);
    if (!adnd)
        return 1;

    uint8 attackId = (adnd->curPrimaryRoll.attacks != 0) ? 1 : 2;

    for (uint8 slot = 1; slot <= 2; ++slot) {
        uint attacks = (slot == 1) ? adnd->curPrimaryRoll.attacks
                                   : adnd->curSecondaryRoll.attacks;
        if (attacks != 0)
            attackId = slot;
    }

    // Force at least 1 attack available on the chosen slot.
    if (attackId == 1 && adnd->curPrimaryRoll.attacks == 0)
        adnd->curPrimaryRoll.attacks = 1;
    else if (attackId == 2 && adnd->curSecondaryRoll.attacks == 0)
        adnd->curSecondaryRoll.attacks = 1;

    return attackId;
}

// ---------------------------------------------------------------------------
// checkAdvanceEngagement
// ---------------------------------------------------------------------------

void CombatContext::checkAdvanceEngagement(
        Data::PlayerCharacter *currentCharacter,
        Direction moveDirection,
        void (*drawCombatInfoCallback)(Data::PlayerCharacter *)) {

    if (!currentCharacter || !currentCharacter->enabled)
        return;

    const int charIdx = table.findIndex(currentCharacter);
    if (charIdx < 0)
        return;

    // Phase 1: build target list at current position.
    buildTargetList(currentCharacter, 1);

    // Phase 2: save the target order before the advance.
    // Entries are nulled with -1 to match the original signed sentinel.
    Common::Array<int8> oldOrder;
    oldOrder.resize(targetList.targetOrder.size());
    for (uint i = 0; i < targetList.targetOrder.size(); ++i)
        oldOrder[i] = (int8)targetList.targetOrder[i];

    if (oldOrder.empty())
        return;

    // Phase 3: temporarily advance one tile.
    const TilePos origPos = table.getTilePos(charIdx);
    const int8 dx = kDirDeltaX[moveDirection];
    const int8 dy = kDirDeltaY[moveDirection];
    table.setPosition(charIdx, TilePos((uint8)(origPos.col + dx),
                                       (uint8)(origPos.row + dy)));

    // Phase 4: build target list at the simulated position.
    buildTargetList(currentCharacter, 1);
    const Common::Array<uint8> newOrder = targetList.targetOrder;

    // Restore original position immediately.
    table.setPosition(charIdx, origPos);

    // Phase 5: null out oldOrder entries still present after the advance.
    // Remaining entries (>= 0) are enemies that lose engagement.
    for (uint oldIdx = 0; oldIdx < oldOrder.size(); ++oldIdx) {
        if (oldOrder[oldIdx] < 0)
            continue;
        for (uint newIdx = 0; newIdx < newOrder.size(); ++newIdx) {
            if (newOrder[newIdx] == (uint8)oldOrder[oldIdx]) {
                oldOrder[oldIdx] = -1;
                break;
            }
        }
    }

    // Phase 6: reaction attack from each enemy that loses engagement.
    for (uint targetIdx = 0; targetIdx < oldOrder.size(); ++targetIdx) {
        if (oldOrder[targetIdx] < 0)
            continue;

        if (!currentCharacter->enabled)
            return;

        Data::PlayerCharacter *enemy = table.getCharacter((uint8)oldOrder[targetIdx]);
        if (!enemy || !enemy->combatState)
            continue;

        if (enemy->hasNegativeEffect())
            continue;

        if (!canEngageTarget(enemy, currentCharacter, *this))
            continue;

        // Effects 0x4B and 0x4A suppress the reaction.
        {
            const Data::Effects::CharacterEffects *fx = enemy->getEffects();
            if (fx && fx->hasEffect(0x4B))
                continue;
            if (fx && fx->hasEffect(0x4A))
                continue;
        }

        bool attackResolved = false;

        // Scan facing+6 through facing+10, normalized % 8 (five directions).
        const int baseFacing = (int)enemy->combatState->direction;
        for (int scan = baseFacing + 6; scan <= baseFacing + 10; ++scan) {
            if (attackResolved)
                break;

            // Arc test only when initiative < 1 AND canAttack != 0.
            if (enemy->combatState->initiative < 1 &&
                enemy->combatState->attackCount != 0) {
                Direction arcDir = static_cast<Direction>(scan % 8);
                const TilePos enemyPos    = table.getTilePos((uint8)oldOrder[targetIdx]);
                const TilePos attackerPos = table.getTilePos(charIdx);
                if (!isTargetInArc(arcDir, attackerPos, enemyPos))
                    continue;
            }

            enemy->combatState->attackId = selectEngagementAttack(enemy);

            Data::PlayerCharacter *savedTarget = enemy->combatState->target;

            // TODO: call resolveAttack(enemy, currentCharacter, true, nullptr)
            // once that function is implemented.

            enemy->combatState->target = savedTarget;
            attackResolved = true;

            if (currentCharacter->enabled && drawCombatInfoCallback)
                drawCombatInfoCallback(currentCharacter);
        }
    }
}

} // namespace Combat
} // namespace Goldbox
