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
#include "goldbox/combat/combat_targeting.h"
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
 * Returns the 8-direction index (0-7) of the best step.
 */
static uint8 directionToward(int fromCol, int fromRow,
                              int toCol, int toRow) {
    int dx = toCol - fromCol;
    int dy = toRow - fromRow;

    // Clamp to -1/0/+1 per axis to get the 8-direction step.
    int sx = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
    int sy = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;

    for (uint8 d = 0; d < 8; d++) {
        if (kDirDeltaX[d] == sx && kDirDeltaY[d] == sy)
            return d;
    }
    return 8; // no-move
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
    buildTargetList(actor, 1, ctx.table, targets);

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
        buildTargetList(actor, 0xFF, ctx.table, longRange);

        if (!longRange.targetOrder.empty()) {
            uint8 nearestIdx = longRange.targetOrder[0];
            int actorIdx = ctx.table.findIndex(actor);

            if (actorIdx >= 0) {
                int fromCol = ctx.table.getTileCol(actorIdx);
                int fromRow = ctx.table.getTileRow(actorIdx);
                int toCol   = ctx.table.getTileCol(nearestIdx);
                int toRow   = ctx.table.getTileRow(nearestIdx);

                uint8 dir = directionToward(fromCol, fromRow, toCol, toRow);
                if (dir < 8) {
                    int newCol = fromCol + kDirDeltaX[dir];
                    int newRow = fromRow + kDirDeltaY[dir];

                    // Only move if destination is unoccupied.
                    if (ctx.table.getOccupant(newCol, newRow) == 0) {
                        ctx.table.setPosition(actorIdx,
                            TilePos((uint8)newCol, (uint8)newRow));
                        cs.direction = dir;
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
