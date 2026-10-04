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
class ADnDCharacter;
namespace Items {
struct CharacterItem;
}
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
 * View delegate for AI presentation callbacks.
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
 * Mirrors COMBAT_ProcessAITurn.
 *
 * Full AI combat-turn processing loop for one AI-controlled character.
 * Handles morale-failure movement, target selection, ranged weapon
 * auto-equip, multi-attack, and single-attack resolution.
 *
 * Returns true when AI processing has ended (control released or turn
 * complete); false if the caller should continue driving the AI.
 *
 * Presentation callbacks are routed through AiMoveViewDelegate.
 * Pass nullptr to skip all presentation (e.g. unit tests).
 */
bool processAiTurn(Data::PlayerCharacter *actor,
                   CombatContext &ctx,
                   AiMoveViewDelegate *view);

/**
 * Execute one AI-controlled combatant's turn.
 *
 * Thin wrapper around processAiTurn for callers that do not need the
 * full loop result. Drives the turn to completion and returns a summary
 * result for the view layer.
 */
AiTurnResult executeAiTurn(Data::PlayerCharacter *actor,
                            CombatContext &ctx);

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

/**
 * Mirrors AI_FindNearbyWoundedAlly.
 *
 * Scans the 8 neighbouring tiles plus the attacker's own tile (9 positions
 * total) for the best wounded ally (same combat side, hp_current < hp_max).
 *
 * Selection rules:
 *   - Primary: lowest absolute hp_current among wounded allies.
 *   - Self exception: the attacker itself qualifies even if not the lowest HP,
 *     provided hp_current < hp_max/2. bestHp is NOT updated in this case.
 *   - Fallback: a downed member on tile 0x1F is preferred over a standing
 *     ally only when bestHp >= 8. Last matching downed record wins.
 *
 * Returns true and sets *outTarget when a candidate is found.
 * Returns false and sets *outTarget = nullptr when none exists.
 *
 * Used by isSpellEligibleForAI() to gate SP_CL1_CURE_LT_WOUNDS:
 * that spell is only eligible when NO nearby wounded ally exists.
 */
bool findNearbyWoundedAlly(Data::PlayerCharacter *attacker,
                        CombatContext &ctx,
                        Data::PlayerCharacter **outTarget);

/**
 * Mirrors AI_SpellEligible.
 *
 * Returns true when spellId is eligible for AI use at the given
 * priorityThreshold:
 *   1. spell.priority >= priorityThreshold
 *   2. spell is offensive OR is SP_CL1_CURE_LT_WOUNDS (id 3)
 *   3. SP_CL1_CURE_LT_WOUNDS is rejected when an adjacent target exists
 *   4. ctx.buildTargetList() must find at least one target
 *   5. when spell.minAITargets != 0, each target is validated via
 *      a per-target enemy check (AI_HasEnemyTargetForSpell — TBD)
 *
 * Side effect: calls ctx.buildTargetList(), which mutates ctx.targetList.
 */
bool isSpellEligibleForAI(Data::PlayerCharacter *actor,
                          uint8 spellId,
                          uint8 priorityThreshold,
                          CombatContext &ctx);

/**
 * Mirrors AI_TryUseItem.
 *
 * Searches the character's ready magic items for an eligible spell item
 * and uses the first one found. Eligibility is gated by:
 *   - combatState->canUse
 *   - ctx.globals.sideCount[actor->combatSide] != 0
 *   - ctx.globals.magicEnabled
 *
 * Rolls 1d7 to determine how many priority levels (7 down to 7-roll+1)
 * are searched. Within each pass the first eligible item wins.
 *
 * Returns true when an item was used.
 */
bool tryUseItem(Data::PlayerCharacter *actor, CombatContext &ctx);

/**
 * Compute a weapon desirability score for AI auto-equip selection.
 *
 * Mirrors AI_ComputeWeaponScore. Scores based on damage dice, item bonus,
 * property damage modifier, undead special case, range bonus, and one-hand
 * preference. Returns 0 for unusable items (hand overflow, alignment
 * restriction, forbidden effect tag 0x53, or cursed).
 */
uint8 computeWeaponScore(const Data::ADnDCharacter &character,
                         const Data::Items::CharacterItem &item);

/**
 * Auto-select and equip the best available weapon and off-hand item.
 *
 * Mirrors AI_AutoEquipWeapons. Scans inventory for the best ranged and
 * ordinary main-hand weapons and best off-hand item, chooses between
 * ranged and melee based on score threshold and adjacent-target check,
 * then performs the necessary toggleReadyItem / recalcCombatStats sequence
 * while respecting cursed equipment and hand capacity.
 *
 * Calls drawCombatInfoCallback(character) when equipment changed.
 */
void autoEquipWeapons(Data::ADnDCharacter *character,
                      CombatContext &ctx,
                      void (*drawCombatInfoCallback)(Data::PlayerCharacter *) = nullptr);

} // namespace Combat
} // namespace Goldbox

#endif // GOLDBOX_COMBAT_COMBAT_AI_H
