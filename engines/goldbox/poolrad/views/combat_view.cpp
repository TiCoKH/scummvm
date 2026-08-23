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
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/effects/effect_host_bridge.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/poolrad/data/poolrad_tile_props.h"
#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/engine.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"
#include "goldbox/data/damage_utils.h"
#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

CombatView::CombatView()
    : View("Combat"), _phase(PHASE_NONE),
      _needsFullRedraw(true) {
}

CombatView::~CombatView() {
}

void CombatView::setup(const Combat::CombatParams &params) {
    _params = params;
    _globals.turnCounter = 0;
    _phase = PHASE_SETUP;
    _bridge = Goldbox::Poolrad::getEffectHostBridge();

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

    // Run the full COMBAT_Setup sequence.
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
    // TODO: AI turn execution, animation stepping.
    // At the end of each full combat turn, increment the turn counter:
    //   _globals.turnCounter++;
    // This mirrors COMBAT_TURN_COUNTER = COMBAT_TURN_COUNTER + 1 in
    // DIALOG_CombatEnd, which fires once per turn after all characters act.
    return false;
}

// --- Internal ---

Combat::CombatContext CombatView::makeContext() {
    return Combat::CombatContext(_globals, _params, _table, _battlefieldMap,
                                 _viewport);
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

        ::Goldbox::Data::PlayerCharacter *ch = _table.getCharacter(i);
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
                pic->trDraw(&s, pixX, pixY, pic->getTransparentIndex());
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

void CombatView::applyDamageMessage(::Goldbox::Data::PlayerCharacter *ch,
    uint8 baseDamage, ::Goldbox::Data::DamageModifier modifier,
    bool applyModifier) {
    if (!ch)
        return;

    Goldbox::Data::DamageSystem damageSystem(nullptr);
    const Goldbox::Data::DamageResult r = damageSystem.applyLegacy(
        *ch, baseDamage, modifier, applyModifier, _globals.behaviorFlags);

    if (r.applied <= 0)
        return;

    const bool isMagic =
            (_globals.behaviorFlags & Combat::CombatGlobals::DMG_MAGIC) ==
            _globals.behaviorFlags;

    // Mirrors COMBAT_DrawDamage: render the character-aware message before
    // playing the hit animation. GameText owns TEXT_BlockPrint-compatible
    // wrapping and the legacy combat message-box coordinates.
    if (g_engine) {
        g_engine->getGameText().showMessage(ch, r.message, 10, false);
        Surface messageSurface = getSurface();
        while (g_engine->getGameText().advance())
            ;
        g_engine->getGameText().draw(messageSurface);
        g_system->updateScreen();
    }

    drawDamage(ch, isMagic, r.message);

    if (ch->combatState)
        ch->combatState->canCast = false;

    if (r.interruptedSpell && _bridge)
        _bridge->postEffectMessage(ch, r.spellLostMessage, true);

    if (r.wentDown) {
        if (_bridge)
            _bridge->postEffectMessage(ch, r.downMessage, false);

        if (r.killed)
            handleDeathOnMap(ch);
        else if (_bridge)
            _bridge->requestRefresh(
                ::Goldbox::Data::Effects::EffectHostBridge::RF_VIEWPORT);
    }

    if (_bridge)
        _bridge->requestRefresh(
            ::Goldbox::Data::Effects::EffectHostBridge::RF_STATUS_PANEL);
}

void CombatView::handleDeathOnMap(::Goldbox::Data::PlayerCharacter *ch) {
    // TODO: remove character token from battlefield, update ground state.
    // Mirrors COMBAT_HandleDeathOnMap.
    (void)ch;
    _needsFullRedraw = true;
    if (_bridge)
        _bridge->requestRefresh(
            ::Goldbox::Data::Effects::EffectHostBridge::RF_VIEWPORT);
}

    void CombatView::drawDamage(::Goldbox::Data::PlayerCharacter *ch,
        bool isMagic, const Common::String &message) {
    // Outside combat: message only, no animation.
    if (_phase == PHASE_NONE || _phase == PHASE_ENDED) {
        if (_bridge)
            _bridge->postEffectMessage(ch, message, true);
        return;
    }

    // Tile IDs: 0x16 = magic damage effect, 0x17 = normal damage effect.
    // Each is a 4-frame sprite strip in SPRIT.DAX.
    const uint8 effectTileId = isMagic ? 0x16 : 0x17;

    // Load all 4 effect frames up front.
    Gfx::Pic *frames[4] = {};
            ::Goldbox::Data::DaxBlockContainer &sprit = g_engine->getDaxSprit();
            ::Goldbox::Data::DaxBlock *block = sprit.getBlockById(effectTileId);
            ::Goldbox::Data::DaxBlockSprit *spritBlock =
                block ? dynamic_cast<::Goldbox::Data::DaxBlockSprit *>(block)
                  : nullptr;
    if (spritBlock) {
        for (int f = 0; f < 4; ++f)
            frames[f] = Gfx::Pic::readSpriteFrame(spritBlock, f);
    }

    // Ensure character is visible; scroll viewport if needed.
    const int idx = _table.findIndex(ch);
    if (idx >= 0) {
        const uint8 col = _table.getTileCol(idx);
        const uint8 row = _table.getTileRow(idx);
        if (!_viewport.isTileVisible(col, row)) {
            _viewport.adjustToInclude(col, row);
            _table.setViewportOrigin(_viewport.getTopLeftCol(),
                                     _viewport.getTopLeftRow());
            drawViewport();
            drawCombatants();
            g_system->updateScreen();
        }
    }

    // Sound.
    // SOUND_ID_MAGIC_DAMAGE = 6, SOUND_ID_NORMAL_DAMAGE = 5 (Poolrad values).
    g_engine->soundPlay(isMagic ? 6 : 5);

    // Animation: magic repeats CFG_GAME_SPEED times, normal runs once.
    // CFG_GAME_SPEED maps to g_engine->getTextDelay() (1-5).
    const int repeatCount = isMagic ? (int)g_engine->getTextDelay() : 0;

    // Pixel position of the character in the viewport.
    // colDist/rowDist are viewport-local tile coords (0-based).
    int pixX = kViewportX;
    int pixY = kViewportY;
    if (idx >= 0) {
        pixX = kViewportX + _table.getColDist(idx) * kTileSize;
        pixY = kViewportY + _table.getRowDist(idx) * kTileSize;
    }

    Surface screenSurface = getSurface();
    Graphics::ManagedSurface *screen = static_cast<Graphics::ManagedSurface *>(&screenSurface);

    for (int rep = 0; rep <= repeatCount; ++rep) {
        for (int f = 0; f < 4; ++f) {
            if (frames[f])
                drawDamageFrame(frames[f], pixX, pixY, screen);

            g_system->updateScreen();
            g_system->delayMillis(46); // mirrors Wait_cycle(0x46)

            // Restore underlying tile by reblitting the tilemap region.
            Common::Rect tileRect(pixX, pixY,
                                  pixX + kTileSize, pixY + kTileSize);
            Common::Rect srcRect(pixX - kViewportX, pixY - kViewportY,
                                 pixX - kViewportX + kTileSize,
                                 pixY - kViewportY + kTileSize);
            _tilemap.blitTo(screen,
                            Common::Point(tileRect.left, tileRect.top),
                            srcRect);
        }
    }

    g_system->updateScreen();

    // Normal damage: one extra wait after animation.
    if (repeatCount == 0)
        g_system->delayMillis(200);

    for (int f = 0; f < 4; ++f)
        delete frames[f];
}

void CombatView::drawDamageFrame(const Gfx::Pic *frame, int pixX, int pixY,
        Graphics::ManagedSurface *dst) {
    frame->trDraw(dst, pixX, pixY, frame->getTransparentIndex());
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
