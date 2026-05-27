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

#include "goldbox/poolrad/views/dialogs/in_game_main_screen_dialog.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/in_game_panel_dialog.h"
#include "goldbox/poolrad/views/dialogs/in_game_state_area_dialog.h"
#include "goldbox/poolrad/views/dialogs/party_list.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

// Movement delta tables indexed by 4-direction (0=N, 1=E, 2=S, 3=W).
static const int kDx[4] = {0, 1, 0, -1};
static const int kDy[4] = {-1, 0, 1, 0};

// -----------------------------------------------------------------------

InGameView::InGameView() : View("InGame") {
	_mainScreenDialog =
		new Dialogs::InGameMainScreenDialog("InGameMainScreenDialog");
	attachDialog(_mainScreenDialog);

	_partyList = new Dialogs::PartyList("InGamePartyList");
	attachDialog(_partyList);

	_stateAreaDialog = new Dialogs::InGameStateAreaDialog("InGameStateArea");
	attachDialog(_stateAreaDialog);

	_shopPanelDialog = new Dialogs::InGamePanelDialog("InGameShopPanel", "Shop");
	_shopPanelDialog->setRuntimeMode(Dialogs::InGamePanelDialog::kRuntimeShop);
	attachDialog(_shopPanelDialog);

	_campingPanelDialog = new Dialogs::InGamePanelDialog("InGameCampPanel", "Camp");
	_campingPanelDialog->setRuntimeMode(Dialogs::InGamePanelDialog::kRuntimeCamping);
	attachDialog(_campingPanelDialog);

	_afterCombatPanelDialog = new Dialogs::InGamePanelDialog("InGameAfterCombatPanel", "AfterFight");
	_afterCombatPanelDialog->setRuntimeMode(Dialogs::InGamePanelDialog::kRuntimeAfterCombat);
	attachDialog(_afterCombatPanelDialog);
}

InGameView::~InGameView() {
	if (_mainScreenDialog) {
		delete _mainScreenDialog;
		_mainScreenDialog = nullptr;
	}

	if (_afterCombatPanelDialog) {
		delete _afterCombatPanelDialog;
		_afterCombatPanelDialog = nullptr;
	}

	if (_campingPanelDialog) {
		delete _campingPanelDialog;
		_campingPanelDialog = nullptr;
	}

	if (_shopPanelDialog) {
		delete _shopPanelDialog;
		_shopPanelDialog = nullptr;
	}

	if (_stateAreaDialog) {
		delete _stateAreaDialog;
		_stateAreaDialog = nullptr;
	}

	if (_partyList) {
		delete _partyList;
		_partyList = nullptr;
	}
}

// -----------------------------------------------------------------------
// Public interface

void InGameView::onEnter(GameState state) {
	applyScreenByState(state);
}

InGameView::InGameCommand InGameView::consumePendingCommand() {
	InGameCommand cmd = _pendingCommand;
	_pendingCommand = kCmdNone;
	return cmd;
}

void InGameView::applyScreenByState(GameState state) {
	_state = state;
	configureByState(state);
	syncMainScreenDialog();
	syncDialogs();
	if (_stateAreaDialog)
		_stateAreaDialog->setState(_state);
	redraw();
}

void InGameView::onUpdate() {
	const GameState runtimeState = VmInterface::getGameStatus();
	if (runtimeState != _state)
		applyScreenByState(runtimeState);
}

void InGameView::configureByState(GameState state) {
	// Original GAME_screenByState mappings (DOS + m68k variants):
	//  - SHOP         -> main windows with inner frame, portrait area, no state area
	//  - CAMPING      -> main windows with inner frame + show party + state area
	//  - DUNGEON_MAP  -> main windows with inner frame + MAP_3DColorUpdate +
	//                    show party + state area
	//  - WILDERNESS   -> main windows without inner frame + MAP_3DColorUpdate +
	//                    show party + state area
	//  - AFTER_COMBAT -> main windows with inner frame + show party
	// Dialog orchestration (legacy intent):
	//  - DIALOG_ShowParty in CAMPING/DUNGEON/WILDERNESS/AFTER_COMBAT
	//  - DIALOG_StateArea in CAMPING/DUNGEON/WILDERNESS
	_showPartyPanel = false;
	_showStateArea = false;

	switch (state) {
	case GS_SHOP:
		break;
	case GS_CAMPING:
		_showPartyPanel = true;
		_showStateArea = true;
		break;
	case GS_DUNGEON_MAP:
		_showPartyPanel = true;
		_showStateArea = true;
		break;
	case GS_WILDERNESS_MAP:
		_showPartyPanel = true;
		_showStateArea = true;
		break;
	case GS_AFTER_COMBAT:
		_showPartyPanel = true;
		break;
	case GS_COMBAT:
		_showPartyPanel = true;
		break;
	default:
		break;
	}
}

void InGameView::syncMainScreenDialog() {
	if (!_mainScreenDialog)
		return;

	Dialogs::InGameMainScreenDialog::DrawMode mode =
		Dialogs::InGameMainScreenDialog::kModeNone;

	switch (_state) {
	case GS_SHOP:
		mode = Dialogs::InGameMainScreenDialog::kModeShop;
		break;
	case GS_CAMPING:
		mode = Dialogs::InGameMainScreenDialog::kModeCamping;
		break;
	case GS_DUNGEON_MAP:
		mode = Dialogs::InGameMainScreenDialog::kModeDungeon;
		break;
	case GS_WILDERNESS_MAP:
		mode = Dialogs::InGameMainScreenDialog::kModeWilderness;
		break;
	case GS_AFTER_COMBAT:
		mode = Dialogs::InGameMainScreenDialog::kModeAfterCombat;
		break;
	case GS_COMBAT:
		mode = Dialogs::InGameMainScreenDialog::kModeCombat;
		break;
	default:
		break;
	}

	_mainScreenDialog->setDrawMode(mode);
	if (!_mainScreenDialog->isActive())
		_mainScreenDialog->activate();
}

void InGameView::syncPartyDialog() {
	if (!_partyList)
		return;

	if (_showPartyPanel) {
		if (!_partyList->isActive())
			_partyList->activate();
	} else if (_partyList->isActive()) {
		_partyList->deactivate();
	}
}

void InGameView::syncLeftPanelDialog() {
	Dialogs::Dialog *nextDialog = nullptr;

	switch (_state) {
	case GS_SHOP:
		nextDialog = _shopPanelDialog;
		break;
	case GS_CAMPING:
		nextDialog = _campingPanelDialog;
		break;
	case GS_AFTER_COMBAT:
		nextDialog = _afterCombatPanelDialog;
		break;
	default:
		break;
	}

	switchActiveDialog(_activeLeftPanelDialog, nextDialog);
}

void InGameView::syncStateAreaDialog() {
	Dialogs::Dialog *nextDialog = _showStateArea ? static_cast<Dialogs::Dialog *>(_stateAreaDialog) : nullptr;
	switchActiveDialog(_activeStateAreaDialog, nextDialog);
}

void InGameView::syncDialogs() {
	syncPartyDialog();
	syncLeftPanelDialog();
	syncStateAreaDialog();
}

// -----------------------------------------------------------------------
// View lifecycle

bool InGameView::msgFocus(const FocusMessage &msg) {
	View::msgFocus(msg);
	applyScreenByState(_state);
	return true;
}

bool InGameView::msgUnfocus(const UnfocusMessage &msg) {
	if (_activeLeftPanelDialog)
		_activeLeftPanelDialog->deactivate();
	if (_activeStateAreaDialog)
		_activeStateAreaDialog->deactivate();
	if (_partyList && _partyList->isActive())
		_partyList->deactivate();
	return true;
}

// -----------------------------------------------------------------------
// Drawing

void InGameView::draw() {
	// Keep long-lived InGameView presentation synchronized with authoritative
	// engine game state even when the view is not recreated.
	const GameState runtimeState = VmInterface::getGameStatus();
	if (runtimeState != _state)
		applyScreenByState(runtimeState);

	if (_mainScreenDialog)
		_mainScreenDialog->draw();

	if (_activeLeftPanelDialog && _activeLeftPanelDialog->isActive())
		_activeLeftPanelDialog->draw();

	if (_showPartyPanel && _partyList && _partyList->isActive())
		_partyList->draw();

	if (_showStateArea && _stateAreaDialog && _stateAreaDialog->isActive()) {
		_stateAreaDialog->setState(_state);
		_stateAreaDialog->draw();
	}
}

// -----------------------------------------------------------------------
// Input

bool InGameView::msgKeypress(const KeypressMessage &msg) {
	// Legacy DIALOG_ShowParty navigation stays active in states that show it.
	if (_showPartyPanel && _partyList && _partyList->isActive()) {
		if (_partyList->msgKeypress(msg)) {
			redraw();
			return true;
		}
	}

	if (_activeLeftPanelDialog && _activeLeftPanelDialog->send(msg))
		return true;

	if (_activeStateAreaDialog && _activeStateAreaDialog->send(msg))
		return true;

	if (_state == GS_DUNGEON_MAP || _state == GS_WILDERNESS_MAP
			|| _state == GS_CAMPING || _state == GS_AFTER_COMBAT
			|| _state == GS_COMBAT)
		return handleDungeonKeypress(msg);

	return false;
}

bool InGameView::handleDungeonKeypress(const KeypressMessage &msg) {
	bool handled = true;

	switch (msg.keycode) {
	// --- Movement (arrow keys) ---
	case Common::KEYCODE_UP:
		stepForward();
		queueCommand(kCmdMove);
		break;
	case Common::KEYCODE_LEFT:
		_mapDir = (_mapDir + 6) % 8;
		queueCommand(kCmdMove);
		break;
	case Common::KEYCODE_RIGHT:
		_mapDir = (_mapDir + 2) % 8;
		queueCommand(kCmdMove);
		break;
	case Common::KEYCODE_DOWN:
		_mapDir = (_mapDir + 4) % 8;
		queueCommand(kCmdMove);
		break;

	// --- Menu keys ---
	case Common::KEYCODE_a:
		// TODO: Toggle area-map overlay.
		break;
	case Common::KEYCODE_s:
		// 'S' toggles persistent search-while-walking (D_SearchFlags bit 0 XOR).
		// Queues to engine for VM memory write; no script runs.
		_searchMode = !_searchMode;
		queueCommand(kCmdSearch);
		break;
	case Common::KEYCODE_e:
		queueCommand(kCmdEncamp);
		break;
	case Common::KEYCODE_v:
		// View character sheet.
		replaceView("ViewCharacter");
		return true;
	case Common::KEYCODE_c:
		// TODO: Open spell menu for selected player.
		break;
	case Common::KEYCODE_l:
		// 'L' (Look): one-shot search. Sets D_SearchFlags |= 2, advances time,
		// then exits to engine which runs ECL_ONSEARCH once and clears bit 1.
		queueCommand(kCmdLook);
		break;

	default:
		handled = false;
		break;
	}

	if (handled)
		redraw();

	return handled;
}

void InGameView::stepForward() {
	// Convert 8-direction to 4-direction index (N=0, E=1, S=2, W=3).
	int dir4 = _mapDir / 2;
	int newX = (int)_mapX + kDx[dir4];
	int newY = (int)_mapY + kDy[dir4];

	// Boundary check: GEO maps are 16×16.
	if (newX < 0 || newX >= 16 || newY < 0 || newY >= 16)
		return;

	// TODO: Check wall / door collision via DaxBlockGeo once geo is loaded.
	_mapX = (uint16)newX;
	_mapY = (uint16)newY;
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
