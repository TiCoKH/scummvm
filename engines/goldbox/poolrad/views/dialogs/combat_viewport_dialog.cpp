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

namespace {

// Directional projectile tile selection.
// Tile IDs from SPRIT DAX (confirmed from tileset):
//
//   Arrow tiles (single frame):
//     N=0, NE=1, E=2, S=128, SE=129, W=130
//
//   Diagonal blend tiles (two frames, layer on top):
//     NE=3/SE=131  (NE group: frame 3, SE group: frame 131)
//     SW=4/NW=132  (SW group: frame 4, NW group: frame 132)
//
// The m68k arithmetic maps direction -> DAX block via:
//   safe_div(dir,2)==0  -> arrow tile, tileId=13+safe_div(dir,4), row=dir>>2
//   SE or SW            -> blend tile 0xe, row=1, copyViaBuffer=(dir==SW)
//   else (E,S,W,NW)     -> blend tile 0xe, row=0, copyViaBuffer=(dir==NW)
//
// We translate that directly to logical tile IDs here.

struct DirectionalTileParams {
    uint8 blockId;   // SPRIT DAX block id
    int   frameIdx;  // frame within block
    bool  withLayer; // render second layer on top
};

const DirectionalTileParams kDirectionalTiles[8] = {
    { 0,   0, false }, // DIR_N
    { 1,   0, false }, // DIR_NE
    { 2,   0, false }, // DIR_E
    { 129, 0, false }, // DIR_SE
    { 128, 0, false }, // DIR_S
    { 4,   0, true  }, // DIR_SW  (blend layer)
    { 130, 0, false }, // DIR_W
    { 132, 0, true  }, // DIR_NW  (blend layer)
};

} // namespace

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
        iconMgr->loadIcon(kDyingIconSlot, ::Goldbox::Gfx::ICON_KIND_SPRITE,
                          kDyingIconBlock);
    }
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
    drawCombatants();
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
        if (ch == _session->getCurrentActor() && iconMgr &&
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

bool CombatViewportDialog::cellOf(::Goldbox::Data::PlayerCharacter *ch,
        Common::Point &pix) const {
    if (!_session || !ch)
        return false;
    const ::Goldbox::Combat::CombatantTable &table = _session->getTable();
    const int idx = table.findIndex(ch);
    if (idx < 0)
        return false;
    const ::Goldbox::Combat::ViewportPos vpos =
        _session->getViewport().getCharacterViewportPosition(table, idx);
    pix = Common::Point(kViewportX + vpos.column * kTileSize,
                        kViewportY + vpos.row * kTileSize);
    return true;
}

void CombatViewportDialog::drawEffectAt(::Goldbox::Data::PlayerCharacter *ch,
        uint8 blockId) {
    Common::Point pix;
    if (cellOf(ch, pix))
        renderEffectTile(blockId, 0, pix.x, pix.y);
}

void CombatViewportDialog::renderEffectTile(uint8 blockId, int frameIdx,
        int pixX, int pixY, bool withLayer) {
    ::Goldbox::Data::DaxBlockContainer &sprit = g_engine->getDaxSprit();
    ::Goldbox::Data::DaxBlock *raw = sprit.getBlockById(blockId);
    ::Goldbox::Data::DaxBlockSprit *block =
        raw ? dynamic_cast<::Goldbox::Data::DaxBlockSprit *>(raw) : nullptr;
    if (!block)
        return;

    Surface s = getSurface();
    Graphics::ManagedSurface *screen =
        static_cast<Graphics::ManagedSurface *>(&s);

    ::Goldbox::Gfx::Pic *tile = ::Goldbox::Gfx::Pic::readSpriteFrame(block,
        frameIdx);
    if (tile) {
        tile->trDraw(screen, pixX, pixY, tile->getTransparentIndex());
        delete tile;
    }

    if (withLayer) {
        ::Goldbox::Gfx::Pic *layer = ::Goldbox::Gfx::Pic::readSpriteFrame(
            block, frameIdx + 1);
        if (layer) {
            layer->trDraw(screen, pixX, pixY, layer->getTransparentIndex());
            delete layer;
        }
    }
}

// ---------------------------------------------------------------------------
// Movement animation

Common::Array<CombatViewportDialog::MovementAnimStep>
CombatViewportDialog::buildMovementAnimationPath(TilePos start, TilePos end) {
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
        // Emit on tile boundaries (both axes aligned).
        if (finePos.col % kFinePerTile == 0 && finePos.row % kFinePerTile == 0) {
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

void CombatViewportDialog::drawMovementAnimation(
        const ScreenPos &finePos, uint8 animFrame, uint8 frameDelay,
        ::Goldbox::Data::PlayerCharacter *ch,
        ::Goldbox::Gfx::IconDirection iconDir,
        Graphics::ManagedSurface *screen) {
    const Common::Point px = fineToPixel(finePos);

    // Draw icon — alternate ready/attack state across the frame cycle.
    const uint8 slotId = ch->iconData.iconSlotId;
    ::Goldbox::Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (ch == _session->getCurrentActor() && iconMgr &&
            !iconMgr->isSlotEmpty(::Goldbox::Gfx::SLOT_SELECTFRAME)) {
        const ::Goldbox::Gfx::Pic *cursor =
            iconMgr->getReadyPic(::Goldbox::Gfx::SLOT_SELECTFRAME);
        if (cursor)
            cursor->trDraw(screen, px.x, px.y,
                ::Goldbox::Gfx::Icon::TRANSPARENT_COLOR_INDEX);
    }

    const bool readyFrame = animFrame < kAnimFrameCount / 2;
    if (slotId != 0 && iconMgr && !iconMgr->isSlotEmpty(slotId)) {
        const ::Goldbox::Gfx::Pic *pic = readyFrame
            ? iconMgr->getReadyPic(slotId)
            : iconMgr->getActionPic(slotId);
        if (pic)
            pic->trDraw(screen, px.x, px.y, pic->getTransparentIndex());
    } else {
        _combatRenderer.drawIcon(
            ch->iconData,
            readyFrame ? ::Goldbox::Gfx::ICON_STATE_READY
                       : ::Goldbox::Gfx::ICON_STATE_ATTACK,
            iconDir, px.x, px.y, screen);
    }

    g_system->updateScreen();
    g_system->delayMillis((uint32)frameDelay);

    // Restore background tile behind the icon.
    const int srcX = px.x - kViewportX;
    const int srcY = px.y - kViewportY;
    _tilemap.blitTo(screen,
        Common::Point(px.x, px.y),
        Common::Rect(srcX, srcY, srcX + kTileSize, srcY + kTileSize));
}

void CombatViewportDialog::animateMove(::Goldbox::Data::PlayerCharacter *ch,
        TilePos start, TilePos end, uint8 initialFrame, uint8 frameDelay) {
    if (!_session || !ch)
        return;

    ::Goldbox::Gfx::IconDirection iconDir =
        ::Goldbox::Gfx::ICON_DIRECTION_RIGHT;
    if (ch->combatState) {
        const uint8 facing = ch->combatState->direction;
        if (facing >= 5 || facing == 0)
            iconDir = ::Goldbox::Gfx::ICON_DIRECTION_LEFT;
    }

    Surface screenSurface = getSurface();
    Graphics::ManagedSurface *screen =
        static_cast<Graphics::ManagedSurface *>(&screenSurface);

    const Common::Array<MovementAnimStep> path =
        buildMovementAnimationPath(start, end);

    uint8 animFrame = initialFrame;
    for (uint i = 0; i < path.size(); ++i) {
        drawMovementAnimation(path[i].finePos, animFrame, frameDelay,
                              ch, iconDir, screen);
        animFrame = getNextAnimationFrame(animFrame);
    }

    // Repaint the character at its final position — the animation loop erases
    // the icon after each step, leaving the screen blank at the end.
    draw();
    g_system->updateScreen();
}

void CombatViewportDialog::animateProjectile(TilePos start, TilePos end,
        uint8 frameDelay) {
    if (!_session)
        return;
    animateMove(_session->getCurrentActor(), start, end, 0, frameDelay);
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

    const Direction facing = ctx->getFacingToward(attacker, target);

    const ::Goldbox::Combat::CombatantTable &table = _session->getTable();
    const int attackerIdx = table.findIndex(attacker);
    const int targetIdx   = table.findIndex(target);
    if (attackerIdx < 0 || targetIdx < 0)
        return;

    const ::Goldbox::Combat::ViewportPos vpos =
        _session->getViewport().getCharacterViewportPosition(table,
            attackerIdx);
    const int pixX = kViewportX + vpos.column * kTileSize;
    const int pixY = kViewportY + vpos.row    * kTileSize;

    const TilePos attackerTile(
        table.getTileCol(attackerIdx), table.getTileRow(attackerIdx));
    const TilePos targetTile(
        table.getTileCol(targetIdx), table.getTileRow(targetIdx));

    uint8 frameDelay = 10;
    const uint8 propId = weaponType;

    // Effect group 1: directional projectile (propId 9,21,28,31,73)
    if (propId == 9 || propId == 21 || propId == 28 ||
            propId == 31 || propId == 73) {
        if (facing < 8) {
            const DirectionalTileParams &p = kDirectionalTiles[facing];
            renderEffectTile(p.blockId, p.frameIdx, pixX, pixY, p.withLayer);
        }
    }
    // Effect group 2: quad effect 0x10 (propId 2,7,20)
    else if (propId == 2 || propId == 7 || propId == 20) {
        renderEffectTile(0x10, 0, pixX, pixY);
        frameDelay = 50;
    }
    // Effect group 3: quad effect 0x11 (propId 85,86)
    else if (propId == 85 || propId == 86) {
        renderEffectTile(0x11, 0, pixX, pixY);
        frameDelay = 50;
    }
    // Effect group 4: default two-tile projectile
    else {
        const uint8 baseBlock = (propId == 0x2f) ? 14 : 13;
        renderEffectTile(baseBlock + 7, 0, pixX, pixY);
        renderEffectTile(baseBlock + 7, 1, pixX, pixY);
        frameDelay = 20;
    }

    g_engine->soundPlay(7);
    animateProjectile(attackerTile, targetTile, frameDelay);
    g_engine->soundPlay(13);
}

// ---------------------------------------------------------------------------
// Damage and death

void CombatViewportDialog::animateDamage(
        ::Goldbox::Data::PlayerCharacter *target, bool isMagic) {
    if (!_session)
        return;

    const uint8 effectTileId = isMagic ? 0x16 : 0x17;

    ::Goldbox::Gfx::Pic *frames[4] = {};
    ::Goldbox::Data::DaxBlockContainer &sprit = g_engine->getDaxSprit();
    ::Goldbox::Data::DaxBlock *block = sprit.getBlockById(effectTileId);
    ::Goldbox::Data::DaxBlockSprit *spritBlock = block
        ? dynamic_cast<::Goldbox::Data::DaxBlockSprit *>(block) : nullptr;
    if (spritBlock) {
        for (int f = 0; f < 4; ++f)
            frames[f] = ::Goldbox::Gfx::Pic::readSpriteFrame(spritBlock, f);
    }

    const ::Goldbox::Combat::CombatantTable &table = _session->getTable();
    const int idx = table.findIndex(target);
    if (idx >= 0) {
        const TilePos pos(table.getTileCol(idx), table.getTileRow(idx));
        if (!_session->getViewport().isTileVisible(pos)) {
            _session->scrollViewport(pos);
            draw();
            g_system->updateScreen();
        }
    }

    g_engine->soundPlay(isMagic ? 6 : 5);

    const int repeatCount = isMagic ? (int)g_engine->getTextDelay() : 0;

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

    for (int rep = 0; rep <= repeatCount; ++rep) {
        for (int f = 0; f < 4; ++f) {
            if (frames[f])
                frames[f]->trDraw(screen, pixX, pixY,
                                  frames[f]->getTransparentIndex());

            g_system->updateScreen();
            g_system->delayMillis(kDefaultFrameDelay);

            Common::Rect srcRect(pixX - kViewportX, pixY - kViewportY,
                                 pixX - kViewportX + kTileSize,
                                 pixY - kViewportY + kTileSize);
            _tilemap.blitTo(screen, Common::Point(pixX, pixY), srcRect);
        }
    }

    g_system->updateScreen();

    if (repeatCount == 0)
        g_system->delayMillis(200);

    for (int f = 0; f < 4; ++f)
        delete frames[f];
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
