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
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/items/base_items.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/spells/spell.h"
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
// moveByAI  (mirrors COMBAT_MoveByAI)
// ---------------------------------------------------------------------------

void moveByAI(Data::PlayerCharacter *actor,
              CombatContext &ctx,
              AiMoveViewDelegate *view) {
    if (!actor || !actor->combatState)
        return;

    Data::CombatAction &cs = *actor->combatState;

    // 1. Allow player/control mechanism to reclaim this actor.
    bool turnHandled = handleAiControlInput(
        actor, ctx.params.roster, 0, ctx.globals, view);

    // 2. Presentation: clear prompt and message areas.
    if (view)
        view->clearPromptAndMessage();

    // 3. Disabled actors have their turn ended.
    //    Execution does NOT return here — original falls through.
    if (!actor->enabled)
        turnHandled = (cs.endTurn(), true);

    // 4. Select or refresh the AI action category.
    //    Values 1..4 are kept 75% of the time; otherwise re-roll.
    uint8 aiAction = cs.aiState;
    if (aiAction == 0 || aiAction > 4 ||
        VmInterface::rollDice(1, 4) == 1) {
        aiAction = (uint8)VmInterface::rollDice(1, 8);
        if (aiAction == 8)
            aiAction = (uint8)(VmInterface::rollDice(1, 2) + 4);
        else
            aiAction = (uint8)VmInterface::rollDice(1, 4);
    }
    cs.aiState = aiAction;

    // 5. Morale/surrender check.
    if (!turnHandled) {
        turnHandled = checkSurrender(actor, ctx, nullptr);
    }

    // Presentation: moral failure without full flee → "Flees in panic".
    if (cs.moralFailure && !cs.fleeing && view)
        view->showFleesInPanic(actor);

    if (turnHandled)
        return;

    // 6. Try using a magic item.
    if (tryUseItem(actor, ctx)) {
        cs.endTurn();
        return;
    }

    // 7. Execute a pending spell (stored from a previous timed cast).
    if (cs.spellId != 0) {
        // TODO: call useSpell(actor, cs.spellId, true) when implemented.
        cs.spellId = 0;
        cs.endTurn();
        return;
    }

    // 8. Try Turn Undead.
    if (tryTurnUndead(actor, ctx)) {
        cs.endTurn();
        return;
    }

    // 9. Select and cast a spell from the spell book.
    if (selectSpellFromBook(actor, ctx))
        return;

    // 10. Ensure the AI has appropriate weapons equipped.
    autoEquipWeapons(dynamic_cast<Data::ADnDCharacter *>(actor), ctx, nullptr);

    // 11. Second control-reclaim opportunity before movement/attack.
    turnHandled = handleAiControlInput(
        actor, ctx.params.roster, 0, ctx.globals, view);

    // 12. Movement/attack loop.
    while (!turnHandled) {
        const bool hasTarget =
            ctx.selectTargetForAction(actor, 0xFF, true, false);

        if (!hasTarget || cs.initiative < 1)
            turnHandled = (concludeAiTurn(actor), true);
        else
            turnHandled = processAiTurn(actor, ctx, view);
    }
}

// ---------------------------------------------------------------------------
// processAiTurn  (mirrors COMBAT_ProcessAITurn)
// ---------------------------------------------------------------------------

bool processAiTurn(Data::PlayerCharacter *actor,
                   CombatContext &ctx,
                   AiMoveViewDelegate *view) {
    if (!actor || !actor->combatState)
        return true;

    Data::CombatAction &cs    = *actor->combatState;
    CombatGlobals       &globals = ctx.globals;

    // AI state initialization.
    globals.aiDirection    = 0xFF; // W8_NONE
    globals.aiFailureCount = 0;

    uint8 iterationCount = 0;
    bool  aiActive       = (cs.initiative != 0);
    bool  turnComplete   = false;

    // ES_ON_SPECIAL_ATTACK (14): initialize special-attack effect state.
    Data::Effects::EffectRuntime *effectRuntime = ctx.params.effectRuntime;
    if (effectRuntime && actor->getEffects())
        effectRuntime->checkEffectSet(Data::Effects::ES_ON_SPECIAL_ATTACK,
                                      *actor->getEffects(), *actor, &globals);

    // Turn-undead attempt (cleric AI action, consumes the turn if triggered).
    if (tryTurnUndead(actor, ctx))
        return true;

    while (!turnComplete && aiActive) {
        // Morale failure: keep processing AI movement while movement and
        // initiative remain (initiative must be 1..19).
        if (cs.moralFailure) {
            while (cs.movePoints != 0 &&
                   (int8)cs.initiative > 0 &&
                   (int8)cs.initiative < 20) {
                processAiMove(actor, ctx.params.roster, 0, ctx, view);
            }
        }

        // Initiative values 0 and 20 terminate AI control.
        if (cs.initiative == 0 || cs.initiative == 20) {
            aiActive = false;
        } else {
            turnComplete = false;
        }

        if (turnComplete || !aiActive)
            break;

        // Safety limit: prevent infinite AI loops.
        if (++iterationCount > 20) {
            turnComplete = true;
            aiActive     = !concludeAiTurn(actor);
            break;
        }

        // Determine current attack range from equipped weapon.
        uint8 attackRange = 1;
        {
            Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(actor);
            const Data::Items::CharacterItem *weapon =
                adnd ? adnd->getEquippedItem(Data::Items::Slot::S_MAIN_HAND) : nullptr;
            if (weapon) {
                uint8 r = weapon->prop().range;
                attackRange = (r > 0) ? r - 1 : 1;
                if (attackRange == 0 || attackRange == 0xFF)
                    attackRange = 1;
            }
        }

        bool canAttack = false;

        // --- Decision 1: can we attack the current target? ---
        Data::PlayerCharacter *target = cs.target;

        if (target && target->enabled &&
            target->combatSide != actor->combatSide) {

            if (ctx.canEngageTarget(actor, target)) {
                uint16 losRange = attackRange;
                TilePos blockedAt;
                const int attackerIdx = ctx.table.findIndex(actor);
                const int targetIdx   = ctx.table.findIndex(target);

                if (attackerIdx >= 0 && targetIdx >= 0) {
                    TilePos attackerPos = ctx.table.getTilePos(attackerIdx);
                    TilePos targetPos   = ctx.table.getTilePos(targetIdx);

                    bool los = ctx.lineOfSightCheck(attackerPos, targetPos,
                                                    losRange, &blockedAt);
                    if (los && (losRange >> 1) <= attackRange)
                        canAttack = true;
                }
            }
        }

        // --- Decision 2: no attackable current target — find one ---
        if (!canAttack) {
            ctx.buildTargetList(actor, attackRange);
            const uint8 targetCount = (uint8)ctx.targetList.targetOrder.size();

            if (targetCount == 0) {
                // No candidates at all: try selecting a distant target and move.
                if (ctx.selectTargetForAction(actor, 0xFF, false, false)) {
                    processAiMove(actor, ctx.params.roster, 0, ctx, view);
                } else {
                    turnComplete = concludeAiTurn(actor);
                }
            } else {
                // Pick a random candidate from the target list.
                const uint8 roll = (uint8)VmInterface::rollDice(
                    1, (int)targetCount);
                const uint8 idx  = (roll > 0 && roll <= targetCount)
                                   ? roll - 1 : 0;
                target = ctx.table.getCharacter(
                    ctx.targetList.targetOrder[idx]);

                // --- Decision 3: ranged weapon auto-equip ---
                Data::ADnDCharacter *adnd =
                    dynamic_cast<Data::ADnDCharacter *>(actor);

                if (adnd && adnd->hasRangedWeapon() &&
                    !adnd->isEquippedRangedWeapon()) {
                    ctx.buildTargetList(actor, 1);
                    if (!ctx.targetList.targetOrder.empty()) {
                        autoEquipWeapons(adnd, ctx, nullptr);
                        if (view)
                            view->drawCombatInfo(actor);
                        turnComplete = true;
                    }
                }

                if (!turnComplete && target) {
                    // Check whether the selected candidate is attackable.
                    const uint8 tRange = ctx.getTargetRange(actor, target);
                    if (tRange == 1 || ctx.canEngageTarget(actor, target))
                        canAttack = true;
                }
            }
        }

        // --- Attack the selected target ---
        if (canAttack && target) {
            const Direction facing = ctx.getFacingToward(actor, target);

            // Presentation: update facing and redraw viewport.
            if (view)
                view->updateCharacterFacingAndRedraw(actor, (uint8)facing);
            else
                ctx.setCharacterFacing(actor, facing);

            // Multi-attack attempt.
            const bool multiHandled = executeMultiAttack(
                actor, target, ctx,
                nullptr /* view delegate handled separately */);

            if (multiHandled) {
                turnComplete = concludeAiTurn(actor);
            } else {
                ctx.applyAttackFacingChange(actor, target);

                // Determine ranged attack item.
                Data::Items::CharacterItem *attackItem = nullptr;
                Data::ADnDCharacter *adnd =
                    dynamic_cast<Data::ADnDCharacter *>(actor);
                if (adnd && adnd->hasRangedWeapon()) {
                    adnd->getRangedAttackItem(&attackItem);
                    // At range 1 with a ranged weapon, use melee (no ammo).
                    if (adnd->isEquippedRangedWeapon() &&
                        ctx.getTargetRange(actor, target) == 1)
                        attackItem = nullptr;
                }

                resolveAttack(actor, target, false, attackItem,
                              &turnComplete,
                              nullptr /* view delegate */);

                if (!turnComplete && !target->enabled)
                    turnComplete = true;
                else if (turnComplete)
                    aiActive = false;
            }
        }
    }

    return !aiActive;
}

// ---------------------------------------------------------------------------
// executeAiTurn  (thin wrapper)
// ---------------------------------------------------------------------------

AiTurnResult executeAiTurn(Data::PlayerCharacter *actor, CombatContext &ctx) {
    AiTurnResult result;
    if (!actor || !actor->combatState)
        return result;

    Data::PlayerCharacter *targetBefore = actor->combatState->target;

    const bool ended = processAiTurn(actor, ctx, nullptr);

    // Infer action from post-turn state.
    if (actor->combatState->fleeing) {
        result.action = AiTurnResult::ACTION_FLEE;
    } else if (actor->combatState->guarding) {
        result.action = AiTurnResult::ACTION_GUARD;
    } else if (targetBefore && !targetBefore->enabled) {
        result.action       = AiTurnResult::ACTION_ATTACK;
        result.target       = targetBefore;
        result.targetWentDown = true;
    } else if (targetBefore) {
        result.action = AiTurnResult::ACTION_ATTACK;
        result.target = targetBefore;
    } else {
        result.action = AiTurnResult::ACTION_MOVE;
    }

    (void)ended;
    return result;
}

// ---------------------------------------------------------------------------
// computeWeaponScore
// ---------------------------------------------------------------------------

uint8 computeWeaponScore(const Data::ADnDCharacter &character,
                         const Data::Items::CharacterItem &item) {
    using namespace Data::Items;

    const ItemProperty &prop = item.prop();

    uint8 score = prop.dmgSmallMed.dices * prop.dmgSmallMed.sides;

    if (item.bonus > 0)
        score += (uint8)(item.bonus * 8);

    if (prop.dmgSmallMed.bonus > 0)
        score += (uint8)(prop.dmgSmallMed.bonus * 2);

    // Special weapon 0x55 gets fixed score 8 when the current combat target
    // is undead.
    if (item.typeIndex == 0x55 && character.combatState && character.combatState->target) {
        const Data::ADnDCharacter *tgt =
            dynamic_cast<const Data::ADnDCharacter *>(character.combatState->target);
        if (tgt && tgt->levelUndead > 0)
            score = 8;
    }

    // Ranged bonus: (range - 1) * 2 when MF_BOW bit is set.
    if (prop.missileType & static_cast<uint8>(MissileFlag::MF_BOW))
        score += (uint8)((prop.range > 0 ? prop.range - 1 : 0) * 2);

    // One-handed preference.
    if (prop.hands < 2)
        score += 3;

    // Reject if adding this weapon exceeds the hand capacity of 3.
    if (character.handsEquipped + prop.hands > 3)
        score = 0;

    // Alignment restriction: effect3==0x84 means effect2 holds the allowed
    // alignment mask.
    if (item.effect3 == 0x84) {
        const uint8 CHAOTIC_EVIL    = 0x01;
        const uint8 CHAOTIC_NEUTRAL = 0x02;
        if ((item.effect2 & (CHAOTIC_EVIL | CHAOTIC_NEUTRAL)) != character.alignment)
            score = 0;
    }

    // Forbidden effect tag.
    if (item.effect2 == 0x53)
        score = 0;

    if (item.cursed)
        score = 0;

    return score;
}

// ---------------------------------------------------------------------------
// autoEquipWeapons
// ---------------------------------------------------------------------------

void autoEquipWeapons(Data::ADnDCharacter *character,
                      CombatContext &ctx,
                      void (*drawCombatInfoCallback)(Data::PlayerCharacter *)) {
    using namespace Data::Items;

    if (!character)
        return;

    // Temporarily remove currently equipped items from hand count.
    // recalcCombatStats() will rebuild the authoritative value later.
    const CharacterItem *curWeapon  = character->getEquippedItem(Slot::S_MAIN_HAND);
    const CharacterItem *curShield  = character->getEquippedItem(Slot::S_OFF_HAND);

    if (curWeapon)
        character->handsEquipped -= character->getEquippedProp(Slot::S_MAIN_HAND)->hands;
    if (curShield)
        character->handsEquipped -= character->getEquippedProp(Slot::S_OFF_HAND)->hands;

    // Baseline ordinary-weapon score from current unarmed/base damage.
    uint8 ordinaryScore = (uint8)(character->curPrimaryRoll.action.roll.diceNum *
                                  character->curPrimaryRoll.action.roll.diceSides);
    if (character->curPrimaryRoll.action.modifier > 0)
        ordinaryScore += (uint8)(character->curPrimaryRoll.action.modifier * 2);

    // Inventory scan.
    CharacterItem *bestRanged   = nullptr;
    CharacterItem *bestOrdinary = nullptr;
    CharacterItem *bestOffhand  = nullptr;
    uint8 bestRangedScore  = 1;
    uint8 bestOffhandScore = 0;

    for (CharacterItem &item : character->inventory.items()) {
        const ItemProperty &prop = item.prop();

        if (static_cast<Slot>(prop.slotID) == Slot::S_MAIN_HAND &&
            (prop.classMask & character->getAllowedItemClassMask()) != 0) {

            uint8 sc = computeWeaponScore(*character, item);

            const bool isMissileBow  = (prop.missileType & static_cast<uint8>(MissileFlag::MF_BOW))  != 0;
            const bool isMissileDart = (prop.missileType & static_cast<uint8>(MissileFlag::MF_DART)) != 0;

            if (isMissileBow || isMissileDart) {
                if (sc > bestRangedScore) {
                    bestRanged      = &item;
                    bestRangedScore = sc;
                }
            }

            if (!isMissileBow && sc > ordinaryScore) {
                bestOrdinary  = &item;
                ordinaryScore = sc;
            }
        }

        if (static_cast<Slot>(prop.slotID) == Slot::S_OFF_HAND &&
            (character->getAllowedItemClassMask() & prop.classMask) != 0) {

            uint8 sc = (item.bonus < 0) ? 0 : (uint8)(item.bonus + 1);
            if (sc > bestOffhandScore) {
                bestOffhand      = &item;
                bestOffhandScore = sc;
            }
        }
    }

    // Determine whether the best ranged weapon is usable (range>=2 and
    // MF_BOW|MF_THROWING both set, i.e. missileType & 0x14 == 0x14).
    // Null out bestRanged if it fails so downstream logic treats it as absent.
    if (bestRanged) {
        const ItemProperty &rp = bestRanged->prop();
        if (rp.range < 2 || (rp.missileType & 0x14) != 0x14)
            bestRanged = nullptr;
    }

    // Determine associated ammunition.
    uint8 missileType = 0;
    CharacterItem *rangedAmmoItem = nullptr;
    if (bestRanged) {
        missileType = bestRanged->prop().missileType;

        if (missileType & static_cast<uint8>(MissileFlag::MF_DART))
            rangedAmmoItem = bestRanged;

        if (missileType & static_cast<uint8>(MissileFlag::MF_BOW)) {
            if (missileType & static_cast<uint8>(MissileFlag::MF_RANGED_MELEE))
                rangedAmmoItem = character->getEquippedItem(Slot::S_ARROW);
            if (missileType & static_cast<uint8>(MissileFlag::MF_CROSSBOW))
                rangedAmmoItem = character->getEquippedItem(Slot::S_BOLT);
        }
    }

    // Special original-game exception: missileType 0x0A is valid without ammo.
    const bool hasRangedEquipment = (rangedAmmoItem != nullptr) || (missileType == 0x0A);

    // Choose ranged vs ordinary weapon.
    CharacterItem *selectedWeapon = bestRanged;

    bool chooseOrdinary = (bestRanged == nullptr) ||
                          (bestRangedScore <= (ordinaryScore >> 1));

    if (!chooseOrdinary && !hasRangedEquipment) {
        ctx.buildTargetList(character, 1);
        if (!ctx.targetList.targetOrder.empty())
            chooseOrdinary = true;
    }

    if (chooseOrdinary)
        selectedWeapon = bestOrdinary;

    // Weapon switching restrictions.
    bool canChangeWeapon = true;
    if (curWeapon != nullptr &&
        (selectedWeapon == curWeapon || curWeapon->cursed))
        canChangeWeapon = false;

    bool equipmentChanged = false;

    if (canChangeWeapon) {
        if (curWeapon != nullptr)
            character->toggleReadyItem(const_cast<CharacterItem *>(curWeapon));

        character->recalcCombatStats();

        // Remove shield hand contribution before readying new weapon.
        const CharacterItem *shieldNow = character->getEquippedItem(Slot::S_OFF_HAND);
        if (shieldNow && !shieldNow->cursed)
            character->handsEquipped -= character->getEquippedProp(Slot::S_OFF_HAND)->hands;

        if (selectedWeapon != nullptr)
            character->toggleReadyItem(selectedWeapon);

        equipmentChanged = true;
    }

    character->recalcCombatStats();
    recalcPrimaryAttacks(character);

    // Shield switching restrictions.
    const CharacterItem *shieldAfter = character->getEquippedItem(Slot::S_OFF_HAND);
    bool canChangeShield = true;
    if (shieldAfter != nullptr &&
        (bestOffhand == shieldAfter || shieldAfter->cursed))
        canChangeShield = false;

    if (character->handsEquipped < 3) {
        if (character->handsEquipped < 2 && canChangeShield) {
            if (shieldAfter != nullptr)
                character->toggleReadyItem(const_cast<CharacterItem *>(shieldAfter));

            character->recalcCombatStats();

            if (bestOffhand != nullptr)
                character->toggleReadyItem(bestOffhand);

            equipmentChanged = true;
        }
    } else {
        // Over hand capacity: resolve by toggling off shield if possible,
        // otherwise toggle the selected weapon.
        if (shieldAfter == nullptr || shieldAfter->cursed) {
            if (selectedWeapon != nullptr)
                character->toggleReadyItem(selectedWeapon);
        } else {
            character->toggleReadyItem(const_cast<CharacterItem *>(shieldAfter));
        }
        equipmentChanged = true;
    }

    character->recalcCombatStats();

    if (equipmentChanged && drawCombatInfoCallback)
        drawCombatInfoCallback(character);
}

// ---------------------------------------------------------------------------
// findNearbyWoundedAlly  (mirrors AI_FindNearbyWoundedAlly)
// ---------------------------------------------------------------------------

bool findNearbyWoundedAlly(Data::PlayerCharacter *attacker,
                        CombatContext &ctx,
                        Data::PlayerCharacter **outTarget) {
    *outTarget = nullptr;

    const int attackerIdx = ctx.table.findIndex(attacker);
    if (attackerIdx < 0)
        return false;

    const TilePos src = ctx.table.getTilePos(attackerIdx);

    Data::PlayerCharacter *bestTarget = nullptr;
    uint8 bestHp = 0xFF;
    Data::PlayerCharacter *downedCandidate = nullptr;

    // Scan all 8 neighbours plus the attacker's own tile (direction 0-8).
    for (int dir = 0; dir < 9; ++dir) {
        const TilePos pos((uint8)(src.col + kDirDeltaX[dir]),
                          (uint8)(src.row + kDirDeltaY[dir]));

        const CombatContext::CombatCell cell = ctx.getTileAndOccupantAt(pos);

        if (cell.occupantId == 0) {
            // Tile 0x1F: downed-member tile — scan downed member records.
            if (cell.tileId == CombatantTable::TILE_DOWNED_MEMBER) {
                const Common::Array<CombatGlobals::DownedMemberRecord> &downed =
                    ctx.globals.downedMembers;
                for (uint i = 0; i < downed.size(); ++i) {
                    Data::PlayerCharacter *ch = downed[i].character;
                    if (!ch)
                        continue;
                    if (ch->healthStatus == Data::S_DEAD ||
                        ch->healthStatus == Data::S_GONE ||
                        ch->healthStatus == Data::S_STONED)
                        continue;
                    if (downed[i].pos.col == pos.col &&
                        downed[i].pos.row == pos.row)
                        downedCandidate = ch;
                }
            }
        } else {
            // occupantId is 1-based.
            Data::PlayerCharacter *candidate =
                ctx.table.getCharacter(cell.occupantId - 1);
            if (!candidate)
                continue;
            if (candidate->combatSide != attacker->combatSide)
                continue;
            if (candidate->hitPoints.current >= candidate->hitPoints.max)
                continue;

            const bool isLowerHp = candidate->hitPoints.current < bestHp;
            const bool selfCritical =
                (candidate == attacker) &&
                candidate->hitPoints.current < (candidate->hitPoints.max >> 1);

            if (isLowerHp || selfCritical) {
                if (isLowerHp)
                    bestHp = candidate->hitPoints.current;
                bestTarget = candidate;
            }
        }
    }

    // Prefer downed member when best standing HP is not critical (>= 8).
    if (bestHp < 8 || downedCandidate == nullptr)
        *outTarget = bestTarget;
    else
        *outTarget = downedCandidate;

    return *outTarget != nullptr;
}

// ---------------------------------------------------------------------------
// hasEnemyTargetForSpell  (mirrors AI_HasEnemyTargetForSpell)
// ---------------------------------------------------------------------------

/**
 * Builds a target list centered at pos using spell.targetType as the
 * footprint/iconSize code and DIR_ANY as the arc, then returns true if
 * at least one resulting target is on the opposing side from actor.
 *
 * Called by isSpellEligibleForAI when spell.minAITargets != 0; the spell
 * is rejected when this returns true for any entry in the target list.
 */
static bool hasEnemyTargetForSpell(Data::PlayerCharacter *actor,
                                   TilePos pos,
                                   const Data::Spells::SpellEntry &spell,
                                   CombatContext &ctx) {
    TargetList saved;
    saved.entries.swap(ctx.targetList.entries);
    saved.targetOrder.swap(ctx.targetList.targetOrder);

    ctx.buildTargetListCore(pos, (uint8)spell.targetType, DIR_ANY, spell.fixedRange);

    bool hasEnemy = false;
    for (uint i = 0; i < ctx.targetList.entries.size(); ++i) {
        const Data::PlayerCharacter *target =
            ctx.table.getCharacter(ctx.targetList.entries[i].idx);
        if (target && target->combatSide != actor->combatSide) {
            hasEnemy = true;
            break;
        }
    }

    saved.entries.swap(ctx.targetList.entries);
    saved.targetOrder.swap(ctx.targetList.targetOrder);
    return hasEnemy;
}

// ---------------------------------------------------------------------------
// isSpellEligibleForAI  (mirrors AI_SpellEligible)
// ---------------------------------------------------------------------------

bool isSpellEligibleForAI(Data::PlayerCharacter *actor,
                          uint8 spellId,
                          uint8 priorityThreshold,
                          CombatContext &ctx) {
    using namespace Data::Spells;

    const Common::Array<SpellEntry> &spells = Data::Rules::getSpellEntries();
    if (spellId >= spells.size())
        return false;

    const SpellEntry &spell = spells[spellId];

    if (priorityThreshold > spell.priority)
        return false;

    // Spell must be offensive or be SP_CL1_CURE_LT_WOUNDS (id 3).
    if (spellId != (uint8)SP_CL1_CURE_LT_WOUNDS && spell.isOffensive == 0)
        return false;

    // SP_CL1_CURE_LT_WOUNDS is rejected when a nearby wounded ally exists
    // (caster's tile + 8 neighbours). The spell is only eligible when no
    // ally in range needs healing — counterintuitive but matches original.
    if (spellId == (uint8)SP_CL1_CURE_LT_WOUNDS) {
        Data::PlayerCharacter *adj = nullptr;
        if (findNearbyWoundedAlly(actor, ctx, &adj) && adj != nullptr)
            return false;
    }

    // Build target list using the spell's fixed range.
    ctx.buildTargetList(actor, spell.fixedRange);
    const uint8 targetCount = (uint8)ctx.targetList.targetOrder.size();

    if (targetCount == 0)
        return false;

    // When minAITargets == 0 any non-empty target list is sufficient.
    if (spell.minAITargets == 0)
        return true;

    // minAITargets != 0: reject the spell if any target in the list has an
    // opposing-side character within the spell's area.
    const Common::Array<uint8> order = ctx.targetList.targetOrder;
    for (uint i = 0; i < order.size(); ++i) {
        const TilePos targetPos = ctx.table.getTilePos(order[i]);
        if (hasEnemyTargetForSpell(actor, targetPos, spell, ctx))
            return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// tryUseItem  (mirrors AI_TryUseItem)
// ---------------------------------------------------------------------------

bool tryUseItem(Data::PlayerCharacter *actor, CombatContext &ctx) {
    if (!actor || !actor->combatState)
        return false;

    if (!actor->combatState->canUse)
        return false;

    if (ctx.globals.sideCount[actor->combatSide] == 0)
        return false;

    if (!ctx.globals.magicEnabled)
        return false;

    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(actor);
    if (!adnd)
        return false;

    const uint8 searchPasses = (uint8)VmInterface::rollDice(1, 7);

    Data::Items::CharacterItem *selectedItem = nullptr;
    uint8 priorityThreshold = 7;

    for (uint8 pass = 0;
         pass < searchPasses && selectedItem == nullptr;
         ++pass, --priorityThreshold) {

        for (Common::List<Data::Items::CharacterItem>::iterator it =
                 adnd->inventory.items().begin();
             it != adnd->inventory.items().end() && selectedItem == nullptr;
             ++it) {

            Data::Items::CharacterItem &item = *it;

            if (item.isMissileOrScroll())
                continue;

            // effect3 >= 0x80 means the item state is not usable.
            if (item.effect3 >= 0x80)
                continue;

            if (item.readied == 0)
                continue;

            // effect2 holds the spell/effect ID for magic items.
            uint8 spellId = item.effect2;
            if (spellId == 0)
                continue;

            // Normalize item-stored spell IDs above 0x38 (56) into the
            // regular spell table range by subtracting 0x17 (23).
            if (spellId > 0x38)
                spellId -= 0x17;

            if (isSpellEligibleForAI(actor, spellId, priorityThreshold, ctx))
                selectedItem = &item;
        }
    }

    if (selectedItem == nullptr)
        return false;

    // TODO: call ITEM_Use(selectedItem) when that function is implemented.
    // The original ignores the return value; we return true unconditionally.
    return true;
}

// ---------------------------------------------------------------------------
// selectSpellFromBook  (mirrors AI_SelectSpellFromBook)
// ---------------------------------------------------------------------------

bool selectSpellFromBook(Data::PlayerCharacter *actor, CombatContext &ctx) {
    if (!actor || !actor->combatState)
        return false;

    // Build compact list of memorized spells (21 entries, index 0-20).
    static const uint8 kMemSpellCount = 21;
    uint8 spellList[kMemSpellCount];
    uint8 spellCount = 0;

    if (actor->combatState->canCast) {
        const Data::ADnDCharacter *adnd =
            dynamic_cast<const Data::ADnDCharacter *>(actor);
        if (adnd) {
            for (uint8 i = 0; i < kMemSpellCount; ++i) {
                if (adnd->spells.memorizedSpells[i] != 0)
                    spellList[spellCount++] = adnd->spells.memorizedSpells[i];
            }
        }
    }

    uint8 selectedSpell = 0;

    // AI/autospell gate.
    const bool aiAllowed = ((uint8)actor->npc > 0x7f) || ctx.globals.magicEnabled;
    const bool sideActive = ctx.globals.sideCount[actor->combatSide] != 0;

    if (spellCount != 0 && aiAllowed && sideActive) {
        const uint8 searchRounds = (uint8)VmInterface::rollDice(1, 7);
        uint8 priorityThreshold = 7;

        for (uint8 round = 1;
             round <= searchRounds && selectedSpell == 0;
             ++round, --priorityThreshold) {

            for (uint8 attempt = 1;
                 attempt < 4 && selectedSpell == 0;
                 ++attempt) {

                const uint8 roll = (uint8)VmInterface::rollDice(1, (int)spellCount);
                const uint8 spellId = spellList[roll - 1];

                if (isSpellEligibleForAI(actor, spellId, priorityThreshold, ctx))
                    selectedSpell = spellId;
            }
        }
    }

    if (selectedSpell == 0)
        return false;

    return castSpell(actor, selectedSpell, true, nullptr);
}

// ---------------------------------------------------------------------------
// Turn-undead table
// Rows = cleric table level (index 0..9, where index = tableLevel - 1).
// Columns = undead level (index 0..9).
// Signed: negative = destroy, positive = d20 flee threshold (roll must be >= value).
// 99 = automatic failure (undead too powerful for this cleric level).
// ---------------------------------------------------------------------------
static const int8 kTurnUndeadTable[10][10] = {
    //  uL1  uL2  uL3  uL4  uL5  uL6  uL7  uL8  uL9 uL10
    {   10,   7,   4,   1,   1,   0,   0,  -1,  -1,  -1 }, // cleric L1
    {   13,  10,   7,   1,   1,   0,   0,   0,  -1,  -1 }, // cleric L2
    {   16,  13,  10,   4,   1,   1,   0,   0,   0,  -1 }, // cleric L3
    {   19,  16,  13,   7,   4,   1,   1,   0,   0,  -1 }, // cleric L4
    {   20,  19,  16,  10,   7,   4,   1,   1,   0,   0 }, // cleric L5
    {   99,  20,  19,  13,  10,   7,   4,   1,   1,   0 }, // cleric L6
    {   99,  99,  20,  16,  13,  10,   7,   4,   1,   0 }, // cleric L7
    {   99,  99,  99,  20,  16,  13,  10,   7,   4,   1 }, // cleric L8
    {   99,  99,  99,  99,  20,  16,  13,  10,   7,   1 }, // cleric L9
    {   99,  99,  99,  99,  99,  20,  16,  13,  10,   4 }, // cleric L10+
};

// ---------------------------------------------------------------------------
// findTurnUndeadTarget
// ---------------------------------------------------------------------------

bool findTurnUndeadTarget(Data::PlayerCharacter *cleric,
                          CombatContext &ctx,
                          Data::PlayerCharacter **outTarget) {
    *outTarget = nullptr;

    ctx.buildTargetList(cleric, 0xFF);
    const uint8 targetCount = (uint8)ctx.targetList.targetOrder.size();

    uint8 bestUndeadLevel = 13; // initial sentinel; only levels < 13 qualify
    bool found = false;

    for (uint8 i = 0; i < targetCount; ++i) {
        Data::PlayerCharacter *candidate =
            ctx.table.getCharacter(ctx.targetList.targetOrder[i]);
        if (!candidate || !candidate->combatState)
            continue;
        if (candidate->combatState->fleeing)
            continue;

        const Data::ADnDCharacter *adnd =
            dynamic_cast<const Data::ADnDCharacter *>(candidate);
        if (!adnd)
            continue;

        const uint8 ul = adnd->levelUndead;
        if (ul > 0 && ul < bestUndeadLevel) {
            bestUndeadLevel = ul;
            *outTarget = candidate;
            found = true;
        }
    }

    return found;
}

// ---------------------------------------------------------------------------
// turnUndead
// ---------------------------------------------------------------------------

void turnUndead(Data::PlayerCharacter *cleric,
                CombatContext &ctx,
                TurnUndeadViewDelegate *view) {
    if (!cleric || !cleric->combatState)
        return;

    cleric->combatState->turnedUndead = true;

    Data::ADnDCharacter *clericAdnd =
        dynamic_cast<Data::ADnDCharacter *>(cleric);
    if (!clericAdnd)
        return;

    const uint8 clericLevel = clericAdnd->levels[Data::C_CLERIC];
    if (clericLevel == 0)
        return;

    // Map cleric level to table row (0-based).
    uint8 tableRow;
    if (clericLevel <= 8)
        tableRow = clericLevel - 1;
    else if (clericLevel <= 13)
        tableRow = 8; // row index 8 = "cleric L9" bucket
    else
        tableRow = 9; // row index 9 = "cleric L10+" bucket

    uint8 turnAttempts      = (uint8)VmInterface::rollDice(1, 12);
    const uint8 turnRoll    = (uint8)VmInterface::rollDice(1, 20);
    uint8 remainingSuccesses = 6;
    bool stopTurning = false;

    while (!stopTurning && turnAttempts != 0) {
        Data::PlayerCharacter *target = nullptr;
        if (!findTurnUndeadTarget(cleric, ctx, &target))
            break;

        const Data::ADnDCharacter *targetAdnd =
            dynamic_cast<const Data::ADnDCharacter *>(target);
        if (!targetAdnd)
            break;

        const uint8 ul = targetAdnd->levelUndead;
        // Column index: undead levels 1-10 map to columns 0-9.
        const uint8 col = (ul >= 1 && ul <= 10) ? ul - 1 : 9;
        const int8 turnResult = kTurnUndeadTable[tableRow][col];

        // 99 = automatic failure.
        if (turnResult == 99) {
            stopTurning = true;
            break;
        }

        const int required = (turnResult < 0) ? -turnResult : turnResult;
        if (turnRoll < required) {
            stopTurning = true;
            break;
        }

        // Successful turn.
        TurnUndeadResult result;
        result.target = target;

        if (turnResult < 0) {
            // Destroy.
            result.outcome = TurnUndeadResult::DESTROYED;
            if (view)
                view->onTurnResult(result);
            if (g_combatSession)
                g_combatSession->setCharacterStatus(target, Data::S_GONE);
            else {
                target->healthStatus = Data::S_GONE;
                target->enabled = false;
            }
        } else {
            // Flee.
            result.outcome = TurnUndeadResult::FLED;
            if (view)
                view->onTurnResult(result);
            target->combatState->fleeing = true;
        }

        if (remainingSuccesses != 0)
            --remainingSuccesses;

        // Destroying undead preserves the last attempt when successes remain.
        const uint8 nextAttempts = turnAttempts - 1;
        if (nextAttempts == 0 &&
                remainingSuccesses != 0 &&
                turnResult < 0) {
            // Keep turnAttempts unchanged.
        } else {
            turnAttempts = nextAttempts;
        }
    }

    ctx.updateSideCount();
    cleric->combatState->endTurn();
}

// ---------------------------------------------------------------------------
// tryTurnUndead
// ---------------------------------------------------------------------------

bool tryTurnUndead(Data::PlayerCharacter *actor, CombatContext &ctx) {
    if (!actor || !actor->combatState)
        return false;
    if (actor->combatState->turnedUndead)
        return false;

    const Data::ADnDCharacter *adnd =
        dynamic_cast<const Data::ADnDCharacter *>(actor);
    if (!adnd || adnd->levels[Data::C_CLERIC] == 0)
        return false;

    Data::PlayerCharacter *target = nullptr;
    if (!findTurnUndeadTarget(actor, ctx, &target))
        return false;

    turnUndead(actor, ctx, nullptr);
    return true;
}

} // namespace Combat
} // namespace Goldbox
