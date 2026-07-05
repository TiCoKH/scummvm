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
#include "goldbox/runtime/runtime_geo.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"

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

    buildBattlefield();
    Combat::initCombatStates(_params.roster, _params.partyCount,
                             _params.mapDirection, _params.moraleThreshold);
    placeCombatants();
    centerViewportOnParty();

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
        // Any key closes combat after it ends
        close();
        return true;
    }

    if (_phase != PHASE_PLAYER_TURN)
        return false;

    // Minimal input: arrow keys scroll viewport, ESC ends combat
    switch (msg.keycode) {
    case Common::KEYCODE_ESCAPE:
        _phase = PHASE_ENDED;
        // TODO: post CombatResult
        close();
        return true;

    case Common::KEYCODE_LEFT:
        _viewport.adjustToInclude(_viewport.getCenterCol() - 1,
                                  _viewport.getCenterRow());
        _needsFullRedraw = true;
        redraw();
        return true;

    case Common::KEYCODE_RIGHT:
        _viewport.adjustToInclude(_viewport.getCenterCol() + 1,
                                  _viewport.getCenterRow());
        _needsFullRedraw = true;
        redraw();
        return true;

    case Common::KEYCODE_UP:
        _viewport.adjustToInclude(_viewport.getCenterCol(),
                                  _viewport.getCenterRow() - 1);
        _needsFullRedraw = true;
        redraw();
        return true;

    case Common::KEYCODE_DOWN:
        _viewport.adjustToInclude(_viewport.getCenterCol(),
                                  _viewport.getCenterRow() + 1);
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

void CombatView::buildBattlefield() {
    RuntimeGeoBlock &geo = VmInterface::getRuntimeGeo();

    _tilemap.build(geo,
                   _params.mapCenterX, _params.mapCenterY, _params.playerY,
                   _params.isDungeon, _params.eclScriptId,
                   _params.wildX, _params.wildY,
                   _params.mapType, _params.terrainOverride);

    _tilemap.render(_iconManager);
}

void CombatView::placeCombatants() {
    _placement.placeAll(_params.roster, _params.partyCount,
                        _params.mapDirection, _params.encounterDistance,
                        _tilemap, _params.isDungeon, _table);
}

void CombatView::centerViewportOnParty() {
    // Center on first placed party member
    for (int i = 0; i < _table.getCount(); i++) {
        if (_table.getSize(i) == 0)
            continue;
        Data::PlayerCharacter *ch = _table.getCharacter(i);
        if (ch && !ch->hostile) {
            _viewport.centerOn(_table.getTileCol(i), _table.getTileRow(i));
            return;
        }
    }
    // Fallback: center of map
    _viewport.centerOn(25, 12);
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

        int col = _table.getTileCol(i);
        int row = _table.getTileRow(i);

        int localCol, localRow;
        if (!_viewport.mapToLocal(col, row, localCol, localRow))
            continue;

        Data::PlayerCharacter *ch = _table.getCharacter(i);
        if (!ch)
            continue;

        int pixX = kViewportX + localCol * kTileSize;
        int pixY = kViewportY + localRow * kTileSize;

        // Determine direction for icon rendering
        Gfx::IconDirection dir = Gfx::ICON_DIRECTION_RIGHT;
        if (ch->combatState) {
            uint8 facing = ch->combatState->direction;
            // Directions 5,6,7,0 face left (W/NW/N side)
            if (facing >= 5 || facing == 0)
                dir = Gfx::ICON_DIRECTION_LEFT;
        }

        _combatRenderer.drawIcon(ch->iconData, Gfx::ICON_STATE_READY,
                                 dir, pixX, pixY, &s);
    }
}

void CombatView::drawUI() {
    // TODO: combat menu bar, HP display, active character highlight
    // For now just draw a border around the viewport
    Surface s = getSurface();
    Common::Rect vpRect(kViewportX - 1, kViewportY - 1,
                        kViewportX + kViewportPixelW + 1,
                        kViewportY + kViewportPixelH + 1);
    s.frameRect(vpRect, 15);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
