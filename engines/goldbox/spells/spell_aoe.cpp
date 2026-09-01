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
 */

#include "goldbox/spells/spell_aoe.h"

#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/rules/saving_throw.h"
#include "goldbox/data/spells/spell.h"

namespace Goldbox {
namespace Spells {

// ---------------------------------------------------------------------------
// Bresenham helpers
// ---------------------------------------------------------------------------

void initBresenham(gbFieldPath &p) {
    p.current_x = p.start_x;
    p.current_y = p.start_y;
    p.step_cost = 0;

    p.delta_x = p.end_x - p.start_x;
    p.delta_y = p.end_y - p.start_y;

    p.step_x = (p.delta_x > 0) ? 1 : (p.delta_x < 0) ? -1 : 0;
    p.step_y = (p.delta_y > 0) ? 1 : (p.delta_y < 0) ? -1 : 0;

    if (p.delta_x < 0) p.delta_x = -p.delta_x;
    if (p.delta_y < 0) p.delta_y = -p.delta_y;

    if (p.delta_x >= p.delta_y) {
        p.major_step       = p.step_x;
        p.error_step       = p.delta_y * 2;
        p.minor_error_step = p.delta_x * 2;
        p.step_direction   = 0; // major axis = X
    } else {
        p.major_step       = p.step_y;
        p.error_step       = p.delta_x * 2;
        p.minor_error_step = p.delta_y * 2;
        p.step_direction   = 1; // major axis = Y
    }

    p.error = p.error_step - (p.minor_error_step / 2);
}

bool stepBresenham(gbFieldPath &p) {
    if (p.current_x == p.end_x && p.current_y == p.end_y)
        return false;

    if (p.step_direction == 0) {
        p.current_x += p.step_x;
        if (p.error >= 0) {
            p.current_y += p.step_y;
            p.error -= p.minor_error_step;
        }
        p.error += p.error_step;
    } else {
        p.current_y += p.step_y;
        if (p.error >= 0) {
            p.current_x += p.step_x;
            p.error -= p.minor_error_step;
        }
        p.error += p.error_step;
    }

    ++p.step_cost;
    return true;
}

// ---------------------------------------------------------------------------
// resolveAoEHitAtTile
// ---------------------------------------------------------------------------

void resolveAoEHitAtTile(Combat::CombatContext &ctx,
                         TilePos pos,
                         uint8 baseDamage,
                         int8 savingThrowMod,
                         uint8 effectTileId,
                         ICombatSpellPresenter *presenter,
                         bool &hitObstacle) {
    hitObstacle = false;

    // --- Terrain check ---
    const uint8 rawTile = ctx.map.getRawTile(pos);
    if (rawTile != 0) {
        const Combat::TilePropertyProvider *props =
            ctx.map.getTilePropertyProvider();
        if (props && props->isImpassable(rawTile))
            hitObstacle = true;
    }

    // --- Character damage (independent of terrain) ---
    const uint8 occupant = ctx.table.getOccupant(pos);
    if (occupant != 0) {
        // occupant is 1-based index into the combatant table
        Data::PlayerCharacter *target =
            ctx.table.getCharacter(static_cast<int>(occupant) - 1);

        if (target) {
            ctx.globals.behaviorFlags = 12;

            // TODO(spell_aoe): wire saving throw through EffectHandlerBase /
            // EffectHostBridge once those are available in SpellContext.
            // For now we call the shared helper with null handler/bridge.
            Data::ADnDCharacter *adnd =
                dynamic_cast<Data::ADnDCharacter *>(target);
            bool saved = false;
            if (adnd)
                saved = Data::Rules::checkSavingThrow(
                    *adnd,
                    &ctx.globals,
                    nullptr,
                    nullptr,
                    Data::Spells::SVS_BREATH,
                    savingThrowMod);

            // TODO(spell_aoe): route through CombatView::applyDamageMessage
            // once the presenter exposes a damage-message path. Until then
            // we call DamageSystem directly via the context's damage layer.
            // Placeholder: damage application deferred to presentation layer.
            (void)saved;
            (void)baseDamage;
            // TODO(spell_aoe): ctx.damageSystem->applyLegacy(
            //     *target, baseDamage, Data::DAMAGE_HALF, saved);

            if (presenter)
                presenter->renderEffectTile(pos, effectTileId);

            ctx.globals.behaviorFlags = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// traceSpellPath
// ---------------------------------------------------------------------------

void traceSpellPath(Combat::CombatContext &ctx,
                    TilePos attackerPos,
                    TilePos targetPos,
                    uint8 initialAnimFrame,
                    int8 savingThrowMod,
                    uint8 baseDamage,
                    uint8 pathLength,
                    uint8 effectTileId,
                    ICombatSpellPresenter *presenter) {
    if (presenter)
        presenter->prepareEffectTile(effectTileId);

    // Nothing to trace when source and target are the same tile.
    if (attackerPos.col == targetPos.col &&
        attackerPos.row == targetPos.row)
        return;

    // The spell can traverse twice the supplied path length.
    int remainingRange = pathLength * 2;

    // Allow the hit logic to affect multiple combatants during traversal.
    // TODO(spell_aoe): replace with ctx.globals.multiTarget flag once added.
    bool savedMultiTarget = false; // placeholder
    (void)savedMultiTarget;

    int traceDirection = 1;
    bool useInitialAnimFrame = (initialAnimFrame != 0);

    TilePos curTarget = targetPos;

    while (remainingRange > 0) {
        gbFieldPath seg{};
        seg.start_x = curTarget.col;
        seg.start_y = curTarget.row;

        // Project away from the attacker in the current trace direction.
        seg.end_x = static_cast<int16>(
            curTarget.col +
            (curTarget.col - attackerPos.col) * traceDirection * remainingRange);
        seg.end_y = static_cast<int16>(
            curTarget.row +
            (curTarget.row - attackerPos.row) * traceDirection * remainingRange);

        initBresenham(seg);

        bool hitObstacle = false;

        do {
            TilePos prevPos(
                static_cast<uint8>(seg.current_x),
                static_cast<uint8>(seg.current_y));

            // Advance until segment ends, a character is hit, blocking
            // terrain is reached, or remaining range is exhausted.
            do {
                const bool reachedEnd = !stepBresenham(seg);

                TilePos stepPos(
                    static_cast<uint8>(seg.current_x),
                    static_cast<uint8>(seg.current_y));

                if (reachedEnd)
                    break;

                if (ctx.table.getOccupant(stepPos) != 0)
                    break;

                const uint8 rawTile = ctx.map.getRawTile(stepPos);
                if (rawTile != 0) {
                    const Combat::TilePropertyProvider *props =
                        ctx.map.getTilePropertyProvider();
                    if (props && props->isImpassable(rawTile))
                        break;
                }

                if (seg.step_cost >= static_cast<uint8>(remainingRange))
                    break;

            } while (true);

            TilePos curPos(
                static_cast<uint8>(seg.current_x),
                static_cast<uint8>(seg.current_y));

            // TODO(spell_aoe): presenter->showProjectile() is the sole
            // presentation call for the moving projectile. CombatView will
            // decide tile size, flip, frame, and platform blitting here.
            if (presenter)
                presenter->showProjectile(prevPos, curPos, effectTileId, 0);

            resolveAoEHitAtTile(ctx, curPos, baseDamage, savingThrowMod,
                                effectTileId, presenter, hitObstacle);

            if (hitObstacle) {
                curTarget = curPos;

                // Measure the return path to the attacker to compute the
                // range offset when the initial-frame adjustment is active.
                gbFieldPath returnPath{};
                returnPath.start_x = curTarget.col;
                returnPath.start_y = curTarget.row;
                returnPath.end_x   = attackerPos.col;
                returnPath.end_y   = attackerPos.row;
                initBresenham(returnPath);

                while (stepBresenham(returnPath)) {
                    if (useInitialAnimFrame && returnPath.step_cost < 9)
                        seg.step_cost = static_cast<uint8>(seg.step_cost + 8);
                }

                traceDirection = -traceDirection;
                useInitialAnimFrame = false;
            }

            if (seg.step_cost < static_cast<uint8>(remainingRange))
                remainingRange -= seg.step_cost;
            else
                remainingRange = 0;

        } while (!hitObstacle && remainingRange > 0);
    }
}

} // namespace Spells
} // namespace Goldbox
