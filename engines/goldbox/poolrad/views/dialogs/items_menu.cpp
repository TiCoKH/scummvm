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
#include "goldbox/data/items/character_inventory.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/party_selector.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/vm_interface.h"
#include "goldbox/spells/spell_casting.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Common::String;
using Common::KeyCode;

ItemsMenu::ItemsMenu(const String &name)
	: Dialog(name),
	  _character(nullptr),
	  _resultFlag(nullptr),
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
	_menuConfig.addExit = false;

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
	if (_scrollSpellMenu) {
		_scrollSpellMenu->setParent(nullptr);
		delete _scrollSpellMenu;
		_scrollSpellMenu = nullptr;
	}
}

void ItemsMenu::activate() {
	debug(7, "ItemsMenu::activate() called");
	Dialog::activate();

	_character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
		VmInterface::getSelectedCharacter()
	);

	buildActionMenu();
	buildItemList();
	buildItemsListMenu();

	if (_verticalMenu) {
		// Regenerate horizontal menu items from updated action labels
		_verticalMenu->setPromptOptions(&_horizontalMenuLabels);

		_verticalMenu->rebuild(&_itemsMenuList, "");
		_verticalMenu->activate();
	}

	// Reset to main stage
	setStage(STAGE_ITEM_SELECTION);

	// Exit immediately if character has no items (mirrors original early return)
	if (!_character || !_character->hasItems()) {
		handleExit();
	}
}

void ItemsMenu::deactivate() {
	debug(7, "ItemsMenu::deactivate() called");
	Dialog::deactivate();

	// Deactivate main vertical menu
	if (_verticalMenu) {
		_verticalMenu->deactivate();
	}

	// Return to main stage (cleans up subdialogs)
	setStage(STAGE_ITEM_SELECTION);

	// Clear pending items and result flag
	_pendingRemoveItem = nullptr;
	_pendingTradeItem = nullptr;
	_pendingUseItem = nullptr;
	_pendingSellItem = nullptr;
	_pendingSellValue = 0;
	_pendingIdentifyItem = nullptr;
	_resultFlag = nullptr;

	if (_scrollSpellMenu) {
		detachDialog(_scrollSpellMenu);
		delete _scrollSpellMenu;
		_scrollSpellMenu = nullptr;
	}
}

void ItemsMenu::draw() {

	if (!isActive()) {
		debug(7, "ItemsMenu::draw() - NOT ACTIVE, skipping draw");
		return;
	}

	if (!_character) {
		debug(7, "ItemsMenu::draw() - no character set");
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

}

bool ItemsMenu::msgKeypress(const KeypressMessage &msg) {
	if (!isActive()) {
		return false;
	}

	debug(7, "ItemsMenu::msgKeypress() keycode=%d ascii=%d", (int)msg.keycode,
		(int)msg.ascii);

	if (_removeConfirm && _removeConfirm->isActive()) {
		return View::msgKeypress(msg);
	}

	// Let child menus handle keypress and bubble valid actions through
	// handleMenuResult().
	return View::msgKeypress(msg);
}

void ItemsMenu::handleMenuResult(const MenuResultMessage &result) {
	debug(7, "ItemsMenu::handleMenuResult() stage=%d success=%d key=%d hasInt=%d int=%d sel=%d",
		(int)_stage, (int)result._success, (int)result._keyCode, (int)result._hasIntValue,
		(int)(result._hasIntValue ? result._intValue : -1),
		(int)_itemsMenuList.currentSelection);

	// Handle stage-specific results
	switch (_stage) {
	case STAGE_CONFIRM_DROP:
		handleDropConfirmResult(result);
		return;
	case STAGE_CONFIRM_SELL:
		handleSellConfirmResult(result);
		return;
	case STAGE_SELECT_TRADE_TARGET:
		handleTradeSelectionResult(result);
		return;
	case STAGE_SELECT_SCROLL_SPELL:
		handleScrollSpellResult(result);
		return;
	case STAGE_CONFIRM_IDENTIFY:
		handleIdentifyConfirmResult(result);
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
	debug(7, "ItemsMenu::handleMenuResult() itemIndex(from int)=%d",
		(int)(result._hasIntValue ? result._intValue : -1));

	if (!success) {
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			debug(7, "ItemsMenu::handleMenuResult() cancel/exit -> handleExit()");
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
		debug(7, "ItemsMenu::handleMenuResult() action=Ready");
		handleReadyItem(selectedItem);
		break;
	case Common::KEYCODE_u:
		debug(7, "ItemsMenu::handleMenuResult() action=Use");
		handleUseItem(selectedItem);
		break;
	case Common::KEYCODE_t:
		debug(7, "ItemsMenu::handleMenuResult() action=Trade");
		handleTradeItem(selectedItem);
		break;
	case Common::KEYCODE_d:
		debug(7, "ItemsMenu::handleMenuResult() action=Drop");
		handleDropItem(selectedItem);
		break;
	case Common::KEYCODE_h:
		debug(7, "ItemsMenu::handleMenuResult() action=Halve");
		handleHalveItem(selectedItem);
		break;
	case Common::KEYCODE_j:
		debug(7, "ItemsMenu::handleMenuResult() action=Join");
		handleJoinItem(selectedItem);
		break;
	case Common::KEYCODE_s:
		debug(7, "ItemsMenu::handleMenuResult() action=Sell");
		handleSellItem(selectedItem);
		break;
	case Common::KEYCODE_i:
		debug(7, "ItemsMenu::handleMenuResult() action=Identify");
		handleIdentifyItem(selectedItem);
		break;
	case Common::KEYCODE_ESCAPE:
	case Common::KEYCODE_e:
		debug(7, "ItemsMenu::handleMenuResult() action=Exit");
		handleExit();
		return; // skip recalc on exit
	default:
		debug(7, "ItemsMenu::handleMenuResult() unhandled key=%d", (int)key);
		return;
	}

	// Recalculate combat stats after every action (mirrors original loop end).
	if (_character)
		_character->recalcCombatStats();
}

void ItemsMenu::buildActionMenu() {
	_horizontalMenuLabels.clear();

	if (!_character)
		return;
	_horizontalMenuLabels.push_back("Ready");

	const int gameStatus = Goldbox::VmInterface::getGameStatus();

	// G_NoMagicFlag: *(short *)(GB_PTR_MEM_GEO + 458) == 0.
	// No RuntimeGeoBlock field maps to this yet; treat as 0 (magic allowed).
	// TODO: wire when the GEO offset is exposed.
	const bool noMagicFlag = false;

	// can_use comes from the character's per-turn CombatAction state.
	const bool combatAllowsUse = (_character->combatState != nullptr) &&
		_character->combatState->canUse;

	if (_character->enabled && !noMagicFlag &&
		((gameStatus == GS_CAMPING) ||
		 (gameStatus == GS_WILDERNESS_MAP) ||
		 (gameStatus == GS_DUNGEON_MAP) ||
		 ((gameStatus == GS_COMBAT) && combatAllowsUse))) {
		_horizontalMenuLabels.push_back("Use");
	}

	// Trade/Sell eligibility: npc < 0x80 (player) OR disabled OR animated.
	// Preserves the original numeric NPC condition: bit 7 of npc byte.
	const bool tradeEligible =
		(!_character->isNpc() || !_character->enabled ||
		 (_character->healthStatus == Goldbox::Data::S_ANIMATED));

	if (tradeEligible && gameStatus != GS_COMBAT)
		_horizontalMenuLabels.push_back("Trade");

	_horizontalMenuLabels.push_back("Drop");

	// Halve only when inventory has room for the split result (item_num < 16).
	if ((int)_itemList.size() < 16)
		_horizontalMenuLabels.push_back("Halve");

	_horizontalMenuLabels.push_back("Join");

	if (tradeEligible && gameStatus == GS_SHOP)
		_horizontalMenuLabels.push_back("Sell");

	if (gameStatus == GS_SHOP)
		_horizontalMenuLabels.push_back("Id");

	_horizontalMenuLabels.push_back("Exit");
}


void ItemsMenu::buildItemList() {
	_itemList.clear();

	if (!_character || !_character->hasItems()) {
		debug(7, "ItemsMenu::buildItemList - no character or no items");
		return;
	}

	// Build a list of pointers to character's items
	for (auto &item : _character->inventory.items()) {
		_itemList.push_back(&item);
	}
	debug(7, "ItemsMenu::buildItemList - built list with %u items", (unsigned)_itemList.size());
}

void ItemsMenu::buildItemsListMenu() {
	const int previousSelection = _itemsMenuList.currentSelection;
	_itemsMenuList.items.clear();
	_itemsMenuList.currentSelection = 0;

	if (!_character || _itemList.empty()) {
		debug(7, "ItemsMenu::buildItemsListMenu - no character or empty item list");
		return;
	}

	debug(7, "ItemsMenu::buildItemsListMenu - building menu for %u items", (unsigned)_itemList.size());

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

	debug(7, "ItemsMenu::buildItemsListMenu - generated %u menu items", (unsigned)_itemsMenuList.items.size());

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
	if (!item || !_character)
		return;

	const Goldbox::Data::Items::CharacterItem *conflictingItem = nullptr;
	const Goldbox::Poolrad::Data::PoolradCharacter::ReadyItemResult result =
		_character->toggleReadyItem(item, &conflictingItem);

	if (result == Goldbox::Poolrad::Data::PoolradCharacter::RIR_SUCCESS) {
		updateReadyItemDisplay(item);
		_character->resolveEquippedItems();
		return;
	}

	// Hands-full message is suppressed for AI-controlled characters in combat.
	// Original: if (BYTE_GAME_STATE != GS_COMBAT || character->ai_control == 0)
	if (result == Goldbox::Poolrad::Data::PoolradCharacter::RIR_HANDS_FULL) {
		if (!isCharacterInCombat() || !_character->ai_control)
			displayMessage("Your hands are full!");
		return;
	}

	if (result == Goldbox::Poolrad::Data::PoolradCharacter::RIR_CURSED) {
		displayMessage("It's Cursed");
		return;
	}

	displayEquipError(static_cast<uint8>(result),
		conflictingItem ? conflictingItem : item);
}

void ItemsMenu::handleUseItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return;

	if (!isItemReadied(item)) {
		displayMessage("Must be Readied");
		return;
	}

	if (item->isMissileOrScroll()) {
		_pendingUseItem = item;
		setStage(STAGE_SELECT_SCROLL_SPELL);
		return;
	}

	// Other item: only usable when effects[1] != 0 && effects[2] < 0x80.
	if (item->effect2 == 0 || item->effect3 >= 0x80)
		return;

	const uint8 spellId = item->getMagicItemSpellId();
	if (spellId == 0)
		return;

	executeItemUse(item, spellId);

	// Outside combat the result flag is always cleared (loop continues).
	// In combat, executeItemUse may have set it via the spell system.
	if (isCharacterInCombat()) {
		if (_resultFlag && *_resultFlag)
			handleExit();
	} else {
		if (_resultFlag)
			*_resultFlag = false;
	}
}

void ItemsMenu::handleTradeItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return;

	if (!isFreeToRemove(item))
		return;

	if (!hasTradeTarget())
		return;

	_pendingTradeItem = item;
	setStage(STAGE_SELECT_TRADE_TARGET);

	// Redraw is always requested after trade attempt, even on failure (spec).
	redraw();
}

void ItemsMenu::handleDropItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return;

	if (!isFreeToRemove(item)) {
		// If it's a missile/scroll with stored effect data, ask for confirmation.
		if (item->readied == 0 && item->isMissileOrScroll() &&
			(item->effect1 >= 0x80 || item->effect2 >= 0x80 || item->effect3 >= 0x80)) {
			_pendingRemoveItem = item;
			setStage(STAGE_CONFIRM_DROP);
		}
		return;
	}

	// Freely removable item: still confirm before dropping.
	_pendingRemoveItem = item;
	setStage(STAGE_CONFIRM_DROP);
}

void ItemsMenu::handleHalveItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return;

	if (!_character->inventory.halveItem(item)) {
		displayMessage("Can't halve that");
		return;
	}

	buildItemList();
	buildItemsListMenu();
	redraw();
}

void ItemsMenu::handleJoinItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return;

	_character->inventory.joinItem(item);

	buildItemList();
	buildItemsListMenu();
	redraw();
}

void ItemsMenu::handleSellItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return;

	if (!isFreeToRemove(item))
		return;

	// Calculate sale value: half the item's listed value.
	// For stacks > 1: (stack * halfValue) / 20.
	uint16 saleValue = 0;
	if (item->value != 0)
		saleValue = item->value >> 1;
	if (item->stackSize > 1)
		saleValue = (uint16)((item->stackSize * saleValue) / 20);

	_pendingSellItem = item;
	_pendingSellValue = saleValue;
	setStage(STAGE_CONFIRM_SELL);
}

void ItemsMenu::handleIdentifyItem(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return;

	_pendingIdentifyItem = item;
	setStage(STAGE_CONFIRM_IDENTIFY);
}


void ItemsMenu::handleExit() {
	debug(7, "ItemsMenu::handleExit() parent=%s", _parent ? _parent->getName().c_str() : "<null>");
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

bool ItemsMenu::isFreeToRemove(Goldbox::Data::Items::CharacterItem *item) {
	if (!item || !_character)
		return false;

	if (item->readied != 0) {
		displayMessage("Must be unreadied");
		return false;
	}

	if (!item->isMissileOrScroll())
		return true;

	// Missile/scroll-slot item: any effect byte >= 0x80 means stored spell data
	// that may be lost — require confirmation before allowing removal.
	if (item->effect1 < 0x80 && item->effect2 < 0x80 && item->effect3 < 0x80)
		return true;

	return false;
}

bool ItemsMenu::canSellItem(const Goldbox::Data::Items::CharacterItem *item) const {
	if (!item || !_character) return false;
	if (Goldbox::VmInterface::getGameStatus() != GS_SHOP) return false;
	if (item->cursed) return false;
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
		// Hands-full is handled directly in handleReadyItem with combat/AI suppression.
		// This case should not be reached.
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

void ItemsMenu::executeItemUse(Goldbox::Data::Items::CharacterItem *item,
		uint8 spellId) {
	if (!item || !_character || spellId == 0)
		return;

	if (!Goldbox::Spells::g_spellCasting) {
		debug(3, "ItemsMenu::executeItemUse: no SpellCastingService");
		return;
	}

	Goldbox::Spells::SpellContext ctx;
	ctx.caster = _character;
	ctx.inCombat = isCharacterInCombat();
	ctx.fromMagicItem = true;

	const Goldbox::Data::Spells::Spells spellEnum =
		static_cast<Goldbox::Data::Spells::Spells>(spellId);

	const Goldbox::Spells::SpellCastResult castResult =
		Goldbox::Spells::g_spellCasting->castSpell(ctx, spellEnum);

	if (castResult.status != Goldbox::Spells::CAST_OK)
		return;

	// ACTION_UseSpell succeeded: set useResult true.
	if (_resultFlag)
		*_resultFlag = true;

	// COMBAT_endActorTurn: if in combat and spell has a casting time (c_avail),
	// end the actor's turn. Result remains true (turn consumed).
	if (isCharacterInCombat()) {
		const Goldbox::Spells::SpellDefinition *def =
			Goldbox::Spells::g_spellCasting->registry().getDefinition(spellEnum);
		if (def && def->entry && def->entry->castTime != 0) {
			if (_character->combatState)
				_character->combatState->endTurn();
			// useResult stays true: turn was consumed.
		}
	}

	// Consume the item only when useResult is true (already guaranteed here).
	if (item->isMissileOrScroll()) {
		_character->inventory.removeEffectTag(*item, spellId,
			&_character->equippedItems.slots);
	} else {
		_character->inventory.consumeItemUse(*item,
			&_character->equippedItems.slots);
	}

	buildItemList();
	buildItemsListMenu();
	redraw();
}

void ItemsMenu::handleIdentifyConfirmResult(const MenuResultMessage &result) {
	const bool confirmed = result._success &&
		(result._keyCode == Common::KEYCODE_y ||
		 (result._hasIntValue && result._intValue == 1));

	Goldbox::Data::Items::CharacterItem *item = _pendingIdentifyItem;
	_pendingIdentifyItem = nullptr;

	setStage(STAGE_ITEM_SELECTION);

	if (!confirmed || !item || !_character)
		return;

	// CHARACTER_sumMoneyValue: character pays first.
	const uint32 charGold = _character->valuableItems.getGoldValue();
	bool isPaid = false;

	if (charGold >= 200) {
		_character->valuableItems.setFromGoldValue(charGold - 200);
		isPaid = true;
	} else {
		// COINPOOL_SumGoldValue: fall back to party pool.
		Goldbox::Data::ValuableItems &pool =
			VmInterface::getTreasurePool().coins();
		const uint32 poolGold = pool.getGoldValue();
		if (poolGold >= 200) {
			pool.setFromGoldValue(poolGold - 200);
			isPaid = true;
		} else {
			displayMessage("Not enough money.");
		}
	}

	if (!isPaid)
		return;

	if (item->hidden == 0) {
		displayMessage(Common::String::format("%s: nothing new.",
			item->getDisplayName().c_str()));
	} else {
		item->hidden = 0;
		displayMessage(item->getListDisplayText(false, true));
		if (_resultFlag)
			*_resultFlag = true;
	}

	buildItemList();
	buildItemsListMenu();
	redraw();
}

void ItemsMenu::handleScrollSpellResult(const MenuResultMessage &result) {
	Goldbox::Data::Items::CharacterItem *item = _pendingUseItem;
	_pendingUseItem = nullptr;

	setStage(STAGE_ITEM_SELECTION);

	if (!result._success || !item)
		return;

	// result._intValue carries the legacy spell index (1-based).
	if (!result._hasIntValue || result._intValue < 0)
		return;

	const uint8 spellId = static_cast<uint8>(result._intValue + 1);
	executeItemUse(item, spellId);

	// Apply same resultFlag semantics as the non-scroll Use path.
	if (isCharacterInCombat()) {
		if (_resultFlag && *_resultFlag)
			handleExit();
	} else {
		if (_resultFlag)
			*_resultFlag = false;
	}

	if (_character)
		_character->recalcCombatStats();
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

void ItemsMenu::handleSellConfirmResult(const MenuResultMessage &result) {
	const bool confirmed = result._success &&
		(result._keyCode == Common::KEYCODE_y ||
		 (result._hasIntValue && result._intValue == 1));

	Goldbox::Data::Items::CharacterItem *item = _pendingSellItem;
	_pendingSellItem = nullptr;
	const uint16 saleValue = _pendingSellValue;
	_pendingSellValue = 0;

	setStage(STAGE_ITEM_SELECTION);

	if (!confirmed || !item || !_character)
		return;

	// Remove the item first.
	if (!_character->removeItem(item))
		return;

	// Convert sale value: platinum = saleValue / 5, gold = saleValue % 5.
	const uint16 platAmount = saleValue / 5;
	const uint16 goldAmount = saleValue % 5;

	// Overload check: can the character carry platAmount + goldAmount coin weight?
	// Uses the same 1500 + capacityModifier encumbrance limit as receiveItemLegacy.
	const int capacity = 1500 + _character->getCapacityModifier();
	const int currentEnc = (int)_character->encumbrance;
	const int coinWeight = (int)(platAmount + goldAmount);
	const bool willOverload = (currentEnc + coinWeight) > capacity;

	if (willOverload) {
		// Credit only as much platinum as fits; overflow goes to party pool.
		const int freeCapacity = capacity - currentEnc;
		const uint16 platCapacity = (freeCapacity > 0) ?
			(uint16)MIN<int>(freeCapacity, platAmount) : 0;

		if (platAmount < platCapacity) {
			_character->valuableItems[Goldbox::Data::VAL_PLATINUM] += platAmount;
		} else {
			_character->valuableItems[Goldbox::Data::VAL_PLATINUM] += platCapacity;
			Goldbox::VmInterface::getTreasurePool().coins()[Goldbox::Data::VAL_PLATINUM] +=
				platAmount - platCapacity;
		}
		displayMessage("Overloaded!");
	} else {
		_character->valuableItems[Goldbox::Data::VAL_PLATINUM] += platAmount;
	}
	_character->valuableItems[Goldbox::Data::VAL_GOLD] += goldAmount;

	_character->recalcCombatStats();
	buildItemList();
	buildItemsListMenu();
	redraw();
}

void ItemsMenu::handleTradeSelectionResult(const MenuResultMessage &result) {
	Goldbox::Data::Items::CharacterItem *itemToTrade = _pendingTradeItem;
	_pendingTradeItem = nullptr;

	setStage(STAGE_ITEM_SELECTION);

	if (!result._success || !result._hasIntValue || !itemToTrade || !_character)
		return;

	const int targetIndex = result._intValue;
	Common::List<Goldbox::Data::PlayerCharacter *> *party = VmInterface::getParty();
	if (!party || targetIndex < 0 || targetIndex >= (int)party->size())
		return;

	Goldbox::Poolrad::Data::PoolradCharacter *tradeTarget = nullptr;
	int idx = 0;
	for (Goldbox::Data::PlayerCharacter *member : *party) {
		if (idx == targetIndex) {
			tradeTarget =
				static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(member);
			break;
		}
		++idx;
	}

	if (!tradeTarget || tradeTarget == _character)
		return;

	// Persist selected recipient (mirrors PTR_TO_CHARACTER = targetCharacter).
	VmInterface::setTargetCharacter(tradeTarget);

	// CHARACTER_checkItemFit: true means item cannot be accepted.
	if (tradeTarget->cannotAcceptItem(*itemToTrade)) {
		displayMessage("Overloaded");
		return;
	}

	// ITEM_moveTo then ITEM_takeFrom — order matches original.
	tradeTarget->inventory.appendCopy(*itemToTrade);
	if (!_character->removeItem(itemToTrade)) {
		warning("Trade: appendCopy succeeded but source removal failed");
		return;
	}

	// CHARACTER_recalcCombatStats for recipient only (original does not recalc source here).
	tradeTarget->recalcCombatStats();

	buildItemList();
	buildItemsListMenu();
	redraw();
}

bool ItemsMenu::hasTradeTarget() const {
	if (!_character)
		return false;

	Common::List<Goldbox::Data::PlayerCharacter *> *party = VmInterface::getParty();
	if (!party)
		return false;

	for (Goldbox::Data::PlayerCharacter *pc : *party) {
		if (pc && pc != _character)
			return true;
	}

	return false;
}

void ItemsMenu::prepareTradeSelectionBackdrop() {
	const int gameStatus = Goldbox::VmInterface::getGameStatus();

	// Modern equivalent of legacy GAME_ScreenByState layout prep.
	switch (gameStatus) {
	case GS_SHOP:
	case GS_CAMPING:
	case GS_DUNGEON_MAP:
	case GS_AFTER_COMBAT:
	case GS_COMBAT:
		drawMainScreenWindows(true);
		break;
	case GS_WILDERNESS_MAP:
		drawMainScreenWindows(false);
		break;
	default:
		// Keep ItemsMenu base layout in unknown states.
		drawWindow(1, 1, 38, 22);
		break;
	}

	redraw();
}

void ItemsMenu::setStage(ItemsMenuStage stage) {
	debug(7, "ItemsMenu::setStage() - changing from %d to %d", (int)_stage, (int)stage);
	_stage = stage;

	switch (_stage) {
	case STAGE_ITEM_SELECTION: {
		debug(7, "ItemsMenu::setStage() - ITEM_SELECTION stage");
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
		debug(7, "ItemsMenu::setStage() - CONFIRM_DROP stage");
		if (_removeConfirm) {
			detachDialog(_removeConfirm);
			_removeConfirm->clear();
			delete _removeConfirm;
			_removeConfirm = nullptr;
		}
		// Create and show drop confirmation dialog
		if (_character && _pendingRemoveItem) {
			HorizontalYesNoConfig ynCfg = {"Drop It?", 13, 10, 15, 0};
			_removeConfirm = new HorizontalYesNo("ItemsRemoveConfirm", ynCfg);
			attachDialog(_removeConfirm);
			_removeConfirm->activate();
			redraw();
		}
		break;
	}
	case STAGE_CONFIRM_SELL: {
		debug(7, "ItemsMenu::setStage() - CONFIRM_SELL stage");
		if (_removeConfirm) {
			detachDialog(_removeConfirm);
			_removeConfirm->clear();
			delete _removeConfirm;
			_removeConfirm = nullptr;
		}
		if (_character && _pendingSellItem) {
			Common::String prompt = Common::String::format(
				"Sell for %u?", (unsigned)_pendingSellValue);
			HorizontalYesNoConfig ynCfg = {prompt, 13, 10, 15, 0};
			_removeConfirm = new HorizontalYesNo("ItemsSellConfirm", ynCfg);
			attachDialog(_removeConfirm);
			_removeConfirm->activate();
			redraw();
		}
		break;
	}
	case STAGE_SELECT_TRADE_TARGET: {
		debug(7, "ItemsMenu::setStage() - SELECT_TRADE_TARGET stage");
		if (_partySelector) {
			detachDialog(_partySelector);
			delete _partySelector;
			_partySelector = nullptr;
		}

		prepareTradeSelectionBackdrop();

		// Create and show party selector
		PartySelectorConfig config;
		config.promptText = "Trade with Whom?";
		config.allowExit = true;
		config.excludedCharacter = _character;
		_partySelector = new PartySelector("ItemsTradeSelector", config);
		attachDialog(_partySelector);
		_partySelector->activate();
		redraw();
		break;
	}
	case STAGE_CONFIRM_IDENTIFY: {
		debug(7, "ItemsMenu::setStage() - CONFIRM_IDENTIFY stage");
		if (_removeConfirm) {
			detachDialog(_removeConfirm);
			_removeConfirm->clear();
			delete _removeConfirm;
			_removeConfirm = nullptr;
		}
		if (_character && _pendingIdentifyItem) {
			const Common::String prompt = Common::String::format(
				"Identify %s for 200gp?",
				_pendingIdentifyItem->getDisplayName().c_str());
			HorizontalYesNoConfig ynCfg = {prompt, 13, 10, 15, 0};
			_removeConfirm = new HorizontalYesNo("ItemsIdentifyConfirm", ynCfg);
			attachDialog(_removeConfirm);
			_removeConfirm->activate();
			redraw();
		}
		break;
	}
	case STAGE_SELECT_SCROLL_SPELL: {
		debug(7, "ItemsMenu::setStage() - SELECT_SCROLL_SPELL stage");
		if (_scrollSpellMenu) {
			detachDialog(_scrollSpellMenu);
			delete _scrollSpellMenu;
			_scrollSpellMenu = nullptr;
		}
		if (_pendingUseItem) {
			_scrollSpellMenu = new SpellsMenu("ItemsScrollSpellMenu");
			_scrollSpellMenu->configure(
				SpellsMenu::SL_ON_SCROLL,
				SpellsMenu::SA_CAST);
			Common::Array<Goldbox::Data::Items::CharacterItem *> scrollList;
			scrollList.push_back(_pendingUseItem);
			_scrollSpellMenu->setScrollItems(&scrollList);
			attachDialog(_scrollSpellMenu);
			_scrollSpellMenu->activate();
			redraw();
		}
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
