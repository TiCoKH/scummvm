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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_ITEMS_MENU_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_ITEMS_MENU_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/party_selector.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "common/array.h"

namespace Goldbox {
namespace Data {
namespace Items {
struct CharacterItem;
}
}
namespace Poolrad {
namespace Data {
class PoolradCharacter;
}
namespace Views {
namespace Dialogs {

/**
 * ItemsMenu provides an interactive in-game item management menu
 * for the currently selected character. The player can view their inventory
 * and perform various actions on items (equipping, using, trading, dropping,
 * halving, joining, selling, identifying).
 *
 * Staging Pattern:
 * - STAGE_ITEM_SELECTION: Main view with item list and action menu
 * - STAGE_CONFIRM_DROP: Drop confirmation for special items (scrolls)
 * - STAGE_SELECT_TRADE_TARGET: Character selection for trade action
 *
 * UI Layout:
 * - Based on x86 DOS implementation from UI_Layout_Specification.md
 * - Window: 38×22 characters, position (1,1)
 * - Content area: y=3-22 with integrated menu at y=5-20
 * - Message area: in horizontal menu prompt line at bottom
 * - Color scheme: x86 DOS values (header=15, text=10, select=13)
 */
class ItemsMenu : public Dialog {
public:
	enum ItemsMenuStage {
		STAGE_ITEM_SELECTION = 0,    // Main item/action menu
		STAGE_CONFIRM_DROP,           // Drop confirmation dialog
		STAGE_SELECT_TRADE_TARGET     // Party selector for trade
	};

	ItemsMenu(const Common::String &name = "ItemsMenu");
	~ItemsMenu() override;

	void activate() override;
	void deactivate() override;
	bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
	ItemsMenuStage _stage = STAGE_ITEM_SELECTION;
	Goldbox::Poolrad::Data::PoolradCharacter *_character;
	Goldbox::MenuItemList _itemsMenuList;
	Common::Array<Goldbox::Data::Items::CharacterItem *> _itemList;
	Common::Array<Common::String> _horizontalMenuLabels;
	VerticalMenuConfig _menuConfig;
	VerticalMenu *_verticalMenu;
	PromptMessage *_activePrompt = nullptr;
	HorizontalYesNo *_removeConfirm = nullptr;
	Goldbox::Data::Items::CharacterItem *_pendingRemoveItem = nullptr;
	PartySelector *_partySelector = nullptr;
	Goldbox::Data::Items::CharacterItem *_pendingTradeItem = nullptr;

	void setStage(ItemsMenuStage stage);
	void buildItemsListMenu();
	void buildItemList();
	void buildActionMenu();
	bool canTradeItem(const Goldbox::Data::Items::CharacterItem *item) const;
	bool isFreeToRemove(Goldbox::Data::Items::CharacterItem *item);
	bool canHalveItem(const Goldbox::Data::Items::CharacterItem *item) const;
	bool canSellItem(const Goldbox::Data::Items::CharacterItem *item) const;

	// Action handlers matching original keycode shortcuts
	void handleReadyItem(Goldbox::Data::Items::CharacterItem *item);
	void handleUseItem(Goldbox::Data::Items::CharacterItem *item);
	void handleTradeItem(Goldbox::Data::Items::CharacterItem *item);
	void handleDropItem(Goldbox::Data::Items::CharacterItem *item);
	void handleHalveItem(Goldbox::Data::Items::CharacterItem *item);
	void handleJoinItem(Goldbox::Data::Items::CharacterItem *item);
	void handleSellItem(Goldbox::Data::Items::CharacterItem *item);
	void handleIdentifyItem(Goldbox::Data::Items::CharacterItem *item);
	void handleExit();

	// Helper functions
	bool isCharacterInCombat() const;
	bool isCharacterAnimated() const;
	bool isItemReadied(const Goldbox::Data::Items::CharacterItem *item) const;

	// UI helpers for ready/equip feedback
	void displayEquipError(uint8 errorCode, const Goldbox::Data::Items::CharacterItem *item);
	void displayMessage(const Common::String &message);
	void updateReadyItemDisplay(Goldbox::Data::Items::CharacterItem *item);
	void prepareTradeSelectionBackdrop();
    bool hasTradeTarget() const;

	// Subdialog management
	void handleDropConfirmResult(const MenuResultMessage &result);
	void handleTradeSelectionResult(const MenuResultMessage &result);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_ITEMS_MENU_H
