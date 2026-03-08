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


#include "goldbox/poolrad/views/dialogs/items_menu.h"
#include "common/system.h"
#include "goldbox/events.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/party_selector.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Common::String;
using Common::KeyCode;

ItemsMenu::ItemsMenu(const String &name)
	: Dialog(name),
	  _character(nullptr),
	  _verticalMenu(nullptr),
	  _partySelector(nullptr),
	  _pendingTradeItem(nullptr) {

	_menuConfig.promptTxt = "";
	_menuConfig.promptOptions = &_horizontalMenuLabels;
	_menuConfig.menuItemList = &_itemsMenuList;
	_menuConfig.headColor = 0;
	_menuConfig.textColor = 10;
	_menuConfig.selectColor = 15;
	_menuConfig.xStart = 1;
	_menuConfig.yStart = 5;
	_menuConfig.xEnd = 38;
	_menuConfig.yEnd = 20;
	_menuConfig.title = "";
	_menuConfig.asHeader = true;

	_verticalMenu = new VerticalMenu("ItemsVerticalMenu", _menuConfig);
	subView(_verticalMenu);
	_verticalMenu->deactivate();
}

ItemsMenu::~ItemsMenu() {
	if (_verticalMenu) {
		_verticalMenu->setParent(nullptr);
		delete _verticalMenu;
		_verticalMenu = nullptr;
	}
	if (_removeConfirm) {
		_removeConfirm->setParent(nullptr);
		delete _removeConfirm;
		_removeConfirm = nullptr;
	}
	if (_activePrompt) {
		_activePrompt->setParent(nullptr);
		delete _activePrompt;
		_activePrompt = nullptr;
	}
	if (_partySelector) {
		_partySelector->setParent(nullptr);
		delete _partySelector;
		_partySelector = nullptr;
	}
}

void ItemsMenu::activate() {
	debug("ItemsMenu::activate() called");
	Dialog::activate();

	_character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
		VmInterface::getSelectedCharacter()
	);

	buildActionMenu();
	buildItemList();
	buildItemsListMenu();

	if (_verticalMenu) {
		// Regenerate horizontal menu items from updated action labels
		_verticalMenu->_hMenuList.items.clear();
		_verticalMenu->_hMenuList.generateMenuItems(_horizontalMenuLabels, true);

		_verticalMenu->rebuild(&_itemsMenuList, "");
		_verticalMenu->activate();
	}

	// Reset to main stage
	setStage(STAGE_ITEM_SELECTION);
}

void ItemsMenu::deactivate() {
	debug("ItemsMenu::deactivate() called");
	Dialog::deactivate();

	// Deactivate main vertical menu
	if (_verticalMenu) {
		_verticalMenu->deactivate();
	}

	// Return to main stage (cleans up subdialogs)
	setStage(STAGE_ITEM_SELECTION);
	
	// Clear pending items
	_pendingRemoveItem = nullptr;
	_pendingTradeItem = nullptr;
}

void ItemsMenu::draw() {

	if (!isActive()) {
		debug("ItemsMenu::draw() - NOT ACTIVE, skipping draw");
		return;
	}

	if (!_character) {
		debug("ItemsMenu::draw() - no character set");
		return;
	}

	Surface s = getSurface();

	drawWindow(1, 1, 38, 22);
	s.writeStringC(1, 1, 11, _character->name);
	s.writeString("'s");
	s.writeStringC(1 + _character->name.size() + 3, 1, 10, "Items");

	// Draw column headers at row 5 (one row above menu which starts at yStart=6)
	drawWindow(1, 3, 38, 22);
	s.writeStringC(1, 3, 15, "Ready Item");

	if (_verticalMenu)
		_verticalMenu->draw();

	// TODO: After item is selected, display HORIZONTAL action menu
	// This menu shows: Ready, Use, Drop, Halve, Join, Sell, Identify, Exit
	// Positioned below the window or as external popup
	// On action selection, route to appropriate handler (handleReadyItem, etc)
}

bool ItemsMenu::msgKeypress(const KeypressMessage &msg) {
	if (!isActive()) {
		return false;
	}

	debug("ItemsMenu::msgKeypress() keycode=%d ascii=%d", (int)msg.keycode,
		(int)msg.ascii);

	if (_removeConfirm && _removeConfirm->isActive()) {
		return View::msgKeypress(msg);
	}

	// Let child menus handle keypress and bubble valid actions through
	// handleMenuResult().
	return View::msgKeypress(msg);
}

void ItemsMenu::handleMenuResult(const MenuResultMessage &result) {
	debug("ItemsMenu::handleMenuResult() stage=%d success=%d key=%d hasInt=%d int=%d sel=%d",
		(int)_stage, (int)result._success, (int)result._keyCode, (int)result._hasIntValue,
		(int)(result._hasIntValue ? result._intValue : -1),
		(int)_itemsMenuList.currentSelection);

	// Handle stage-specific results
	switch (_stage) {
	case STAGE_CONFIRM_DROP:
		handleDropConfirmResult(result);
		return;
	case STAGE_SELECT_TRADE_TARGET:
		handleTradeSelectionResult(result);
		return;
	case STAGE_ITEM_SELECTION:
		// Fall through to main item action handling
		break;
	default:
		break;
	}

	bool success = result._success;
	Common::KeyCode key = result._keyCode;

	// NOTE: result._intValue is propagated by VerticalMenu as current ITEM index,
	// not horizontal ACTION index. Action must be resolved from key.
	debug("ItemsMenu::handleMenuResult() itemIndex(from int)=%d",
		(int)(result._hasIntValue ? result._intValue : -1));

	if (!success) {
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			debug("ItemsMenu::handleMenuResult() cancel/exit -> handleExit()");
			handleExit();
		}
		return;
	}

	Goldbox::Data::Items::CharacterItem *selectedItem = nullptr;
	if (_itemsMenuList.currentSelection >= 0 &&
		_itemsMenuList.currentSelection < (int)_itemList.size()) {
		selectedItem = _itemList[_itemsMenuList.currentSelection];
	}

	switch (key) {
	case Common::KEYCODE_r:
		debug("ItemsMenu::handleMenuResult() action=Ready");
		handleReadyItem(selectedItem);
		break;
	case Common::KEYCODE_u:
		debug("ItemsMenu::handleMenuResult() action=Use");
		handleUseItem(selectedItem);
		break;
	case Common::KEYCODE_t:
		debug("ItemsMenu::handleMenuResult() action=Trade");
		handleTradeItem(selectedItem);
		break;
	case Common::KEYCODE_d:
		debug("ItemsMenu::handleMenuResult() action=Drop");
		handleDropItem(selectedItem);
		break;
	case Common::KEYCODE_h:
		debug("ItemsMenu::handleMenuResult() action=Halve");
		handleHalveItem(selectedItem);
		break;
	case Common::KEYCODE_j:
		debug("ItemsMenu::handleMenuResult() action=Join");
		handleJoinItem(selectedItem);
		break;
	case Common::KEYCODE_s:
		debug("ItemsMenu::handleMenuResult() action=Sell");
		handleSellItem(selectedItem);
		break;
	case Common::KEYCODE_i:
		debug("ItemsMenu::handleMenuResult() action=Identify");
		handleIdentifyItem(selectedItem);
		break;
	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_e:
		debug("ItemsMenu::handleMenuResult() action=Exit");
		handleExit();
		break;
	default:
		debug("ItemsMenu::handleMenuResult() unhandled key=%d", (int)key);
		break;
	}
}

void ItemsMenu::buildActionMenu() {
	_horizontalMenuLabels.clear();

	if (!_character)
		return;
	_horizontalMenuLabels.push_back("Ready");

	const int gameStatus = Goldbox::VmInterface::getGameStatus();

	// TODO: Replace with real GEO memory check equivalent to
	// *(short *)(GB_PTR_MEM_GEO + 458) == 0 once available.
	const bool geoAllowsUse = true;

	// TODO: Combat readiness source is not wired yet. Replace this with
	// _character->combat_address[2] != 0 (or equivalent runtime flag).
	const bool combatAllowsUse = false;

	if (_character->enabled && geoAllowsUse &&
		((gameStatus == GS_CAMPING) ||
		 (gameStatus == GS_WILDERNESS_MAP) ||
		 (gameStatus == GS_DUNGEON_MAP) ||
		 ((gameStatus == GS_COMBAT) && combatAllowsUse))) {
		_horizontalMenuLabels.push_back("Use");
	}



	if ((_character->enabled || !_character->isNpc() || (_character->healthStatus == Goldbox::Data::S_ANIMATED)) &&
		gameStatus != GS_COMBAT) {
		_horizontalMenuLabels.push_back("Trade");
	}

	_horizontalMenuLabels.push_back("Drop");
	_horizontalMenuLabels.push_back("Halve");
	_horizontalMenuLabels.push_back("Join");

	if ((_character->enabled || !_character->isNpc() || (_character->healthStatus == Goldbox::Data::S_ANIMATED)) &&
		gameStatus == GS_SHOP) {
		_horizontalMenuLabels.push_back("Sell");
	}

	if (gameStatus == GS_SHOP) {
		_horizontalMenuLabels.push_back("Id");
	}

	_horizontalMenuLabels.push_back("Exit");

}


void ItemsMenu::buildItemList() {
	_itemList.clear();

	if (!_character || !_character->hasItems()) {
		debug("ItemsMenu::buildItemList - no character or no items");
		return;
	}

	// Build a list of pointers to character's items
	for (auto &item : _character->inventory.items()) {
		_itemList.push_back(&item);
	}
	debug("ItemsMenu::buildItemList - built list with %u items", (unsigned)_itemList.size());
}

void ItemsMenu::buildItemsListMenu() {
	const int previousSelection = _itemsMenuList.currentSelection;
	_itemsMenuList.items.clear();
	_itemsMenuList.currentSelection = 0;

	if (!_character || _itemList.empty()) {
		debug("ItemsMenu::buildItemsListMenu - no character or empty item list");
		return;
	}

	debug("ItemsMenu::buildItemsListMenu - building menu for %u items", (unsigned)_itemList.size());

	Common::Array<Common::String> itemLabels;
	for (auto &item : _itemList) {
		if (item) {
			// Use CharacterItem helper to keep list formatting logic centralized.
			// TODO: Replace false with party Identify effect state when implemented.
			itemLabels.push_back(item->getListDisplayText(true, false));
		} else {
			itemLabels.push_back("");
		}
	}

	// Inventory rows are display text, not command labels; keep text literal.
	_itemsMenuList.generateMenuItems(itemLabels, false);

	debug("ItemsMenu::buildItemsListMenu - generated %u menu items", (unsigned)_itemsMenuList.items.size());

	if (_itemsMenuList.items.empty()) {
		_itemsMenuList.currentSelection = 0;
	} else if (previousSelection < 0) {
		_itemsMenuList.currentSelection = 0;
	} else if (previousSelection >= (int)_itemsMenuList.items.size()) {
		_itemsMenuList.currentSelection = _itemsMenuList.items.size() - 1;
	} else {
		_itemsMenuList.currentSelection = previousSelection;
	}
}


void ItemsMenu::handleReadyItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	const Goldbox::Data::Items::CharacterItem *conflictingItem = nullptr;
	const Goldbox::Poolrad::Data::PoolradCharacter::ReadyItemResult result =
		_character->toggleReadyItem(item, &conflictingItem);

	if (result != Goldbox::Poolrad::Data::PoolradCharacter::RIR_SUCCESS) {
		if (result == Goldbox::Poolrad::Data::PoolradCharacter::RIR_CURSED) {
			displayMessage("It's Cursed");
		} else {
			displayEquipError(static_cast<uint8>(result),
						 conflictingItem ? conflictingItem : item);
		}
		return;
	}

	// Update menu display and recalculate equipment
	updateReadyItemDisplay(item);
	_character->resolveEquippedItems();
}

void ItemsMenu::handleUseItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	// Check if item is readied
	if (!isItemReadied(item)) {
		// TODO: Display message "Must be readied"
		return;
	}

	// TODO: Implement item use logic
	// Check if item is a ring or has proper effect flags
	// Call appropriate use handling


}

void ItemsMenu::handleTradeItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	if (item->readied != 0) {
		debug("ItemsMenu::handleTradeItem() blocked: readied item");
		displayMessage("Must be unreadied");
		return;
	}

	// Check if item is free to trade (not in combat, or char is player/disabled/animated)
	if (!canTradeItem(item)) {
		debug("ItemsMenu::handleTradeItem() blocked: canTradeItem=false");
		displayMessage("Cannot trade this item");
		return;
	}

	// Store pending item and switch to trade selection stage
	_pendingTradeItem = item;
	setStage(STAGE_SELECT_TRADE_TARGET);
}

void ItemsMenu::handleDropItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	// Check if item is free to drop
	if (!isFreeToRemove(item)) {
		return;
	}

	if (_character->removeItem(item)) {
		buildItemList();
		buildItemsListMenu();
		redraw();
	}
}

void ItemsMenu::handleHalveItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	if (!canHalveItem(item)) {
		return;
	}

	// TODO: Implement ITEM_Halve equivalent
	// Split the stackSize in half, creating a new item if successful
}

void ItemsMenu::handleJoinItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	// TODO: Implement ITEM_Join equivalent
	// Find matching items and combine them if possible
}

void ItemsMenu::handleSellItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	if (!canSellItem(item)) {
		return;
	}

	// TODO: Implement ITEM_Sell equivalent
	// Remove item and give character gold value of item
}

void ItemsMenu::handleIdentifyItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	// TODO: Implement ITEM_Identify equivalent
	// Reveal hidden item properties/effects
}


void ItemsMenu::handleExit() {
	debug("ItemsMenu::handleExit() parent=%s", _parent ? _parent->getName().c_str() : "<null>");
	deactivate();
	// Notify parent view that we're done
	if (_parent) {
		g_events->postMenuResult(_parent->getName(), false,
			Common::KEYCODE_ESCAPE, 0, Common::String(), true, false);
	}
}

bool ItemsMenu::isCharacterInCombat() const {
	return Goldbox::VmInterface::getGameStatus() == GS_COMBAT;
}

bool ItemsMenu::isCharacterAnimated() const {
	if (!_character) return false;
	return _character->healthStatus == Goldbox::Data::S_ANIMATED;
}

bool ItemsMenu::isItemReadied(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item) return false;
	return item->readied != 0;
}

bool ItemsMenu::canTradeItem(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item || !_character) return false;

	// Cannot trade in combat
	if (isCharacterInCombat()) {
		return false;
	}

	// Can trade if character is player (not NPC), disabled, or animated
	// NPC check: npc flag < 0x80 is player character, >= 0x80 is NPC
	return !_character->isNpc() || !_character->enabled || isCharacterAnimated();
}

bool ItemsMenu::isFreeToRemove(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return false;
	}

	if (item->readied != 0) {
		displayMessage("Must be unreadied");
		return false;
	}

	if (!item->isMissile()) {
		return true;
	}

/* Special effect threshold: 0x80 (128) is the magic number separating normal charges from special effects
Missile + special effects: The warning only appears for missiles with special effects (any effect >= 128),
 likely because these are scrolls that can be scribed */
	if (item->effect1 < 0x80 && item->effect2 < 0x80 && item->effect3 < 0x80) {
		return true;
	}

	if (_removeConfirm && _removeConfirm->isActive()) {
		return false;
	}

	// Store pending item and switch to confirmation stage
	_pendingRemoveItem = item;
	setStage(STAGE_CONFIRM_DROP);
	return false;
}

bool ItemsMenu::canHalveItem(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item) return false;
	// Can halve if item has more than 1 in stack and there's room in inventory
	return item->stackSize > 1 && _itemList.size() < 16;
}

bool ItemsMenu::canSellItem(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item || !_character) return false;

	// Can only sell in shop
	if (Goldbox::VmInterface::getGameStatus() != GS_SHOP) {
		return false;
	}

	// Can sell if character is player (not NPC), disabled, or animated
	// NPC check: npc flag < 0x80 is player character, >= 0x80 is NPC
	if (_character->isNpc() && _character->enabled && !isCharacterAnimated()) {
		return false;
	}

	// Cannot sell cursed items
	if (item->cursed) {
		return false;
	}

	return true;
}

void ItemsMenu::displayEquipError(uint8 errorCode, const Goldbox::Data::Items::CharacterItem *item) {
	if (!item) {
		return;
	}

	Common::String message;

	switch (errorCode) {
	case 1:
		message = "Wrong Class";
		break;
	case 2: {
		// "Already using [item name]"
		const Goldbox::Data::Items::ItemProperty &prop = item->prop();
		const Goldbox::Data::Items::Slot slot = static_cast<Goldbox::Data::Items::Slot>(prop.slotID);

		Goldbox::Data::Items::CharacterItem *equippedItem = nullptr;
		if (slot == Goldbox::Data::Items::Slot::S_RING1 ||
		    slot == Goldbox::Data::Items::Slot::S_RING2) {
			// Find which ring is equipped
			if (_character->equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING1]) {
				equippedItem = _character->equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING1];
			} else if (_character->equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING2]) {
				equippedItem = _character->equippedItems.slots[(int)Goldbox::Data::Items::Slot::S_RING2];
			}
		} else {
			equippedItem = _character->equippedItems.slots[(int)slot];
		}

		if (equippedItem) {
			message = Common::String::format("Already using %s", equippedItem->getDisplayName().c_str());
		} else {
			message = "Slot already in use";
		}
		break;
	}
	case 3:
		message = "Your hands are full!";
		break;
	default:
		message = "Cannot equip this item";
		break;
	}

	displayMessage(message);
}

void ItemsMenu::displayMessage(const Common::String &message) {
	// Clean up old prompt if any
	if (_activePrompt) {
		detachDialog(_activePrompt);
		delete _activePrompt;
		_activePrompt = nullptr;
	}

	// Create and display new prompt message
	PromptMessageConfig cfg;
	cfg.message = message;
	cfg.textColor = 10;      // Light green (standard message color)
	cfg.backgroundColor = 0; // Black

	_activePrompt = new PromptMessage("ItemsPromptMsg", cfg);
	attachDialog(_activePrompt);
	_activePrompt->activate();
}

void ItemsMenu::handleDropConfirmResult(const MenuResultMessage &result) {
	const bool confirmed = result._success &&
		(result._keyCode == Common::KEYCODE_y ||
		 (result._hasIntValue && result._intValue == 1));

	Goldbox::Data::Items::CharacterItem *itemToRemove = _pendingRemoveItem;
	_pendingRemoveItem = nullptr;

	// Return to item selection stage
	setStage(STAGE_ITEM_SELECTION);

	// Process the drop if confirmed
	if (confirmed && itemToRemove && _character &&
		_character->removeItem(itemToRemove)) {
		buildItemList();
		buildItemsListMenu();
		redraw();
	}
}

void ItemsMenu::handleTradeSelectionResult(const MenuResultMessage &result) {
	Goldbox::Data::Items::CharacterItem *itemToTrade = _pendingTradeItem;
	_pendingTradeItem = nullptr;

	// Return to item selection stage
	setStage(STAGE_ITEM_SELECTION);

	// Check if user cancelled or invalid result
	if (!result._success || !result._hasIntValue || !itemToTrade || !_character) {
		return;
	}

	// Get target character from party
	const int targetIndex = result._intValue;
	Common::Array<Goldbox::Data::PlayerCharacter *> *party = VmInterface::getParty();
	if (!party || targetIndex < 0 || targetIndex >= (int)party->size()) {
		return;
	}

	Goldbox::Poolrad::Data::PoolradCharacter *tradeTarget =
		static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[targetIndex]);

	if (!tradeTarget || tradeTarget == _character) {
		return;
	}

	// Attempt to transfer item
	if (!tradeTarget->receiveItem(*itemToTrade)) {
		displayMessage("Overloaded");
		return;
	}

	if (!_character->removeItem(itemToTrade)) {
		warning("Trade: item added to target but removal from source failed");
		return;
	}

	// Update character state and refresh UI
	_character->recalcCombatStats();
	buildItemList();
	buildItemsListMenu();
	redraw();
}

void ItemsMenu::setStage(ItemsMenuStage stage) {
	debug("ItemsMenu::setStage() - changing from %d to %d", (int)_stage, (int)stage);
	_stage = stage;

	switch (_stage) {
	case STAGE_ITEM_SELECTION: {
		debug("ItemsMenu::setStage() - ITEM_SELECTION stage");
		// Clean up any active subdialogs
		if (_removeConfirm) {
			detachDialog(_removeConfirm);
			_removeConfirm->clear();
			delete _removeConfirm;
			_removeConfirm = nullptr;
		}
		if (_partySelector) {
			detachDialog(_partySelector);
			delete _partySelector;
			_partySelector = nullptr;
		}
		// Main vertical menu should already be active from activate()
		redraw();
		break;
	}
	case STAGE_CONFIRM_DROP: {
		debug("ItemsMenu::setStage() - CONFIRM_DROP stage");
		if (_removeConfirm) {
			detachDialog(_removeConfirm);
			_removeConfirm->clear();
			delete _removeConfirm;
			_removeConfirm = nullptr;
		}
		// Create and show drop confirmation dialog
		if (_character && _pendingRemoveItem) {
			const Common::String prompt = Common::String::format(
				"%s was going to scribe from that scroll. Is it okay to lose it? ",
				_character->name.c_str());
			HorizontalYesNoConfig ynCfg = {prompt, 13, 10, 15, 0};
			_removeConfirm = new HorizontalYesNo("ItemsRemoveConfirm", ynCfg);
			attachDialog(_removeConfirm);
			_removeConfirm->activate();
			redraw();
		}
		break;
	}
	case STAGE_SELECT_TRADE_TARGET: {
		debug("ItemsMenu::setStage() - SELECT_TRADE_TARGET stage");
		if (_partySelector) {
			detachDialog(_partySelector);
			delete _partySelector;
			_partySelector = nullptr;
		}
		// Create and show party selector
		PartySelectorConfig config;
		config.promptText = "Trade with Whom?";
		config.allowExit = true;
		_partySelector = new PartySelector("ItemsTradeSelector", config);
		attachDialog(_partySelector);
		_partySelector->activate();
		redraw();
		break;
	}
	default:
		break;
	}
}

void ItemsMenu::updateReadyItemDisplay(Goldbox::Data::Items::CharacterItem *item) {
	if (!item) {
		return;
	}

	// Find the item's index in _itemList
	int itemIndex = -1;
	for (int i = 0; i < (int)_itemList.size(); ++i) {
		if (_itemList[i] == item) {
			itemIndex = i;
			break;
		}
	}

	if (itemIndex >= 0 && itemIndex < (int)_itemsMenuList.items.size()) {
		// Update display text (ready status + name formatting in CharacterItem)
		// TODO: Replace false with party Identify effect state when implemented.
		_itemsMenuList.items[itemIndex].text = item->getListDisplayText(true, false);

		// Redraw menu
		if (_verticalMenu) {
			_verticalMenu->redrawLine(itemIndex);
		}
	}
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
