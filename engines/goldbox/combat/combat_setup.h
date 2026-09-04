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

#ifndef GOLDBOX_COMBAT_COMBAT_SETUP_H
#define GOLDBOX_COMBAT_COMBAT_SETUP_H

#include "common/scummsys.h"
#include "common/array.h"

namespace Goldbox {

namespace Gfx {
class BattlefieldTilemap;
}

namespace Data {
class PlayerCharacter;
namespace Effects {
class EffectRuntime;
}
}

namespace Combat {

struct CombatGlobals;
struct CombatParams;
class CombatantTable;
class CombatPlacement;
class CombatViewport;
class BattlefieldMap;

/**
 * Initialize combat state for all combatants.
 *
 * Mirrors original COMBAT_InitCombatStates:
 * - Allocates CombatState for each character
 * - Sets initial facing from approach direction (way_flag)
 * - Flips hostile characters 180 degrees
 * - Marks characters beyond partyCount as not-in-team
 * - Assigns morale-based NPC value for neutral non-team characters
 *
 * @param combatants  All characters participating in combat
 * @param partyCount  Number of player-controlled characters
 * @param wayFlag     Approach direction (0-7, from position struct)
 * @param moraleThreshold  Party morale threshold for neutral NPC assignment
 */
void initCombatStates(Common::Array<Data::PlayerCharacter *> &combatants,
                         int partyCount, uint8 wayFlag,
                         uint8 moraleThreshold);

/**
 * Free combat state for all combatants (end of combat cleanup).
 */
void freeCombatStates(Common::Array<Data::PlayerCharacter *> &combatants);

/**
 * Full combat setup orchestrator — mirrors original COMBAT_Setup.
 *
 * Sequence:
 *   1. Reset combat globals
 *   2. Clamp morale threshold to 100
 *   3. Build battlefield map data (COMBAT_BuildPlayfield)
 *   4. Init combatant states (facing, team flags)
 *   5. Place all combatants on battlefield
 *   6. Center viewport on first party member
 *   7. Apply combat aura effects (trigger set 8) to all combatants
 *   8. Update hostile health percentages
 *
 * @param params     Combat parameters from VM/caller
 * @param globals    Global combat state (reset here)
 * @param map        Battlefield map data (built here)
 * @param table      Combatant table (populated here)
 * @param placement  Placement engine
 * @param viewport   Camera viewport (centered here)
 * @param effectRuntime  Effect system for aura application (may be nullptr)
 */
void setupCombat(CombatParams &params,
                 CombatGlobals &globals,
                 BattlefieldMap &map,
                 CombatantTable &table,
                 CombatPlacement &placement,
                 CombatViewport &viewport,
                 Data::Effects::EffectRuntime *effectRuntime);

/**
 * Compute hostile health ratio and store in globals.handicapValue.
 *
 * Mirrors original COMBAT_UpdateHostileHealthPercent:
 *   healthRatio = (totalCurrentHP * 20) / totalMaxHP
 *   C_HANDICAP_VALUE = healthRatio * 5
 *
 * @param table    Combatant table with placed characters
 * @param globals  Combat globals (handicapValue written here)
 */
void updateHostileHealthPercent(const CombatantTable &table,
                                CombatGlobals &globals);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_SETUP_H
