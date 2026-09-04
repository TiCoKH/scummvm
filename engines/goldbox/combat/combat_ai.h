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

#ifndef GOLDBOX_COMBAT_COMBAT_AI_H
#define GOLDBOX_COMBAT_COMBAT_AI_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

struct CombatContext;

/**
 * Result of an AI turn execution.
 *
 * The view layer uses this to decide what animation/message to show
 * without the AI needing to call into the view directly.
 */
struct AiTurnResult {
    enum Action {
        ACTION_NONE    = 0,
        ACTION_ATTACK  = 1,  // moved to target and attacked
        ACTION_MOVE    = 2,  // moved toward target, no attack
        ACTION_FLEE    = 3,  // morale failed, character fled
        ACTION_GUARD   = 4   // no valid target, character guards
    };

    Action action;
    Data::PlayerCharacter *target; // non-null when ACTION_ATTACK
    uint8 damage;                  // resolved damage (ACTION_ATTACK only)
    bool targetWentDown;

    AiTurnResult()
        : action(ACTION_NONE), target(nullptr),
          damage(0), targetWentDown(false) {}
};

/**
 * Execute one AI-controlled combatant's turn.
 *
 * Mirrors COMBAT_ExecuteTurn for the enemy side:
 *   1. Check morale — flee if moralFailure
 *   2. Build target list (melee range = 1)
 *   3. If target in range: attack (roll to-hit, apply damage)
 *   4. Else: move one step toward nearest target
 *   5. Mark character as acted (delay = 0xFF)
 *
 * Pure data layer — no rendering, no sound, no UI.
 * The view layer reads AiTurnResult and drives presentation.
 *
 * @param actor   The AI-controlled character taking its turn
 * @param ctx     Live combat context (globals, table, map)
 * @return        Result describing what the AI did
 */
AiTurnResult executeAiTurn(Data::PlayerCharacter *actor,
                            CombatContext &ctx);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_AI_H
