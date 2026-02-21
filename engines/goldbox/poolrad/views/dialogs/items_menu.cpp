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
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Common::String;
using Common::KeyCode;

ItemsMenu::ItemsMenu(Goldbox::Poolrad::Data::PoolradCharacter *character, const String &name)
	: Dialog(name),
	  _character(character),
	  _verticalMenu(nullptr),
	  _continueAfterAction(false),
	  _selectedItemIndex(-1),
	  _showingActions(false) {

	buildActionMenu();
	buildItemList();
	buildItemsListMenu();

	VerticalMenuConfig menuConfig;
	menuConfig.promptTxt = "";
	menuConfig.promptOptions = &_actionMenuList;
	menuConfig.menuItemList = &_menuList;
	menuConfig.headColor = 0;
	menuConfig.textColor = 10;
	menuConfig.selectColor = 15;
	menuConfig.xStart = 1;
	menuConfig.yStart = 5;
	menuConfig.xEnd = 38;
	menuConfig.yEnd = 20;
	menuConfig.title = "ITEMS";
	menuConfig.asHeader = true;

	_verticalMenu = new VerticalMenu("ItemsVerticalMenu", menuConfig);
	subView(_verticalMenu);
	deactivate();
}

ItemsMenu::~ItemsMenu() {
	if (_verticalMenu) {
		delete _verticalMenu;
		_verticalMenu = nullptr;
	}
}

void ItemsMenu::draw() {
	Surface s = getSurface();

	if (!_character) {
		return;
	}

	drawWindow(1, 1, 38, 22);
	s.writeStringC(1, 1, _character->getNameColor(), _character->name);
	s.writeString("'s");
	s.writeStringC(1 + _character->name.size() + 4, 1, 14, "Items");
	drawWindow(1, 3, 38, 22);
	s.writeStringC(1, 5, 15, "Ready Item");

	// Rebuild the menu with updated items from character's inventory
	if (_verticalMenu) {
		_verticalMenu->rebuild(&_menuList, "ITEMS");
		_verticalMenu->draw();
	}

	// TODO: After item is selected, display HORIZONTAL action menu
	// This menu shows: Ready, Use, Drop, Halve, Join, Sell, Identify, Exit
	// Positioned below the window or as external popup
	// On action selection, route to appropriate handler (handleReadyItem, etc)
}

bool ItemsMenu::msgKeypress(const KeypressMessage &msg) {
	if (!isActive()) {
		return false;
	}

	// Handle navigation within item list
	if (!_showingActions) {
		// HOME/END for vertical menu navigation
		if (msg.keycode == Common::KEYCODE_HOME) {
			if (_verticalMenu && !_itemList.empty()) {
				_menuList.currentSelection = 0;
				_verticalMenu->rebuild(&_menuList, "ITEMS");
			}
			return true;
		}
		if (msg.keycode == Common::KEYCODE_END) {
			if (_verticalMenu && !_itemList.empty()) {
				_menuList.currentSelection = _itemList.size() - 1;
				_verticalMenu->rebuild(&_menuList, "ITEMS");
			}
			return true;
		}
	}

	// Global exit
	if (msg.keycode == Common::KEYCODE_ESCAPE) {
		handleExit();
		return true;
	}

	// Let the View base class handle keypress delegate to active menus
	return View::msgKeypress(msg);
}

void ItemsMenu::handleMenuResult(bool success, Common::KeyCode key, short value) {
	if (!success) {
		// Exit action menu and return to item list
		if (_showingActions) {
			_showingActions = false;
			_selectedItemIndex = -1;
		} else {
			handleExit();
		}
		return;
	}

	if (!_showingActions) {
		// Item selected - show action menu
		if (value < 0 || value >= (int)_itemList.size()) {
			return;
		}

		_selectedItemIndex = value;
		Goldbox::Data::Items::CharacterItem *selectedItem = _itemList[value];
		if (!selectedItem) {
			return;
		}

		// Build and show action menu based on character state (already built, just show)
		_showingActions = true;

	} else {
		// Action selected - execute it
		if (_selectedItemIndex < 0 || _selectedItemIndex >= (int)_itemList.size()) {
			_showingActions = false;
			_selectedItemIndex = -1;
			return;
		}

		Goldbox::Data::Items::CharacterItem *selectedItem = _itemList[_selectedItemIndex];
		if (!selectedItem) {
			_showingActions = false;
			_selectedItemIndex = -1;
			return;
		}

		// Execute action based on keyCode
		switch (key) {
		case Common::KEYCODE_r:
			handleReadyItem(selectedItem);
			break;
		case Common::KEYCODE_u:
			handleUseItem(selectedItem);
			break;
		case Common::KEYCODE_t:
			handleTradeItem(selectedItem);
			break;
		case Common::KEYCODE_d:
			handleDropItem(selectedItem);
			break;
		case Common::KEYCODE_h:
			handleHalveItem(selectedItem);
			break;
		case Common::KEYCODE_j:
			handleJoinItem(selectedItem);
			break;
		case Common::KEYCODE_s:
			handleSellItem(selectedItem);
			break;
		case Common::KEYCODE_i:
			handleIdentifyItem(selectedItem);
			break;
		case Common::KEYCODE_e:
		case Common::KEYCODE_ESCAPE:
			// Just close action menu, return to item list
			break;
		default:
			break;
		}

		// Return to item list
		_showingActions = false;
		_selectedItemIndex = -1;
	}
}

void ItemsMenu::buildActionMenu() {
	// Build the horizontal action menu based on character state and game state
	_actionMenuList.clear();

	if (!_character) {
		return;
	}

	// Ready - always available
	_actionMenuList.push_back("Ready");

	// Use - if character enabled + no geo restriction + valid game state
	if (_character->enabled) {
		// TODO: Check geo restriction at GB_PTR_MEM_GEO + 458
		// For now, assume geo check passes
		bool geoCheckPassed = true;
		if (geoCheckPassed) {
			_actionMenuList.push_back("Use");
		}
	}

	// Trade - if character is player (not NPC) OR disabled OR animated + not in combat
	if ((_character->enabled || !_character->isNpc() || (_character->healthStatus == Goldbox::Data::S_ANIMATED)) &&
		Goldbox::VmInterface::getGameStatus() != GS_COMBAT) {
		_actionMenuList.push_back("Trade");
	}

	// Drop - always available
	_actionMenuList.push_back("Drop");

	// Halve - always available (inventory check done in handler)
	_actionMenuList.push_back("Halve");

	// Join - always available
	_actionMenuList.push_back("Join");

	// Sell - if character is player (not NPC) OR disabled OR animated + in shop
	if ((_character->enabled || !_character->isNpc() || (_character->healthStatus == Goldbox::Data::S_ANIMATED)) &&
		Goldbox::VmInterface::getGameStatus() == GS_SHOP) {
		_actionMenuList.push_back("Sell");
	}

	// Identify - if in shop
	if (Goldbox::VmInterface::getGameStatus() == GS_SHOP) {
		_actionMenuList.push_back("Id");
	}

	// Add Exit option
	_actionMenuList.push_back("Exit");
}

void ItemsMenu::buildItemList() {
	_itemList.clear();

	if (!_character || !_character->hasItems()) {
		return;
	}

	// Build a list of pointers to character's items
	for (auto &item : _character->inventory.items()) {
		_itemList.push_back(&item);
	}
}

void ItemsMenu::buildItemsListMenu() {
	const int previousSelection = _menuList.currentSelection;
	_menuList.items.clear();
	_menuList.currentSelection = 0;

	if (!_character || _itemList.empty()) {
		return;
	}

	// Build a simple list of display strings
	// Index correlation: _menuList.items[n] corresponds to _itemList[n]
	Common::Array<Common::String> itemLabels;
	for (auto &item : _itemList) {
		// Format: "Item Name" + readied status column
		// Example: "Longsword                        YES"
		// or:      "Shield                           NO"
		Common::String displayText;
		if (item) {
			displayText = item->name;
		}

		// Pad item name to 33 characters
		while (displayText.size() < 33) {
			displayText += " ";
		}

		// Add readied status (YES/NO)
		if (item && item->readied != 0) {
			displayText += "YES";
		} else {
			displayText += " NO";
		}

		itemLabels.push_back(displayText);
	}

	// Generate menu items from the label list
	_menuList.generateMenuItems(itemLabels, true);

	// Preserve cursor position when possible after rebuild.
	if (_menuList.items.empty()) {
		_menuList.currentSelection = 0;
	} else if (previousSelection < 0) {
		_menuList.currentSelection = 0;
	} else if (previousSelection >= (int)_menuList.items.size()) {
		_menuList.currentSelection = _menuList.items.size() - 1;
	} else {
		_menuList.currentSelection = previousSelection;
	}

}


void ItemsMenu::handleReadyItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	// TODO: Implement equipping/readying logic
	// This should call character's equipItem method with appropriate slot
	// For now, just toggle the readied flag as a placeholder
	item->readied = item->readied ? 0 : 1;

	// Keep menu text in sync with updated inventory item state.
	buildItemList();
	buildItemsListMenu();
	if (_verticalMenu) {
		_verticalMenu->rebuild(&_menuList, "ITEMS");
	}
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

	// If not in combat, clear continue action
	if (!isCharacterInCombat()) {
		_continueAfterAction = false;
	}
}

void ItemsMenu::handleTradeItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	// Check if item is free to trade
	if (!canTradeItem(item)) {
		// TODO: Display message about item being restricted
		return;
	}

	// TODO: Implement DIALOG_tradeItem equivalent
}

void ItemsMenu::handleDropItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character) {
		return;
	}

	// Check if item is free to drop
	if (!canDropItem(item)) {
		// TODO: Display message about item being restricted
		return;
	}

	// TODO: Implement drop confirmation dialog
	// Build confirmation message with item name
	// If confirmed, remove item from character's inventory
	if (_character->removeItem(item)) {
		buildItemList();
		buildItemsListMenu();
		if (_verticalMenu) {
			_verticalMenu->rebuild(&_menuList, "ITEMS");
		}
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
	deactivate();
}

bool ItemsMenu::isCharacterInCombat() const {
	return Goldbox::VmInterface::getGameStatus() == GS_COMBAT;
}

bool ItemsMenu::isCharacterNPC() const {
	if (!_character) return false;
	// Use the isNpc() method from ADnDCharacter base class
	// npc flag: < 0x80 player character, >= 0x80 NPC
	return _character->isNpc();
}

bool ItemsMenu::isCharacterAnimated() const {
	if (!_character) return false;
	return _character->healthStatus == Goldbox::Data::S_ANIMATED;
}

bool ItemsMenu::isItemRing(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item) {
		return false;
	}

	// Check if item is a ring based on its slot property
	const Goldbox::Data::Items::ItemProperty &prop = item->prop();
	const Goldbox::Data::Items::Slot itemSlot =
		static_cast<Goldbox::Data::Items::Slot>(prop.slotID);

	return itemSlot == Goldbox::Data::Items::Slot::S_RING1 ||
		   itemSlot == Goldbox::Data::Items::Slot::S_RING2;
}

bool ItemsMenu::isItemReadied(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item) return false;
	return item->readied != 0;
}

bool ItemsMenu::canUseItem(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item || !_character) return false;

	// Character must be enabled
	if (!_character->enabled) {
		return false;
	}

	// TODO: Check geo restriction at GB_PTR_MEM_GEO + 458
	// if (*(int16 *)(GB_PTR_MEM_GEO + 458) != 0) return false;
	// For now, assume geo check passes
	bool geoCheckPassed = true;
	if (!geoCheckPassed) {
		return false;
	}

	// Valid game states: camping, wilderness, dungeon, or combat
	int gameStatus = Goldbox::VmInterface::getGameStatus();
	if (gameStatus == GS_CAMPING || gameStatus == GS_WILDERNESS_MAP ||
		gameStatus == GS_DUNGEON_MAP) {
		return true;
	}

	// TODO: In combat, verify character has combat_address set with combat flag
	// Combat handling not yet fully implemented
	// if (gameStatus == GS_COMBAT && _character->combat_address != nullptr &&
	//     _character->combat_address[2] != 0) {
	//     return true;
	// }

	if (gameStatus == GS_COMBAT) {
		// Default false for now until combat handling is implemented
		return false;
	}

	return false;
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

bool ItemsMenu::canDropItem(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item) return false;
	// Cannot drop cursed items
	if (item->cursed) {
		return false;
	}
	// All non-cursed items can be dropped
	return true;
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

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
