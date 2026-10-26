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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_VIEWPORT_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_VIEWPORT_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/gfx/battlefield_tilemap.h"
#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/combat_renderer.h"
#include "common/array.h"

namespace Graphics {
class ManagedSurface;
} // namespace Graphics

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data
namespace Gfx {
class Pic;
} // namespace Gfx

namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * Left combat window (cols 1-21, rows 1-21): battlefield terrain,
 * combatant icons and all battlefield animations.
 *
 * Presentation only. Reads combat state from the bound CombatSession and
 * may move the camera (CombatViewport), but never changes combatants,
 * facing or turn state; CombatView applies those before calling in.
 * Mirrors the original COMBAT_RedrawViewport family of routines.
 *
 * Not attached to the UI tree: CombatView draws it in a fixed order.
 */
class CombatViewportDialog : public Dialog {
public:
    static const int kViewportX = 8;
    static const int kViewportY = 8;
    static const int kTileSize  = 24;
    static const uint8 kDefaultFrameDelay = 46;

    CombatViewportDialog();
    ~CombatViewportDialog() override {}

    /** Bind the session whose battlefield is presented. */
    void bindSession(::Goldbox::Combat::CombatSession *session) {
        _session = session;
    }

    /** Load the dungeon or wilderness combat tileset. */
    void loadTileset(bool isDungeon);

    /** Load the selection frame and dying icons into the icon slots. */
    void loadCombatIcons();

    /** Render the whole battlefield map into the terrain cache. */
    void rebuildTerrain();

    /** Re-render changed map tiles. Returns true if any were dirty. */
    bool refreshDirtyTiles();

    /** Full viewport redraw: terrain, then combatants. */
    void draw() override;

    void drawTerrain();
    void drawCombatants(
        ::Goldbox::Data::PlayerCharacter *skipCharacter = nullptr);

    /** Keep an actor's attack pose across result and animation redraws. */
    void setAttackPose(::Goldbox::Data::PlayerCharacter *ch, bool attacking);

    /** Center the camera on a tile and redraw the viewport. */
    void centerOn(TilePos center);

    /** Draw an SPRIT effect tile on top of a character's viewport cell. */
    void drawEffectAt(::Goldbox::Data::PlayerCharacter *ch, uint8 blockId);

    /** Walk ch's icon along the path from start to end. */
    void animateMove(::Goldbox::Data::PlayerCharacter *ch,
                     TilePos start, TilePos end,
                     uint8 initialFrame = 0,
                     uint8 frameDelay = kDefaultFrameDelay);

    /**
     * Projectile path from start to end. Uses the current actor's icon as
     * the moving sprite (dedicated projectile sprites are not animated yet).
     */
    void animateProjectile(TilePos start, TilePos end, uint8 frameDelay);

    /** Weapon-type specific launch effect plus projectile path. */
    void animateRangedAttack(::Goldbox::Data::PlayerCharacter *attacker,
                             ::Goldbox::Data::PlayerCharacter *target,
                             uint8 weaponType);

    /** Damage frames over the target (normal or magic). */
    void animateDamage(::Goldbox::Data::PlayerCharacter *target,
                       bool isMagic);

    /** Scroll ch into view and overlay the dying icon on its footprint. */
    void showDying(::Goldbox::Data::PlayerCharacter *ch);

    const ::Goldbox::Gfx::BattlefieldTilemap &getTilemap() const { return _tilemap; }
    const ::Goldbox::Gfx::CombatTileCache &getTileCache() const { return _tileCache; }

private:
    struct MovementAnimStep {
        ScreenPos finePos;  // fine-grid position relative to viewport top-left
    };

    static const int kFinePerTile = 3;
    static const int kAnimFrameCount = 4;
    static const int kPanTiles = 3;

    // Original loads COMSPR blocks 0-11 into icon slots 13-24; the dying
    // icon drawn by COMBAT_HandleDeathOnMap is slot 24 (COMSPR block 11).
    static const uint8 kDyingIconSlot  = 24;
    static const uint8 kDyingIconBlock = 11;
    static const uint8 kSelectFrameBlock = 25;

    ::Goldbox::Combat::CombatSession *_session = nullptr;
    ::Goldbox::Gfx::BattlefieldTilemap _tilemap;
    ::Goldbox::Gfx::CombatTileCache _tileCache;
    ::Goldbox::Gfx::CombatRenderer _combatRenderer;
    Common::Array<::Goldbox::Data::PlayerCharacter *> _attackingCharacters;

    bool isAttacking(::Goldbox::Data::PlayerCharacter *ch) const;

    bool cellOf(::Goldbox::Data::PlayerCharacter *ch,
                Common::Point &pix) const;

    void renderEffectTile(uint8 blockId, int frameIdx,
                          int pixX, int pixY, bool withLayer = false);

    Common::Array<MovementAnimStep> buildMovementAnimationPath(
        TilePos start, TilePos end);

    void drawMovementAnimation(const ScreenPos &finePos, uint8 animFrame,
                               uint8 frameDelay,
                               ::Goldbox::Data::PlayerCharacter *ch,
                               ::Goldbox::Gfx::IconDirection iconDir,
                               Graphics::ManagedSurface *screen);

    static uint8 getNextAnimationFrame(uint8 frame) {
        return (frame + 1) % kAnimFrameCount;
    }

    Common::Point fineToPixel(ScreenPos fine) const {
        return Common::Point(kViewportX + fine.col * (kTileSize / kFinePerTile),
                             kViewportY + fine.row * (kTileSize / kFinePerTile));
    }
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_VIEWPORT_DIALOG_H
