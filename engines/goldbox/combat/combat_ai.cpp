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
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_state.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/core/direction.h"
#include "common/random.h"
#include "common/util.h"

namespace Goldbox {
namespace Combat {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

/** Roll d20 to-hit check. Returns true if the attack lands. */
static bool rollToHit(const Data::PlayerCharacter *attacker,
                      const Data::PlayerCharacter *target,
                      CombatGlobals &globals) {
    // d20 roll + attacker THAC0 modifier vs target AC.
    // THAC0: lower is better; AC: lower is better.
    // Hit if: roll >= (THAC0 - AC)
    int thac0 = 20; // default
    if (const Data::ADnDCharacter *adnd =
            dynamic_cast<const Data::ADnDCharacter *>(attacker))
        thac0 = adnd->thac0.getCurrent();

    int targetAC = 10;
    if (const Data::ADnDCharacter *adnd =
            dynamic_cast<const Data::ADnDCharacter *>(target))
        targetAC = adnd->armorClass.getCurrent();

    // Use a simple LCG for now; the engine's g_vm->_random is not accessible
    // from the pure data layer. This will be replaced when the engine random
    // source is wired through CombatContext.
    static Common::RandomSource rng("combat_ai");
    int roll = (int)(rng.getRandomNumber(19) + 1); // 1-20

    globals.attackRoll = (uint8)roll;
    return roll >= (thac0 - targetAC);
}

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
 * Find the direction from (fromCol, fromRow) toward (toCol, toRow).
 * Maps the sign of each axis directly to the delta table index.
 * Returns 8 (no-move) if already at the target.
 */
static Direction directionToward(int fromCol, int fromRow,
                                  int toCol, int toRow) {
    int sx = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
    int sy = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
    if (sx == 0 && sy == 0)
        return DIR_NONE;
    static const Direction kSignToDir[3][3] = {
        { DIR_NW, DIR_N, DIR_NE },
        { DIR_W,  DIR_NONE, DIR_E },
        { DIR_SW, DIR_S, DIR_SE },
    };
    return kSignToDir[sy + 1][sx + 1];
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
    TargetList targets;
    ctx.buildTargetList(actor, 1, targets);

    if (!targets.targetOrder.empty()) {
        // 3. Attack the first (nearest) target.
        uint8 targetIdx = targets.targetOrder[0];
        Data::PlayerCharacter *target = ctx.table.getCharacter(targetIdx);

        if (target && target->enabled) {
            ctx.globals.attacker = actor;
            ctx.globals.behaviorFlags = 0;

            if (rollToHit(actor, target, ctx.globals)) {
                uint8 dmg = rollDamage(actor);
                DamageResult dr = applyDamage(ctx, target, dmg,
                                              DAMAGE_NORMAL, false, nullptr);
                result.action = AiTurnResult::ACTION_ATTACK;
                result.target = target;
                result.damage = dr.finalDamage;
                result.targetWentDown = dr.wentDown;

                // Update hostile health ratio after damage.
                if (target->combatSide == Data::CS_ENEMY)
                    ctx.globals.handicapValue = 0; // recalculated by view
            } else {
                // Miss — still counts as the attack action.
                result.action = AiTurnResult::ACTION_ATTACK;
                result.target = target;
                result.damage = 0;
            }
        }
    } else {
        // 4. No target in melee range — move one step toward nearest enemy.
        TargetList longRange;
        ctx.buildTargetList(actor, 0xFF, longRange);

        if (!longRange.targetOrder.empty()) {
            uint8 nearestIdx = longRange.targetOrder[0];
            int actorIdx = ctx.table.findIndex(actor);

            if (actorIdx >= 0) {
                TilePos from = ctx.table.getTilePos(actorIdx);
                TilePos to   = ctx.table.getTilePos(nearestIdx);

                Direction dir = directionToward(from.col, from.row, to.col, to.row);
                if (dir != DIR_NONE) {
                    int newCol = from.col + kDirDeltaX[dir];
                    int newRow = from.row + kDirDeltaY[dir];

                    // Only move if destination is unoccupied.
                    if (ctx.table.getOccupant(newCol, newRow) == 0) {
                        ctx.table.setPosition(actorIdx,
                            TilePos((uint8)newCol, (uint8)newRow));
                        cs.direction = static_cast<uint8>(dir);
                        ctx.rebuildPlacementMap();
                        ctx.rebuildDistances();
                    }
                }
            }
            result.action = AiTurnResult::ACTION_MOVE;
        } else {
            // No targets at all — guard.
            cs.guarding = true;
            result.action = AiTurnResult::ACTION_GUARD;
        }
    }

    // 5. Mark as acted this round.
    cs.initiative = 0xFF;
    return result;
}

} // namespace Combat
} // namespace Goldbox
