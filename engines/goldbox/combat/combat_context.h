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

#ifndef GOLDBOX_COMBAT_COMBAT_CONTEXT_H
#define GOLDBOX_COMBAT_COMBAT_CONTEXT_H

#include "common/scummsys.h"
#include "common/array.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/core/field_path.h"
#include "goldbox/core/coords.h"
#include "goldbox/core/direction.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

struct CombatParams;
class CombatantTable;
class BattlefieldMap;
class CombatViewport;

struct FootprintOffsetPair {
    int8 col;
    int8 row;
};

/**
 * Non-owning bundle of live combat object references.
 *
 * Passed to any combat subsystem (AI, movement, spells, etc.) that needs
 * to read or mutate global combat state during the combat phase.
 * CombatGlobals owns all members; this struct just provides access.
 *
 * Mirrors the original pattern where COMBAT_* functions accessed scattered
 * globals (ARRAY_HOSTILITY, C_FIELD_PLACEMENT_MAP, BYTE_ARRAY_COL_DIST, etc.)
 * directly. Here those are reached through the owning objects.
 */
struct CombatContext {
    CombatGlobals       &globals;
    CombatParams        &params;
    CombatantTable      &table;
    BattlefieldMap      &map;
    CombatViewport      &viewport;
    CloudEffectManager  &clouds;  // alias for globals.clouds
    TargetList          &targetList; // TARGET_LIST / TARGET_COUNT / ARRAY_TARGET_ORDER

    CombatContext(CombatGlobals &globals_,
                  CombatParams &params_,
                  CombatantTable &table_,
                  BattlefieldMap &map_,
                  CombatViewport &viewport_,
                  TargetList &targetList_);

    /** Mirrors COMBAT_updateSideCount — call after any roster change. */
    void updateSideCount();

    /** Rebuild occupancy grid — call after any combatant moves or is removed. */
    void rebuildPlacementMap();

    /** Rebuild viewport distances — call after viewport scrolls or combatant moves. */
    void rebuildDistances();

    /**
     * Returns the col/row delta for a footprint slot.
     * iconSize: combat footprint code (1-4); slot: 0-3.
     * Returns false for invalid size/slot or sentinel {-1,-1} entries.
     */
    static bool getFootprintOffset(uint8 iconSize, uint8 slot, FootprintOffsetPair &off);

    /** Query ground tile and occupant one step in direction from ch. */
    void getGroundInfo(Data::PlayerCharacter *ch, uint8 direction,
                       int *outPlayerIndex, uint8 *outTile) const;

    /**
     * Returns the tile id and 1-based combatant index at pos.
     * tileId==0 means out-of-bounds or void. occupantId==0 means empty.
     * Mirrors CombatSystem::getTileAndOccupantAt.
     */
    struct CombatCell {
        uint8 tileId;
        uint8 occupantId;
    };
    CombatCell getTileAndOccupantAt(TilePos pos) const;

    /** Returns true if pos is within the 50x25 battlefield bounds. */
    static bool isValidTilePos(TilePos pos) { return Goldbox::isValidTilePos(pos); }

    /** Check LOS from source to target. Returns true if clear; range holds walk cost.
     *  On failure, blockedAt receives the blocking tile. Mirrors COMBAT_lineOfSightCheck. */
    bool lineOfSightCheck(TilePos source, TilePos target,
                          uint16 &range, TilePos *blockedAt = nullptr) const;
    /**
     * Mirrors COMBAT_BuildTargetListCore.
     * Populates targetList.entries with all combatants reachable from
     * pos within maxRange, passing arc and LOS checks. Sorted by range.
     */
    void buildTargetListCore(TilePos pos, uint8 iconSize,
                             Direction facing, uint8 maxRange);

    /**
     * Mirrors COMBAT_BuildTargetList.
     * Calls buildTargetListCore with DIR_NONE, filters to same side as
     * attacker, and populates targetList.targetOrder.
     */
    void buildTargetList(const Data::PlayerCharacter *attacker, uint8 maxRange);

    /**
     * Returns true if the attacker tile is within the frontal arc of a
     * target facing direction. direction==DIR_NONE means no restriction.
     * Mirrors COMBAT_IsTargetInArc.
     */
    static bool isTargetInArc(Direction direction,
                               TilePos attackerPos, TilePos targetPos);

    /**
     * Reset per-turn CombatAction fields for one character.
     * Mirrors COMBAT_InitCharacterTurnState.
     */
    void initCharacterTurnState(Data::PlayerCharacter *ch);

    /** Reset turn state for every character in the roster. */
    void initAllTurnStates();

    /**
     * Select the next character to act this round.
     * Returns nullptr when all characters have acted or a side has no members.
     */
    Data::PlayerCharacter *selectNextActor() const;

    /**
     * Remove a character from the combat roster.
     *
     * Mirrors PARTY_removeMember — never touches engine _party.
     *   freeIconSlot=true   → release the character's icon resource
     *   keepPartyCount=true → remove from roster but do not shrink partyCount
     *                         (e.g. character fled, slot stays reserved)
     *   keepPartyCount=false → remove and decrement partyCount
     *                         (e.g. permanent removal from encounter)
     *
     * Always calls updateSideCount() after removal.
     */
    void removeMember(Data::PlayerCharacter *ch, bool keepPartyCount, bool freeIconSlot);
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_CONTEXT_H
