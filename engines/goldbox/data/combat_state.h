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

#ifndef GOLDBOX_DATA_COMBAT_STATE_H
#define GOLDBOX_DATA_COMBAT_STATE_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Data {

/**
 * Per-combatant runtime state for the tactical combat system.
 *
 * Mirrors original gbCombatAction (22-byte allocation at combat_address).
 * Allocated at combat start via COMBAT_InitCombatStates and freed at end.
 *
 * Fields are zero-initialized on creation; direction is set from the
 * approach direction table during init.
 */
struct CombatState {
    uint8 spellId;       // Active spell being cast (0 = none)
    uint8 direction;     // Facing direction (0=N,1=NE,2=E,3=SE,4=S,5=SW,6=W,7=NW)
    bool notInTeam;      // True if combatant is beyond party count (NPC/monster)
    uint8 targetX;       // Target tile X
    uint8 targetY;       // Target tile Y
    uint8 actionType;    // Current action (0=none, attack/move/cast/etc)
    uint8 movePoints;    // Remaining movement this round
    uint8 attackCount;   // Attacks remaining this round
    uint8 status;        // Combat status flags
    uint8 aiState;       // AI behavior state
    uint8 delay;         // Initiative/delay counter

    CombatState()
        : spellId(0), direction(0), notInTeam(false),
          targetX(0), targetY(0), actionType(0),
          movePoints(0), attackCount(0), status(0),
          aiState(0), delay(0) {
    }

    void clear() {
        spellId = 0;
        direction = 0;
        notInTeam = false;
        targetX = 0;
        targetY = 0;
        actionType = 0;
        movePoints = 0;
        attackCount = 0;
        status = 0;
        aiState = 0;
        delay = 0;
    }
};

/**
 * Direction lookup table for converting map cardinal facing (way_flag >> 1)
 * to isometric diagonal direction for combat sprites.
 *
 * Index: (way_flag >> 1), values 0..3
 * Value: 8-direction wire format
 *
 *   way_flag 0-1 (index 0) -> 7 (NW, facing left-up)
 *   way_flag 2-3 (index 1) -> 2 (E,  facing right)
 *   way_flag 4-5 (index 2) -> 3 (SE, facing right-down)
 *   way_flag 6-7 (index 3) -> 6 (W,  facing left)
 */
static const uint8 kCombatDirectionTable[4] = { 7, 2, 3, 6 };

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_COMBAT_STATE_H
