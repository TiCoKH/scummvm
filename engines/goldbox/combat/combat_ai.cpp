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
#include "goldbox/combat/cloud_effect_manager.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
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
 * Mirrors AI_CheckMoveStep.
 * Evaluates one AI movement candidate without moving the character.
 *
 * Selects the candidate direction from kAiDirectionOffset using the
 * character's current aiState and the attempt number, then queries
 * ground info in that direction.
 *
 * Returns true if the movement step is affordable (movementCost <= movePoints).
 * Sets *outMoveResult to a non-zero value for the morale-failure running
 * condition (destination is at the map edge).
 * Sets *outDirection to the candidate direction that was evaluated.
 *
 * @param actor          Character attempting the move
 * @param attempt        Which candidate (1-5)
 * @param desiredDir     Base direction toward target (or flee direction)
 * @param ctx            Live combat context (for ground info and tile props)
 * @param outMoveResult  Secondary output: non-zero signals running condition
 * @param outDirection   The candidate direction that was evaluated
 */
static bool checkMoveStep(Data::PlayerCharacter *actor,
                           uint8 attempt,
                           uint8 desiredDir,
                           CombatContext &ctx,
                           uint8 *outMoveResult) {
    *outMoveResult = 0;

    const uint8 aiStateIdx = (actor->combatState->aiState >= 1 &&
                              actor->combatState->aiState <= 6)
                             ? actor->combatState->aiState - 1 : 0;
    const uint8 offset = kAiDirectionOffset[aiStateIdx][attempt - 1];
    const uint8 candidateDir = (uint8)((desiredDir + offset) % 8);

    int groundInfo = 0;
    uint8 tileId = 0;
    ctx.getGroundInfo(actor, candidateDir, &groundInfo, &tileId);

    // No tile at destination: out of bounds.
    if (tileId == 0)
        return false;

    // Destination occupied.
    if (groundInfo != 0)
        return false;

    const TilePropertyProvider *tileProps = ctx.params.tilePropertyProvider;
    if (!tileProps)
        return false;

    const TileProp *prop = tileProps->getTileProp(tileId - 1);
    if (!prop || prop->passable <= 0)
        return false;

    // passable is a movement cost factor, not a boolean.
    // cardinal = 2, diagonal = 3; total cost = directionCost * moveCostFactor.
    const uint8 moveCostFactor = (uint8)prop->passable;
    const uint8 directionCost  = (candidateDir & 1) ? 3 : 2;
    uint8 movementCost = directionCost * moveCostFactor;

    // Cloud tile (0x1E): make the step unaffordable unless the character
    // already has one of the immunity effects (0x20, 0x1E, 0x6F, 0x7D).
    if (tileId == CloudEffectManager::kTileCloud) {
        const Data::Effects::CharacterEffects *fx = actor->getEffects();
        if (!fx || (!fx->hasEffect(0x20) && !fx->hasEffect(0x1E) &&
                    !fx->hasEffect(0x6F) && !fx->hasEffect(0x7D))) {
            movementCost = actor->combatState->movePoints + 1;
        }
    }

    if (movementCost > actor->combatState->movePoints)
        return false;

    // Morale-failure running: signal when destination is at the map edge.
    if (actor->combatState->moralFailure) {
        const int idx = ctx.table.findIndex(actor);
        if (idx >= 0) {
            const TilePos src = ctx.table.getTilePos(idx);
            const TilePos dst((uint8)(src.col + kDirDeltaX[candidateDir]),
                              (uint8)(src.row + kDirDeltaY[candidateDir]));
            if (dst.col == 0 || dst.col == 49 || dst.row == 0 || dst.row == 24)
                *outMoveResult = 1;
        }
    }

    return true;
}

/** Mirrors the post-loop direction table lookup in COMBAT_ProcessAIMove. */
static uint8 calcAiDirection(uint8 aiState, uint8 attempt, uint8 desiredDir) {
    const uint8 aiStateIdx = (aiState >= 1 && aiState <= 6) ? aiState - 1 : 0;
    return (uint8)((desiredDir + kAiDirectionOffset[aiStateIdx][attempt - 1]) % 8);
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
                          const Common::Array<Data::PlayerCharacter *> &roster,
                          char key,
                          CombatGlobals &globals,
                          AiMoveViewDelegate *view) {
    if (key == 0)
        return false;

    // Autospell toggle.
    if (key == 'm' || key == 'M') {
        globals.magicEnabled = !globals.magicEnabled;
        if (view)
            view->showMessage(globals.magicEnabled ? "Magic On" : "Magic Off");
        return false;
    }

    // Space: return eligible party members from AI to player control.
    if (key == ' ') {
        for (uint i = 0; i < roster.size(); ++i) {
            Data::PlayerCharacter *ch = roster[i];
            if (!ch)
                continue;
            // npc < 0x80 = player character; status != STATUS_ANIMATED.
            if (ch->npc >= 0 && (uint8)ch->npc < 0x80 &&
                ch->healthStatus != Data::S_ANIMATED) {
                ch->ai_control = false;
            }
        }

        if (!actor->ai_control) {
            if (actor->combatState)
                actor->combatState->initiative = 20;
            return true;
        }
        return false;
    }

    // '-': cheat handler — no-op in modern engine.
    return false;
}

// ---------------------------------------------------------------------------
// updateActionState
// ---------------------------------------------------------------------------

static bool concludeAiTurn(Data::PlayerCharacter *actor) {
    if (!actor || !actor->combatState)
        return false;

    Data::CombatAction &cs = *actor->combatState;
    const Data::ADnDCharacter *adnd = dynamic_cast<const Data::ADnDCharacter *>(actor);
    const bool hasRanged = adnd ? adnd->hasRangedWeapon() : false;

    if (!actor->hasNegativeEffect() && !hasRanged && cs.initiative != 0) {
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
                   const Common::Array<Data::PlayerCharacter *> &roster,
                   char key,
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
    if (handleAiControlInput(actor, roster, key, globals, view))
        return;

    // Bypass: no movement, initiative expired, or normal mage.
    if ((cs.movePoints >> 1) == 0 ||
        cs.initiative < 1 ||
        (!cs.moralFailure && actor->classType == Data::C_MAGICUSER)) {
        concludeAiTurn(actor);
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

    while (attempt < 6 && !actionFinished &&
           !checkMoveStep(actor, attempt, desiredDirection, ctx, &moveCheckResult)) {
        if (cs.moralFailure && moveCheckResult != 0) {
            if (g_combatSession) {
                CombatSession::TryFleeResult fr = g_combatSession->trySetFleeing(actor);
                actionFinished = fr.actionComplete;
            } else {
                cs.fleeing = true;
                actionFinished = true;
            }
        } else {
            attempt++;
        }
    }

    if (!actionFinished) {
        // Direction table lookup happens here (post-loop), mirroring original.
        const uint8 selectedDirection = calcAiDirection(cs.aiState, attempt, desiredDirection);

        // All attempts failed, or selected direction reverses stored AI direction.
        if (attempt == 6 ||
            globals.aiDirection == (uint8)((selectedDirection + 4) % 8)) {

            globals.aiFailureCount++;
            cs.aiState = (uint8)(cs.aiState % 6 + 1);

            if (globals.aiFailureCount > 1) {
                cs.target = nullptr;

                if (globals.aiFailureCount < 3) {
                    if (!ctx.selectTargetForAction(actor, 0xFF, false, false))
                        actionFinished = concludeAiTurn(actor);
                } else {
                    cs.movePoints = 0;
                    actionFinished = true;
                }
            }
        }

        if (!actionFinished) {
            if (attempt < 6)
                globals.aiDirection = selectedDirection;
            else
                actionFinished = true;
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
        processAiMove(actor, ctx.params.roster, 0, ctx, nullptr);
        result.action = AiTurnResult::ACTION_MOVE;
    }

    // 5. Mark as acted this round.
    cs.initiative = 0xFF;
    return result;
}

} // namespace Combat
} // namespace Goldbox
