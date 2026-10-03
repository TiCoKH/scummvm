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
#include "common/array.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
}

namespace Combat {

struct CombatContext;
struct CombatGlobals;

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
 * Called by the view layer when a keypress arrives during AI movement
 * processing. The view decodes the key from msgKeypress and passes it here.
 *
 * Handles three keys:
 *   - autospell toggle key: flip globals.magicEnabled, show message via view
 *   - Space: clear ai_control on eligible party members, set actor initiative=20
 *   - '-': cheat handler (no-op in modern engine)
 *
 * Returns true if the actor has been returned to player control (Space was
 * pressed and actor->ai_control is now 0) — caller should stop AI processing.
 * Returns false when no key was pending or no control transfer occurred.
 *
 * Pass key=0 when no keypress is pending (normal AI tick with no input).
 *
 * @param actor    The AI-controlled character currently acting
 * @param roster   Full combat roster (for Space key party scan)
 * @param key      ASCII key value from the view's msgKeypress, or 0 if none
 * @param globals  Combat globals (magicEnabled toggle)
 * @param view     View delegate for autospell message display (may be nullptr)
 */
bool handleAiControlInput(Data::PlayerCharacter *actor,
                          const Common::Array<Data::PlayerCharacter *> &roster,
                          char key,
                          CombatGlobals &globals,
                          AiMoveViewDelegate *view);


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
 * @param actor   The AI-controlled character taking its movement step
 * @param roster  Full combat roster (forwarded to handleAiControlInput)
 * @param key     ASCII key from view's msgKeypress, or 0 if no input this tick
 * @param ctx     Live combat context
 * @param view    View delegate (may be nullptr)
 */
void processAiMove(Data::PlayerCharacter *actor,
                   const Common::Array<Data::PlayerCharacter *> &roster,
                   char key,
                   CombatContext &ctx,
                   AiMoveViewDelegate *view);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_AI_H
