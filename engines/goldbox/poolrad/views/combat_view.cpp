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
#include "goldbox/core/field_path.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/data/daxblockcontainer.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
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
#include "goldbox/data/items/character_item.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_ai.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/gfx/game_text.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

// ---------------------------------------------------------------------------
// Concrete AiMoveViewDelegate — routes AI presentation through CombatView

struct CombatViewAiDelegate : public Combat::AiMoveViewDelegate {
    CombatView *_view;
    explicit CombatViewAiDelegate(CombatView *v) : _view(v) {}

    // Let the normal frame loop draw and flip — mirrors the ECL→InGameView pattern.
    // redraw() sets _needsRedraw so drawElements()→draw() renders correctly,
    // then pumpModalInputFrame() calls drawElements()+update() to show it.
    void present(uint32 delayMs = 300) {
        if (!g_events)
            return;
        _view->forceFullRedraw();
        _view->draw();
        if (g_events->getScreen()) {
            g_events->getScreen()->makeAllDirty();
            g_events->getScreen()->update();
        }
        g_system->delayMillis(delayMs);
    }

    void drawMoveRemaining(uint8 moveHalf) override {
        _view->setAiMessage(
            Common::String::format("Move/Attack, Move Left = %d", moveHalf));
        present();
    }

    void showMessage(const char *msg) override {
        _view->setAiMessage(Common::String(msg));
        present();
    }

    void updateCharacterFacingAndRedraw(::Goldbox::Data::PlayerCharacter *ch,
                                        uint8 direction) override {
        _view->setAiCharacterFacing(ch, static_cast<Direction>(direction));
        present();
    }

    void drawCombatInfo(::Goldbox::Data::PlayerCharacter *ch) override {
        _view->setAiInfoActor(ch);
        present();
    }

    void clearPromptAndMessage() override {
        if (g_engine)
            g_engine->getGameText().clearMessageArea();
    }

    void showFleesInPanic(::Goldbox::Data::PlayerCharacter *ch) override {
        _view->printCombatMessage(ch, "Flees in panic!", 1);
    }
};

// ---------------------------------------------------------------------------
// Concrete CombatViewDelegate — routes attack result messages through CombatView

struct CombatViewAttackDelegate : public Combat::CombatViewDelegate {
    CombatView *_view;
    explicit CombatViewAttackDelegate(CombatView *v) : _view(v) {}

    void flipScreen(uint32 delayMs = 0) {
        if (!g_events)
            return;
        _view->forceFullRedraw();
        _view->draw();
        if (g_events->getScreen()) {
            g_events->getScreen()->makeAllDirty();
            g_events->getScreen()->update();
        }
        if (delayMs > 0)
            g_system->delayMillis(delayMs);
    }

    void drawCombatInfo(::Goldbox::Data::PlayerCharacter *attacker) override {
        _view->setAiInfoActor(attacker);
        flipScreen(200);
    }

    void updateCharacterFacingAndRedraw(::Goldbox::Data::PlayerCharacter *ch,
                                        uint8 direction, uint8 /*redrawMode*/,
                                        bool restoreOld) override {
        _view->setAiCharacterFacing(ch, static_cast<Direction>(direction));
        flipScreen(150);
    }

    void animateRangedAttack(::Goldbox::Data::PlayerCharacter *attacker,
                             ::Goldbox::Data::PlayerCharacter *target,
                             ::Goldbox::Data::Items::CharacterItem *item) override {
        _view->animateRangedAttack(attacker, target,
            static_cast<const ::Goldbox::Data::Items::CharacterItem *>(item));
    }

    void drawAttackResult(::Goldbox::Data::PlayerCharacter *attacker,
                          ::Goldbox::Data::PlayerCharacter *target,
                          uint8 /*facingMode*/, uint8 damage,
                          uint8 damageDisplay, uint8 resultType) override {
        if (resultType == 0) {
            _view->printCombatMessage(attacker, "Misses!", 10);
        } else {
            _view->printCombatMessage(attacker,
                Common::String::format("Hits for %d!", damageDisplay), 10);
            if (damage > 0)
                _view->applyDamageMessage(target, damage,
                    ::Goldbox::Data::DAMAGE_NORMAL, false);
        }
    }
};


CombatView::CombatView()
    : View("Combat"), _needsFullRedraw(true), _combatMenu(nullptr),
      _combatMove(nullptr), _combatEnd(nullptr), _currentInfoActor(nullptr),
      _playerActionPending(false),
      _pendingPlayerAction(Combat::CombatSession::PA_NONE),
      _hasAiMessage(false), _aiFacingChar(nullptr), _aiFacingDir(DIR_N) {
    _combatMenu = new Dialogs::CombatMenuDialog();
    _combatMove = new Dialogs::CombatMoveDialog();
    _combatEnd  = new Dialogs::CombatEndDialog();
    _aiDelegate     = new CombatViewAiDelegate(this);
    _attackDelegate = new CombatViewAttackDelegate(this);
}

CombatView::~CombatView() {
    delete _combatMenu;
    delete _combatMove;
    delete _combatEnd;
    delete _aiDelegate;
    delete _attackDelegate;
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
    _session.setViewDelegates(_aiDelegate, _attackDelegate);
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
    if (g_engine)
        g_engine->getGameText().clearMessageArea();
    _needsFullRedraw = true;
    redraw();
    onUpdate();
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

    // Forward to CombatEndDialog while it's waiting for surrender input.
    if (_combatEnd && _combatEnd->isActive())
        return _combatEnd->msgKeypress(msg);

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

    if (!_needsFullRedraw)
        return;

    drawUI();

    // Apply pending AI character facing before drawing viewport.
    if (_aiFacingChar) {
        Combat::CombatContext *ctx = _session.getContext();
        if (ctx) {
            if (!ctx->isCharacterInBounds(_aiFacingChar, true)) {
                const TilePos pos = _session.getTable().getCharacterPos(_aiFacingChar);
                _session.getViewport().centerOn(pos);
            }
            ctx->setCharacterFacing(_aiFacingChar, _aiFacingDir);
        }
        _aiFacingChar = nullptr;
    }

    drawViewport();
    drawCombatants();
    if (_currentInfoActor)
        drawCombatInfo(_currentInfoActor);

    // Draw pending AI message overlay.
    if (_hasAiMessage) {
        Surface s = getSurface();
        s.clearBox(0, 24, 39, 24, kBackgroundColor);
        s.writeStringC(0, 24, 10, _aiMessage);
        _hasAiMessage = false;
    }

    if (_combatMenu && _combatMenu->isActive())
        _combatMenu->draw();
    _needsFullRedraw = false;
}

void CombatView::onUpdate() {
    debug(0, "[COMBATVIEW] onUpdate entered");
    // Mirrors COMBAT_MainLoop: fully synchronous blocking loop.
    // AI turns run here directly; player turns block in a pumpModalInputFrame()
    // wait loop until handleMenuResult() sets _playerActionPending.
    while (!_session.isEnded() && g_events) {
        const Combat::CombatSession::Phase phase = _session.getPhase();
        debug(0, "[COMBATVIEW] loop top: phase=%d ended=%d",
            (int)phase, (int)_session.isEnded());

        if (phase == Combat::CombatSession::PHASE_NONE ||
                phase == Combat::CombatSession::PHASE_ROUND_END ||
                phase == Combat::CombatSession::PHASE_COMBAT_END) {
            debug(0, "[COMBATVIEW] breaking: terminal phase=%d", (int)phase);
            break;
        }

        if (phase == Combat::CombatSession::PHASE_AWAITING_PLAYER) {
            debug(0, "[COMBATVIEW] AWAITING_PLAYER: blocking for input");
            // Block until player submits an action via handleMenuResult().
            while (!_playerActionPending)
                if (!g_events->pumpModalInputFrame()) break;

            if (!_playerActionPending)
                break;

            const Combat::CombatSession::PlayerAction action = _pendingPlayerAction;
            _playerActionPending = false;

            // PA_VIEW: show battlefield without consuming the turn — loop back to menu.
            if (action == Combat::CombatSession::PA_VIEW) {
                _needsFullRedraw = true;
                redraw();
                continue;
            }

            // PA_MOVE is handled by CombatMoveDialog — wait for it to finish.
            if (action == Combat::CombatSession::PA_MOVE) {
                ::Goldbox::Data::PlayerCharacter *actor = _session.getCurrentActor();
                if (actor && _combatMove) {
                    _combatMove->setAnimateCallback(
                        [](TilePos from, TilePos to,
                           ::Goldbox::Data::PlayerCharacter *ch, void *ctx) {
                            (void)ch;
                            static_cast<CombatView *>(ctx)->animateMovementPath(from, to);
                        }, this);
                    _combatMove->beginMove(actor);
                    attachDialog(_combatMove);
                    _combatMove->activate();
                    _needsFullRedraw = true;
                    redraw();

                    // Block until move dialog posts PA_MOVE result.
                    _playerActionPending = false;
                    while (!_playerActionPending)
                        if (!g_events->pumpModalInputFrame()) break;

                    if (_combatMove->isActive()) {
                        _combatMove->deactivate();
                        detachDialog(_combatMove);
                    }
                    _playerActionPending = false;
                }
                // finishMoveAction already called inside CombatMoveDialog;
                // session phase is now PLAYER_TURN or ROUND_END — loop continues.
                _currentInfoActor = nullptr;
                _needsFullRedraw = true;
                continue;
            }

            // Dismiss combat menu.
            if (_combatMenu && _combatMenu->isActive()) {
                _combatMenu->deactivate();
                detachDialog(_combatMenu);
            }
            _currentInfoActor = nullptr;

            const Combat::CombatSession::TurnResult tickResult =
                _session.submitPlayerAction(action);
            if (tickResult.event == Combat::CombatSession::TurnResult::EV_AI_ATTACK &&
                    tickResult.target && tickResult.damage > 0)
                applyDamageMessage(tickResult.target, (uint8)tickResult.damage,
                                   ::Goldbox::Data::DAMAGE_NORMAL, false);
            _needsFullRedraw = true;
            continue;
        }

        const Combat::CombatSession::TurnResult result = _session.executeTurn();
        debug(0, "[COMBATVIEW] executeTurn returned event=%d actor=%s phase=%d",
            (int)result.event,
            result.actor ? result.actor->name.c_str() : "NULL",
            (int)_session.getPhase());

        if (result.event == Combat::CombatSession::TurnResult::EV_NONE) {
            debug(0, "[COMBATVIEW] EV_NONE -> break");
            break;
        }

        if (result.event == Combat::CombatSession::TurnResult::EV_ROUND_END) {
            debug(0, "[COMBATVIEW] EV_ROUND_END -> acknowledgeRoundEnd");
            _session.acknowledgeRoundEnd(true);
            _needsFullRedraw = true;
            redraw();
            g_events->pumpModalInputFrame();
            continue;
        }

        if (result.event == Combat::CombatSession::TurnResult::EV_COMBAT_END) {
            debug(0, "[COMBATVIEW] EV_COMBAT_END -> close");
            close();
            break;
        }

        if (result.actor) {
            const Combat::CombatantTable &table = _session.getTable();
            const int idx = table.findIndex(result.actor);
            if (idx >= 0)
                _session.scrollViewport(
                    TilePos(table.getTileCol(idx), table.getTileRow(idx)), 2);
        }

        if (result.event == Combat::CombatSession::TurnResult::EV_ACTOR_FOCUSED) {
            const Combat::CombatSession::Phase focusPhase = _session.getPhase();
            debug(0, "[COMBATVIEW] EV_ACTOR_FOCUSED: actor='%s' phase=%d",
                result.actor ? result.actor->name.c_str() : "NULL",
                (int)focusPhase);
            _currentInfoActor = result.actor;
            _needsFullRedraw = true;

            if (focusPhase == Combat::CombatSession::PHASE_AWAITING_PLAYER) {
                // Player actor: draw and show combat menu.
                debug(0, "[COMBATVIEW] -> showing combat menu for player actor '%s'",
                    result.actor ? result.actor->name.c_str() : "NULL");
                ::Goldbox::Data::PlayerCharacter *actor = result.actor;
                ::Goldbox::Data::CombatAction *cs = actor ? actor->combatState : nullptr;

                if (actor && !actor->enabled) {
                    if (cs) cs->clear();
                    _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
                    continue;
                }
                if (cs && cs->spellId != 0) {
                    cs->spellId = 0;
                    cs->clear();
                    _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
                    continue;
                }

                if (_combatMenu && !_combatMenu->isActive()) {
                    _combatMenu->beginMenu(actor, &_session);
                    attachDialog(_combatMenu);
                    _combatMenu->activate();
                }
                redraw();
                // Loop back — next iteration hits PHASE_AWAITING_PLAYER and blocks.
            } else {
                // AI actor: just draw the focused actor, no input pump.
                draw();
                if (g_events->getScreen()) {
                    g_events->getScreen()->makeAllDirty();
                    g_events->getScreen()->update();
                }
            }
            continue;
        }

        if (result.event == Combat::CombatSession::TurnResult::EV_AI_ATTACK &&
                result.target && result.damage > 0)
            applyDamageMessage(result.target, (uint8)result.damage,
                               ::Goldbox::Data::DAMAGE_NORMAL, false);
    }
}

void CombatView::handleMenuResult(const MenuResultMessage &result) {
    if (!result._hasIntValue)
        return;

    // CombatEndDialog sentinel.
    if (result._intValue == -1) {
        if (_combatEnd && _combatEnd->isActive())
            detachDialog(_combatEnd);
        if (!result._success)
            close();
        else
            _session.acknowledgeRoundEnd(true);
        return;
    }

    if (!result._success)
        return;

    // Signal the blocking onUpdate() wait loop.
    _pendingPlayerAction =
        static_cast<Combat::CombatSession::PlayerAction>(result._intValue);
    _playerActionPending = true;
}

// ---------------------------------------------------------------------------
// Draw helpers

void CombatView::refreshViewport(::Goldbox::Data::PlayerCharacter *ch,
        Direction direction) {
    if (ch) {
        Combat::CombatContext *ctx = _session.getContext();
        if (ctx) {
            if (!ctx->isCharacterInBounds(ch, true)) {
                const TilePos pos = _session.getTable().getCharacterPos(ch);
                _session.getViewport().centerOn(pos);
            }
            ctx->setCharacterFacing(ch, direction);
        }
    }
    drawUI();
    drawViewport();
    drawCombatants();
    if (_currentInfoActor)
        drawCombatInfo(_currentInfoActor);
}

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

void CombatView::printCombatMessage(::Goldbox::Data::PlayerCharacter *ch,
        const Common::String &text, uint8 line) {
    if (!g_engine)
        return;
    // Ensure the viewport is drawn before overlaying text.
    _needsFullRedraw = true;
    draw();
    g_engine->getGameText().showMessage(ch, text, line);
    Surface s = getSurface();
    while (g_engine->getGameText().advance())
        ;
    g_engine->getGameText().draw(s);
    g_system->updateScreen();
}

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

    printCombatMessage(ch, r.message);
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

    // Clear only the static info area (rows 1-9); rows 10-21 are the
    // GameText combat message area and must not be clobbered here.
    s.clearBox(23, 1, 38, 9, kBackgroundColor);

    // Row 1: character name.
    s.writeStringC(23, 1, 10, ch->name);

    // Row 3: Hitpoints label + value.
    s.writeStringC(23, 3, 10, "Hitpoints");
    s.writeStringC(33, 3, 10, Common::String::format("%d/%d",
        ch->hitPoints.current, ch->hitPoints.max));

    // Row 5: Armor class label + value.
    s.writeStringC(23, 5, 10, "AC");
    s.writeStringC(26, 5, 10, Common::String::format("%d",
        ch->armorClass.getCurrent()));

    // BYTE_CHAR_POS_Y = 5 (AC row). Status anchors to row 7 (BYTE_CHAR_POS_Y + 2).
    static const int kStatusRow = 7;

    // Rows 7-8: equipped weapon display text (wraps up to 2 rows).
    const ::Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<const ::Goldbox::Data::ADnDCharacter *>(ch);
    const ::Goldbox::Data::Items::CharacterItem *weapon = adnd ?
        adnd->getEquippedItem(::Goldbox::Data::Items::Slot::S_MAIN_HAND) : nullptr;
    if (weapon) {
        // getListDisplayText(false) mirrors ITEM_buildListDisplayText(..., false, false).
        const Common::String weaponText = weapon->getListDisplayText(false);
        // First line at row 7, overflow at row 8 — mirrors TEXT_BlockPrint rect {23,7,38,9}.
        const uint kMaxCols = 38 - 23 + 1; // 16 chars (cols 23..38 inclusive)
        if (weaponText.size() <= kMaxCols) {
            s.writeStringC(23, kStatusRow, 10, weaponText);
        } else {
            // Split at last word-break at or before kMaxCols.
            uint split = kMaxCols;
            while (split > 0 && !GameText::isWordBreak(weaponText[split - 1]))
                --split;
            if (split == 0)
                split = kMaxCols; // no word-break found, hard-split
            s.writeStringC(23, kStatusRow,     10, weaponText.substr(0, split));
            s.writeStringC(23, kStatusRow + 1, 10, weaponText.substr(split));
        }
    }

    // Row 7: status/condition (overlays weapon area when character is disabled/helpless).
    if (!ch->enabled) {
        s.writeStringC(23, kStatusRow, 10, ch->getStatusName());
    } else if (ch->hasNegativeEffect()) {
        s.writeStringC(23, kStatusRow, 10, "(Helpless)");
    }
}

void CombatView::drawDamageFrame(const Gfx::Pic *frame, int pixX, int pixY,
        Graphics::ManagedSurface *dst) {
    frame->trDraw(dst, pixX, pixY, frame->getTransparentIndex());
}

// ---------------------------------------------------------------------------
// Movement animation

void CombatView::redrawViewportAt(TilePos center) {
    _session.getViewport().centerOn(center);
    drawViewport();
    drawCombatants();
}

Common::Array<CombatView::MovementAnimStep>
CombatView::buildMovementAnimationPath(TilePos start, TilePos end) {
    Common::Array<MovementAnimStep> steps;

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
    if (_session.getViewport().isTileVisible(start) &&
            _session.getViewport().isTileVisible(end)) {
        redrawCenter = _session.getViewport().getCenter();
    } else if (ABS(deltaX) < Combat::CombatViewport::VIEW_COLS &&
               ABS(deltaY) < Combat::CombatViewport::VIEW_ROWS) {
        redrawCenter = TilePos((uint8)(start.col + deltaX / 2),
                               (uint8)(start.row + deltaY / 2));
    } else {
        redrawCenter = _session.getViewport().getCenter();
    }
    redrawViewportAt(redrawCenter);

    const int kFineViewCols = Combat::CombatViewport::VIEW_COLS * kFinePerTile;
    const int kFineViewRows = Combat::CombatViewport::VIEW_ROWS * kFinePerTile;

    TilePos vpTopLeft = _session.getViewport().getTopLeft();
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
            redrawViewportAt(redrawCenter);

            vpTopLeft = _session.getViewport().getTopLeft();
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
    if (!_session.getViewport().isTileVisible(end))
        redrawViewportAt(end);
    vpTopLeft = _session.getViewport().getTopLeft();
    MovementAnimStep last;
    last.finePos = ScreenPos(
        (int16)((end.col - vpTopLeft.col) * kFinePerTile),
        (int16)((end.row - vpTopLeft.row) * kFinePerTile));
    steps.push_back(last);

    return steps;
}

void CombatView::drawMovementAnimation(
        const ScreenPos &finePos, uint8 animFrame, uint8 frameDelay,
        ::Goldbox::Data::PlayerCharacter *ch, Gfx::IconDirection iconDir,
        Graphics::ManagedSurface *screen) {
    const Common::Point px = fineToPixel(finePos);

    // Draw icon — alternate ready/attack state across the frame cycle.
    const uint8 slotId = ch->iconData.iconSlotId;
    Gfx::IconManager *iconMgr = VmInterface::getIconManager();
    if (slotId != 0 && iconMgr && !iconMgr->isSlotEmpty(slotId)) {
        const Gfx::Pic *pic = (animFrame < kAnimFrameCount / 2)
            ? iconMgr->getReadyPic(slotId)
            : iconMgr->getActionPic(slotId);
        if (pic)
            pic->trDraw(screen, px.x, px.y, pic->getTransparentIndex());
    } else {
        _combatRenderer.drawIcon(
            ch->iconData,
            (animFrame < kAnimFrameCount / 2) ? Gfx::ICON_STATE_READY
                                              : Gfx::ICON_STATE_ATTACK,
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

void CombatView::animateMovementPath(
        TilePos start, TilePos end,
        uint8 initialFrame, uint8 frameDelay) {
    ::Goldbox::Data::PlayerCharacter *ch = _session.getCurrentActor();
    if (!ch)
        return;
    Gfx::IconDirection iconDir = Gfx::ICON_DIRECTION_RIGHT;
    if (ch->combatState) {
        const uint8 facing = ch->combatState->direction;
        if (facing >= 5 || facing == 0)
            iconDir = Gfx::ICON_DIRECTION_LEFT;
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

    g_system->updateScreen();
    _needsFullRedraw = true;
}

void CombatView::updateCharacterFacingAndRedraw(
        ::Goldbox::Data::PlayerCharacter *ch,
        Direction direction,
        uint8 iconFrame,
        bool noRedraw) {
    if (!ch)
        return;

    Combat::CombatContext *ctx = _session.getContext();
    if (!ctx)
        return;

    // Scroll viewport to include the full footprint if needed.
    if (!ctx->isCharacterInBounds(ch, true)) {
        const TilePos pos = _session.getTable().getCharacterPos(ch);
        redrawViewportAt(pos);
    }

    const Direction oldDirection =
        static_cast<Direction>(ch->combatState ? ch->combatState->direction : 0);

    // Restore entity tiles when the coarse facing group changes,
    // a non-idle frame is requested, or the final draw is suppressed.
    // direction >> 2 groups the 8 directions into two mirror groups.
    if (((oldDirection >> 2) != (direction >> 2)) ||
        iconFrame != 0 ||
        noRedraw) {
        drawViewport();
        drawCombatants();
    }

    ctx->setCharacterFacing(ch, direction);

    if (!noRedraw && ctx->isCharacterInBounds(ch, true)) {
        drawCombatants();
        g_system->updateScreen();
    }
}

// ---------------------------------------------------------------------------
// Ranged attack animation

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

static const DirectionalTileParams kDirectionalTiles[8] = {
    { 0,   0, false }, // DIR_N
    { 1,   0, false }, // DIR_NE
    { 2,   0, false }, // DIR_E
    { 129, 0, false }, // DIR_SE
    { 128, 0, false }, // DIR_S
    { 4,   0, true  }, // DIR_SW  (blend layer)
    { 130, 0, false }, // DIR_W
    { 132, 0, true  }, // DIR_NW  (blend layer)
};

void CombatView::renderEffectTile(uint8 blockId, int frameIdx,
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

    Gfx::Pic *tile = Gfx::Pic::readSpriteFrame(block, frameIdx);
    if (tile) {
        tile->trDraw(screen, pixX, pixY, tile->getTransparentIndex());
        delete tile;
    }

    if (withLayer) {
        Gfx::Pic *layer = Gfx::Pic::readSpriteFrame(block, frameIdx + 1);
        if (layer) {
            layer->trDraw(screen, pixX, pixY, layer->getTransparentIndex());
            delete layer;
        }
    }
}

void CombatView::animateRangedAttack(
        ::Goldbox::Data::PlayerCharacter *attacker,
        ::Goldbox::Data::PlayerCharacter *target,
        const ::Goldbox::Data::Items::CharacterItem *item) {
    if (!attacker || !target || !item)
        return;

    Combat::CombatContext *ctx = _session.getContext();
    if (!ctx)
        return;

    const Direction facing = ctx->getFacingToward(attacker, target);

    const Combat::CombatantTable &table = _session.getTable();
    const int attackerIdx = table.findIndex(attacker);
    const int targetIdx   = table.findIndex(target);
    if (attackerIdx < 0 || targetIdx < 0)
        return;

    const Combat::ViewportPos vpos =
        _session.getViewport().getCharacterViewportPosition(table, attackerIdx);
    const int pixX = kViewportX + vpos.column * kTileSize;
    const int pixY = kViewportY + vpos.row    * kTileSize;

    const TilePos attackerTile(
        table.getTileCol(attackerIdx), table.getTileRow(attackerIdx));
    const TilePos targetTile(
        table.getTileCol(targetIdx), table.getTileRow(targetIdx));

    uint8 pathSteps  = 1;
    uint8 frameDelay = 10;

    const uint8 propId = item->prop().wpnType;

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
        pathSteps  = 4;
        frameDelay = 50;
    }
    // Effect group 3: quad effect 0x11 (propId 85,86)
    else if (propId == 85 || propId == 86) {
        renderEffectTile(0x11, 0, pixX, pixY);
        pathSteps  = 4;
        frameDelay = 50;
    }
    // Effect group 4: default two-tile projectile
    else {
        const uint8 baseBlock = (propId == 0x2f) ? 14 : 13;
        renderEffectTile(baseBlock + 7, 0, pixX, pixY);
        renderEffectTile(baseBlock + 7, 1, pixX, pixY);
        pathSteps  = 2;
        frameDelay = 20;
    }

    g_engine->soundPlay(7);

    animateMovementPath(attackerTile, targetTile, 0, frameDelay);

    g_engine->soundPlay(13);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
