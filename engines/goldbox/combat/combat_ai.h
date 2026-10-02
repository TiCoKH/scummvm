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

/**
 * View delegate for processAiMove presentation callbacks.
 * Pass nullptr to skip all presentation (e.g. unit tests).
 */
struct AiMoveViewDelegate {
    virtual ~AiMoveViewDelegate() {}

    /** Show remaining movement. Mirrors TEXT_DrawToScreen "Move/Attack, Move Left = X". */
    virtual void drawMoveRemaining(uint8 moveHalf) = 0;

    /** Show autospell toggle message. */
    virtual void showMessage(const char *msg) = 0;

    /** Update character facing and redraw. */
    virtual void updateCharacterFacingAndRedraw(Data::PlayerCharacter *ch,
                                                uint8 direction) = 0;

    /** Draw combat info panel for character. */
    virtual void drawCombatInfo(Data::PlayerCharacter *ch) = 0;
};

/**
 * Mirrors COMBAT_HandleAIControlInput.
 *
 * Checks for player keyboard input during AI movement processing.
 * Handles autospell toggle, Space (transfer control to player), and cheat key.
 * Returns true if the currently acting character has been returned to player
 * control (initiative set to 20) — caller should stop AI processing.
 *
 * @param actor    The AI-controlled character currently acting
 * @param ctx      Live combat context
 * @param view     View delegate for message display (may be nullptr)
 */
bool handleAiControlInput(Data::PlayerCharacter *actor,
                          CombatContext &ctx,
                          AiMoveViewDelegate *view);

/**
 * Mirrors COMBAT_UpdateState.
 *
 * Decides whether the character enters guarding or ends its turn.
 * Called when the normal movement/action path is not taken.
 * Returns true when the turn action is complete.
 *
 * Conditions for guarding: no negative effect, no ranged weapon, initiative != 0.
 * Otherwise ends the turn immediately.
 *
 * @param actor  Character whose action state is being updated
 */
bool updateActionState(Data::PlayerCharacter *actor);

/**
 * Mirrors COMBAT_ProcessAIMove.
 *
 * AI movement step for a character whose current action is being resolved.
 * Handles movement display, autospell check, mage/initiative bypass,
 * direction selection (toward target or morale-failure flee), up to 5
 * movement attempts with the AI direction table, failure/target-retry
 * logic, disengagement reactions, and the final move + cloud check.
 *
 * Pure data layer — presentation is delegated through AiMoveViewDelegate.
 *
 * @param actor  The AI-controlled character taking its movement step
 * @param ctx    Live combat context
 * @param view   View delegate (may be nullptr)
 */
void processAiMove(Data::PlayerCharacter *actor,
                   CombatContext &ctx,
                   AiMoveViewDelegate *view);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_AI_H
