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
#include "goldbox/poolrad/views/dialogs/combat_attack_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_spell_dialog.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/effects/effect_handler_base.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/runtime/effect_host_bridge.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_tile_props.h"
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

    void present(uint32 delayMs = CombatView::kStateChangeDelayMs) {
        _view->presentFrame(delayMs);
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
        // Apply facing to combatState immediately so drawCombatants() reads
        // the correct direction during animateMovement (which calls
        // drawCombatants before draw() processes _aiFacingChar).
        Combat::CombatContext *ctx = Combat::g_combatSession
            ? Combat::g_combatSession->getContext() : nullptr;
        if (ctx)
            ctx->setCharacterFacing(ch, static_cast<Direction>(direction));
        // Keep ch so draw() can scroll the viewport to include the character.
        // The setCharacterFacing call inside draw() is a harmless double-write.
        _view->setAiCharacterFacing(ch, static_cast<Direction>(direction));
        present();
    }

    void redrawAfterMove(::Goldbox::Data::PlayerCharacter *ch) override {
        _view->setAiInfoActor(ch);
        present(0);
    }

    void drawCombatInfo(::Goldbox::Data::PlayerCharacter *ch) override {
        _view->setAiInfoActor(ch);
        present();
    }

    void clearPromptAndMessage() override {
        _view->clearCombatMessage();
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

    void drawCombatInfo(::Goldbox::Data::PlayerCharacter *attacker) override {
        _view->setAiInfoActor(attacker);
        _view->presentFrame();
    }

    void updateCharacterFacingAndRedraw(::Goldbox::Data::PlayerCharacter *ch,
                                        uint8 direction, uint8 /*redrawMode*/,
                                        bool restoreOld) override {
        _view->setAiCharacterFacing(ch, static_cast<Direction>(direction));
        _view->presentFrame();
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
            if (damage > 0) {
                const Combat::CombatContext *ctx =
                    VmInterface::getCombatContext();
                if (!ctx) {
                    warning("[COMBATVIEW] attack display has no combat context");
                    return;
                }
                _view->drawDamage(target,
                    ctx->globals.behaviorFlags == Combat::CombatGlobals::DMG_MAGIC,
                    ::Goldbox::Data::DamageUtils::buildDamageMessage(
                        damageDisplay, ctx->globals.behaviorFlags));
            }
        }

        // Mirrors COMBAT_ApplyDamageMessage: TIME_Wait after the result,
        // then the down text that stays up during the dying icon.
        g_system->delayMillis(CombatView::kStateChangeDelayMs);
        if (target && !target->enabled)
            _view->printCombatMessage(target,
                ::Goldbox::Data::DamageSystem::buildDownMessage(*target));
    }

    void drawCharacterDying(::Goldbox::Data::PlayerCharacter *ch) override {
        _view->drawCharacterDying(ch);
    }

    void presentViewport(uint32 delayMs) override {
        _view->presentFrame(delayMs);
    }
};


CombatView::CombatView()
    : View("Combat"), _viewport(nullptr), _messageBox(nullptr), _info(nullptr),
      _combatMenu(nullptr),
      _combatMove(nullptr), _combatAttack(nullptr), _combatEnd(nullptr),
      _combatSpell(nullptr), _combatSubmenu(nullptr), _needsFullRedraw(true),
      _playerActionPending(false),
      _pendingPlayerAction(Combat::CombatSession::PA_NONE),
      _aiFacingChar(nullptr), _aiFacingDir(DIR_N) {
    _viewport      = new Dialogs::CombatViewportDialog();
    _messageBox    = new Dialogs::CombatMessageBoxDialog();
    _info          = new Dialogs::CombatInfoDialog(this);
    _viewport->bindSession(&_session);
    _combatMenu    = new Dialogs::CombatMenuDialog();
    _combatMove    = new Dialogs::CombatMoveDialog();
    _combatAttack  = new Dialogs::CombatAttackDialog();
    _combatEnd     = new Dialogs::CombatEndDialog();
    {
        Dialogs::HorizontalYesNoConfig cfg;
        cfg.promptTxt       = "Abort Spell? ";
        cfg.promptColor     = 15;
        cfg.textColor       = 10;
        cfg.selectColor     = 15;
        cfg.backgroundColor = 8;
        _abortYesNo = new Dialogs::HorizontalYesNo("CombatAbortSpell", cfg);
    }
    _combatSpell   = new Dialogs::CombatSpellDialog();
    _combatSubmenu = new Dialogs::CombatSubmenuDialog();
    _aiDelegate     = new CombatViewAiDelegate(this);
    _attackDelegate = new CombatViewAttackDelegate(this);
}

CombatView::~CombatView() {
    // The info line no longer owns content once the dialogs are deleted.
    _info->closeContent();
    delete _combatMenu;
    delete _combatMove;
    delete _combatAttack;
    delete _combatEnd;
    delete _abortYesNo;
    delete _combatSpell;
    delete _combatSubmenu;
    delete _aiDelegate;
    delete _attackDelegate;
    delete _effectRuntime;
    delete _info;
    delete _messageBox;
    delete _viewport;
}

void CombatView::setup(const Combat::CombatParams &params) {
    _bridge = Goldbox::Poolrad::getEffectHostBridge();

    g_engine->getPictureDisplayCache().clear();

    _viewport->loadTileset(params.isDungeon);

    if (!_effectRuntime) {
        _effectRuntime = new ::Goldbox::Data::Effects::EffectRuntime(
            ::Goldbox::Data::Effects::g_effectHandler, _bridge);
    } else {
        _effectRuntime->setHandler(::Goldbox::Data::Effects::g_effectHandler);
        _effectRuntime->setHostBridge(_bridge);
    }

    Combat::CombatParams p = params;
    p.tilePropertyProvider = &PoolradTilePropertyProvider::instance();
    p.effectRuntime = _effectRuntime;
    p.effectBridge = _bridge;
    if (!p.vmGlobalLayout)
        p.vmGlobalLayout = &Data::getPoolradGlobalVmLayout();
    _session.setup(p);
    _session.setViewDelegates(_aiDelegate, _attackDelegate, this);
    Combat::g_combatSession = &_session;

    _viewport->rebuildTerrain();
    _viewport->loadCombatIcons();

    _needsFullRedraw = true;
}

bool CombatView::msgFocus(const FocusMessage &msg) {
    View::msgFocus(msg);
    if (g_engine)
        g_engine->getGameText().clearMessageArea();
    _needsFullRedraw = true;
    redraw();
    if (!_viewingCharacter)
        onUpdate();
    return true;
}

bool CombatView::msgUnfocus(const UnfocusMessage &msg) {
    if (!_viewingCharacter && Combat::g_combatSession == &_session)
        Combat::g_combatSession = nullptr;
    return View::msgUnfocus(msg);
}

bool CombatView::msgKeypress(const KeypressMessage &msg) {
    if (_session.isEnded()) {
        close();
        return true;
    }

    if (_info->hasContent())
        return _info->msgKeypress(msg);

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

    if (_viewport->refreshDirtyTiles())
        _needsFullRedraw = true;

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

    _viewport->draw();
    _messageBox->draw();
    _info->draw();
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

        if (phase == Combat::CombatSession::PHASE_NONE) {
            debug(0, "[COMBATVIEW] breaking: terminal phase=%d", (int)phase);
            break;
        }

        if (phase == Combat::CombatSession::PHASE_COMBAT_END) {
            debug(0, "[COMBATVIEW] COMBAT_END -> close");
            close();
            break;
        }

        // Mirrors DIALOG_CombatEnd: round bookkeeping already ran in the
        // session; the view shows messages and the "Continue Battle?" prompt.
        if (phase == Combat::CombatSession::PHASE_ROUND_END) {
            handleRoundEnd();
            continue;
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
            debug(0, "[COMBATVIEW] player action=%d actor='%s'",
                (int)action, _session.getCurrentActor()
                    ? _session.getCurrentActor()->name.c_str() : "NONE");

            // VIEW_ViewCharacter inspects the acting character, not the map.
            if (action == Combat::CombatSession::PA_VIEW) {
                _info->closeContent();
                ::Goldbox::Data::PlayerCharacter *savedSelected =
                    VmInterface::getSelectedCharacter();
                debug(0, "[COMBATVIEW] opening ViewCharacter for '%s'",
                    _session.getCurrentActor()->name.c_str());
                VmInterface::setSelectedCharacter(_session.getCurrentActor());
                _viewingCharacter = true;
                addView("ViewCharacter");
                while (g_events->focusedView() &&
                        g_events->focusedView() != this) {
                    if (!g_events->pumpModalInputFrame())
                        break;
                }
                _viewingCharacter = false;
                VmInterface::setSelectedCharacter(savedSelected);
                if (g_events->focusedView() != this)
                    break;

                if (::Goldbox::Data::ADnDCharacter *adnd =
                        dynamic_cast<::Goldbox::Data::ADnDCharacter *>(
                            _session.getCurrentActor())) {
                    adnd->recalcCombatStats();
                    Combat::recalcPrimaryAttacks(adnd);
                }
                finishPlayerAction();
                continue;
            }

            // PA_MOVE is handled by CombatMoveDialog — wait for it to finish.
            if (action == Combat::CombatSession::PA_MOVE) {
                ::Goldbox::Data::PlayerCharacter *actor = _session.getCurrentActor();
                if (actor && _combatMove) {
                    _combatMove->setAnimateCallback(
                        [](TilePos from, TilePos to,
                           ::Goldbox::Data::PlayerCharacter *ch, void *ctx) {
                            static_cast<CombatView *>(ctx)->animateMovement(from, to, ch);
                        }, this);
                    _combatMove->beginMove(actor);
                    _info->showContent(_combatMove);
                    _needsFullRedraw = true;
                    redraw();

                    _playerActionPending = false;
                    while (!_playerActionPending)
                        if (!g_events->pumpModalInputFrame()) break;

                    _info->closeContent();
                    _playerActionPending = false;
                }
                // Mirrors DIALOG_CombatMain looping on a false result: the
                // actor returns to the menu unless the move ended its turn.
                finishPlayerAction();
                continue;
            }

            // PA_CAST is handled by CombatSpellDialog — wait for it to finish.
            if (action == Combat::CombatSession::PA_CAST) {
                ::Goldbox::Data::PlayerCharacter *actor = _session.getCurrentActor();
                ::Goldbox::Data::PlayerCharacter *savedSelected =
                    VmInterface::getSelectedCharacter();
                debug(0, "[COMBATVIEW] cast begin actor='%s' selected='%s'",
                    actor ? actor->name.c_str() : "NONE",
                    savedSelected ? savedSelected->name.c_str() : "NONE");
                if (actor && _combatSpell) {
                    // Legacy COMBAT_CastSpell uses PTR_SELECTED_CHAR; the
                    // spell menu also reads the selected character directly.
                    VmInterface::setSelectedCharacter(actor);
                    _combatSpell->beginCast(actor);
                    _info->showContent(_combatSpell);
                    _needsFullRedraw = true;
                    redraw();

                    _playerActionPending = false;
                    _lastResultSuccess = false;
                    while (!_playerActionPending) {
                        if (!g_events->pumpModalInputFrame()) break;
                    }

                    debug(0, "[COMBATVIEW] cast selector returned pending=%d success=%d",
                        (int)_playerActionPending, (int)_lastResultSuccess);
                    _info->closeContent();
                    _playerActionPending = false;

                    // Cast outside the dialog's event handler so targeting
                    // can aim interactively (COMBAT_CastSpell).
                    const uint8 chosenSpell = _combatSpell->getSelectedSpell();
                    debug(0, "[COMBATVIEW] cast selection actor='%s' spell=%u",
                        actor->name.c_str(), (unsigned)chosenSpell);
                    if (chosenSpell != 0) {
                        debug(0, "[COMBATVIEW] resolving spell=%u for '%s'",
                            (unsigned)chosenSpell, actor->name.c_str());
                        Combat::castSpell(actor, chosenSpell, false, this);
                        _messageBox->setActor(actor);
                        _needsFullRedraw = true;
                    }
                    VmInterface::setSelectedCharacter(savedSelected);
                    debug(0, "[COMBATVIEW] cast finished actor='%s' selected restored='%s'",
                        actor->name.c_str(),
                        savedSelected ? savedSelected->name.c_str() : "NONE");
                } else {
                    debug(0, "[COMBATVIEW] cast skipped actor=%p dialog=%p",
                        (void *)actor, (void *)_combatSpell);
                }
                finishPlayerAction();
                continue;
            }

            // PA_USE: open items dialog, then recalculate primary attacks.
            // Mirrors original: DIALOG_Items(&result) then COMBAT_RecalcPrimaryAttacks().
            // TODO: replace stub with full ItemsDialog when implemented.
            if (action == Combat::CombatSession::PA_USE) {
                // Stub: recalc stats and loop back to menu (turn not consumed).
                if (::Goldbox::Data::ADnDCharacter *adnd =
                        dynamic_cast<::Goldbox::Data::ADnDCharacter *>(
                            _session.getCurrentActor()))
                    adnd->recalcCombatStats();
                _needsFullRedraw = true;
                _messageBox->draw();
                continue;
            }

            // PA_AUTOSPELL: toggle autospell flag and show message; does not end turn.
            // submitPlayerAction(PA_AUTOSPELL) returns early without advancing turn.
            if (action == Combat::CombatSession::PA_AUTOSPELL) {
                // AUTOSPELL_STATUS lives in the session globals so AI spell
                // and item selection sees the same toggle.
                Combat::CombatContext *ctx = _session.getContext();
                if (ctx)
                    ctx->globals.magicEnabled = !ctx->globals.magicEnabled;
                _autoSpell = ctx && ctx->globals.magicEnabled;
                printCombatMessage(nullptr,
                    _autoSpell ? "Magic On" : "Magic Off", 10);
                _session.submitPlayerAction(Combat::CombatSession::PA_AUTOSPELL);
                _needsFullRedraw = true;
                _messageBox->draw();
                continue;
            }

            // PA_PARTY_MANUAL: submitPlayerAction handles the state mutation and
            // returns early (turn not consumed). Just redraw and loop back to menu.
            if (action == Combat::CombatSession::PA_PARTY_MANUAL) {
                _session.submitPlayerAction(Combat::CombatSession::PA_PARTY_MANUAL);
                _needsFullRedraw = true;
                _messageBox->draw();
                continue;
            }

            // PA_SUBMENU is handled by CombatSubmenuDialog — wait for it to finish.
            if (action == Combat::CombatSession::PA_SUBMENU) {
                ::Goldbox::Data::PlayerCharacter *actor = _session.getCurrentActor();
                debug(0, "[COMBATVIEW] opening submenu actor='%s'",
                    actor ? actor->name.c_str() : "NONE");
                if (actor && _combatSubmenu) {
                    _combatSubmenu->beginSubmenu(actor);
                    _info->showContent(_combatSubmenu);
                    _needsFullRedraw = true;
                    redraw();

                    _playerActionPending = false;
                    debug(0, "[COMBATVIEW] submenu active; waiting for result");
                    while (!_playerActionPending) {
                        if (!g_events->pumpModalInputFrame()) break;
                    }

                    debug(0, "[COMBATVIEW] submenu returned pending=%d action=%d success=%d",
                        (int)_playerActionPending, (int)_pendingPlayerAction,
                        (int)_lastResultSuccess);
                    _info->closeContent();
                    _playerActionPending = false;
                }
                // PA_SUBMENU cancel returns PA_SUBMENU; re-show main menu.
                // PA_DONE from submenu ends the activation (Guard/Quit/Bandage
                // already ended the turn; Delay left initiative at 1).
                if (_pendingPlayerAction == Combat::CombatSession::PA_SUBMENU) {
                    _pendingPlayerAction = Combat::CombatSession::PA_NONE;
                    if (_session.getCurrentActor())
                        showCombatMenu(_session.getCurrentActor());
                    continue;
                }
                if (_session.getPhase() == Combat::CombatSession::PHASE_AWAITING_PLAYER)
                    _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
                _messageBox->setActor(nullptr);
                _needsFullRedraw = true;
                continue;
            }

            // PA_ATTACK is handled by CombatAttackDialog — wait for it to finish.
            if (action == Combat::CombatSession::PA_ATTACK) {
                ::Goldbox::Data::PlayerCharacter *actor = _session.getCurrentActor();
                if (actor && _combatAttack) {
                    _combatAttack->beginAttack(actor, &_session);
                    _info->showContent(_combatAttack);
                    _needsFullRedraw = true;
                    redraw();

                    _playerActionPending = false;
                    while (!_playerActionPending)
                        if (!g_events->pumpModalInputFrame()) break;

                    _info->closeContent();
                    _playerActionPending = false;
                }
                finishPlayerAction();
                continue;
            }

            // Dismiss combat menu before submitting other actions.
            _info->closeContent(_combatMenu);

            _messageBox->setActor(nullptr);

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

        if (result.event ==
                Combat::CombatSession::TurnResult::EV_ACTOR_SKIPPED) {
            debug(0, "[COMBATVIEW] actor turn cancelled -> continue");
            _messageBox->setActor(nullptr);
            _needsFullRedraw = true;
            continue;
        }

        if (result.event == Combat::CombatSession::TurnResult::EV_ROUND_END) {
            // Phase is now PHASE_ROUND_END; the loop top runs handleRoundEnd().
            debug(0, "[COMBATVIEW] EV_ROUND_END");
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
            _messageBox->setActor(result.actor);
            _needsFullRedraw = true;

            if (focusPhase == Combat::CombatSession::PHASE_AWAITING_PLAYER) {
                // Player actor: draw and show combat menu.
                debug(0, "[COMBATVIEW] -> showing combat menu for player actor '%s'",
                    result.actor ? result.actor->name.c_str() : "NULL");
                ::Goldbox::Data::PlayerCharacter *actor = result.actor;
                ::Goldbox::Data::CombatAction *cs = actor ? actor->combatState : nullptr;

                if (actor && !actor->enabled) {
                    // Mirrors DIALOG_CombatMain: disabled actor -> endActorTurn.
                    if (cs) cs->endTurn();
                    _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
                    continue;
                }

                // Pending spell path (mirrors DIALOG_CombatMain pending-spell branch):
                // resolve the stored spell (without re-queuing it) and end the turn.
                if (cs && cs->spellId != 0) {
                    Combat::resolvePendingSpell(actor, false, this);
                    _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
                    _messageBox->setActor(nullptr);
                    _needsFullRedraw = true;
                    continue;
                }

                showCombatMenu(actor);
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

void CombatView::showCombatMenu(::Goldbox::Data::PlayerCharacter *actor) {
    if (!actor)
        return;
    debug(0, "[COMBATVIEW] showCombatMenu actor='%s' activeDialog='%s'",
        actor->name.c_str(),
        _info->hasContent() ? _info->getContent()->getName().c_str() : "NONE");
    // Mirrors COMBAT_FocusCharacter + COMBATVIEW_drawCombatInfo before the menu.
    _messageBox->setActor(actor);
    _needsFullRedraw = true;
    if (!_info->hasContent()) {
        _combatMenu->beginMenu(actor, &_session);
        _info->showContent(_combatMenu);
    }
    redraw();
}

void CombatView::finishPlayerAction() {
    // The session may already have advanced (e.g. finishMoveAction).
    if (_session.getPhase() != Combat::CombatSession::PHASE_AWAITING_PLAYER) {
        debug(0, "[COMBATVIEW] finishPlayerAction skipped phase=%d",
            (int)_session.getPhase());
        return;
    }

    ::Goldbox::Data::PlayerCharacter *actor = _session.getCurrentActor();
    debug(0, "[COMBATVIEW] finishPlayerAction actor='%s' complete=%d init=%d spell=%d",
        actor ? actor->name.c_str() : "NONE",
        (int)_session.isActorTurnComplete(actor),
        actor && actor->combatState
            ? (int)(int8)actor->combatState->initiative : -1,
        actor && actor->combatState ? (int)actor->combatState->spellId : -1);
    if (actor && !_session.isActorTurnComplete(actor)) {
        // DIALOG_CombatMain keeps looping while the action result is false.
        showCombatMenu(actor);
        return;
    }

    _session.submitPlayerAction(Combat::CombatSession::PA_NONE);
    _messageBox->setActor(nullptr);
    _needsFullRedraw = true;
}

void CombatView::handleRoundEnd() {
    const Combat::CombatSession::RoundEndInfo info = _session.getRoundEndInfo();

    _messageBox->setActor(nullptr);
    if (info.teammateDying)
        printCombatMessage(nullptr, "Your Teammate is Dying", 10);
    _needsFullRedraw = true;
    redraw();

    bool continueBattle = !info.exitCombat;
    if (info.exitCombat && info.askContinue) {
        // DIALOG_InputYesNo("Continue Battle?"): Y keeps the encounter going.
        _roundEndAnswered = false;
        _roundEndContinue = false;
        _combatEnd->prepare();
        _info->showContent(_combatEnd);
        redraw();
        while (!_roundEndAnswered)
            if (!g_events || !g_events->pumpModalInputFrame())
                break;
        _info->closeContent(_combatEnd);

        continueBattle = _roundEndAnswered && _roundEndContinue;
    }

    _session.acknowledgeRoundEnd(continueBattle);
    _needsFullRedraw = true;
    if (!continueBattle)
        close();
}

void CombatView::handleMenuResult(const MenuResultMessage &result) {
    debug(0, "[COMBATVIEW] menu result success=%d key=%d hasInt=%d int=%d",
        (int)result._success, (int)result._keyCode,
        (int)result._hasIntValue,
        result._hasIntValue ? result._intValue : -1);
    if (!result._hasIntValue)
        return;

    // Answer of the "Abort Spell?" yes/no (1 = YES, 0 = NO).
    if (_awaitingAbortAnswer) {
        _abortAnswer = (result._intValue == 1);
        _abortAnswered = true;
        return;
    }

    // CombatEndDialog sentinel: answer to "Continue Battle?".
    if (result._intValue == -1) {
        _roundEndContinue = result._success;
        _roundEndAnswered = true;
        return;
    }

    // Sub-dialogs report cancellation with _success == false; the wait loop
    // must still be released so the actor returns to the menu.
    _lastResultSuccess = result._success;
    _pendingPlayerAction =
        static_cast<Combat::CombatSession::PlayerAction>(result._intValue);
    _playerActionPending = true;
}

bool CombatView::aim(::Goldbox::Data::PlayerCharacter *caster, uint8 spellId,
        uint8 range, bool tileMode,
        ::Goldbox::Data::PlayerCharacter *&target, TilePos &tile) {
    (void)spellId;
    target = nullptr;
    if (!_combatAttack || !caster || !g_events)
        return false;

    _combatAttack->beginSpellAim(caster, &_session, range, tileMode);
    _info->showContent(_combatAttack);
    _needsFullRedraw = true;
    redraw();

    _playerActionPending = false;
    _lastResultSuccess = false;
    while (!_playerActionPending)
        if (!g_events->pumpModalInputFrame()) break;

    const bool chosen = _playerActionPending && _lastResultSuccess;
    if (chosen) {
        target = _combatAttack->getAimTarget();
        tile = _combatAttack->getAimTile();
    }

    _info->closeContent();
    _playerActionPending = false;
    _needsFullRedraw = true;
    return chosen;
}

bool CombatView::confirmAbortSpell() {
    if (!_abortYesNo || !g_events)
        return true;

    _awaitingAbortAnswer = true;
    _abortAnswered = false;
    _abortAnswer = false;

    _info->showContent(_abortYesNo);
    redraw();
    while (!_abortAnswered)
        if (!g_events->pumpModalInputFrame()) break;

    _info->closeContent();
    _awaitingAbortAnswer = false;
    _needsFullRedraw = true;

    // Closing the game while the prompt is up counts as aborting.
    return !_abortAnswered || _abortAnswer;
}

// ---------------------------------------------------------------------------
// CastSpellViewDelegate

void CombatView::focusCaster(::Goldbox::Data::PlayerCharacter *caster) {
    _needsFullRedraw = true;
    draw();
    drawCombatInfo(caster);
}

void CombatView::showBeginsCasting(::Goldbox::Data::PlayerCharacter *caster) {
    printCombatMessage(caster, "Begins Casting!");
}

void CombatView::showSpellCastEffect(::Goldbox::Data::PlayerCharacter *caster,
        uint8 spellId, TilePos target) {
    if (!caster || !g_engine)
        return;

    Combat::CombatContext *ctx = _session.getContext();
    const Combat::CombatantTable &table = _session.getTable();
    const int casterIdx = table.findIndex(caster);
    if (!ctx || casterIdx < 0)
        return;

    const TilePos casterTile(table.getTileCol(casterIdx),
                             table.getTileRow(casterIdx));
    if (!isValidTilePos(target))
        target = casterTile;

    // Effect quad 0x12 at the caster, then turn toward the target tile.
    _viewport->drawEffectAt(caster, 0x12);

    Direction facing = DIR_NONE;
    for (uint8 d = 0; d < 8; ++d) {
        if (Combat::CombatContext::isTargetInArc(static_cast<Direction>(d),
                target, casterTile)) {
            facing = static_cast<Direction>(d);
            break;
        }
    }
    if (facing != DIR_NONE)
        ctx->setCharacterFacing(caster, facing);

    // Spells 0x2F and 0x33 have their own sound; the rest use effect 3.
    if (spellId == 0x2F)
        g_engine->soundPlay(0x0C);
    else if (spellId == 0x33)
        g_engine->soundPlay(0x09);
    else
        g_engine->soundPlay(0x03);

    _viewport->animateProjectile(casterTile, target, 0x1E);
    _needsFullRedraw = true;
}

void CombatView::showSpellProjectile(
        ::Goldbox::Data::PlayerCharacter *caster,
        ::Goldbox::Data::PlayerCharacter *target) {
    if (!caster || !target || !g_engine)
        return;
    const Combat::CombatantTable &table = _session.getTable();
    const int casterIdx = table.findIndex(caster);
    const int targetIdx = table.findIndex(target);
    if (casterIdx < 0 || targetIdx < 0)
        return;

    g_engine->soundPlay(0x03);
    _viewport->drawEffectAt(caster, 0x12);
    _viewport->animateProjectile(
        TilePos(table.getTileCol(casterIdx), table.getTileRow(casterIdx)),
        TilePos(table.getTileCol(targetIdx), table.getTileRow(targetIdx)),
        0x1E);
    _needsFullRedraw = true;
}

void CombatView::showSpellMessage(::Goldbox::Data::PlayerCharacter *caster,
        const Common::String &text) {
    printCombatMessage(caster, text, 10);
}

void CombatView::refreshViewport(::Goldbox::Data::PlayerCharacter *ch,
        Direction direction) {
    if (ch) {
        Combat::CombatContext *ctx = _session.getContext();
        if (ctx) {
            centerCameraOn(ch);
            ctx->setCharacterFacing(ch, direction);
        }
    }
    drawUI();
    _viewport->draw();
    _messageBox->draw();
}

// ---------------------------------------------------------------------------
// Screen regions

void CombatView::attachPromptContent(Dialogs::Dialog *content) {
    attachDialog(content);
}

void CombatView::detachPromptContent(Dialogs::Dialog *content) {
    detachDialog(content);
}

void CombatView::drawUI() {
    Surface s = getSurface();
    s.clear(kBackgroundColor);
    s.drawWindow(kWin1Left, kWin1Top, kWin1Right, kWin1Bottom, kBackgroundColor);
    s.drawWindow(kWin2Left, kWin2Top, kWin2Right, kWin2Bottom, kBackgroundColor);
}

bool CombatView::centerCameraOn(::Goldbox::Data::PlayerCharacter *ch) {
    Combat::CombatContext *ctx = _session.getContext();
    if (!ch || !ctx || ctx->isCharacterInBounds(ch, true))
        return false;
    _session.getViewport().centerOn(_session.getTable().getCharacterPos(ch));
    return true;
}

void CombatView::drawCombatInfo(::Goldbox::Data::PlayerCharacter *ch) {
    _messageBox->drawActorInfo(ch);
}

// ---------------------------------------------------------------------------
// Messages and damage

void CombatView::printCombatMessage(::Goldbox::Data::PlayerCharacter *ch,
        const Common::String &text, uint8 line) {
    if (!g_engine)
        return;
    // Ensure the viewport is drawn before overlaying text.
    _needsFullRedraw = true;
    draw();
    _messageBox->showMessage(ch, text, line, false);
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
        const bool inCombat =
            _session.getPhase() != Combat::CombatSession::PHASE_NONE &&
            !_session.isEnded();
        if (inCombat)
            printCombatMessage(ch, r.downMessage);
        else if (_bridge)
            _bridge->postEffectMessage(ch, r.downMessage, false);

        if (inCombat) {
            ch->clearStatusEffects();
            Combat::CombatContext *ctx = _session.getContext();
            if (ctx && ctx->params.effectRuntime && ch->getEffects())
                ctx->params.effectRuntime->checkEffectSet(
                    ::Goldbox::Data::Effects::ES_ON_DEATH,
                    *ch->getEffects(), *ch, &ctx->globals);
            if (!ch->enabled)
                handleDeathOnMap(ch);
        } else if (_bridge)
            _bridge->requestRefresh(
                ::Goldbox::Data::Effects::EffectHostBridge::RF_VIEWPORT);
    }

    if (_bridge)
        _bridge->requestRefresh(
            ::Goldbox::Data::Effects::EffectHostBridge::RF_STATUS_PANEL);
}

void CombatView::handleDeathOnMap(::Goldbox::Data::PlayerCharacter *ch) {
    _session.handleCharacterDown(ch);
    _needsFullRedraw = true;
    if (_bridge)
        _bridge->requestRefresh(
            ::Goldbox::Data::Effects::EffectHostBridge::RF_VIEWPORT);
}

void CombatView::presentFrame(uint32 delayMs) {
    if (!g_events)
        return;
    _needsFullRedraw = true;
    draw();
    if (g_events->getScreen()) {
        g_events->getScreen()->makeAllDirty();
        g_events->getScreen()->update();
    }
    if (delayMs > 0)
        g_system->delayMillis(delayMs);
}

void CombatView::drawCharacterDying(::Goldbox::Data::PlayerCharacter *ch) {
    if (!ch || !g_events)
        return;

    const Combat::CombatantTable &table = _session.getTable();
    const int idx = table.findIndex(ch);
    if (idx < 0 || table.getSize(idx) == 0)
        return;

    _viewport->showDying(ch);

    if (g_events->getScreen()) {
        g_events->getScreen()->makeAllDirty();
        g_events->getScreen()->update();
    }
    g_system->delayMillis(kStateChangeDelayMs);
}

void CombatView::drawDamage(::Goldbox::Data::PlayerCharacter *ch,
        bool isMagic, const Common::String &message) {
    if (_session.getPhase() == Combat::CombatSession::PHASE_NONE ||
            _session.isEnded()) {
        if (_bridge)
            _bridge->postEffectMessage(ch, message, true);
        return;
    }

    _viewport->animateDamage(ch, isMagic);
}

// ---------------------------------------------------------------------------
// Movement, facing and ranged attacks

void CombatView::animateMovement(TilePos from, TilePos to,
        ::Goldbox::Data::PlayerCharacter *ch) {
    if (!ch)
        ch = _session.getCurrentActor();
    if (!ch)
        return;
    _viewport->animateMove(ch, from, to);
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
    if (!ctx->isCharacterInBounds(ch, true))
        _viewport->centerOn(_session.getTable().getCharacterPos(ch));

    const Direction oldDirection =
        static_cast<Direction>(ch->combatState ? ch->combatState->direction : 0);

    // Restore entity tiles when the coarse facing group changes,
    // a non-idle frame is requested, or the final draw is suppressed.
    // direction >> 2 groups the 8 directions into two mirror groups.
    if (((oldDirection >> 2) != (direction >> 2)) ||
        iconFrame != 0 ||
        noRedraw) {
        _viewport->draw();
    }

    ctx->setCharacterFacing(ch, direction);

    if (!noRedraw && ctx->isCharacterInBounds(ch, true)) {
        _viewport->drawCombatants();
        g_system->updateScreen();
    }
}

void CombatView::animateRangedAttack(
        ::Goldbox::Data::PlayerCharacter *attacker,
        ::Goldbox::Data::PlayerCharacter *target,
        const ::Goldbox::Data::Items::CharacterItem *item) {
    if (!attacker || !target || !item || !_session.getContext())
        return;

    _viewport->animateRangedAttack(attacker, target, item->prop().wpnType);
    _needsFullRedraw = true;
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox