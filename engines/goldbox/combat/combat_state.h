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

#ifndef GOLDBOX_COMBAT_COMBAT_STATE_H
#define GOLDBOX_COMBAT_COMBAT_STATE_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Data {

class PlayerCharacter;

/**
 * Per-combatant runtime action state for the tactical combat system.
 *
 * Mirrors original gbCombatAction (allocated at combat_address).
 * Allocated at combat start and freed at end.
 *
 * Fields are zero-initialized on creation; direction is set from the
 * approach direction table during init.
 */
struct CombatAction {
	uint8 spellId;              // spell_id: active spell being cast (0 = none)
	bool canCast;               // can_cast
	bool canUse;                // can_use
	uint8 delay;                // delay: initiative/delay counter
	uint8 attackId;             // attack_id
	uint8 maxTargets;           // max_targets
	uint8 movePoints;           // move: remaining movement this round
	bool guarding;              // guarding
	bool unknownBool;           // unknown_bool
	uint8 direction;            // direction: facing (0=N,1=NE,2=E,3=SE,4=S,5=SW,6=W,7=NW)
	PlayerCharacter *target;    // target: current attack target (ch_ptr->combat_address->target)
	uint8 bleeding;             // bleeding
	uint8 attackCount;          // get_attack: attacks remaining this round
	bool fleeing;               // fleeing
	bool turnedUndead;          // turned_undead
	uint8 directionChange;      // direction_change
	bool notInTeam;             // not_in_team: true if beyond party count (NPC/monster)
	bool moralFailure;          // moral_failure
	uint8 aiState;              // ai_action: AI behavior state

	CombatAction()
		: spellId(0), canCast(false), canUse(false), delay(0),
		  attackId(0), maxTargets(0), movePoints(0),
		  guarding(false), unknownBool(false), direction(0),
		  target(nullptr), bleeding(0), attackCount(0),
		  fleeing(false), turnedUndead(false), directionChange(0),
		  notInTeam(false), moralFailure(false), aiState(0) {
	}

	void clear() {
		spellId = 0;
		canCast = false;
		canUse = false;
		delay = 0;
		attackId = 0;
		maxTargets = 0;
		movePoints = 0;
		guarding = false;
		unknownBool = false;
		direction = 0;
		target = nullptr;
		bleeding = 0;
		attackCount = 0;
		fleeing = false;
		turnedUndead = false;
		directionChange = 0;
		notInTeam = false;
		moralFailure = false;
		aiState = 0;
	}
};

// Legacy typedef so existing code using CombatState still compiles during migration.
typedef CombatAction CombatState;

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

#endif // GOLDBOX_COMBAT_COMBAT_STATE_H
