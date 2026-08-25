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
#include "goldbox/poolrad/views/dialogs/camp_menu_dialog.h"
#include "goldbox/poolrad/views/dialogs/door_dialog.h"
#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"
#include "goldbox/poolrad/views/dialogs/store_dialog.h"
#include "goldbox/poolrad/views/dialogs/temple_dialog.h"
#include "goldbox/poolrad/views/dialogs/treasure_dialog.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/data/damage_system.h"
#include "goldbox/combat/damage_utils.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/core/direction.h"
#include "goldbox/runtime/runtime_exchange.h"
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

	// CampMenuDialog handles camping picture + menu directly.
	_campingPanelDialog = nullptr;

	_afterCombatPanelDialog = new Dialogs::InGamePanelDialog("InGameAfterCombatPanel", "AfterFight");
	_afterCombatPanelDialog->setRuntimeMode(Dialogs::InGamePanelDialog::kRuntimeAfterCombat);
	_afterCombatPanelDialog->deactivate();
	attachDialog(_afterCombatPanelDialog);

	_textBoxDialog = new Dialogs::TextBoxDialog("InGameTextBox");
	attachDialog(_textBoxDialog);

	_inGameMenuDialog = new Dialogs::InGameMenuDialog("InGameMenu");
	_inGameMenuDialog->deactivate();

	_campMenuDialog = new Dialogs::CampMenuDialog("CampMenu");
	_campMenuDialog->deactivate();
	attachDialog(_campMenuDialog);

	_doorDialog = new Dialogs::DoorDialog("DoorDialog");
	_doorDialog->deactivate();
	attachDialog(_doorDialog);

	_shopDialog = new Dialogs::StoreDialog("Store");
	_shopDialog->deactivate();
	attachDialog(_shopDialog);

	_templeDialog = new Dialogs::TempleDialog("Temple");
	_templeDialog->deactivate();
	attachDialog(_templeDialog);

	_treasureDialog = new Dialogs::TreasureDialog("Treasure");
	_treasureDialog->deactivate();
	attachDialog(_treasureDialog);
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

	if (_campMenuDialog) {
		delete _campMenuDialog;
		_campMenuDialog = nullptr;
	}

	if (_doorDialog) {
		delete _doorDialog;
		_doorDialog = nullptr;
	}

	if (_shopDialog) {
		delete _shopDialog;
		_shopDialog = nullptr;
	}

	if (_templeDialog) {
		delete _templeDialog;
		_templeDialog = nullptr;
	}

	if (_treasureDialog) {
		delete _treasureDialog;
		_treasureDialog = nullptr;
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

	// Enter/exit camp dialog based on state.
	if (state == GS_CAMPING) {
		enterCamp();
	} else if (_campMenuDialog && _campMenuDialog->isActive()) {
		_campMenuDialog->deactivate();
	}

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
		_showPartyPanel = true;
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
		// CampMenuDialog handles the campfire picture directly.
		nextDialog = nullptr;
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
	if (!_wasMenuActiveBeforeUnfocus && _inGameMenuDialog
			&& _inGameMenuDialog->isActive())
		_inGameMenuDialog->deactivate();
	_wasMenuActiveBeforeUnfocus = false;
	applyScreenByState(_state);

	// Reactivate shop dialog if it was active before unfocus.
	if (_activeShopDialog && !_activeShopDialog->isActive())
		_activeShopDialog->activate();

	return true;
}

bool InGameView::msgUnfocus(const UnfocusMessage &msg) {
	_wasMenuActiveBeforeUnfocus = _inGameMenuDialog
			&& _inGameMenuDialog->isActive();
	if (_activeLeftPanelDialog)
		_activeLeftPanelDialog->deactivate();
	if (_activeStateAreaDialog)
		_activeStateAreaDialog->deactivate();
	if (_partyList && _partyList->isActive())
		_partyList->deactivate();
	if (_activeShopDialog && _activeShopDialog->isActive())
		_activeShopDialog->deactivate();
	return true;
}

// -----------------------------------------------------------------------
// Drawing

void InGameView::draw() {
	if (_mainScreenDialog)
		_mainScreenDialog->draw();

	// Camp dialog takes over left panel + menu when active.
	if (_campMenuDialog && _campMenuDialog->isActive()) {
		// Tick campfire animation from the view's draw cycle.
		if (g_engine) {
			::Goldbox::Gfx::PictureDisplayCache &cache =
				g_engine->getPictureDisplayCache();
			if (cache.tickAnimation())
				_mainScreenDialog->draw(); // Redraw viewport with new frame.
		}
		_campMenuDialog->draw();

		if (_showPartyPanel && _partyList && _partyList->isActive())
			_partyList->draw();

		if (_showStateArea && _stateAreaDialog && _stateAreaDialog->isActive()) {
			_stateAreaDialog->setState(_state);
			_stateAreaDialog->draw();
		}
		return;
	}

	// Shop/Temple/Treasure dialog active.
	if (_activeShopDialog && _activeShopDialog->isActive()) {
		if (_showPartyPanel && _partyList && _partyList->isActive())
			_partyList->draw();

		_activeShopDialog->draw();
		return;
	}

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

	if (_doorDialog && _doorDialog->isActive())
		_doorDialog->draw();
}

// -----------------------------------------------------------------------
// Input

bool InGameView::msgKeypress(const KeypressMessage &msg) {
	// Camp dialog consumes most keys when active — blocks navigation.
	// Party list gets first pass at scroll keys.
	if (_campMenuDialog && _campMenuDialog->isActive()) {
		if (_showPartyPanel && _partyList && _partyList->isActive()) {
			if (_partyList->msgKeypress(msg)) {
				redraw();
				return true;
			}
		}
		if (_campMenuDialog->msgKeypress(msg)) {
			redraw();
			return true;
		}
		// Camp dialog returned false for passthrough keys — already handled above.
		return true;
	}

	// Shop/Temple/Treasure dialog: shop gets first pass at keys.
	// Unhandled keys (e.g. HOME/END at base menu) fall through to party list.
	if (_activeShopDialog && _activeShopDialog->isActive()) {
		if (_activeShopDialog->msgKeypress(msg)) {
			redraw();
			return true;
		}
		if (_showPartyPanel && _partyList && _partyList->isActive()) {
			if (_partyList->msgKeypress(msg)) {
				redraw();
				return true;
			}
		}
		return true;
	}

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
	if (_doorDialog && _doorDialog->isActive()) {
		if (_doorDialog->send(msg)) {
			redraw();
			return true;
		}
	}

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
			&& !VmInterface::isRuntimeSuspended()
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
		syncDirectionAndRedraw();
		break;
	case Common::KEYCODE_RIGHT:
		_mapDir = (_mapDir + 2) % 8;
		syncDirectionAndRedraw();
		break;
	case Common::KEYCODE_DOWN:
		_mapDir = (_mapDir + 4) % 8;
		syncDirectionAndRedraw();
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
		addView("ViewCharacter");
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

void InGameView::applyDamageMessage(Goldbox::Data::PlayerCharacter *ch,
		uint8 baseDamage, Goldbox::Data::DamageModifier modifier,
		bool applyModifier) {
	if (!ch)
		return;

	Goldbox::Data::DamageSystem damageSystem(nullptr);
	const Goldbox::Data::DamageResult r = damageSystem.applyLegacy(*ch,
		baseDamage, modifier, applyModifier);

	if (r.applied <= 0)
		return;

	if (_textBoxDialog) {
		if (!_textBoxDialog->isActive())
			_textBoxDialog->activate();
		_textBoxDialog->setText(r.message, true);
	}

	if (r.interruptedSpell)
		printToTextBox(r.spellLostMessage, false);

	if (r.wentDown)
		printToTextBox(r.downMessage, false);
}

void InGameView::printToTextBox(const Common::String &text, bool clearBox) {
	if (_textBoxDialog) {
		if (!_textBoxDialog->isActive())
			_textBoxDialog->activate();
		_textBoxDialog->setText(text, clearBox);
	}
}

void InGameView::clearTextBox() {
	if (_textBoxDialog) {
		if (!_textBoxDialog->isActive())
			_textBoxDialog->activate();
		_textBoxDialog->clearText();
	}
}

bool InGameView::isTextBoxBusy() const {
	return _textBoxDialog && _textBoxDialog->isBusy();
}

void InGameView::handleMenuResult(const MenuResultMessage &result) {
	if (result._hasStringValue &&
			result._stringValue == "EffectStatusChanged") {
		onUpdate();
		redraw();
		return;
	}

	// Door dialog result.
	if (result._hasStringValue &&
			result._stringValue == "DoorResult") {
		if (result._hasIntValue &&
				result._intValue == Dialogs::DoorDialog::kDoorOpened) {
			// Door opened — play step sound and advance forward.
			if (g_engine)
				g_engine->soundPlay(0x0B);
			stepForward();
			queueCommand(kCmdMove);
		} else {
			// Door blocked — play blocked sound.
			if (g_engine)
				g_engine->soundPlay(0x08);
		}
		redraw();
		return;
	}

	// Camp exit result from CampMenuDialog.
	if (result._success && result._keyCode == Common::KEYCODE_e
			&& _campMenuDialog && _campMenuDialog->isActive()) {
		exitCamp(result._hasIntValue && result._intValue != 0);
		return;
	}

	// Shop/Temple/Treasure exit result from ShopBaseDialog.
	if (result._success && result._keyCode == Common::KEYCODE_e
			&& _activeShopDialog) {
		exitShop();
		return;
	}

	// Shop dialog View action.
	if (result._success && result._keyCode == Common::KEYCODE_v
			&& _activeShopDialog) {
		addView("ViewCharacter");
		return;
	}

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
		addView("ViewCharacter");
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

	VmInterface::syncViewDirection(static_cast<uint8>((_mapDir / 2) & 0x03));
}

void InGameView::setInGameMenuVisible(bool visible) {
	if (!_inGameMenuDialog)
		return;
	if (visible) {
		Dialogs::InGameMenuDialog::MapMode menuMode;
		if (_state == GS_CAMPING)
			menuMode = Dialogs::InGameMenuDialog::kModeCamping;
		else if (_state == GS_WILDERNESS_MAP)
			menuMode = Dialogs::InGameMenuDialog::kModeWilderness;
		else
			menuMode = Dialogs::InGameMenuDialog::kModeDungeon;
		_inGameMenuDialog->setMode(menuMode);
		if (!_inGameMenuDialog->isActive()) {
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

bool InGameView::tick() {
	// Detect textbox busy->idle transition and signal RuntimeExchange.
	const bool busy = _textBoxDialog && _textBoxDialog->isBusy();
	if (_textBoxWasBusy && !busy) {
		RuntimeExchange *exchange = g_engine
			? g_engine->getRuntimeExchange() : nullptr;
		if (exchange)
			exchange->signalAsync(RuntimeExchange::kAsyncTextBoxDone);
	}
	_textBoxWasBusy = busy;

	// Keep view dirty while camp animation is active.
	if (_campMenuDialog && _campMenuDialog->isActive() && g_engine
			&& g_engine->getPictureDisplayCache().isAnimated())
		redraw();

	return UIElement::tick();
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
			if (VmInterface::captureRuntimeMapSnapshot(snapshot) && snapshot.valid) {
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
		case EclVmMessage::ST_INGAME_MENU_VISIBLE:
			setInGameMenuVisible(msg.asUint8() != 0);
			return;
		case EclVmMessage::ST_ENTER_SHOP:
			enterShop(msg.asUint8());
			return;
		default:
			break;
		}
	}

	if (msg._kind == EclVmMessage::MK_SYSCALL) {
		switch (msg._tag) {
		case EclVmMessage::SC_MAP_DATA_READY:
			redraw();
			return;
		case EclVmMessage::SC_PRINT_ASYNC:
			if (_textBoxDialog) {
				if (!_textBoxDialog->isActive())
					_textBoxDialog->activate();
				_textBoxDialog->setText(msg._stringPayload,
					msg._result != 0);
				_textBoxWasBusy = true;
			}
			redraw();
			return;
		case EclVmMessage::SC_CLEAR_TEXTBOX:
			if (_textBoxDialog) {
				if (!_textBoxDialog->isActive())
					_textBoxDialog->activate();
				_textBoxDialog->clearText();
			}
			redraw();
			return;
		case EclVmMessage::SC_PRINT:
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

void InGameView::openDoor() {
	RuntimeGeoBlock &rtGeo = VmInterface::getRuntimeGeo();
	if (!rtGeo.isLoaded())
		return;

	const uint8 wireDir = _mapDir & 0x06;
	uint8 doorFlag = rtGeo.getWallFlag(
		(int)_mapX, (int)_mapY, wireDir);

	if (doorFlag < 2)
		return; // No locked door (0=wall, 1=open).

	// Hide in-game menu while door dialog is active.
	if (_inGameMenuDialog && _inGameMenuDialog->isActive())
		_inGameMenuDialog->deactivate();

	if (_doorDialog)
		_doorDialog->openDoor(doorFlag);
}

void InGameView::enterCamp() {
	// Hide normal in-game menu.
	if (_inGameMenuDialog && _inGameMenuDialog->isActive())
		_inGameMenuDialog->deactivate();

	// Activate camp dialog — it owns picture + menu + key handling.
	if (_campMenuDialog && !_campMenuDialog->isActive())
		_campMenuDialog->activate();

	redraw();
}

void InGameView::exitCamp(bool wasInterrupted) {
	if (_campMenuDialog && _campMenuDialog->isActive())
		_campMenuDialog->deactivate();

	// Restore previous dungeon state via engine.
	if (g_engine) {
		g_engine->setGameState(GS_DUNGEON_MAP);
		if (g_events) {
			g_events->postEclStateMessage(EclVmMessage::ST_INGAME_MENU_VISIBLE,
				1, EclVmMessage::VT_UINT8);
		}
	}
}

void InGameView::enterShop(uint8 shopType) {
	// Hide normal in-game menu.
	if (_inGameMenuDialog && _inGameMenuDialog->isActive())
		_inGameMenuDialog->deactivate();

	// Deactivate any previously active shop dialog.
	if (_activeShopDialog && _activeShopDialog->isActive())
		_activeShopDialog->deactivate();

	// Clear text box area (ECL prompt leftover from Y/N question).
	if (_textBoxDialog && _textBoxDialog->isActive()) {
		_textBoxDialog->clearText();
		_textBoxDialog->deactivate();
	}

	// Shop/temple/treasure show party panel but hide state area.
	_showPartyPanel = true;
	_showStateArea = false;
	syncDialogs();

	switch (shopType) {
	case Dialogs::SHOP_STORE:
		_activeShopDialog = _shopDialog;
		break;
	case Dialogs::SHOP_TEMPLE:
		_activeShopDialog = _templeDialog;
		break;
	case Dialogs::SHOP_TREASURE:
		_activeShopDialog = _treasureDialog;
		break;
	default:
		_activeShopDialog = _shopDialog;
		break;
	}

	if (_activeShopDialog)
		_activeShopDialog->activate();

	redraw();
}

void InGameView::exitShop() {
	if (_activeShopDialog && _activeShopDialog->isActive())
		_activeShopDialog->deactivate();
	_activeShopDialog = nullptr;

	// Signal the engine host that the shop interaction is complete.
	RuntimeExchange *exchange = g_engine
		? g_engine->getRuntimeExchange() : nullptr;
	if (exchange)
		exchange->signalAsync(RuntimeExchange::kAsyncShopDone);

	// Restore previous dungeon state via engine.
	if (g_engine) {
		g_engine->setGameState(GS_DUNGEON_MAP);
		if (g_events) {
			g_events->postEclStateMessage(EclVmMessage::ST_INGAME_MENU_VISIBLE,
				1, EclVmMessage::VT_UINT8);
		}
	}
}

bool InGameView::isShopActive() const {
	return _activeShopDialog && _activeShopDialog->isActive();
}

void InGameView::stepForward() {
	// Check wall passability before moving.
	// Door handling is done by the engine's tryOpenDoor() after ONMOVE,
	// NOT here. This only blocks solid walls (wallFlag == 0).
	RuntimeGeoBlock &rtGeo = VmInterface::getRuntimeGeo();
	if (rtGeo.isLoaded()) {
		const uint8 wireDir = _mapDir & 0x06;
		const uint8 wallFlag = rtGeo.getWallFlag(
			(int)_mapX, (int)_mapY, wireDir);
		if (wallFlag == 0)
			return; // Solid wall blocks movement.
	}

	// Compute new position using 8-direction deltas.
	int newX = (int)_mapX + kDirDeltaX[_mapDir];
	int newY = (int)_mapY + kDirDeltaY[_mapDir];

	// Clamp to map borders (0-15).
	if (newX > 15) newX = 15;
	if (newX < 0)  newX = 0;
	if (newY > 15) newY = 15;
	if (newY < 0)  newY = 0;

	_mapX = (uint16)newX;
	_mapY = (uint16)newY;
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
