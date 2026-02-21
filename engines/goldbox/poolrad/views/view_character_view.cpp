/* ScummVM - Graphic Adventure Engine
 *
 * ViewCharacterView - Character view with horizontal menu
 */

#include "goldbox/vm_interface.h"
#include "goldbox/data/player_character.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/view_character_view.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/views/dialogs/items_menu.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

// Global pointer to selected character (define in a suitable cpp file)
Goldbox::Poolrad::Data::PoolradCharacter *g_selectedCharacter = nullptr;

ViewCharacterView::ViewCharacterView()
    : ViewCharacterView(static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(Goldbox::VmInterface::getSelectedCharacter())) {}

ViewCharacterView::ViewCharacterView(Goldbox::Poolrad::Data::PoolradCharacter *character)
        : View("ViewCharacter"),
            _character(character),
            _horizontalMenu(nullptr),
            _profileDialog(nullptr),
            _itemsMenu(nullptr) {

    Dialogs::HorizontalMenuConfig menuConfig = {
        "View:", // promptTxt
        &_menuList,
        10, // textColor
        15, // selectColor
        13, // promptColor
        false // allowNumPad
    };
    _horizontalMenu = new Dialogs::HorizontalMenu("CharacterHorizontalMenu", menuConfig);
    _profileDialog = new Dialogs::CharacterProfile(_character, "CharacterProfile");
    _itemsMenu = new Dialogs::ItemsMenu(_character, "ItemsMenu");
    subView(_profileDialog);
    subView(_horizontalMenu);
    subView(_itemsMenu);
}

ViewCharacterView::~ViewCharacterView() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    if (_profileDialog) {
        delete _profileDialog;
        _profileDialog = nullptr;
    }
    if (_itemsMenu) {
        delete _itemsMenu;
        _itemsMenu = nullptr;
    }
}

void ViewCharacterView::buildMenu() {
    _menuList.items.clear();
    _menuList.currentSelection = 0;

    if (!_character)
        return;

    Common::Array<Common::String> labels;

    // Add Items if character has any
    if (_character->hasItems()) {
        labels.push_back("Items");
    }

    // Check for memorized spells using PoolradCharacter helper
    const bool hasMemoSpells = _character->haveMemorizedSpell();

    // Check for valuables using proper API
    const bool hasValuables = _character->hasValuables();

    // Add Spells if character has any memorized spells
    if (hasMemoSpells) {
        labels.push_back("Spells");
    }

    // Add Trade if: has valuables AND not in combat AND
    // (npc < 0x80 OR enabled == 0 OR status == STATUS_ANIMATED)
    const bool tradeAllowed = !_character->isNpc() || (!_character->enabled) ||
        (_character->healthStatus == Goldbox::Data::S_ANIMATED);
    if (hasValuables && tradeAllowed) {
        if (Goldbox::VmInterface::getGameStatus() != GS_COMBAT) {
            labels.push_back("Trade");
        }
    }

    // Add Drop if character has valuables
    if (hasValuables) {
        labels.push_back("Drop");
    }
/* Addendum: For now, always show Rename for player characters, as it doesn't require a working prompt dialog yet
    // Add Rename if character is not NPC
    if (!_character->isNpc()) {
        labels.push_back("Rename");
    }
*/
    labels.push_back("Exit");

    _menuList.generateMenuItems(labels, true);
}

void ViewCharacterView::draw() {
    // Always use the current selected character from the engine
    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(Goldbox::VmInterface::getSelectedCharacter());
	
	if (!_character) {
		return;
	}
	
    buildMenu();
    if (_itemsMenu && _itemsMenu->isActive()) {
        _itemsMenu->setCharacter(_character);
        _itemsMenu->draw();
        return;
    }
    if (_profileDialog) {
        // Directly update the internal pointer before drawing
        _profileDialog->_poolradPc = _character;
        _profileDialog->draw();
    }
    if (_horizontalMenu) _horizontalMenu->draw();
}

bool ViewCharacterView::msgKeypress(const KeypressMessage &msg) {
    if (_itemsMenu && _itemsMenu->isActive()) {
        return _itemsMenu->msgKeypress(msg);
    }
    if (_horizontalMenu) {
        // Let the menu handle the keypress first
        if (_horizontalMenu->msgKeypress(msg))
            return true;
    }
    // Handle other keypresses here if needed
    return false;
}

// Skeleton for handling menu results from HorizontalMenu
void ViewCharacterView::handleMenuResult(bool accepted, Common::KeyCode keyCode, int index) {
    if (!accepted) {
        if (_parent) {
            _parent->handleMenuResult(false,  keyCode, index);
        }
        return;
    }

    if (!_character)
        return;

    switch (keyCode) {
    case Common::KEYCODE_i:
        handleViewItems();
        break;
    case Common::KEYCODE_s:
        handleViewSpells();
        break;
    case Common::KEYCODE_t:
        handleTradeValuables();
        break;
    case Common::KEYCODE_d:
        handleDropValuables();
        break;
/*     case Common::KEYCODE_r:
        handleRenameCharacter();
        break; */
    case Common::KEYCODE_e:
    case Common::KEYCODE_ESCAPE:
        handleExit();
        break;
    default:
        break;
    }
}

void ViewCharacterView::handleViewItems() {
    if (_itemsMenu) {
        _itemsMenu->setCharacter(_character);
        _itemsMenu->activate();
    }
}

void ViewCharacterView::handleViewSpells() {
    // TODO: Implement VIEW_Spells equivalent
    // This should show memorized spells
    // Redraw profile after spells dialog
    if (_profileDialog) {
        _profileDialog->draw();
    }
}

void ViewCharacterView::handleTradeValuables() {
    // TODO: Implement DIALOG_TradeValuables equivalent
    // This should allow trading/managing valuables
}

void ViewCharacterView::handleDropValuables() {
    // TODO: Implement DIALOG_DropValuables equivalent
    // This should allow dropping valuables
    if (_profileDialog) {
        _profileDialog->redrawValuables();
    }
}

void ViewCharacterView::handleRenameCharacter() {
    // Prompt for new character name
    Common::String prompt = "NEW NAME FOR " + _character->name;
    // TODO: Implement PromptLine_TextInput equivalent
    // Should update character name and redraw profile
    if (_profileDialog) {
        _profileDialog->redrawName();
    }
}

void ViewCharacterView::handleExit() {
    // Signal exit from this view to parent
    if (_parent) {
        _parent->handleMenuResult(true, Common::KEYCODE_ESCAPE, 0);
    }
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
