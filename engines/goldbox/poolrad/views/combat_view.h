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
#include "goldbox/poolrad/views/dialogs/combat_info_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_message_box_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_viewport_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_menu_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_move_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_attack_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_end_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_spell_dialog.h"
#include "goldbox/poolrad/views/dialogs/combat_submenu_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/spells/spell_context.h"

// Forward declarations — avoid pulling full headers into the Poolrad::Views
// namespace where unqualified 'Data::' would resolve to Goldbox::Poolrad::Data.
namespace Goldbox {
namespace Data {
class PlayerCharacter;
namespace Items { struct CharacterItem; }
namespace Effects {
class EffectHostBridge;
class EffectRuntime;
} // namespace Effects
} // namespace Data
namespace Combat {
struct AiMoveViewDelegate;
struct CombatViewDelegate;
} // namespace Combat
} // namespace Goldbox

namespace Goldbox {
namespace Poolrad {
namespace Views {

/**
 * Tactical combat view — coordinator between the combat backend and the UI.
 *
 * Owns a CombatSession for all combat logic and state, and is the only
 * object the backend delegates talk to. Presentation is split into three
 * screen regions (original SCREEN_drawCombatWindows layout):
 * - CombatViewportDialog:   left window, battlefield and animations
 * - CombatMessageBoxDialog: right window, actor info and combat messages
 * - CombatInfoDialog:       bottom line, menus/prompts and info text
 * The view applies game-state changes (facing, damage, turn flow) and then
 * forwards presentation to the matching region.
 */
class CombatView : public View, public Combat::CastSpellViewDelegate,
                   public ::Goldbox::Spells::ISpellAimer,
                   public Dialogs::CombatInfoHost {
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

    /**
     * Print a message into the combat right-panel message area (cols 23-38).
     * Routes through GameText::showMessage which auto-selects combat placement
     * when GS_COMBAT is active. ch may be nullptr for system messages.
     * line is the name row (text starts at line+1); default 10 matches
     * the original COMBAT_ApplyDamageMessage anchor row.
     */
    void printCombatMessage(Goldbox::Data::PlayerCharacter *ch,
                            const Common::String &text, uint8 line = 10);

    bool msgFocus(const FocusMessage &msg) override;
    bool msgUnfocus(const UnfocusMessage &msg) override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void onUpdate() override;
    void handleMenuResult(const MenuResultMessage &result) override;

    void drawCombatInfo(Goldbox::Data::PlayerCharacter *ch);
    void refreshViewport(Goldbox::Data::PlayerCharacter *ch, Direction direction);

    // CastSpellViewDelegate
    void focusCaster(Goldbox::Data::PlayerCharacter *caster) override;
    void showBeginsCasting(Goldbox::Data::PlayerCharacter *caster) override;
    void showSpellMessage(Goldbox::Data::PlayerCharacter *caster,
                          const Common::String &text) override;
    bool confirmAbortSpell() override;
    void showSpellCastEffect(Goldbox::Data::PlayerCharacter *caster,
                             uint8 spellId, TilePos target) override;
    void showSpellProjectile(Goldbox::Data::PlayerCharacter *caster,
                             Goldbox::Data::PlayerCharacter *target) override;
    ::Goldbox::Spells::ISpellAimer *getSpellAimer() override { return this; }

    // Spells::ISpellAimer — blocks on the aim dialog until the player picks
    // a target/tile or cancels.
    bool aim(Goldbox::Data::PlayerCharacter *caster, uint8 spellId,
             uint8 range, bool tileMode,
             Goldbox::Data::PlayerCharacter *&target, TilePos &tile) override;
    void suppressRedraw() { _needsRedraw = false; }
    void forceFullRedraw() { _needsFullRedraw = true; }

    // --- Viewport update API ---
    // Hold time for a visible icon/state change (AI step, facing, attack
    // result, dying icon). Mirrors the original TIME_Wait pacing.
    static const uint32 kStateChangeDelayMs = 300;

    /**
     * Single entry point for presenting a combat frame: full redraw of
     * viewport, combatants, info panel and pending overlay, flip to the
     * screen, then hold for delayMs.
     */
    void presentFrame(uint32 delayMs = kStateChangeDelayMs);

    /**
     * Presentation half of COMBAT_HandleDeathOnMap: scroll ch into view,
     * overlay the dying icon on its footprint and hold it. Message text
     * already on screen is kept (no full redraw).
     */
    void drawCharacterDying(Goldbox::Data::PlayerCharacter *ch);

    // State setters called by AI/attack delegates before redraw().
    void clearCombatMessage() {
        _messageBox->clearMessage();
    }
    void setAiMessage(const Common::String &msg) {
        _info->showInfo(msg);
        _needsFullRedraw = true;
    }
    void setAiInfoActor(Goldbox::Data::PlayerCharacter *ch) {
        _messageBox->setActor(ch);
        _needsFullRedraw = true;
    }
    void setAiCharacterFacing(Goldbox::Data::PlayerCharacter *ch, Direction dir) {
        _aiFacingChar = ch;
        _aiFacingDir = dir;
        _needsFullRedraw = true;
    }

    void updateCharacterFacingAndRedraw(Goldbox::Data::PlayerCharacter *ch,
                                        Direction direction,
                                        uint8 iconFrame,
                                        bool noRedraw);
    void animateRangedAttack(Goldbox::Data::PlayerCharacter *attacker,
                             Goldbox::Data::PlayerCharacter *target,
                             const Goldbox::Data::Items::CharacterItem *item);

    // --- Debug accessors ---
    const Combat::BattlefieldMap &debugBattlefieldMap() const {
        return _session.getBattlefieldMap();
    }
    const ::Goldbox::Gfx::BattlefieldTilemap &debugTilemap() const {
        return _viewport->getTilemap();
    }
    const ::Goldbox::Gfx::CombatTileCache &debugTileCache() const {
        return _viewport->getTileCache();
    }

    // Dialogs::CombatInfoHost — prompt content stays a child of this view
    // so its menu results are posted to "Combat".
    void attachPromptContent(Dialogs::Dialog *content) override;
    void detachPromptContent(Dialogs::Dialog *content) override;
    const Combat::CombatantTable &debugCombatantTable() const {
        return _session.getTable();
    }
    int debugPartyCount() const {
        return _session.getParams().partyCount;
    }

private:
    // --- Backend ---
    Combat::CombatSession _session;

    // --- Screen regions (owned; drawn by draw() in a fixed order) ---
    Dialogs::CombatViewportDialog   *_viewport;
    Dialogs::CombatMessageBoxDialog *_messageBox;
    Dialogs::CombatInfoDialog       *_info;

    // --- Prompt content dialogs (shown through _info) ---
    Dialogs::CombatMenuDialog   *_combatMenu;
    Dialogs::CombatMoveDialog   *_combatMove;
    Dialogs::CombatAttackDialog *_combatAttack;
    Dialogs::CombatEndDialog    *_combatEnd;
    Dialogs::CombatSpellDialog  *_combatSpell;
    Dialogs::CombatSubmenuDialog *_combatSubmenu;

    // --- Effect bridge (wired at setup time) ---
    Goldbox::Data::Effects::EffectHostBridge *_bridge = nullptr;

    // Trigger-set evaluator handed to the session through CombatParams
    // (ES7/8/15/18 hooks). Owned by the view.
    Goldbox::Data::Effects::EffectRuntime *_effectRuntime = nullptr;

    // --- View delegates (owned, wired into session at setup time) ---
    Combat::AiMoveViewDelegate  *_aiDelegate     = nullptr;
    Combat::CombatViewDelegate  *_attackDelegate = nullptr;

    bool _needsFullRedraw;

    // --- Player action synchronization ---
    // Set by handleMenuResult() while onUpdate() is blocking in the player-input wait loop.
    bool _playerActionPending;
    Combat::CombatSession::PlayerAction _pendingPlayerAction;
    bool _viewingCharacter = false;

    // --- Autospell toggle (mirrors AUTOSPELL_STATUS) ---
    bool _autoSpell = false;

    // --- Pending AI facing (applied to game state by draw()) ---
    ::Goldbox::Data::PlayerCharacter *_aiFacingChar = nullptr;
    Direction _aiFacingDir = DIR_N;

    static const uint8 kBackgroundColor = 8;

    // --- Window layout (character coords, matching original SCREEN_DrawWindow calls) ---
    static const int kWin1Left   = 1;
    static const int kWin1Top    = 1;
    static const int kWin1Right  = 21;
    static const int kWin1Bottom = 21;
    static const int kWin2Left   = Dialogs::CombatMessageBoxDialog::kLeft;
    static const int kWin2Top    = Dialogs::CombatMessageBoxDialog::kTop;
    static const int kWin2Right  = Dialogs::CombatMessageBoxDialog::kRight;
    static const int kWin2Bottom = Dialogs::CombatMessageBoxDialog::kBottom;

    bool _roundEndAnswered = false;
    bool _roundEndContinue = false;

    // "Abort Spell?" prompt state (see confirmAbortSpell).
    Dialogs::HorizontalYesNo *_abortYesNo = nullptr;
    bool _awaitingAbortAnswer = false;
    bool _abortAnswered = false;
    bool _abortAnswer = false;

    // Success flag of the last sub-dialog result (cancel vs. confirm).
    bool _lastResultSuccess = true;

    /** Focus the actor, refresh its info panel and (re)show the combat menu. */
    void showCombatMenu(::Goldbox::Data::PlayerCharacter *actor);

    /**
     * Called after a player sub-dialog (move/cast/attack) returns. Mirrors
     * DIALOG_CombatMain: returns the actor to the menu while its turn is
     * not over, otherwise advances the session.
     */
    void finishPlayerAction();

    /** Mirrors DIALOG_CombatEnd presentation (messages, "Continue Battle?"). */
    void handleRoundEnd();

    /** Clear the screen and draw both combat window frames. */
    void drawUI();

    /**
     * Move the camera onto ch if its footprint is not fully visible.
     * Returns true if the camera moved (nothing is redrawn).
     */
    bool centerCameraOn(::Goldbox::Data::PlayerCharacter *ch);

    /** Animate a move and schedule a full redraw afterwards. */
    void animateMovement(TilePos from, TilePos to,
                         ::Goldbox::Data::PlayerCharacter *ch);
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_COMBAT_VIEW_H
