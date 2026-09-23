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
#include "goldbox/poolrad/views/dialogs/combat_move_dialog.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/poolrad/data/poolrad_tile_props.h"
#include "goldbox/gfx/combat_tile_cache.h"
#include "goldbox/gfx/icon_manager.h"
#include "goldbox/gfx/icon.h"
#include "goldbox/engine.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"
#include "goldbox/combat/damage_utils.h"
#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

CombatView::CombatView()
    : View("Combat"), _needsFullRedraw(true), _combatMenu(nullptr),
      _combatMove(nullptr), _currentInfoActor(nullptr) {
    _combatMenu = new Dialogs::CombatMenuDialog();
    _combatMove = new Dialogs::CombatMoveDialog();
}

CombatView::~CombatView() {
    delete _combatMenu;
    delete _combatMove;
}

void CombatView::setup(const Combat::CombatParams &params) {
    _bridge = Goldbox::Poolrad::getEffectHostBridge();

    g_engine->getPictureDisplayCache().clear();

    if (params.isDungeon) {
        _tileCache.loadDungeon(g_engine->getDaxDungcom(),
                               g_engine->getDaxRandcom());
    } else {
        _tileCache.loadWilderness(g_engine->getDaxWildcom(),
                                   g_engine->getDaxRandcom());
    }

    Combat::CombatParams p = params;
    p.tilePropertyProvider = &PoolradTilePropertyProvider::instance();
    _session.setup(p);
    Combat::g_combatSession = &_session;

    _tilemap.render(_session.getBattlefieldMap(), _tileCache,
                    _session.getBattlefieldMap().getTilePropertyProvider());

    // Load selection cursor frame from COMSPR block 25 (mirrors set_icon SLOT_SELECTFRAME).
    Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (iconMgr)
        iconMgr->loadIcon(Gfx::SLOT_SELECTFRAME, Gfx::ICON_KIND_SPRITE, 25);

    _needsFullRedraw = true;
}

bool CombatView::msgFocus(const FocusMessage &msg) {
    View::msgFocus(msg);
    _needsFullRedraw = true;
    redraw();
    return true;
}

bool CombatView::msgUnfocus(const UnfocusMessage &msg) {
    if (Combat::g_combatSession == &_session)
        Combat::g_combatSession = nullptr;
    return View::msgUnfocus(msg);
}

bool CombatView::msgKeypress(const KeypressMessage &msg) {
    if (_session.isEnded()) {
        close();
        return true;
    }

    if (_session.getPhase() != Combat::CombatSession::PHASE_PLAYER_TURN)
        return false;

    Combat::CombatViewport &vp = _session.getViewport();
    TilePos center = vp.getCenter();

    switch (msg.keycode) {
    case Common::KEYCODE_ESCAPE:
        close();
        return true;
    case Common::KEYCODE_LEFT:
        _session.scrollViewport(TilePos((uint8)(center.col - 1), center.row));
        break;
    case Common::KEYCODE_RIGHT:
        _session.scrollViewport(TilePos((uint8)(center.col + 1), center.row));
        break;
    case Common::KEYCODE_UP:
        _session.scrollViewport(TilePos(center.col, (uint8)(center.row - 1)));
        break;
    case Common::KEYCODE_DOWN:
        _session.scrollViewport(TilePos(center.col, (uint8)(center.row + 1)));
        break;
    default:
        return false;
    }

    _needsFullRedraw = true;
    redraw();
    return true;
}

void CombatView::draw() {
    if (_session.getPhase() == Combat::CombatSession::PHASE_NONE)
        return;

    Combat::BattlefieldMap &map = _session.getBattlefieldMap();

    if (map.hasDirtyTiles()) {
        _tilemap.renderDirtyTiles(map, _tileCache, map.getTilePropertyProvider());
        _needsFullRedraw = true;
    }

    if (_needsFullRedraw) {
        drawUI();
        drawViewport();
        drawCombatants();
        if (_currentInfoActor)
            drawCombatInfo(_currentInfoActor);
        if (_combatMenu && _combatMenu->isActive())
            _combatMenu->draw();
        _needsFullRedraw = false;
    }
}

bool CombatView::tick() {
    // Don't advance while waiting for player input — menu keypresses drive that.
    if (_session.getPhase() == Combat::CombatSession::PHASE_NONE ||
            _session.getPhase() == Combat::CombatSession::PHASE_AWAITING_PLAYER ||
            _session.isEnded())
        return false;

    const Combat::CombatSession::TickResult result = _session.tick();

    if (result.event == Combat::CombatSession::TickResult::EV_NONE)
        return false;

    // Focus viewport on the new actor (mirrors COMBAT_FocusCharacter radius=2).
    if (result.actor) {
        const Combat::CombatantTable &table = _session.getTable();
        const int idx = table.findIndex(result.actor);
        if (idx >= 0)
            _session.scrollViewport(
                TilePos(table.getTileCol(idx), table.getTileRow(idx)), 2);
    }

    // Party actor reached — mirrors DIALOG_CombatMain entry checks.
    if (_session.getPhase() == Combat::CombatSession::PHASE_AWAITING_PLAYER) {
        ::Goldbox::Data::PlayerCharacter *actor = result.actor;
        ::Goldbox::Data::CombatAction *cs = actor ? actor->combatState : nullptr;

        // Disabled character: reset and skip menu (mirrors !character->enabled path).
        if (actor && !actor->enabled) {
            if (cs) cs->reset();
            _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
            _needsFullRedraw = true;
            return true;
        }

        // Pre-selected spell: consume and skip menu (mirrors spell_id != 0 path).
        if (cs && cs->spellId != 0) {
            cs->spellId = 0;
            cs->reset();
            _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
            _needsFullRedraw = true;
            return true;
        }

        _currentInfoActor = actor;
        if (_combatMenu && !_combatMenu->isActive()) {
            attachDialog(_combatMenu);
            _combatMenu->activate();
        }
        _needsFullRedraw = true;
        return true;
    }

    // AI turn completed synchronously — apply damage and redraw.
    if (result.event == Combat::CombatSession::TickResult::EV_AI_ATTACK &&
            result.target && result.damage > 0)
        applyDamageMessage(result.target, (uint8)result.damage,
                           ::Goldbox::Data::DAMAGE_NORMAL, false);

    _needsFullRedraw = true;
    return true;
}

void CombatView::handleMenuResult(const MenuResultMessage &result) {
    if (!result._success || !result._hasIntValue)
        return;

    const Combat::CombatSession::PlayerAction action =
        static_cast<Combat::CombatSession::PlayerAction>(result._intValue);

    // PA_MOVE result from CombatMoveDialog — turn already advanced inside
    // finishMoveAction(); just dismiss the move dialog and redraw.
    if (action == Combat::CombatSession::PA_MOVE &&
            _combatMove && _combatMove->isActive()) {
        _combatMove->deactivate();
        detachDialog(_combatMove);
        _currentInfoActor = nullptr;
        _needsFullRedraw = true;
        return;
    }

    // Dismiss the action menu.
    if (_combatMenu && _combatMenu->isActive()) {
        _combatMenu->deactivate();
        detachDialog(_combatMenu);
    }
    _currentInfoActor = nullptr;

    // Launch move dialog.
    if (action == Combat::CombatSession::PA_MOVE) {
        ::Goldbox::Data::PlayerCharacter *actor = _session.getCurrentActor();
        if (actor && _combatMove) {
            _combatMove->beginMove(actor);
            attachDialog(_combatMove);
            _combatMove->activate();
        }
        _needsFullRedraw = true;
        return;
    }

    const Combat::CombatSession::TickResult tickResult =
        _session.submitPlayerAction(action);

    if (tickResult.event == Combat::CombatSession::TickResult::EV_AI_ATTACK &&
            tickResult.target && tickResult.damage > 0) {
        applyDamageMessage(tickResult.target, (uint8)tickResult.damage,
                           ::Goldbox::Data::DAMAGE_NORMAL, false);
    }

    _needsFullRedraw = true;
}

// ---------------------------------------------------------------------------
// Draw helpers

void CombatView::drawViewport() {
    if (!_tilemap.isBuilt())
        return;

    const Combat::CombatViewport &vp = _session.getViewport();
    Common::Rect srcRect = vp.getSourceRect(kTileSize);
    Surface s = getSurface();
    _tilemap.blitTo(&s, Common::Point(kViewportX, kViewportY), srcRect);
}

void CombatView::drawCombatants() {
    Surface s = getSurface();
    const Combat::CombatantTable &table = _session.getTable();

    for (int i = 0; i < table.getCount(); i++) {
        if (table.getSize(i) == 0)
            continue;

        const Combat::ViewportPos vpos =
            _session.getViewport().getCharacterViewportPosition(table, i);
        const int16 localCol = vpos.column;
        const int16 localRow = vpos.row;

        if (!vpos.isVisible(Combat::CombatViewport::VIEW_COLS,
                             Combat::CombatViewport::VIEW_ROWS))
            continue;

        ::Goldbox::Data::PlayerCharacter *ch = table.getCharacter(i);
        if (!ch)
            continue;

        Gfx::IconDirection dir = Gfx::ICON_DIRECTION_RIGHT;
        if (ch->combatState) {
            uint8 facing = ch->combatState->direction;
            if (facing >= 5 || facing == 0)
                dir = Gfx::ICON_DIRECTION_LEFT;
        }

        const uint8 slotId = ch->iconData.iconSlotId;
        int pixX = kViewportX + localCol * kTileSize;
        int pixY = kViewportY + localRow * kTileSize;
        Gfx::IconManager *iconMgr = VmInterface::getIconManager();

        // Draw selection cursor under the character (cursor first, icon on top).
        if (ch == _session.getCurrentActor() && iconMgr &&
                !iconMgr->isSlotEmpty(Gfx::SLOT_SELECTFRAME)) {
            const Gfx::Pic *cursor = iconMgr->getReadyPic(Gfx::SLOT_SELECTFRAME);
            if (cursor)
                cursor->trDraw(&s, pixX, pixY, Gfx::Icon::TRANSPARENT_COLOR_INDEX);
        }

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
    s.clear(kBackgroundColor);
    s.drawWindow(kWin1Left, kWin1Top, kWin1Right, kWin1Bottom, kBackgroundColor);
    s.drawWindow(kWin2Left, kWin2Top, kWin2Right, kWin2Bottom, kBackgroundColor);
}

// ---------------------------------------------------------------------------
// Damage

void CombatView::applyDamageMessage(::Goldbox::Data::PlayerCharacter *ch,
        uint8 baseDamage, ::Goldbox::Data::DamageModifier modifier,
        bool applyModifier) {
    if (!ch)
        return;

    Goldbox::Data::DamageSystem damageSystem(nullptr);
    const Goldbox::Data::DamageResult r = damageSystem.applyLegacy(
        *ch, baseDamage, modifier, applyModifier,
        _session.getGlobals().behaviorFlags);

    if (r.applied <= 0)
        return;

    const bool isMagic =
        (_session.getGlobals().behaviorFlags & Combat::CombatGlobals::DMG_MAGIC) ==
        _session.getGlobals().behaviorFlags;

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
    (void)ch;
    _needsFullRedraw = true;
    if (_bridge)
        _bridge->requestRefresh(
            ::Goldbox::Data::Effects::EffectHostBridge::RF_VIEWPORT);
}

void CombatView::drawDamage(::Goldbox::Data::PlayerCharacter *ch,
        bool isMagic, const Common::String &message) {
    if (_session.getPhase() == Combat::CombatSession::PHASE_NONE ||
            _session.isEnded()) {
        if (_bridge)
            _bridge->postEffectMessage(ch, message, true);
        return;
    }

    const uint8 effectTileId = isMagic ? 0x16 : 0x17;

    Gfx::Pic *frames[4] = {};
    ::Goldbox::Data::DaxBlockContainer &sprit = g_engine->getDaxSprit();
    ::Goldbox::Data::DaxBlock *block = sprit.getBlockById(effectTileId);
    ::Goldbox::Data::DaxBlockSprit *spritBlock =
        block ? dynamic_cast<::Goldbox::Data::DaxBlockSprit *>(block) : nullptr;
    if (spritBlock) {
        for (int f = 0; f < 4; ++f)
            frames[f] = Gfx::Pic::readSpriteFrame(spritBlock, f);
    }

    const Combat::CombatantTable &table = _session.getTable();
    const int idx = table.findIndex(ch);
    if (idx >= 0) {
        const uint8 col = table.getTileCol(idx);
        const uint8 row = table.getTileRow(idx);
        if (!_session.getViewport().isTileVisible(TilePos(col, row))) {
            _session.scrollViewport(TilePos(col, row));
            drawViewport();
            drawCombatants();
            g_system->updateScreen();
        }
    }

    g_engine->soundPlay(isMagic ? 6 : 5);

    const int repeatCount = isMagic ? (int)g_engine->getTextDelay() : 0;

    int pixX = kViewportX;
    int pixY = kViewportY;
    if (idx >= 0) {
        const Combat::ViewportPos vpos =
            _session.getViewport().getCharacterViewportPosition(table, idx);
        pixX = kViewportX + vpos.column * kTileSize;
        pixY = kViewportY + vpos.row    * kTileSize;
    }

    Surface screenSurface = getSurface();
    Graphics::ManagedSurface *screen =
        static_cast<Graphics::ManagedSurface *>(&screenSurface);

    for (int rep = 0; rep <= repeatCount; ++rep) {
        for (int f = 0; f < 4; ++f) {
            if (frames[f])
                drawDamageFrame(frames[f], pixX, pixY, screen);

            g_system->updateScreen();
            g_system->delayMillis(46);

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

void CombatView::drawCombatInfo(::Goldbox::Data::PlayerCharacter *ch) {
    if (!ch)
        return;

    Surface s = getSurface();

    // Clear the info panel (col 23-38, rows 1-21).
    // Mirrors SCREEN_ClearRect(23, 1, 38, 21).
    s.clearBox(23, 1, 38, 21, kBackgroundColor);

    // Row 1: character name.
    s.writeStringC(23, 1, 10, ch->name);

    // Row 3: Hitpoints.
    s.writeStringC(23, 3, 10, "Hitpoints");
    s.writeStringC(33, 3, 10, Common::String::format("%d/%d",
        ch->hitPoints.current, ch->hitPoints.max));

    // Row 5: Armor class.
    s.writeStringC(23, 5, 10, "AC");
    s.writeStringC(26, 5, 10, Common::String::format("%d",
        ch->armorClass.getCurrent()));

    // Row 6: status (anchored to AC row + 2, mirrors BYTE_CHAR_POS_Y + 2).
    if (!ch->enabled) {
        s.writeStringC(23, 6, 10, ch->getStatusName());
    } else {
        const ::Goldbox::Data::Effects::CharacterEffects *fx = ch->getEffects();
        if (fx && (
                fx->hasEffect(::Goldbox::Data::Effects::E_POOLRAD_HELPLESS) ||
                fx->hasEffect(::Goldbox::Data::Effects::E_POOLRAD_HELPLESS_33) ||
                fx->hasEffect(::Goldbox::Data::Effects::E_POOLRAD_HELPLESS_34) ||
                fx->hasEffect(::Goldbox::Data::Effects::E_POOLRAD_HELPLESS_35) ||
                fx->hasEffect(::Goldbox::Data::Effects::E_POOLRAD_NAUSEATED) ||
                fx->hasEffect(::Goldbox::Data::Effects::E_POOLRAD_ENFEEBLED)))
            s.writeStringC(23, 6, 10, "(Helpless)");
    }

    // Row 7: equipped weapon name (if any).
    const ::Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<const ::Goldbox::Data::ADnDCharacter *>(ch);
    const ::Goldbox::Data::Items::CharacterItem *weapon = adnd ?
        adnd->getEquippedItem(::Goldbox::Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon)
        s.writeStringC(23, 7, 10, weapon->getDisplayName());
}

void CombatView::drawDamageFrame(const Gfx::Pic *frame, int pixX, int pixY,
        Graphics::ManagedSurface *dst) {
    frame->trDraw(dst, pixX, pixY, frame->getTransparentIndex());
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
