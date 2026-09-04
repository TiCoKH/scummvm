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
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/combat_placement.h"
#include "goldbox/combat/combat_viewport.h"
#include "goldbox/combat/battlefield_map.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/combat/combat_ai.h"
#include "goldbox/combat/combat_setup.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/combat_renderer.h"

// Forward declarations — avoid pulling full headers into the Poolrad::Views
// namespace where unqualified 'Data::' would resolve to Goldbox::Poolrad::Data.
namespace Goldbox {
namespace Data {
class PlayerCharacter;
namespace Effects {
class EffectRuntime;
class EffectHostBridge;
} // namespace Effects
} // namespace Data
namespace Gfx {
class Pic;
} // namespace Gfx
} // namespace Goldbox

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

    /**
     * Apply damage to a character: runs data mutation, plays the damage
     * tile animation via drawDamage, interrupts spell casting, and
     * handles death. Mirrors COMBAT_ApplyDamageMessage.
     */
    void applyDamageMessage(Goldbox::Data::PlayerCharacter *ch,
                            uint8 baseDamage,
                            Goldbox::Data::DamageModifier modifier,
                            bool applyModifier);

    void drawDamage(Goldbox::Data::PlayerCharacter *ch,
                    bool isMagic,
                    const Common::String &message);

    // Called when a character dies outside the normal damage path
    // (e.g. poison wear-off via EffectHostBridge::onCharacterDied).
    void handleDeathOnMap(Goldbox::Data::PlayerCharacter *ch);

    bool msgFocus(const FocusMessage &msg) override;
    bool msgUnfocus(const UnfocusMessage &msg) override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    bool tick() override;

    /** Debug read-only access for console dumps. */
    const Combat::BattlefieldMap &debugBattlefieldMap() const {
        return _battlefieldMap;
    }

    /** Debug read-only access to the currently rendered tilemap. */
    const Gfx::BattlefieldTilemap &debugTilemap() const {
        return _tilemap;
    }

    /** Debug read-only access to combat terrain tile cache. */
    const Gfx::CombatTileCache &debugTileCache() const {
        return _tileCache;
    }

    /** Debug read-only access to combatant placement table. */
    const Combat::CombatantTable &debugCombatantTable() const {
        return _table;
    }

    /** Debug: number of player-controlled characters in the roster. */
    int debugPartyCount() const {
        return _params.partyCount;
    }

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
    Goldbox::Data::PlayerCharacter *_currentActor; // actor taking its turn this tick

    // --- Effect bridge (wired at setup time) ---
    Goldbox::Data::Effects::EffectHostBridge *_bridge = nullptr;

    // --- Rendering ---
    Gfx::BattlefieldTilemap _tilemap;
    Gfx::CombatTileCache _tileCache;
    Gfx::CombatRenderer _combatRenderer;
    bool _needsFullRedraw;

    // --- Layout constants (pixel coords on 320x200 screen) ---
    static const int kViewportX = 8;      // pixel X of viewport area
    static const int kViewportY = 8;      // pixel Y of viewport area
    static const int kTileSize = 24;      // pixels per tile
    static const int kViewportPixelW = Combat::CombatViewport::VIEW_COLS * kTileSize;
    static const int kViewportPixelH = Combat::CombatViewport::VIEW_ROWS * kTileSize;

    // --- Internal methods ---
    Combat::CombatContext makeContext();
    void drawDamageFrame(const Goldbox::Gfx::Pic *frame, int pixX, int pixY,
                         Graphics::ManagedSurface *dst);

    void drawViewport();
    void drawCombatants();
    void drawUI();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_COMBAT_VIEW_H
