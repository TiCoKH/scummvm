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

#include "goldbox/combat/combat_turn.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_state.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"

namespace Goldbox {
namespace Combat {

void initCharacterTurnState(Data::PlayerCharacter *ch) {
    if (!ch || !ch->combatState)
        return;

    Data::CombatAction &cs = *ch->combatState;

    // Per-turn flags reset each round.
    cs.fleeing = false;
    cs.guarding = false;
    cs.moralFailure = false;
    cs.directionChange = 0;
    cs.target = nullptr;
    cs.spellId = 0;
    cs.attackId = 0;

    // Attack count from primary roll (mirrors COMBAT_InitCharacterTurnState
    // reading get_attack from the character's combat data).
    if (Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(ch))
        cs.attackCount = adnd->curPrimaryRoll.attacks;
    else
        cs.attackCount = 1;

    // Move points from current movement stat.
    cs.movePoints = ch->movement.current;

    // delay == 0xFF marks "has acted this round"; reset to 0 so the
    // character is eligible for selectNextActor.
	cs.initiative = 0;
}

void initAllTurnStates(Common::Array<Data::PlayerCharacter *> &roster) {
    for (uint i = 0; i < roster.size(); i++)
        initCharacterTurnState(roster[i]);
}

Data::PlayerCharacter *selectNextActor(
        const Common::Array<Data::PlayerCharacter *> &roster,
        const CombatGlobals &globals) {
    // Both sides must still have members for combat to continue.
    if (globals.sideCount[0] == 0 || globals.sideCount[1] == 0)
        return nullptr;

    // Find the eligible character with the lowest delay value.
    // delay == 0xFF means the character has already acted this round.
    Data::PlayerCharacter *best = nullptr;
    uint8 bestInitiative = 0xFF;

    for (uint i = 0; i < roster.size(); i++) {
        Data::PlayerCharacter *ch = roster[i];
        if (!ch || !ch->enabled || !ch->combatState)
            continue;
		if (ch->combatState->initiative == 0xFF)
            continue;
		if (ch->combatState->initiative < bestInitiative) {
			bestInitiative = ch->combatState->initiative;
            best = ch;
        }
    }

    return best;
}

} // namespace Combat
} // namespace Goldbox
