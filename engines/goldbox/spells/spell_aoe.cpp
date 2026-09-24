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

using Combat::TileProp;
using Combat::TilePropertyProvider;

// ---------------------------------------------------------------------------
// resolveAoEHitAtTile
// ---------------------------------------------------------------------------

void resolveAoEHitAtTile(Combat::CombatContext &ctx,
                         TilePos pos,
                         uint8 baseDamage,
                         int8 savingThrowMod,
                         uint8 effectTileId,
                         ICombatSpellPresenter *presenter,
                         bool &aoeTriggered) {
    aoeTriggered = false;

    const uint8 occupant = ctx.table.getOccupant(pos);
    if (occupant == 0)
        return;

    Data::PlayerCharacter *target =
        ctx.table.getCharacter(static_cast<int>(occupant) - 1);
    if (!target)
        return;

    ctx.globals.behaviorFlags = 12;

    Data::ADnDCharacter *adnd = dynamic_cast<Data::ADnDCharacter *>(target);
    bool saved = false;
    if (adnd)
        saved = Data::Rules::checkSavingThrow(
            *adnd, &ctx.globals, nullptr, nullptr,
            Data::Spells::SVS_BREATH, savingThrowMod);

    (void)saved;
    (void)baseDamage;
    // TODO(spell_aoe): ctx.damageSystem->applyLegacy(
    //     *target, baseDamage, Data::DAMAGE_HALF, saved);

    if (presenter)
        presenter->renderEffectTile(pos, effectTileId);

    ctx.globals.behaviorFlags = 0;
    aoeTriggered = true;
}

// ---------------------------------------------------------------------------
// traceSpellPath
// ---------------------------------------------------------------------------

void traceSpellPath(Combat::CombatContext &ctx,
                    TilePos attackerPos,
                    uint8 pathLength,
                    uint8 baseDamage,
                    int8 savingThrowMod,
                    bool animatePath,
                    uint8 effectTileId,
                    ICombatSpellPresenter *presenter) {
    if (presenter)
        presenter->prepareEffectTile(effectTileId);

    TilePos &targetPos = ctx.globals.targetPos;

    Combat::CombatContext::CombatCell cell = ctx.getTileAndOccupantAt(targetPos);
    uint8 tileId     = cell.tileId;
    uint8 occupantId = cell.occupantId;

    if (attackerPos == targetPos)
        return;

    uint8 remainingPath = pathLength * 2;
    bool initialAnimation = animatePath;
    int8 traceDirection = 1;
    uint8 traceOccupantId = occupantId;

    ctx.globals.multiTarget = true;

    while (remainingPath != 0) {
        FieldPath seg{};
        seg.start  = targetPos;
        seg.endCol = (int16)targetPos.col + (int16)(targetPos.col - attackerPos.col) * traceDirection * remainingPath;
        seg.endRow = (int16)targetPos.row + (int16)(targetPos.row - attackerPos.row) * traceDirection * remainingPath;
        Goldbox::initBresenham(seg);

        bool aoeTriggered = false;

        do {
            TilePos prevPos = seg.current;

            if (seg.start.col != (uint8)seg.endCol || seg.start.row != (uint8)seg.endRow) {
                while (true) {
                    const bool stepped = Goldbox::stepBresenham(seg);

                    cell       = ctx.getTileAndOccupantAt(seg.current);
                    tileId     = cell.tileId;
                    occupantId = cell.occupantId;

                    if (!stepped)
                        break;

                    const TileProp *prop = nullptr;
                    if (tileId != 0) {
                        const TilePropertyProvider *props = ctx.map.getTilePropertyProvider();
                        if (props)
                            prop = props->getTileProp(tileId - 1);
                    }

                    if ((occupantId != 0 && occupantId != traceOccupantId) ||
                        tileId == 0 ||
                        (prop && prop->passable > 1) ||
                        remainingPath <= (uint8)seg.moveCost)
                        break;
                }
            }

            traceOccupantId = occupantId;

            if (tileId == 0)
                remainingPath = 0;

            if (presenter)
                presenter->showProjectile(prevPos, seg.current, effectTileId, 0);

            resolveAoEHitAtTile(ctx, seg.current, baseDamage, savingThrowMod,
                                effectTileId, presenter, aoeTriggered);

            if (aoeTriggered) {
                targetPos = seg.current;

                FieldPath returnPath{};
                returnPath.start  = targetPos;
                returnPath.endCol = (int16)attackerPos.col;
                returnPath.endRow = (int16)attackerPos.row;
                Goldbox::initBresenham(returnPath);

                while (Goldbox::stepBresenham(returnPath)) {
                    if (initialAnimation && returnPath.moveCost < 9)
                        seg.moveCost += 8;
                }

                traceDirection = -traceDirection;
                traceOccupantId = 0;
                initialAnimation = false;
            }

            if ((uint8)seg.moveCost < remainingPath)
                remainingPath -= (uint8)seg.moveCost;
            else
                remainingPath = 0;

        } while (!aoeTriggered && remainingPath != 0);
    }

    ctx.globals.multiTarget = false;
}

} // namespace Spells
} // namespace Goldbox
