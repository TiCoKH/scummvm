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

#ifndef GOLDBOX_POOLRAD_VIEWS_IN_GAME_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_IN_GAME_VIEW_H

#include "goldbox/poolrad/views/view.h"
#include "goldbox/poolrad/wilderness_travel.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/core/global.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

namespace Dialogs {
class PartyList;
class Dialog;
class InGameMainScreenDialog;
class InGameStateAreaDialog;
class InGamePanelDialog;
class TextBoxDialog;
class InGameMenuDialog;
class CampMenuDialog;
class DoorDialog;
class ShopBaseDialog;
class PromptMessage;
class SpellBookDialog;
}

/**
 * Persistent aggregated in-game view covering all map-runtime states:
 * dungeon, wilderness, shop, camping, after-combat, and combat.
 *
 * Replaces per-state top-level view churn with a single long-lived View
 * whose internal mode switches when GameState changes.  This matches the
 * original engine's single continuous runtime surface with state-dependent
 * drawing (see VM_MAIN_TODO_PICKUP_GUIDE.md § "single InGameView aggregator").
 *
 * Architecture guardrails:
 *  - EclVM is owned by PoolradEngine/orchestrator, NOT by this view.
 *  - GameState remains authoritative; view only adjusts presentation.
 *  - Mode switch is idempotent: re-entering the same mode must not reload
 *    assets or reset transient UI unnecessarily.
 *  - Script dispatch (ONMOVE/ONSEARCH/ONREST) stays in the orchestrator.
 *
 * Layout (40×25 character grid):
 *  - Cols  0-15, rows  0-15: 3D viewport / left panel
 *  - Cols  3-13, rows  3-13: Inner mini-window (dungeon/camping/shop)
 *  - Cols 17-39, rows  1-14: Party summary (Name / AC / HP)
 *  - Cols 17-39, row  15:    Position / direction / status
 *  - Cols  0-39, rows 17-22: Prompt / text area
 *
 * Direction convention (8-direction, only even values used):
 *   0=N, 2=E, 4=S, 6=W
 */
class InGameView : public View {
public:
	/** Commands consumed by PoolradEngine legacy map-loop orchestrator. */
	enum InGameCommand {
		kCmdNone = 0,
		kCmdMove,
		kCmdSearch,
		kCmdLook,
		kCmdEncamp,
		// GB13-023: wilderness directional travel (distinct from the dungeon
		// kCmdMove turn-then-step convention). The compass direction is held
		// in _travelDir.
		kCmdWildernessMove,
		// CampMainMenu ended; _campInterrupted selects ONRESTINTR.
		kCmdLeaveCamp
	};

private:
	GameState _state = GS_START_MENU;
	bool _showPartyPanel = false;
	bool _showStateArea = false;

	Dialogs::InGameMainScreenDialog *_mainScreenDialog = nullptr;
	Dialogs::PartyList *_partyList = nullptr;
	Dialogs::InGameStateAreaDialog *_stateAreaDialog = nullptr;
	Dialogs::InGamePanelDialog *_shopPanelDialog = nullptr;
	Dialogs::InGamePanelDialog *_campingPanelDialog = nullptr;
	Dialogs::InGamePanelDialog *_afterCombatPanelDialog = nullptr;

	Dialogs::Dialog *_activeLeftPanelDialog = nullptr;
	Dialogs::Dialog *_activeStateAreaDialog = nullptr;
	Dialogs::TextBoxDialog *_textBoxDialog = nullptr;
	Dialogs::InGameMenuDialog *_inGameMenuDialog = nullptr;
	Dialogs::CampMenuDialog *_campMenuDialog = nullptr;
	Dialogs::DoorDialog *_doorDialog = nullptr;
	Dialogs::ShopBaseDialog *_shopDialog = nullptr;
	Dialogs::ShopBaseDialog *_templeDialog = nullptr;
	Dialogs::ShopBaseDialog *_treasureDialog = nullptr;
	Dialogs::ShopBaseDialog *_activeShopDialog = nullptr;
	/** Map 'C' cast menu (Camp.NoncombatCastMenu), cast-only spell book. */
	Dialogs::SpellBookDialog *_mapCastDialog = nullptr;
	/** _mapCastDialog was opened and its close is not handled yet. */
	bool _inMapCast = false;
	/** Row-24 PrintAndPause message ("Not Here"). */
	Dialogs::PromptMessage *_notHerePrompt = nullptr;

	// --- Dungeon navigation state ---
	/** Party map X position (column, 0-15). */
	uint16 _mapX = 8;
	/** Party map Y position (row, 0-15). */
	uint16 _mapY = 8;
	/** Party facing direction: 0=N, 2=E, 4=S, 6=W (8-dir wire format). */
	uint8 _mapDir = 0;
	/** Search mode active flag. */
	bool _searchMode = false;
	/** Tracks textbox busy state for async completion signaling. */
	bool _textBoxWasBusy = false;
	/** Area map overlay active (toggled by 'A' key). */
	bool _areaMapMode = false;
	/** Tracks whether in-game menu was active before unfocus (for stack restore). */
	bool _wasMenuActiveBeforeUnfocus = false;
	/** A script message is still shown (inverse of original bottomMsgCleared). */
	bool _scriptMessageShown = false;
	/** The text box holds a view message no VM operation waits for. */
	bool _localTextBoxMessage = false;
	/** The camp ended with an interrupted rest (for kCmdLeaveCamp). */
	bool _campInterrupted = false;
	/** Next command for engine-side map-loop. */
	InGameCommand _pendingCommand = kCmdNone;
	/**
	 * GB13-023: compass travel direction for a queued kCmdWildernessMove. Set
	 * at the single arrow->compass conversion boundary; read by the engine
	 * wilderness-travel path.
	 */
	WildernessTravel::Compass _travelDir = WildernessTravel::TRAVEL_NORTH;

	// --- Orchestration helpers ---
	void syncMainScreenDialog();
	/** Configure mode/layout flags from authoritative GameState. */
	void configureByState(GameState state);
	/** Activate/deactivate PartyList dialog according to current state flags. */
	void syncPartyDialog();
	/** Select active left inner-panel dialog based on GameState. */
	void syncLeftPanelDialog();
	/** Select active state-area dialog based on GameState. */
	void syncStateAreaDialog();
	/** Apply all dialog switching for current state flags. */
	void syncDialogs();

	// --- Mode-specific input ---
	bool handleDungeonKeypress(const KeypressMessage &msg);
	/** Wilderness arrow: set the compass facing and queue a travel step. */
	void travelWilderness(char key);
	/** Toggle the area map, or print "Not Here" when the zone forbids it. */
	void toggleAreaMap();
	/** Queue a command that ends the original MapMainMenu loop. */
	void endMapMenu(InGameCommand cmd);
	/** Map 'C': Camp.NoncombatCastMenu entry checks, then the cast menu. */
	void castFromMap();
	/** Return to the map menu once the cast menu has closed. */
	void finishMapCast();
	/** PrintNameString(curChar, text, row 18, pause). */
	void showTimedNameMessage(Goldbox::Data::PlayerCharacter *ch,
		const char *text);
	void syncDirectionAndRedraw();
	/** 3D left/right turn plays the step sound; turning around does not. */
	void playTurnSound();

public:
	InGameView();
	~InGameView() override;

	/**
	 * Apply screen layout and internal mode for the given GameState.
	 * Called from onEnter() and may be re-called on state refresh.
	 * Safe to call when already in the matching mode (idempotent).
	 */
	void applyScreenByState(GameState state);
	void onUpdate() override;

	bool msgFocus(const FocusMessage &msg) override;
	bool msgUnfocus(const UnfocusMessage &msg) override;
	bool msgKeypress(const KeypressMessage &msg) override;
	void draw() override;
	bool tick() override;
	void onEnter(GameState state) override;

	/** Queue a command for the engine runtime loop. */
	void queueCommand(InGameCommand cmd) { _pendingCommand = cmd; }

	/**
	 * Initiate door interaction at the current facing direction.
	 * Reads the GEO door flag and either auto-opens or shows DoorDialog.
	 */
	void openDoor();

	/** Enter camp mode: activate CampMenuDialog, hide normal menu. */
	void enterCamp();

	/** Exit camp mode: deactivate CampMenuDialog, queue kCmdLeaveCamp. */
	void exitCamp(bool wasInterrupted);

	/** True if the camp that just ended had its rest interrupted. */
	bool wasCampInterrupted() const { return _campInterrupted; }

	/** Enter shop/temple/treasure dialog based on encounter type. */
	void enterShop(uint8 shopType);

	/** Exit active shop dialog, restore previous state. */
	void exitShop();

	/** Returns true if a shop/temple/treasure dialog is currently active. */
	bool isShopActive() const;

	/** Returns true if a command is waiting for the engine runtime loop. */
	bool hasPendingCommand() const { return _pendingCommand != kCmdNone; }

	/** Pop and clear pending command (DIALOG_InGame equivalent handoff). */
	InGameCommand consumePendingCommand();

	/** True if the 2D area map overlay is active instead of 3D view. */
	bool isAreaMapMode() const { return _areaMapMode; }

	/** Get current map X position. */
	uint16 getMapX() const { return _mapX; }
	/** Get current map Y position. */
	uint16 getMapY() const { return _mapY; }
	/** Get current facing direction (wire format: 0=N,2=E,4=S,6=W). */
	uint8 getMapDir() const { return _mapDir; }
	/** GB13-023: compass direction for a queued wilderness travel command. */
	WildernessTravel::Compass getTravelDir() const { return _travelDir; }
	/** Set map position and direction from VM memory. */
	void setMapPosition(uint16 x, uint16 y, uint8 wireDir) {
		_mapX = x; _mapY = y; _mapDir = wireDir;
	}

	/**
	 * Print text into the message text box area (rows 17-22).
	 * Implements TEXT_boxMessage / TEXT_BlockPrint word-wrapping logic.
	 * @param text     Text to display
	 * @param clearBox If true, clear the text area before printing
	 */
	void printToTextBox(const Common::String &text, bool clearBox);
	void clearTextBox();

	/** Returns true if the text box is still rendering or waiting for key. */
	bool isTextBoxBusy() const;

	/**
	 * Handle menu result from InGameMenuDialog.
	 * Routes keycode to the appropriate game command.
	 */
	void handleMenuResult(const MenuResultMessage &result) override;

	/**
	 * Handle a map menu key (Explore.MapMainMenu equivalent): '8'/'2'/'4'/'6'
	 * for the arrows, otherwise the menu shortcut letter.
	 * @return false if the key has no meaning in the current map mode
	 */
	bool handleInGameMenuKey(char key);

	/** Show/hide the in-game menu based on map runtime ready state. */
	void setInGameMenuVisible(bool visible);
	bool isInGameMenuVisible() const;

	/**
	 * Apply damage to a character using the same message+textbox flow as the
	 * original STATS_ApplyDamage / TEXT_drawIntoMsgBox logic.
	 */
	void applyDamageMessage(Goldbox::Data::PlayerCharacter *ch,
			uint8 baseDamage,
			Goldbox::Data::DamageModifier modifier,
			bool applyModifier);

	/** React to VM opcode/syscall/state notifications. */
	void handleEclVmMessage(const EclVmMessage &msg) override;
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_IN_GAME_VIEW_H
