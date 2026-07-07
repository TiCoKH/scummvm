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

#include "goldbox/combat/combat_setup.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/combat_state.h"
#include "goldbox/core/direction.h"

namespace Goldbox {
namespace Combat {

void initCombatStates(Common::Array<Data::PlayerCharacter *> &combatants,
                         int partyCount, uint8 wayFlag,
                         uint8 moraleThreshold) {
    int combatantCount = 0;

    for (uint i = 0; i < combatants.size(); i++) {
        Data::PlayerCharacter *ch = combatants[i];
        if (!ch)
            continue;

        // Recalculate derived combat stats (AC, movement, weapon bonuses)
        // TODO: CHARACTER_recalcCombatStats equivalent

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

        // Hostile characters face the opposite direction: (dir + 4) % 8
        if (ch->hostile)
            ch->combatState->direction = dirReverse(ch->combatState->direction);

        // Neutral NPCs (non-hostile, not-in-team) with npc class == 0 or > 102
        // get morale override: npc = moraleThreshold | 0x80
        // TODO: uncomment when npc field is ported to PlayerCharacter
        // uint8 npcClass = ch->npc & 0x7F;
        // if (!ch->hostile && ch->combatState->notInTeam &&
        //     (npcClass == 0 || npcClass > 0x66)) {
        //     ch->npc = (uint8)moraleThreshold | 0x80;
        // }
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

} // namespace Combat
} // namespace Goldbox
