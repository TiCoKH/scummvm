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

#ifndef GOLDBOX_POOLRAD_VIEWS_COMBAT_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_COMBAT_VIEW_H

#include "goldbox/poolrad/views/view.h"
#include "goldbox/combat/combat_params.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_placement.h"
#include "goldbox/combat/combat_viewport.h"
#include "goldbox/combat/battlefield_map.h"
#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/gfx/combat_renderer.h"
#include "goldbox/gfx/icon_manager.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

/**
 * Tactical combat view — self-contained combat screen.
 *
 * Pushed onto the view stack by the ECL ENCOUNTER opcode handler.
 * Owns the full combat loop: terrain, placement, turns, AI, rendering.
 * Does not access the VM directly; receives all setup data via CombatParams.
 *
 * Posts a CombatResult via MenuResult event when combat ends.
 */
class CombatView : public View {
public:
    CombatView();
    ~CombatView() override;

    /** Initialize combat from params (called before push or in msgFocus). */
    void setup(const Combat::CombatParams &params);

    bool msgFocus(const FocusMessage &msg) override;
    bool msgUnfocus(const UnfocusMessage &msg) override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    bool tick() override;

private:
    enum CombatPhase {
        PHASE_NONE = 0,
        PHASE_SETUP,       // terrain + placement done, ready to draw
        PHASE_PLAYER_TURN, // waiting for player input
        PHASE_AI_TURN,     // enemy AI executing
        PHASE_ANIMATING,   // animation playing
        PHASE_ENDED        // combat over, waiting to close
    };

    // --- Combat state ---
    Combat::CombatParams _params;
    Combat::CombatGlobals _globals;
    Combat::BattlefieldMap _battlefieldMap;
    Combat::CombatantTable _table;
    Combat::CombatPlacement _placement;
    Combat::CombatViewport _viewport;
    CombatPhase _phase;
    int _combatRound;

    // --- Rendering ---
    Gfx::BattlefieldTilemap _tilemap;
    Gfx::IconManager _iconManager;
    Gfx::CombatRenderer _combatRenderer;
    bool _needsFullRedraw;

    // --- Layout constants (pixel coords on 320x200 screen) ---
    static const int kViewportX = 8;      // pixel X of viewport area
    static const int kViewportY = 8;      // pixel Y of viewport area
    static const int kTileSize = 24;      // pixels per tile
    static const int kViewportPixelW = Combat::CombatViewport::VIEW_COLS * kTileSize;
    static const int kViewportPixelH = Combat::CombatViewport::VIEW_ROWS * kTileSize;

    // --- Internal methods ---
    void drawViewport();
    void drawCombatants();
    void drawUI();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_COMBAT_VIEW_H
