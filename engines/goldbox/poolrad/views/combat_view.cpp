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

#include "goldbox/poolrad/views/combat_view.h"
#include "goldbox/combat/combat_setup.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/poolrad/data/poolrad_tile_props.h"
#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/engine.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

CombatView::CombatView()
    : View("Combat"), _phase(PHASE_NONE), _combatRound(0),
      _needsFullRedraw(true) {
}

CombatView::~CombatView() {
}

void CombatView::setup(const Combat::CombatParams &params) {
    _params = params;
    _combatRound = 0;
    _phase = PHASE_SETUP;

    // Invalidate cached portrait data (mirrors VM_LOADED_HEAD = 0xFF,
    // VM_LOADED_BODY = 0xFF and SYS_FreeRes calls in original COMBAT_Setup)
    g_engine->getPictureDisplayCache().clear();

    // Load terrain tiles (mirrors DAX_LoadIconBlock in original)
    if (_params.isDungeon) {
        _tileCache.loadDungeon(g_engine->getDaxDungcom(),
                               g_engine->getDaxRandcom());
    } else {
        _tileCache.loadWilderness(g_engine->getDaxWildcom(),
                                   g_engine->getDaxRandcom());
    }

    // Wire up game-specific tile property provider
    _battlefieldMap.setTilePropertyProvider(&PoolradTilePropertyProvider::instance());

    // Run the full COMBAT_Setup sequence
    // TODO: pass actual EffectRuntime* when effect system is wired to combat
    Combat::setupCombat(_params, _globals, _battlefieldMap, _table,
                        _placement, _viewport, nullptr);

    _tilemap.render(_battlefieldMap, _tileCache,
                     _battlefieldMap.getTilePropertyProvider());

    // Set viewport-relative position cache origin
    _table.setViewportOrigin(_viewport.getTopLeftCol(),
                             _viewport.getTopLeftRow());

    _needsFullRedraw = true;
    _phase = PHASE_PLAYER_TURN;
}

bool CombatView::msgFocus(const FocusMessage &msg) {
    View::msgFocus(msg);
    _needsFullRedraw = true;
    redraw();
    return true;
}

bool CombatView::msgUnfocus(const UnfocusMessage &msg) {
    return View::msgUnfocus(msg);
}

bool CombatView::msgKeypress(const KeypressMessage &msg) {
    if (_phase == PHASE_ENDED) {
        close();
        return true;
    }

    if (_phase != PHASE_PLAYER_TURN)
        return false;

    switch (msg.keycode) {
    case Common::KEYCODE_ESCAPE:
        _phase = PHASE_ENDED;
        close();
        return true;

    case Common::KEYCODE_LEFT:
        _viewport.adjustToInclude(_viewport.getCenterCol() - 1,
                                  _viewport.getCenterRow());
        _table.setViewportOrigin(_viewport.getTopLeftCol(),
                                 _viewport.getTopLeftRow());
        _needsFullRedraw = true;
        redraw();
        return true;

    case Common::KEYCODE_RIGHT:
        _viewport.adjustToInclude(_viewport.getCenterCol() + 1,
                                  _viewport.getCenterRow());
        _table.setViewportOrigin(_viewport.getTopLeftCol(),
                                 _viewport.getTopLeftRow());
        _needsFullRedraw = true;
        redraw();
        return true;

    case Common::KEYCODE_UP:
        _viewport.adjustToInclude(_viewport.getCenterCol(),
                                  _viewport.getCenterRow() - 1);
        _table.setViewportOrigin(_viewport.getTopLeftCol(),
                                 _viewport.getTopLeftRow());
        _needsFullRedraw = true;
        redraw();
        return true;

    case Common::KEYCODE_DOWN:
        _viewport.adjustToInclude(_viewport.getCenterCol(),
                                  _viewport.getCenterRow() + 1);
        _table.setViewportOrigin(_viewport.getTopLeftCol(),
                                 _viewport.getTopLeftRow());
        _needsFullRedraw = true;
        redraw();
        return true;

    default:
        break;
    }

    return false;
}

void CombatView::draw() {
    if (_phase == PHASE_NONE)
        return;

    Surface s = getSurface();

    // Incremental tilemap update: re-render only tiles that changed
    // (downed-member stamps, spell cloud effects, etc.)
    if (_battlefieldMap.hasDirtyTiles()) {
        _tilemap.renderDirtyTiles(_battlefieldMap, _tileCache,
                                  _battlefieldMap.getTilePropertyProvider());
        _needsFullRedraw = true;
    }

    if (_needsFullRedraw) {
        s.clear(0);
        drawViewport();
        drawCombatants();
        drawUI();
        _needsFullRedraw = false;
    }
}

bool CombatView::tick() {
    // TODO: AI turn execution, animation stepping
    return false;
}

// --- Internal ---

Combat::CombatContext CombatView::makeContext() {
    return Combat::CombatContext(_globals, _params, _table, _battlefieldMap, _viewport);
}

void CombatView::drawViewport() {
    if (!_tilemap.isBuilt())
        return;

    Common::Rect srcRect = _viewport.getSourceRect(kTileSize);
    Common::Point dstPos(kViewportX, kViewportY);

    Surface s = getSurface();
    _tilemap.blitTo(&s, dstPos, srcRect);
}

void CombatView::drawCombatants() {
    Surface s = getSurface();

    for (int i = 0; i < _table.getCount(); i++) {
        if (_table.getSize(i) == 0)
            continue;

        // Use viewport-relative positions (mirrors original
        // BYTE_ARRAY_COL_DIST / BYTE_ARRAY_ROW_DIST usage in
        // COMBAT_RedrawViewport)
        int8 localCol = _table.getColDist(i);
        int8 localRow = _table.getRowDist(i);

        if (localCol < 0 || localCol >= Combat::CombatViewport::VIEW_COLS ||
            localRow < 0 || localRow >= Combat::CombatViewport::VIEW_ROWS)
            continue;

        Data::PlayerCharacter *ch = _table.getCharacter(i);
        if (!ch)
            continue;

        Gfx::IconDirection dir = Gfx::ICON_DIRECTION_RIGHT;
        if (ch->combatState) {
            uint8 facing = ch->combatState->direction;
            if (facing >= 5 || facing == 0)
                dir = Gfx::ICON_DIRECTION_LEFT;
        }

        // If a pre-built icon slot was assigned (monsters loaded via loadMonster),
        // use the engine's IconManager's pre-composited Pic drawn at the correct pixel
        // position. Otherwise fall back to CombatRenderer (player characters).
        const uint8 slotId = ch->iconData.iconSlotId;
        int pixX = kViewportX + localCol * kTileSize;
        int pixY = kViewportY + localRow * kTileSize;
        Gfx::IconManager *iconMgr = VmInterface::getIconManager();
        if (slotId != 0 && iconMgr && !iconMgr->isSlotEmpty(slotId)) {
            const Gfx::Pic *pic = iconMgr->getReadyPic(slotId);
            if (pic)
                pic->draw(&s, pixX, pixY);
        } else {
            _combatRenderer.drawIcon(ch->iconData, Gfx::ICON_STATE_READY,
                                     dir, pixX, pixY, &s);
        }
    }
}

void CombatView::drawUI() {
    Surface s = getSurface();
    Common::Rect vpRect(kViewportX - 1, kViewportY - 1,
                        kViewportX + kViewportPixelW + 1,
                        kViewportY + kViewportPixelH + 1);
    s.frameRect(vpRect, 15);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
