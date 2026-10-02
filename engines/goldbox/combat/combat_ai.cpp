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

#include "goldbox/combat/combat_ai.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combat_damage.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_state.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/core/direction.h"
#include "goldbox/vm_interface.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

// ---------------------------------------------------------------------------
// AI direction offset table  (ARRAY_AI_DIRECTION_OFFSET[aiState 1-6][attempt 1-5])
// Row index = aiState-1 (0-5); column index = attempt-1 (0-4).
// Values are added to desiredDirection mod 8 to pick an alternate direction.
// 8 is used in the original Pascal source where the offset is 0 (8 % 8 == 0).
// ---------------------------------------------------------------------------
static const uint8 kAiDirectionOffset[6][5] = {
    { 8, 7, 6, 1, 2 },  // aiState 1
    { 8, 1, 2, 7, 6 },  // aiState 2
    { 7, 1, 8, 6, 2 },  // aiState 3
    { 1, 7, 8, 2, 6 },  // aiState 4
    { 8, 7, 6, 5, 4 },  // aiState 5
    { 8, 1, 2, 3, 4 },  // aiState 6
};

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/** Roll damage for one attack from attacker's primary roll. */
static uint8 rollDamage(const Data::PlayerCharacter *attacker) {
    const Data::ADnDCharacter *adnd =
        dynamic_cast<const Data::ADnDCharacter *>(attacker);
    if (!adnd)
        return 1;

    const Data::CombatRoll &roll = adnd->curPrimaryRoll;
    if (roll.action.roll.diceSides == 0)
        return 1;

    static Common::RandomSource rng("combat_ai_dmg");
    int total = 0;
    for (uint i = 0; i < roll.action.roll.diceNum; i++)
        total += (int)(rng.getRandomNumber(roll.action.roll.diceSides - 1) + 1);
    total += roll.action.modifier;

    return (uint8)MAX(1, total);
}

/**
 * Mirrors COMBAT_CheckMoveStep.
 * Tests whether the character can move one tile in the given direction.
 * Returns true if movement is possible; sets *outMoveResult to a non-zero
 * value when a morale-failure running condition is detected.
 */
static bool checkMoveStep(Data::PlayerCharacter *actor,
                           uint8 direction,
                           CombatContext &ctx,
                           uint8 *outMoveResult) {
    *outMoveResult = 0;

    const int idx = ctx.table.findIndex(actor);
    if (idx < 0)
        return false;

    const TilePos src = ctx.table.getTilePos(idx);
    const int8 dx = kDirDeltaX[direction];
    const int8 dy = kDirDeltaY[direction];
    const TilePos dst((uint8)(src.col + dx), (uint8)(src.row + dy));

    if (!CombatContext::isValidTilePos(dst))
        return false;

    const CombatContext::CombatCell cell = ctx.getTileAndOccupantAt(dst);
    if (cell.occupantId != 0)
        return false;
    if (cell.tileId == 0)
        return false;

    // Passability check.
    const uint8 cost = (direction & 1) ? 3 : 2;
    if (!actor->combatState || actor->combatState->movePoints < cost)
        return false;

    // Morale-failure running: signal via outMoveResult when the destination
    // tile is at the map edge (col==0, col==49, row==0, row==24).
    if (actor->combatState->moralFailure) {
        if (dst.col == 0 || dst.col == 49 || dst.row == 0 || dst.row == 24)
            *outMoveResult = 1;
    }

    return true;
}

/**
 * Mirrors the moral-failure direction calculation.
 * Derives a flee direction from the map/party orientation flag and
 * offsets it by 4 for party-side characters.
 * way_flag is stored in globals.combatFlag1 (STRUCT_POSITION.way_flag).
 */
static uint8 calcMoralFailureDirection(uint8 wayFlag, Data::CombatSide side) {
    uint8 dir = wayFlag & 7;
    if (side == Data::CS_PARTY)
        dir += 4;
    return dir % 8;
}

// ---------------------------------------------------------------------------
// handleAiControlInput
// ---------------------------------------------------------------------------

bool handleAiControlInput(Data::PlayerCharacter *actor,
                          CombatContext &ctx,
                          AiMoveViewDelegate *view) {
    // In the original, this checks for a keypress and handles three keys.
    // In the modern engine there is no direct keyboard polling here;
    // player-control transfer is handled by CombatSession::submitPlayerAction.
    // We preserve the autospell toggle path via globals.magicEnabled.
    //
    // Space-key path: if the actor is no longer ai_control, set initiative=20
    // and return true so the caller stops AI processing.
    if (!actor->ai_control) {
        if (actor->combatState)
            actor->combatState->initiative = 20;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// updateActionState
// ---------------------------------------------------------------------------

bool updateActionState(Data::PlayerCharacter *actor) {
    if (!actor || !actor->combatState)
        return false;

    Data::CombatAction &cs = *actor->combatState;

    // Guard conditions: no negative effect, no ranged weapon, initiative != 0.
    const Data::ADnDCharacter *adnd = dynamic_cast<const Data::ADnDCharacter *>(actor);
    const bool hasRanged = adnd ? adnd->hasRangedWeapon() : false;

    if (!actor->hasNegativeEffect() && !hasRanged && cs.initiative != 0) {
        // COMBAT_Guarding: end turn first, then set guarding.
        cs.endTurn();
        cs.guarding = true;
        return true;
    }

    cs.endTurn();
    return true;
}

// ---------------------------------------------------------------------------
// processAiMove
// ---------------------------------------------------------------------------

void processAiMove(Data::PlayerCharacter *actor,
                   CombatContext &ctx,
                   AiMoveViewDelegate *view) {
    if (!actor || !actor->combatState)
        return;

    Data::CombatAction &cs = *actor->combatState;
    CombatGlobals &globals = ctx.globals;

    // Presentation: show remaining movement.
    if (view)
        view->drawMoveRemaining(cs.movePoints >> 1);

    // Autospell / player-control interception.
    if (handleAiControlInput(actor, ctx, view))
        return;

    // Bypass: no movement, initiative expired, or normal mage.
    if ((cs.movePoints >> 1) == 0 ||
        cs.initiative < 1 ||
        (!cs.moralFailure && actor->classType == Data::C_MAGICUSER)) {
        updateActionState(actor);
        return;
    }

    // Determine desired direction.
    uint8 desiredDirection;
    if (!cs.moralFailure) {
        desiredDirection = (uint8)ctx.getFacingToward(actor, cs.target);
    } else {
        cs.aiState = (uint8)VmInterface::rollDice(1, 2);
        desiredDirection = calcMoralFailureDirection(globals.combatFlag1,
                                                     actor->combatSide);
    }

    // Try up to five movement alternatives.
    uint8 attempt = 1;
    uint8 moveCheckResult = 0;
    bool actionFinished = false;

    while (attempt < 6 && !actionFinished) {
        const uint8 aiStateIdx = (cs.aiState >= 1 && cs.aiState <= 6) ? cs.aiState - 1 : 0;
        const uint8 offset = kAiDirectionOffset[aiStateIdx][attempt - 1];
        const uint8 tryDir = (uint8)((desiredDirection + offset) % 8);

        if (checkMoveStep(actor, tryDir, ctx, &moveCheckResult))
            break;

        if (cs.moralFailure && moveCheckResult != 0) {
            // Morale-failure running: try to set fleeing.
            if (g_combatSession)
                actionFinished = g_combatSession->trySetFleeing(actor);
            else {
                cs.fleeing = true;
                actionFinished = true;
            }
        } else {
            attempt++;
        }
    }

    if (!actionFinished) {
        const uint8 aiStateIdx = (cs.aiState >= 1 && cs.aiState <= 6) ? cs.aiState - 1 : 0;
        const uint8 offset = kAiDirectionOffset[aiStateIdx][attempt < 6 ? attempt - 1 : 4];
        const uint8 selectedDirection = (uint8)((desiredDirection + offset) % 8);

        // All attempts failed, or selected direction is opposite the stored AI direction.
        if (attempt == 6 ||
            globals.aiDirection == (uint8)((selectedDirection + 4) % 8)) {

            globals.aiFailureCount++;
            cs.aiState = (uint8)(cs.aiState % 6 + 1);

            if (globals.aiFailureCount > 1) {
                cs.target = nullptr;

                if (globals.aiFailureCount < 3) {
                    // Try to select another target.
                    ctx.buildTargetList(actor, 0xFF);
                    if (ctx.targetList.targetOrder.empty()) {
                        updateActionState(actor);
                        actionFinished = true;
                    }
                    // If a new target was found it is now in targetList;
                    // the caller's next tick will re-enter with the new target.
                } else {
                    cs.movePoints = 0;
                    actionFinished = true;
                }
            }
        }

        if (!actionFinished) {
            if (attempt < 6) {
                globals.aiDirection = selectedDirection;
            } else {
                actionFinished = true;
            }
        }

    } else {
        // Running / morale-failure action finished the turn.
        cs.movePoints = 0;
        cs.moralFailure = false;
        cs.endTurn();
        return;
    }

    if (actionFinished)
        return;

    // Apply the selected facing.
    if (view)
        view->updateCharacterFacingAndRedraw(actor, globals.aiDirection);
    else
        ctx.setCharacterFacing(actor, static_cast<Direction>(globals.aiDirection));

    // Disengagement reactions (movement away from an engaged enemy).
    ctx.handleDisengagementReactions(
        actor,
        static_cast<Direction>(cs.direction));

    if (!actor->enabled) {
        cs.endTurn();
        return;
    }

    // Apply one movement step (includes cloud check and guard reactions).
    if (g_combatSession)
        g_combatSession->applyMoveStep(actor, globals.aiDirection);

    if (!actor->enabled)
        cs.endTurn();
}

// ---------------------------------------------------------------------------
// executeAiTurn
// ---------------------------------------------------------------------------

AiTurnResult executeAiTurn(Data::PlayerCharacter *actor, CombatContext &ctx) {
    AiTurnResult result;

    if (!actor || !actor->combatState)
        return result;

    Data::CombatAction &cs = *actor->combatState;

    // 1. Morale check — if already failed, flee.
    if (cs.moralFailure) {
        cs.fleeing = true;
        cs.initiative = 0xFF; // mark as acted
        result.action = AiTurnResult::ACTION_FLEE;
        return result;
    }

    // 2. Build melee target list (range = 1).
    ctx.buildTargetList(actor, 1);

    if (!ctx.targetList.targetOrder.empty()) {
        // 3. Attack the first (nearest) target.
        uint8 targetIdx = ctx.targetList.targetOrder[0];
        Data::PlayerCharacter *target = ctx.table.getCharacter(targetIdx);

        if (target && target->enabled) {
            ctx.globals.attacker = actor;
            ctx.globals.behaviorFlags = 0;

            if (rollToHit(actor, target, (uint8)target->armorClass.getCurrent())) {
                uint8 dmg = rollDamage(actor);
                DamageResult dr = applyDamage(ctx, target, dmg,
                                              DAMAGE_NORMAL, false, nullptr);
                result.action = AiTurnResult::ACTION_ATTACK;
                result.target = target;
                result.damage = dr.finalDamage;
                result.targetWentDown = dr.wentDown;

                if (target->combatSide == Data::CS_ENEMY)
                    ctx.globals.handicapValue = 0;
            } else {
                result.action = AiTurnResult::ACTION_ATTACK;
                result.target = target;
                result.damage = 0;
            }
        }
    } else {
        // 4. No target in melee range — use processAiMove for the movement step.
        processAiMove(actor, ctx, nullptr);
        result.action = AiTurnResult::ACTION_MOVE;
    }

    // 5. Mark as acted this round.
    cs.initiative = 0xFF;
    return result;
}

} // namespace Combat
} // namespace Goldbox
