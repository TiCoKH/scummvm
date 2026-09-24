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
#include "goldbox/core/field_path.h"
#include "goldbox/core/coords.h"

namespace Goldbox {

namespace Data {
class PlayerCharacter;
} // namespace Data

namespace Combat {
struct CombatContext;
} // namespace Combat

namespace Spells {

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
 * Resolve one AoE/projectile tile hit.
 *
 * Applies character damage (saving throw + DAMAGE_HALF) at pos.
 * Sets aoeTriggered=true if a character was hit (drives path reflection).
 * Terrain blocking is handled by the traversal loop, not here.
 *
 * @param ctx            Live combat context.
 * @param pos            Tile being resolved.
 * @param baseDamage     Raw damage before saving-throw halving.
 * @param savingThrowMod Flat modifier added to the target's saving throw.
 * @param effectTileId   Tile graphic used for the hit flash.
 * @param presenter      Presentation layer (may be null; skips visuals).
 * @param aoeTriggered   Set to true when a character is hit.
 */
void resolveAoEHitAtTile(Combat::CombatContext &ctx,
                         TilePos pos,
                         uint8 baseDamage,
                         int8 savingThrowMod,
                         uint8 effectTileId,
                         ICombatSpellPresenter *presenter,
                         bool &aoeTriggered);

/**
 * Trace a linear projectile path through the combat map.
 *
 * Mirrors SPELL_TraceSpellPath. Starts at ctx.globals.targetPos, projects
 * away from the attacker, resolves hits at each tile, animates via presenter,
 * and reflects off blocking terrain back toward the source for the remaining
 * range. ctx.globals.multiTarget is set for the duration of the call.
 *
 * @param ctx            Live combat context (targetPos read/written).
 * @param attackerPos    Position of the casting character.
 * @param pathLength     Maximum range; budget is pathLength*2 move-cost units.
 * @param baseDamage     Passed through to resolveAoEHitAtTile.
 * @param savingThrowMod Passed through to resolveAoEHitAtTile.
 * @param animatePath    Enables the initial-frame range adjustment on first reflection.
 * @param effectTileId   Tile graphic index for the projectile.
 * @param presenter      Presentation layer (may be null).
 */
void traceSpellPath(Combat::CombatContext &ctx,
                    TilePos attackerPos,
                    uint8 pathLength,
                    uint8 baseDamage,
                    int8 savingThrowMod,
                    bool animatePath,
                    uint8 effectTileId,
                    ICombatSpellPresenter *presenter);

} // namespace Spells
} // namespace Goldbox

#endif // GOLDBOX_SPELLS_SPELL_AOE_H
