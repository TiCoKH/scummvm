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

#include "goldbox/gfx/combat_setup.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/combat_state.h"
#include "goldbox/core/direction.h"

namespace Goldbox {
namespace Gfx {

void initCombatStates(Common::Array<Data::PlayerCharacter *> &combatants,
                         int partyCount, uint8 wayFlag,
                         uint8 moraleThreshold) {
    int combatantCount = 0;

    for (uint i = 0; i < combatants.size(); i++) {
        Data::PlayerCharacter *ch = combatants[i];
        if (!ch)
            continue;

        combatantCount++;

        // Allocate and zero-fill combat state
        delete ch->combatState;
        ch->combatState = new Data::CombatState();

        // Mark characters beyond the player party as not in team
        if (partyCount < combatantCount)
            ch->combatState->notInTeam = true;

        // Set initial facing from approach direction table
        // wayFlag >> 1 gives index 0..3 into the direction table
        uint8 dirIndex = (wayFlag >> 1) & 0x03;
        ch->combatState->direction = Data::kCombatDirectionTable[dirIndex];

        // Hostile characters face the opposite direction (180 degree flip)
        if (ch->hostile)
            ch->combatState->direction = dirReverse(ch->combatState->direction);

        // Neutral NPCs with no class or above standard range
        // receive a morale-based NPC value with the override flag set
        // TODO: npc field and class check when character data is fully ported
    }
}

void freeCombatStates(Common::Array<Data::PlayerCharacter *> &combatants) {
    for (uint i = 0; i < combatants.size(); i++) {
        if (combatants[i]) {
            delete combatants[i]->combatState;
            combatants[i]->combatState = nullptr;
        }
    }
}

} // namespace Gfx
} // namespace Goldbox
