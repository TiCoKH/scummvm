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
#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/poolrad/views/dialogs/in_game_menu_dialog.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/core/direction.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

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
	_shopPanelDialog->deactivate();
	attachDialog(_shopPanelDialog);

	_campingPanelDialog = new Dialogs::InGamePanelDialog("InGameCampPanel", "Camp");
	_campingPanelDialog->setRuntimeMode(Dialogs::InGamePanelDialog::kRuntimeCamping);
	_campingPanelDialog->deactivate();
	attachDialog(_campingPanelDialog);

	_afterCombatPanelDialog = new Dialogs::InGamePanelDialog("InGameAfterCombatPanel", "AfterFight");
	_afterCombatPanelDialog->setRuntimeMode(Dialogs::InGamePanelDialog::kRuntimeAfterCombat);
	_afterCombatPanelDialog->deactivate();
	attachDialog(_afterCombatPanelDialog);

	_textBoxDialog = new Dialogs::TextBoxDialog("InGameTextBox");
	attachDialog(_textBoxDialog);

	_inGameMenuDialog = new Dialogs::InGameMenuDialog("InGameMenu");
	// Not attached to view hierarchy — managed via setInGameMenuVisible.
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

	if (_textBoxDialog) {
		delete _textBoxDialog;
		_textBoxDialog = nullptr;
	}

	if (_inGameMenuDialog) {
		delete _inGameMenuDialog;
		_inGameMenuDialog = nullptr;
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
	// Safety-net polling path: primary state routing is event-driven via
	// EclVmMessage(ST_GAME_STATE).
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

	if (_textBoxDialog && _textBoxDialog->isActive())
		_textBoxDialog->draw();

	if (_inGameMenuDialog && _inGameMenuDialog->isActive())
		_inGameMenuDialog->draw();
}

// -----------------------------------------------------------------------
// Input

bool InGameView::msgKeypress(const KeypressMessage &msg) {
	// Text box gets priority when waiting for key (overflow prompt).
	if (_textBoxDialog && _textBoxDialog->isActive()
			&& _textBoxDialog->msgKeypress(msg)) {
		redraw();
		return true;
	}

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

	// Propagate to children first (ECL async menus during ONINIT).
	for (Common::Array<UIElement *>::iterator it = _children.begin();
			it != _children.end(); ++it) {
		if ((*it)->send(msg))
			return true;
	}

	// In-game menu: explicitly forward keypresses when active.
	// Not in the child tree — only active after ONINIT completes.
	if (_inGameMenuDialog && _inGameMenuDialog->isActive()) {
		if (_inGameMenuDialog->send(msg)) {
			redraw();
			return true;
		}
	}

	// Fallback: dungeon keys only when no in-game menu and not suspended.
	if ((_state == GS_DUNGEON_MAP || _state == GS_WILDERNESS_MAP
			|| _state == GS_CAMPING || _state == GS_AFTER_COMBAT
			|| _state == GS_COMBAT)
			&& !(g_engine && g_engine->getLegacySharedRuntimeState().boolSuspendFlag)
			&& !(_inGameMenuDialog && _inGameMenuDialog->isActive()))
		if (handleDungeonKeypress(msg))
			return true;

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
		_areaMapMode = !_areaMapMode;
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

void InGameView::printToTextBox(const Common::String &text, bool clearBox) {
	if (_textBoxDialog) {
		if (!_textBoxDialog->isActive())
			_textBoxDialog->activate();
		_textBoxDialog->setText(text, clearBox);
	}
}

bool InGameView::isTextBoxBusy() const {
	return _textBoxDialog && _textBoxDialog->isBusy();
}

void InGameView::handleMenuResult(const MenuResultMessage &result) {
	if (!result._success)
		return;

	Common::KeyCode key = result._keyCode;

	// Navigation keys.
	switch (key) {
	case Common::KEYCODE_UP:
	case Common::KEYCODE_KP8:
		handleInGameMenuKey('8');
		return;
	case Common::KEYCODE_DOWN:
	case Common::KEYCODE_KP2:
		handleInGameMenuKey('2');
		return;
	case Common::KEYCODE_LEFT:
	case Common::KEYCODE_KP4:
		handleInGameMenuKey('4');
		return;
	case Common::KEYCODE_RIGHT:
	case Common::KEYCODE_KP6:
		handleInGameMenuKey('6');
		return;
	default:
		break;
	}

	// Shortcut letter keys.
	char ascii = static_cast<char>(key);
	if (ascii >= 'a' && ascii <= 'z')
		ascii = ascii - 32;
	if (ascii >= 'A' && ascii <= 'Z')
		handleInGameMenuKey(ascii);
}

void InGameView::handleInGameMenuKey(char key) {
	switch (key) {
	case '8': // Forward — step and dispatch ONMOVE
		queueCommand(kCmdMove);
		break;
	case '2': // Turn around — rotate only, no ONMOVE
		_mapDir = (_mapDir + 4) % 8;
		syncDirectionAndRedraw();
		break;
	case '4': // Turn left — rotate only, no ONMOVE
		_mapDir = (_mapDir + 6) % 8;
		syncDirectionAndRedraw();
		break;
	case '6': // Turn right — rotate only, no ONMOVE
		_mapDir = (_mapDir + 2) % 8;
		syncDirectionAndRedraw();
		break;
	case 'A': // Area map toggle
		_areaMapMode = !_areaMapMode;
		break;
	case 'E': // Encamp
		queueCommand(kCmdEncamp);
		break;
	case 'S': // Search toggle
		_searchMode = !_searchMode;
		queueCommand(kCmdSearch);
		break;
	case 'L': // Look
		queueCommand(kCmdLook);
		break;
	case 'V': // View character
		replaceView("ViewCharacter");
		break;
	case 'C': // Cast spell
		// TODO: Open spell menu
		break;
	default:
		break;
	}
	redraw();
}

void InGameView::syncDirectionAndRedraw() {
	if (!g_engine)
		return;
	ECL::AddressSpace *mem = g_engine->getEclMemory();
	if (!mem)
		return;
	// Write direction directly to the known VM global field.
	// DungeonDir is at a fixed offset in bank 4 (global fields).
	// Use the runtime exchange to get the address.
	RuntimeMapSnapshot snap;
	const RuntimeExchange *exchange = g_engine->getRuntimeExchange();
	if (!exchange)
		return;
	// The direction field address can be obtained from the layout,
	// but since poolrad.h is included we can cast and use getEclMemory.
	// Simpler: just write cardinal direction at the known offset.
	// kVmGlobalFieldDungeonDir is at offset within bank 4.
	// We already have the address from the snapshot system.
	// Actually just use the Poolrad engine's writeDirection helper.
	Poolrad::PoolradEngine *pe = Poolrad::g_engine;
	if (pe) {
		pe->syncViewDirection(static_cast<uint8>((_mapDir / 2) & 0x03));
	}
}

void InGameView::setInGameMenuVisible(bool visible) {
	if (!_inGameMenuDialog)
		return;
	if (visible) {
		if (!_inGameMenuDialog->isActive()) {
			_inGameMenuDialog->setMode(
				(_state == GS_WILDERNESS_MAP)
					? Dialogs::InGameMenuDialog::kModeWilderness
					: Dialogs::InGameMenuDialog::kModeDungeon);
			_inGameMenuDialog->activate();
			redraw();
		}
	} else {
		if (_inGameMenuDialog->isActive()) {
			_inGameMenuDialog->deactivate();
			redraw();
		}
	}
}

void InGameView::handleEclVmMessage(const EclVmMessage &msg) {
	if (!g_engine)
		return;

	if (msg._kind == EclVmMessage::MK_STATE) {
		switch (msg._tag) {
		case EclVmMessage::ST_GAME_STATE:
			applyScreenByState(static_cast<GameState>(msg.asUint8()));
			return;
		case EclVmMessage::ST_POSITION_DIRTY: {
			RuntimeMapSnapshot snapshot;
			if (g_engine->captureRuntimeMapSnapshot(snapshot) && snapshot.valid) {
				setMapPosition(snapshot.dungeonX, snapshot.dungeonY,
					static_cast<uint8>((snapshot.dungeonDir & 0x03) * 2));
				_searchMode = snapshot.searchActive;
				if (snapshot.gameState != _state)
					applyScreenByState(snapshot.gameState);
				else
					redraw();
			}
			return;
		}
		case EclVmMessage::ST_SCREEN_REFRESH:
		case EclVmMessage::ST_SKYBOX_DIRTY:
		case EclVmMessage::ST_CHARACTER_DIRTY:
		case EclVmMessage::ST_STATUS_DIRTY:
			redraw();
			return;
		default:
			break;
		}
	}

	if (msg._kind == EclVmMessage::MK_SYSCALL) {
		switch (msg._tag) {
		case EclVmMessage::SC_MAP_DATA_READY:
			setInGameMenuVisible(true);
			redraw();
			return;
		case EclVmMessage::SC_PRINT:
		case EclVmMessage::SC_PRINT_ASYNC:
		case EclVmMessage::SC_CLEAR_TEXTBOX:
		case EclVmMessage::SC_DISPLAY_PICTURE:
		case EclVmMessage::SC_LOAD_GEO:
		case EclVmMessage::SC_LOAD_WALLSET:
		case EclVmMessage::SC_LOAD_ICON:
		case EclVmMessage::SC_SPRITE_OFF:
			redraw();
			return;
		case EclVmMessage::SC_START_COMBAT:
		case EclVmMessage::SC_EXECUTE_PROGRAM:
			onUpdate();
			redraw();
			return;
		default:
			break;
		}
	}

	if (msg._kind == EclVmMessage::MK_OPCODE &&
			msg._phase == EclVmMessage::OP_EXIT) {
		switch (msg._opcode) {
		case 0x0E:
		case 0x11:
		case 0x12:
		case 0x21:
		case 0x31:
		case 0x37:
		case 0x3D:
			redraw();
			return;
		case 0x24:
		case 0x38:
			onUpdate();
			redraw();
			return;
		default:
			break;
		}
	}

	if (msg._kind == EclVmMessage::MK_MEMORY_WRITE) {
		const Goldbox::VmGlobalLayout &globalLayout =
			Data::getPoolradGlobalVmLayout();
		const uint16 xAddr = globalLayout.field(kVmGlobalFieldDungeonX).vmAddr;
		const uint16 yAddr = globalLayout.field(kVmGlobalFieldDungeonY).vmAddr;
		const uint16 dirAddr =
			globalLayout.field(kVmGlobalFieldDungeonDir).vmAddr;
		const uint16 searchAddr =
			globalLayout.field(kVmGlobalFieldSearchFlags).vmAddr;

		if (msg._address == xAddr) {
			_mapX = msg.asUint8();
			redraw();
			return;
		}

		if (msg._address == yAddr) {
			_mapY = msg.asUint8();
			redraw();
			return;
		}

		if (msg._address == dirAddr) {
			_mapDir = static_cast<uint8>((msg.asUint8() & 0x03) * 2);
			redraw();
			return;
		}

		if (msg._address == searchAddr) {
			_searchMode = ((msg.asUint8() & 1) != 0);
			redraw();
			return;
		}
	}
}

void InGameView::stepForward() {
	// Use 8-direction deltas directly from the shared table.
	const int newX = (int)_mapX + kDirDeltaX[_mapDir];
	const int newY = (int)_mapY + kDirDeltaY[_mapDir];

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
