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

ItemsMenu::ItemsMenu(const String &name)
	: Dialog(name),
	  _character(nullptr),
	  _verticalMenu(nullptr) {

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
	_verticalMenu->deactivateHorizontalMenu();
}

ItemsMenu::~ItemsMenu() {
	if (_verticalMenu) {
		delete _verticalMenu;
		_verticalMenu = nullptr;
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
		_verticalMenu->activate();
		_verticalMenu->activateHorizontalMenu();
	}
}

void ItemsMenu::deactivate() {
	debug("ItemsMenu::deactivate() called");
	Dialog::deactivate();

	if (_verticalMenu) {
		_verticalMenu->deactivateHorizontalMenu();
		_verticalMenu->deactivate();
	}
}

void ItemsMenu::draw() {
	// Only draw when active
	if (!isActive()) {
		debug("ItemsMenu::draw() - NOT ACTIVE, skipping draw");
		return;
	}

	debug("ItemsMenu::draw() - ACTIVE, drawing");

	// Ensure character is set
	if (!_character) {
		debug("ItemsMenu::draw() - no character set");
		return;
	}

	Surface s = getSurface();

	drawWindow(1, 1, 38, 22);
	s.writeStringC(1, 1, _character->getNameColor(), _character->name);
	s.writeString("'s");
	s.writeStringC(1 + _character->name.size() + 4, 1, 14, "Items");
	drawWindow(1, 3, 38, 22);
	s.writeStringC(1, 5, 15, "Ready Item");

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

	// Handle navigation within item list
	// HOME/END for vertical menu navigation
	if (msg.keycode == Common::KEYCODE_HOME) {
		if (_verticalMenu && !_itemList.empty()) {
			_itemsMenuList.currentSelection = 0;
			redraw();
		}
		return true;
	}
	if (msg.keycode == Common::KEYCODE_END) {
		if (_verticalMenu && !_itemList.empty()) {
			_itemsMenuList.currentSelection = _itemList.size() - 1;
			redraw();
		}
		return true;
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
	if (success) {
		if (key == Common::KEYCODE_ESCAPE) {
			handleExit();
			return;
		}
	}
	// TODO: Handle other action menu selections
}

void ItemsMenu::buildActionMenu() {
	_horizontalMenuLabels.clear();

	if (!_character)
		return;

	if ((_character->enabled || !_character->isNpc() || (_character->healthStatus == Goldbox::Data::S_ANIMATED)) &&
		Goldbox::VmInterface::getGameStatus() != GS_COMBAT) {
		_horizontalMenuLabels.push_back("Trade");
	}

	_horizontalMenuLabels.push_back("Drop");
	_horizontalMenuLabels.push_back("Halve");
	_horizontalMenuLabels.push_back("Join");

	if ((_character->enabled || !_character->isNpc() || (_character->healthStatus == Goldbox::Data::S_ANIMATED)) &&
		Goldbox::VmInterface::getGameStatus() == GS_SHOP) {
		_horizontalMenuLabels.push_back("Sell");
	}

	if (Goldbox::VmInterface::getGameStatus() == GS_SHOP) {
		_horizontalMenuLabels.push_back("Id");
	}

	_horizontalMenuLabels.push_back("Exit");
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
	const int previousSelection = _itemsMenuList.currentSelection;
	_itemsMenuList.items.clear();
	_itemsMenuList.currentSelection = 0;

	if (!_character || _itemList.empty()) {
		return;
	}

	Common::Array<Common::String> itemLabels;
	for (auto &item : _itemList) {
		Common::String displayText;
		if (item) {
			displayText = item->name;
		}

		while (displayText.size() < 33) {
			displayText += " ";
		}

		if (item && item->readied != 0) {
			displayText += "YES";
		} else {
			displayText += " NO";
		}

		itemLabels.push_back(displayText);
	}

	_itemsMenuList.generateMenuItems(itemLabels, true);

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

	// TODO: Implement equipping/readying logic
	// This should call character's equipItem method with appropriate slot
	// For now, just toggle the readied flag as a placeholder
	item->readied = item->readied ? 0 : 1;

	buildItemList();
	buildItemsListMenu();
	redraw();
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
	deactivate();
	// Notify parent view that we're done
	if (_parent) {
		_parent->handleMenuResult(false, Common::KEYCODE_ESCAPE, 0);
	}
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

bool ItemsMenu::canIdentifyItem(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item) return false;

	// Can only identify in shop
	if (Goldbox::VmInterface::getGameStatus() != GS_SHOP) {
		return false;
	} else
		return true;

	// Can identify unidentified items
	//return !item->identified;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
