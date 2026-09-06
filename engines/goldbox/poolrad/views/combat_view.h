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
#include "goldbox/combat/combat_session.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/combat_renderer.h"
#include "goldbox/poolrad/views/dialogs/combat_menu_dialog.h"

// Forward declarations — avoid pulling full headers into the Poolrad::Views
// namespace where unqualified 'Data::' would resolve to Goldbox::Poolrad::Data.
namespace Goldbox {
namespace Data {
class PlayerCharacter;
namespace Effects {
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
 * Tactical combat view — presentation and input only.
 *
 * Owns a CombatSession for all combat logic and state.
 * Responsible for: terrain/combatant rendering, damage animation,
 * viewport scroll input, and reacting to TickResult events from the session.
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
    void handleMenuResult(const MenuResultMessage &result) override;

    // --- Debug accessors ---
    const Combat::BattlefieldMap &debugBattlefieldMap() const {
        return _session.getBattlefieldMap();
    }
    const Gfx::BattlefieldTilemap &debugTilemap() const {
        return _tilemap;
    }
    const Gfx::CombatTileCache &debugTileCache() const {
        return _tileCache;
    }
    const Combat::CombatantTable &debugCombatantTable() const {
        return _session.getTable();
    }
    int debugPartyCount() const {
        return _session.getParams().partyCount;
    }

private:
    // --- Backend ---
    Combat::CombatSession _session;

    // --- Dialogs ---
    Dialogs::CombatMenuDialog *_combatMenu;

    // --- Effect bridge (wired at setup time) ---
    Goldbox::Data::Effects::EffectHostBridge *_bridge = nullptr;

    // --- Rendering ---
    Gfx::BattlefieldTilemap _tilemap;
    Gfx::CombatTileCache    _tileCache;
    Gfx::CombatRenderer     _combatRenderer;
    bool                    _needsFullRedraw;

    // --- Layout constants ---
    static const int kViewportX = 8;
    static const int kViewportY = 8;
    static const int kTileSize  = 24;
    static const int kViewportPixelW =
        Combat::CombatViewport::VIEW_COLS * kTileSize;
    static const int kViewportPixelH =
        Combat::CombatViewport::VIEW_ROWS * kTileSize;
    static const uint8 kBackgroundColor = 8;

    // --- Window layout (character coords, matching original SCREEN_DrawWindow calls) ---
    static const int kWin1Left   = 1;
    static const int kWin1Top    = 1;
    static const int kWin1Right  = 21;
    static const int kWin1Bottom = 21;
    static const int kWin2Left   = 23;
    static const int kWin2Top    = 1;
    static const int kWin2Right  = 38;
    static const int kWin2Bottom = 21;

    // --- Internal methods ---
    void drawViewport();
    void drawCombatants();
    void drawUI();
    void drawDamageFrame(const Goldbox::Gfx::Pic *frame, int pixX, int pixY,
                         Graphics::ManagedSurface *dst);
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_COMBAT_VIEW_H
