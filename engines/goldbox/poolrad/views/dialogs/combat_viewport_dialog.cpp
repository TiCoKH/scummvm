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

#include "goldbox/poolrad/views/dialogs/combat_viewport_dialog.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/core/field_path.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/data/player_character.h"
#include "goldbox/engine.h"
#include "goldbox/gfx/icon.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/vm_interface.h"
#include "common/debug.h"
#include "common/system.h"
#include "common/util.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatViewportDialog::CombatViewportDialog()
        : Dialog("CombatViewport") {
}

void CombatViewportDialog::loadTileset(bool isDungeon) {
    _attackingCharacters.clear();
    if (isDungeon) {
        _tileCache.loadDungeon(g_engine->getDaxDungcom(),
                               g_engine->getDaxRandcom());
    } else {
        _tileCache.loadWilderness(g_engine->getDaxWildcom(),
                                  g_engine->getDaxRandcom());
    }
}

void CombatViewportDialog::loadCombatIcons() {
    // Selection cursor frame from COMSPR block 25 (mirrors set_icon SLOT_SELECTFRAME).
    ::Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (iconMgr) {
        iconMgr->loadIcon(::Goldbox::Gfx::SLOT_SELECTFRAME,
                          ::Goldbox::Gfx::ICON_KIND_SPRITE, kSelectFrameBlock);
        // GameInit: COMSPR blocks 0..11 fill icon cells 13..24 (24 is the
        // dying icon).
        for (uint8 cell = 13; cell <= kDyingIconSlot; ++cell)
            iconMgr->loadIcon(cell, ::Goldbox::Gfx::ICON_KIND_SPRITE,
                              static_cast<uint16>(cell - 13));
    }
}

const ::Goldbox::Gfx::Pic *CombatViewportDialog::iconCellFrame(uint8 cell,
        uint8 half, bool mirror) const {
    ::Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    ::Goldbox::Gfx::Icon *icon = iconMgr ? iconMgr->getIcon(cell) : nullptr;
    if (!icon)
        return nullptr;
    if (half == 0)
        return mirror ? icon->getReadyIconFlipped() : icon->getReadyIcon();
    return mirror ? icon->getActionIconFlipped() : icon->getActionIcon();
}

Common::Array<const ::Goldbox::Gfx::Pic *> CombatViewportDialog::iconFacings(
        uint8 cell) const {
    Common::Array<const ::Goldbox::Gfx::Pic *> frames;
    frames.push_back(iconCellFrame(cell, 0, false));
    frames.push_back(iconCellFrame(cell, 0, true));
    frames.push_back(iconCellFrame(cell, 1, true));
    frames.push_back(iconCellFrame(cell, 1, false));
    return frames;
}

void CombatViewportDialog::rebuildTerrain() {
    if (!_session)
        return;
    const ::Goldbox::Combat::BattlefieldMap &map =
        _session->getBattlefieldMap();
    _tilemap.render(map, _tileCache, map.getTilePropertyProvider());
}

bool CombatViewportDialog::refreshDirtyTiles() {
    if (!_session)
        return false;
    ::Goldbox::Combat::BattlefieldMap &map = _session->getBattlefieldMap();
    if (!map.hasDirtyTiles())
        return false;
    _tilemap.renderDirtyTiles(map, _tileCache, map.getTilePropertyProvider());
    return true;
}

void CombatViewportDialog::draw() {
    drawTerrain();
    drawAimCursor();
    drawCombatants();
}

void CombatViewportDialog::setAimCursor(TilePos tile, uint8 size) {
    _aimCursorOn = true;
    _aimCursorTile = tile;
    _aimCursorSize = size > 1 ? 2 : 1;
}

// DrawCursorCell: the cursor sits on the terrain, under any creature.
void CombatViewportDialog::drawAimCursor() {
    if (!_aimCursorOn || !_session)
        return;
    ::Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (!iconMgr || iconMgr->isSlotEmpty(::Goldbox::Gfx::SLOT_SELECTFRAME))
        return;
    const ::Goldbox::Gfx::Pic *cursor =
        iconMgr->getReadyPic(::Goldbox::Gfx::SLOT_SELECTFRAME);
    if (!cursor)
        return;

    Surface s = getSurface();
    const ::Goldbox::Combat::CombatViewport &vp = _session->getViewport();
    for (uint8 dy = 0; dy < _aimCursorSize; ++dy) {
        for (uint8 dx = 0; dx < _aimCursorSize; ++dx) {
            const ::Goldbox::Combat::ViewportPos pos = vp.mapToViewport(
                TilePos(_aimCursorTile.col + dx, _aimCursorTile.row + dy));
            if (!pos.isVisible(::Goldbox::Combat::CombatViewport::VIEW_COLS,
                               ::Goldbox::Combat::CombatViewport::VIEW_ROWS))
                continue;
            cursor->trDraw(&s, kViewportX + pos.column * kTileSize,
                kViewportY + pos.row * kTileSize,
                ::Goldbox::Gfx::Icon::TRANSPARENT_COLOR_INDEX);
        }
    }
}

void CombatViewportDialog::flyAimCursor(TilePos from, TilePos to) {
    ::Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (!iconMgr || iconMgr->isSlotEmpty(::Goldbox::Gfx::SLOT_SELECTFRAME))
        return;
    Common::Array<const ::Goldbox::Gfx::Pic *> frames;
    frames.push_back(iconMgr->getReadyPic(::Goldbox::Gfx::SLOT_SELECTFRAME));
    const bool wasOn = _aimCursorOn;
    _aimCursorOn = false;
    animateMovingIcon(from, to, frames, 0);
    _aimCursorOn = wasOn;
}

void CombatViewportDialog::drawTerrain() {
    if (!_session || !_tilemap.isBuilt())
        return;

    const ::Goldbox::Combat::CombatViewport &vp = _session->getViewport();
    Common::Rect srcRect = vp.getSourceRect(kTileSize);
    Surface s = getSurface();
    _tilemap.blitTo(&s, Common::Point(kViewportX, kViewportY), srcRect);
}

bool CombatViewportDialog::isAttacking(
        ::Goldbox::Data::PlayerCharacter *ch) const {
    for (uint i = 0; i < _attackingCharacters.size(); ++i) {
        if (_attackingCharacters[i] == ch)
            return true;
    }
    return false;
}

void CombatViewportDialog::setAttackPose(
        ::Goldbox::Data::PlayerCharacter *ch, bool attacking) {
    if (!ch)
        return;
    for (uint i = 0; i < _attackingCharacters.size(); ++i) {
        if (_attackingCharacters[i] == ch) {
            if (!attacking)
                _attackingCharacters.remove_at(i);
            return;
        }
    }
    if (attacking)
        _attackingCharacters.push_back(ch);
}

void CombatViewportDialog::drawCombatants(
        ::Goldbox::Data::PlayerCharacter *skipCharacter) {
    if (!_session)
        return;

    Surface s = getSurface();
    const ::Goldbox::Combat::CombatantTable &table = _session->getTable();

    for (int i = 0; i < table.getCount(); i++) {
        if (table.getSize(i) == 0)
            continue;

        const ::Goldbox::Combat::ViewportPos vpos =
            _session->getViewport().getCharacterViewportPosition(table, i);
        const int16 localCol = vpos.column;
        const int16 localRow = vpos.row;

        if (!vpos.isVisible(::Goldbox::Combat::CombatViewport::VIEW_COLS,
                            ::Goldbox::Combat::CombatViewport::VIEW_ROWS))
            continue;

        ::Goldbox::Data::PlayerCharacter *ch = table.getCharacter(i);
        if (!ch || ch == skipCharacter)
            continue;

        const bool attacking = isAttacking(ch);
        ::Goldbox::Gfx::IconDirection dir = ::Goldbox::Gfx::ICON_DIRECTION_RIGHT;
        if (ch->combatState) {
            uint8 facing = ch->combatState->direction;
            // LEFT for west-facing directions: SW(5), W(6), NW(7).
            // N(0), NE(1), E(2), SE(3), S(4) all face right.
            if (facing == 5 || facing == 6 || facing == 7)
                dir = ::Goldbox::Gfx::ICON_DIRECTION_LEFT;
        }

        const uint8 slotId = ch->iconData.iconSlotId;
        int pixX = kViewportX + localCol * kTileSize;
        int pixY = kViewportY + localRow * kTileSize;
        ::Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();

        // Draw the frame first so it does not obscure the character sprite.
        if (!_aimCursorOn && ch == _session->getCurrentActor() && iconMgr &&
                !iconMgr->isSlotEmpty(::Goldbox::Gfx::SLOT_SELECTFRAME)) {
            const ::Goldbox::Gfx::Pic *cursor =
                iconMgr->getReadyPic(::Goldbox::Gfx::SLOT_SELECTFRAME);
            if (cursor)
                cursor->trDraw(&s, pixX, pixY,
                    ::Goldbox::Gfx::Icon::TRANSPARENT_COLOR_INDEX);
        }

        if (slotId != 0 && iconMgr && !iconMgr->isSlotEmpty(slotId)) {
            ::Goldbox::Gfx::Icon *icon = iconMgr->getIcon(slotId);
            const ::Goldbox::Gfx::Pic *pic = nullptr;
            if (icon) {
                if (dir == ::Goldbox::Gfx::ICON_DIRECTION_LEFT)
                    pic = attacking ? icon->getActionIconFlipped()
                                    : icon->getReadyIconFlipped();
                else
                    pic = attacking ? icon->getActionIcon()
                                    : icon->getReadyIcon();
            }
            if (pic)
                pic->trDraw(&s, pixX, pixY, pic->getTransparentIndex());
        } else {
            _combatRenderer.drawIcon(ch->iconData,
                                     attacking
                                         ? ::Goldbox::Gfx::ICON_STATE_ATTACK
                                         : ::Goldbox::Gfx::ICON_STATE_READY,
                                     dir, pixX, pixY, &s);
        }
    }
}

void CombatViewportDialog::centerOn(TilePos center) {
    if (!_session)
        return;
    _session->getViewport().centerOn(center);
    draw();
}

// ---------------------------------------------------------------------------
// Movement animation

Common::Array<CombatViewportDialog::MovementAnimStep>
CombatViewportDialog::buildMovementAnimationPath(TilePos start, TilePos end,
        bool everyStep) {
    Common::Array<MovementAnimStep> steps;
    ::Goldbox::Combat::CombatViewport &viewport = _session->getViewport();

    // Build Bresenham path in fine-grid coords (tile * kFinePerTile).
    FieldPath path;
    path.start   = TilePos((uint8)(start.col * kFinePerTile),
                           (uint8)(start.row * kFinePerTile));
    path.current = path.start;
    path.endCol  = (int16)(end.col * kFinePerTile);
    path.endRow  = (int16)(end.row * kFinePerTile);
    initBresenham(path);

    static const int kMaxSteps = 256;
    Direction directions[kMaxSteps];
    int stepCount = 0;
    do {
        bool stepped = stepBresenham(path);
        if (stepCount < kMaxSteps)
            directions[stepCount++] = path.stepDirection;
        if (!stepped)
            break;
    } while (stepCount < kMaxSteps);

    if (stepCount < 2)
        return steps;

    // Determine initial viewport center.
    const int deltaX = (int)end.col - (int)start.col;
    const int deltaY = (int)end.row - (int)start.row;
    TilePos redrawCenter;
    if (viewport.isTileVisible(start) && viewport.isTileVisible(end)) {
        redrawCenter = viewport.getCenter();
    } else if (ABS(deltaX) < ::Goldbox::Combat::CombatViewport::VIEW_COLS &&
               ABS(deltaY) < ::Goldbox::Combat::CombatViewport::VIEW_ROWS) {
        redrawCenter = TilePos((uint8)(start.col + deltaX / 2),
                               (uint8)(start.row + deltaY / 2));
    } else {
        redrawCenter = viewport.getCenter();
    }
    centerOn(redrawCenter);

    const int kFineViewCols =
        ::Goldbox::Combat::CombatViewport::VIEW_COLS * kFinePerTile;
    const int kFineViewRows =
        ::Goldbox::Combat::CombatViewport::VIEW_ROWS * kFinePerTile;

    TilePos vpTopLeft = viewport.getTopLeft();
    ScreenPos finePos(
        (int16)((start.col - vpTopLeft.col) * kFinePerTile),
        (int16)((start.row - vpTopLeft.row) * kFinePerTile));
    ScreenPos subtileOffset(0, 0);

    // Walk every direction step, emitting one MovementAnimStep per tile boundary.
    for (int i = 0; i < stepCount; ++i) {
        // Emit every third-cell step, or only on tile boundaries.
        if (everyStep || (finePos.col % kFinePerTile == 0
                && finePos.row % kFinePerTile == 0)) {
            MovementAnimStep s;
            s.finePos = finePos;
            steps.push_back(s);
        }

        if (i + 1 >= stepCount)
            break;

        const Direction dir = directions[i + 1];
        finePos.col += kDirDeltaX[dir];
        finePos.row += kDirDeltaY[dir];

        const bool leftViewport =
            finePos.col < 0 || finePos.col >= kFineViewCols ||
            finePos.row < 0 || finePos.row >= kFineViewRows;

        if (!leftViewport) {
            subtileOffset.col += kDirDeltaX[dir];
            subtileOffset.row += kDirDeltaY[dir];
            if (ABS((int)subtileOffset.col) == kFinePerTile) {
                redrawCenter.col = (uint8)((int)redrawCenter.col +
                    (subtileOffset.col > 0 ? 1 : -1));
                subtileOffset.col = 0;
            }
            if (ABS((int)subtileOffset.row) == kFinePerTile) {
                redrawCenter.row = (uint8)((int)redrawCenter.row +
                    (subtileOffset.row > 0 ? 1 : -1));
                subtileOffset.row = 0;
            }
        } else {
            // Pan viewport kPanTiles toward movement direction and recalculate.
            redrawCenter.col = (uint8)((int)redrawCenter.col +
                kDirDeltaX[dir] * kPanTiles);
            redrawCenter.row = (uint8)((int)redrawCenter.row +
                kDirDeltaY[dir] * kPanTiles);
            centerOn(redrawCenter);

            vpTopLeft = viewport.getTopLeft();
            const TilePos curTile(
                (uint8)(start.col + subtileOffset.col / kFinePerTile),
                (uint8)(start.row + subtileOffset.row / kFinePerTile));
            finePos.col = (int16)((curTile.col - vpTopLeft.col) * kFinePerTile +
                kDirDeltaX[dir]);
            finePos.row = (int16)((curTile.row - vpTopLeft.row) * kFinePerTile +
                kDirDeltaY[dir]);
            subtileOffset = ScreenPos(0, 0);
        }
    }

    // Always emit the final tile position.
    if (!viewport.isTileVisible(end))
        centerOn(end);
    vpTopLeft = viewport.getTopLeft();
    MovementAnimStep last;
    last.finePos = ScreenPos(
        (int16)((end.col - vpTopLeft.col) * kFinePerTile),
        (int16)((end.row - vpTopLeft.row) * kFinePerTile));
    steps.push_back(last);

    return steps;
}

void CombatViewportDialog::animateProjectile(TilePos start, TilePos end,
        uint8 cell, uint8 pace) {
    animateMovingIcon(start, end, iconFacings(cell), pace);
}

static void flushCombatScreen() {
    if (g_events && g_events->getScreen()) {
        g_events->getScreen()->makeAllDirty();
        g_events->getScreen()->update();
    } else {
        g_system->updateScreen();
    }
}

void CombatViewportDialog::animateMovingIcon(TilePos start, TilePos end,
        const Common::Array<const ::Goldbox::Gfx::Pic *> &frames,
        uint8 pace) {
    if (!_session || frames.empty())
        return;

    const Common::Array<MovementAnimStep> path =
        buildMovementAnimationPath(start, end, pace > 0);
    if (path.empty())
        return;

    // The background, icons included, comes back after each step.
    draw();
    Surface screenSurface = getSurface();
    Graphics::ManagedSurface *screen =
        static_cast<Graphics::ManagedSurface *>(&screenSurface);
    Graphics::ManagedSurface backup;
    backup.copyFrom(*screen);

    uint frame = 0;
    for (uint i = 0; i < path.size(); ++i) {
        const Common::Point px = fineToPixel(path[i].finePos);
        const ::Goldbox::Gfx::Pic *pic = frames[frame];
        if (pic)
            pic->trDraw(screen, px.x, px.y, pic->getTransparentIndex());
        flushCombatScreen();
        if (pace > 0)
            g_system->delayMillis(pace);

        const Common::Rect cell(px.x, px.y, px.x + kTileSize, px.y + kTileSize);
        Common::Rect clipped = cell;
        clipped.clip(Common::Rect(0, 0, backup.w, backup.h));
        if (!clipped.isEmpty())
            screen->blitFrom(backup, clipped,
                Common::Point(clipped.left, clipped.top));

        frame = (frame + 1) % frames.size();
    }

    draw();
    flushCombatScreen();
}

// ---------------------------------------------------------------------------
// Ranged attack animation

void CombatViewportDialog::animateRangedAttack(
        ::Goldbox::Data::PlayerCharacter *attacker,
        ::Goldbox::Data::PlayerCharacter *target, uint8 weaponType) {
    if (!_session || !attacker || !target)
        return;

    ::Goldbox::Combat::CombatContext *ctx = _session->getContext();
    if (!ctx)
        return;

    const ::Goldbox::Combat::CombatantTable &table = _session->getTable();
    const int attackerIdx = table.findIndex(attacker);
    const int targetIdx   = table.findIndex(target);
    if (attackerIdx < 0 || targetIdx < 0)
        return;

    const TilePos attackerTile(
        table.getTileCol(attackerIdx), table.getTileRow(attackerIdx));
    const TilePos targetTile(
        table.getTileCol(targetIdx), table.getTileRow(targetIdx));

    // RangedAttackAnim: frames from icon cells 13.., pace in ms per step.
    const uint8 dir = static_cast<uint8>(ctx->getFacingToward(attacker, target));
    Common::Array<const ::Goldbox::Gfx::Pic *> frames;
    uint8 pace = 10;
    switch (weaponType) {
    case 9:  // Dart
    case 21: // Javelin
    case 28: // Quarrel
    case 31: // Spear
    case 73: // Arrow
        if (dir & 1) {
            if (dir == DIR_SE || dir == DIR_SW)
                frames.push_back(iconCellFrame(14, 1, dir == DIR_SW));
            else
                frames.push_back(iconCellFrame(14, 0, dir == DIR_NW));
        } else {
            frames.push_back(iconCellFrame(13 + dir % 4, dir / 4, false));
        }
        break;
    case 2:  // Hand Axe
    case 7:  // Club
    case 20: // Hammer
        frames = iconFacings(16);
        pace = 50;
        break;
    case 85: // Holy water vial
    case 86: // Flask of oil
        frames = iconFacings(17);
        pace = 50;
        break;
    default: {
        // Sling stone (base item 47) or other missile: two frames.
        const uint8 cell = (weaponType == 47) ? 21 : 20;
        frames.push_back(iconCellFrame(cell, 0, false));
        frames.push_back(iconCellFrame(cell, 1, false));
        pace = 20;
        break;
    }
    }

    g_engine->soundPlay(7);
    animateMovingIcon(attackerTile, targetTile, frames, pace);
    g_engine->soundPlay(13);
}

// ---------------------------------------------------------------------------
// Damage and death

void CombatViewportDialog::animateDamage(
        ::Goldbox::Data::PlayerCharacter *target, bool isMagic) {
    if (!_session)
        return;

    // PrintNameStringAnim: four facings of cell 22 (effect) or 23 (damage),
    // 70 ms each; an effect repeats gameDelay + 1 times, damage flashes once
    // and then pauses.
    const Common::Array<const ::Goldbox::Gfx::Pic *> frames =
        iconFacings(isMagic ? 22 : 23);

    const ::Goldbox::Combat::CombatantTable &table = _session->getTable();
    const int idx = table.findIndex(target);
    if (idx >= 0) {
        const TilePos pos(table.getTileCol(idx), table.getTileRow(idx));
        if (!_session->getViewport().isTileVisible(pos))
            _session->scrollViewport(pos);
    }
    draw();

    g_engine->soundPlay(isMagic ? 5 : 4);

    const uint8 gameDelay = VmInterface::getGameSpeed();
    const int repeatCount = isMagic ? (int)gameDelay : 0;

    int pixX = kViewportX;
    int pixY = kViewportY;
    if (idx >= 0) {
        const ::Goldbox::Combat::ViewportPos vpos =
            _session->getViewport().getCharacterViewportPosition(table, idx);
        pixX = kViewportX + vpos.column * kTileSize;
        pixY = kViewportY + vpos.row    * kTileSize;
    }

    Surface screenSurface = getSurface();
    Graphics::ManagedSurface *screen =
        static_cast<Graphics::ManagedSurface *>(&screenSurface);
    Graphics::ManagedSurface backup;
    backup.copyFrom(*screen);
    Common::Rect cell(pixX, pixY, pixX + kTileSize, pixY + kTileSize);
    cell.clip(Common::Rect(0, 0, backup.w, backup.h));

    for (int rep = 0; rep <= repeatCount; ++rep) {
        for (uint f = 0; f < frames.size(); ++f) {
            if (frames[f])
                frames[f]->trDraw(screen, pixX, pixY,
                                  frames[f]->getTransparentIndex());
            flushCombatScreen();
            g_system->delayMillis(70);
            if (!cell.isEmpty())
                screen->blitFrom(backup, cell, Common::Point(cell.left, cell.top));
        }
    }
    flushCombatScreen();

    if (repeatCount == 0)
        g_system->delayMillis(static_cast<uint32>(gameDelay) * 225);
}

void CombatViewportDialog::showDying(::Goldbox::Data::PlayerCharacter *ch) {
    if (!_session || !ch)
        return;

    const ::Goldbox::Combat::CombatantTable &table = _session->getTable();
    const int idx = table.findIndex(ch);
    if (idx < 0 || table.getSize(idx) == 0)
        return;

    ::Goldbox::Combat::CombatContext *ctx = _session->getContext();
    if (ctx && !ctx->isCharacterInBounds(ch, true)) {
        _session->scrollViewport(table.getTilePos(idx));
        draw();
    }

    ::Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    const ::Goldbox::Gfx::Pic *pic =
        (iconMgr && !iconMgr->isSlotEmpty(kDyingIconSlot))
        ? iconMgr->getReadyPic(kDyingIconSlot) : nullptr;
    debug(0, "[COMBATVIEW] dying icon for '%s' index=%d size=%u icon=%d",
        ch->name.c_str(), idx, (unsigned)table.getSize(idx), pic != nullptr);
    if (!pic)
        return;

    const ::Goldbox::Combat::ViewportPos vpos =
        _session->getViewport().getCharacterViewportPosition(table, idx);
    Surface s = getSurface();
    for (uint8 slot = 0; slot < 4; ++slot) {
        ::Goldbox::Combat::FootprintOffsetPair off;
        if (!::Goldbox::Combat::CombatContext::getFootprintOffset(
                table.getSize(idx), slot, off))
            continue;
        const int col = vpos.column + off.col;
        const int row = vpos.row + off.row;
        if (col < 0 || row < 0 ||
                col >= ::Goldbox::Combat::CombatViewport::VIEW_COLS ||
                row >= ::Goldbox::Combat::CombatViewport::VIEW_ROWS)
            continue;
        pic->trDraw(&s, kViewportX + col * kTileSize,
                    kViewportY + row * kTileSize,
                    pic->getTransparentIndex());
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
