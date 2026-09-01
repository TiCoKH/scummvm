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

#ifndef GOLDBOX_SPELLS_SPELL_AOE_H
#define GOLDBOX_SPELLS_SPELL_AOE_H

#include "common/scummsys.h"
#include "goldbox/core/tile_pos.h"

namespace Goldbox {

namespace Data {
class PlayerCharacter;
} // namespace Data

namespace Combat {
struct CombatContext;
} // namespace Combat

namespace Spells {

/**
 * Bresenham line-walk state for projectile/AoE path traversal.
 *
 * Mirrors the original gbFieldPath struct used by COMBAT_initBresenham /
 * COMBAT_stepBresenham. All fields are signed shorts to match the m68k layout.
 */
struct gbFieldPath {
    int16 start_x;
    int16 start_y;
    int16 end_x;
    int16 end_y;

    int16 error;
    int16 delta_x;
    int16 delta_y;

    int16 error_step;
    int16 minor_error_step;
    int16 major_step;

    int16 current_x;
    int16 current_y;

    int8  step_x;
    int8  step_y;

    uint8 step_direction;
    uint8 step_cost;    // tiles consumed by the last step sequence
};

/**
 * Presentation interface for AoE/projectile spell effects.
 *
 * CombatView implements this so the spell traversal logic never depends on
 * the rendering layer. The interface receives only tile positions and an
 * effect-tile id; it decides how to composite, animate, and display the
 * projectile (1×1, 2×1, flipped, EGA/Amiga/ScummVM surface, etc.).
 *
 * All methods are called from the spell traversal loop and must be
 * synchronous from the caller's perspective (they may internally queue
 * animation frames, but must return before the next traversal step).
 */
class ICombatSpellPresenter {
public:
    virtual ~ICombatSpellPresenter() {}

    /**
     * Prepare the effect tile resource before traversal begins.
     * Called once per spell cast, before the first showProjectile().
     *
     * @param effectTileId  Tile graphic index (e.g. 19 for lightning).
     */
    virtual void prepareEffectTile(uint8 effectTileId) = 0;

    /**
     * Render/animate the projectile moving from prevPos to curPos.
     *
     * The presenter decides the visual representation: single tile,
     * stretched tile, alternating frames, platform-specific blitting.
     *
     * @param prevPos       Tile the projectile left.
     * @param curPos        Tile the projectile arrived at.
     * @param effectTileId  Tile graphic index.
     * @param animFrame     Animation frame hint (0-based; presenter may ignore).
     */
    virtual void showProjectile(TilePos prevPos,
                                TilePos curPos,
                                uint8 effectTileId,
                                uint8 animFrame) = 0;

    /**
     * Flash the damage/hit effect on the tile where a character was struck.
     *
     * @param pos           Tile of the struck character.
     * @param effectTileId  Tile graphic index.
     */
    virtual void renderEffectTile(TilePos pos, uint8 effectTileId) = 0;
};

/**
 * Initialise a gbFieldPath for Bresenham traversal from start to end.
 * Mirrors COMBAT_initBresenham.
 */
void initBresenham(gbFieldPath &path);

/**
 * Advance the Bresenham walker by one step.
 * Updates current_x/current_y and step_cost.
 * Returns true while the path has not yet reached the end point.
 * Mirrors COMBAT_stepBresenham.
 */
bool stepBresenham(gbFieldPath &path);

/**
 * Resolve one AoE/projectile tile hit.
 *
 * Two independent jobs:
 *   1. Terrain check — sets *hitObstacle if the tile is impassable.
 *   2. Character damage — saving throw + DAMAGE_HALF; sets behaviorFlags=12
 *      around the damage call, then resets to 0.
 *
 * @param ctx               Live combat context (table, map, globals).
 * @param pos               Tile being resolved.
 * @param baseDamage        Raw damage before saving-throw halving.
 * @param savingThrowMod    Flat modifier added to the target's saving throw.
 * @param effectTileId      Tile graphic used for the hit flash.
 * @param presenter         Presentation layer (may be null; skips visuals).
 * @param hitObstacle       Set to true when impassable terrain is encountered.
 */
void resolveAoEHitAtTile(Combat::CombatContext &ctx,
                         TilePos pos,
                         uint8 baseDamage,
                         int8 savingThrowMod,
                         uint8 effectTileId,
                         ICombatSpellPresenter *presenter,
                         bool &hitObstacle);

/**
 * Trace a linear projectile path through the combat map.
 *
 * Used by Lightning Bolt and the lingering breath attack. Starts at
 * ctx.globals.targetX/Y, projects away from the attacker, resolves hits
 * at each tile, animates via presenter, and reflects off blocking terrain
 * back toward the source for the remaining range.
 *
 * @param ctx               Live combat context.
 * @param attackerPos       Position of the casting character.
 * @param targetPos         Initial target tile (spell origin).
 * @param initialAnimFrame  Non-zero enables the initial-frame range offset.
 * @param savingThrowMod    Passed through to resolveAoEHitAtTile.
 * @param baseDamage        Passed through to resolveAoEHitAtTile.
 * @param pathLength        Maximum range in tiles (traversed up to 2× total).
 * @param effectTileId      Tile graphic index for the projectile.
 * @param presenter         Presentation layer (may be null).
 */
void traceSpellPath(Combat::CombatContext &ctx,
                    TilePos attackerPos,
                    TilePos targetPos,
                    uint8 initialAnimFrame,
                    int8 savingThrowMod,
                    uint8 baseDamage,
                    uint8 pathLength,
                    uint8 effectTileId,
                    ICombatSpellPresenter *presenter);

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_AOE_H
