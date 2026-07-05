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

#ifndef GOLDBOX_COMBAT_COMBAT_PARAMS_H
#define GOLDBOX_COMBAT_COMBAT_PARAMS_H

#include "common/scummsys.h"
#include "common/array.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

/**
 * Parameters passed from the VM/game layer to initialize a combat encounter.
 *
 * The caller (ECL syscall or in-game view) populates this from VM state
 * before pushing the combat view. The combat system reads these at setup
 * and does not access the VM directly during the encounter.
 *
 * Mirrors values from COMBAT_Setup decompilation:
 *   VMBANK1_PARTY_STATE, map direction, encounter distance, etc.
 */
struct CombatParams {
    // --- Roster ---
    Common::Array<Data::PlayerCharacter *> roster;
    int partyCount;           // number of player-controlled characters

    // --- Map/approach ---
    uint8 mapDirection;       // way_flag (0-7 wire format)
    int encounterDistance;    // distance between party and enemy origins
    bool isDungeon;           // true = dungeon terrain, false = wilderness

    // --- Terrain generation inputs ---
    int8 mapCenterX;         // party map X for dungeon generation
    int8 mapCenterY;         // party map Y for dungeon generation
    int8 playerY;            // player Y row for OOB checks
    uint8 eclScriptId;       // ECL script ID (affects random floor tiles)
    uint8 wildX;             // wilderness X coordinate
    uint8 wildY;             // wilderness Y coordinate
    uint8 mapType;           // VM map type (1=dungeon, 3/4=wilderness)
    uint8 terrainOverride;   // terrain override flags

    // --- VM state (read at setup) ---
    uint8 moraleThreshold;   // VMBANK1_PARTY_STATE->D_MoraleThreshold (clamped to 100)
    bool magicEnabled;       // COMBAT_MAGIC_ENABLED (reset to false at start)
    bool slowMode;           // BYTE_COMFLAG_SLOW (reset to false at start)
    bool isAmbush;           // D_CombatIsAmbush (cleared each round)
    bool monsterLoadReady;   // BOOL_MONST_LOAD_READY (pre-checked by caller)
    bool combatTrigger;      // BOOL_COMBAT_TRIGGER (caller clears after combat)

    CombatParams()
        : partyCount(0), mapDirection(0), encounterDistance(2),
          isDungeon(true), mapCenterX(0), mapCenterY(0), playerY(0),
          eclScriptId(0), wildX(0), wildY(0), mapType(1),
          terrainOverride(0), moraleThreshold(100),
          magicEnabled(false), slowMode(false), isAmbush(false),
          monsterLoadReady(false), combatTrigger(false) {}
};

/**
 * Result returned from combat view to the caller after combat ends.
 *
 * The ECL opcode handler uses this to decide post-combat flow
 * (DIALOG_AfterCombat, icon reloads, game state restore).
 */
struct CombatResult {
    enum Outcome {
        OUTCOME_VICTORY = 0,   // all enemies defeated
        OUTCOME_DEFEAT  = 1,   // all party members down
        OUTCOME_FLED    = 2,   // party fled combat
        OUTCOME_ENDED   = 3    // scripted end (parley, surrender)
    };

    Outcome outcome;
    bool combatTriggerWasSet;  // caller should clear BOOL_COMBAT_TRIGGER

    CombatResult()
        : outcome(OUTCOME_VICTORY), combatTriggerWasSet(false) {}
};

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_PARAMS_H
