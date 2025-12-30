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
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

// Global pointer to selected character (define in a suitable cpp file)
Goldbox::Poolrad::Data::PoolradCharacter *g_selectedCharacter = nullptr;

ViewCharacterView::ViewCharacterView()
    : ViewCharacterView(static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(Goldbox::VmInterface::getSelectedCharacter())) {}

ViewCharacterView::ViewCharacterView(Goldbox::Poolrad::Data::PoolradCharacter *character)
    : View("ViewCharacter"), _character(character), _horizontalMenu(nullptr), _profileDialog(nullptr) {

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
    subView(_profileDialog);
    subView(_horizontalMenu);
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
}

void ViewCharacterView::buildMenu() {
    _menuList.items.clear();

    if (!_character)
        return;

    // Add Items if character has any
    if (_character->getInventory().count() != 0) {
        _menuList.push_back("Items");
    }

    // Check for memorized spells using modern SpellBook API
    bool hasMemoSpells = _character->spellBook.hasAnyMemorized();

    // Check for valuables using proper API
    bool hasValuables = false;
    for (int i = 0; i < Goldbox::Data::VALUABLE_COUNT; i++) {
        if (_character->valuableItems.values[i] != 0) {
            hasValuables = true;
            break;
        }
    }

    // Add Spells if character has any memorized spells
    if (hasMemoSpells) {
        _menuList.push_back("Spells");
    }

    // Add Trade if: character has valuables AND not in combat
    // Check npc status: npc < 0x80 means player character, >= 0x80 means NPC
    if (hasValuables && _character->npc < 0x80) {
        if (Goldbox::VmInterface::getGameStatus() != GS_COMBAT) {
            _menuList.push_back("Trade");
        }
    }

    // Add Drop if character has valuables
    if (hasValuables) {
        _menuList.push_back("Drop");
    }

    // Add Rename if character is not NPC
    if (_character->npc < 0x80) {
        _menuList.push_back("Rename");
    }

    _menuList.push_back("Exit");
}

void ViewCharacterView::draw() {
    buildMenu();
    // Always use the current selected character from the engine
    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(Goldbox::VmInterface::getSelectedCharacter());
    if (_profileDialog) {
        // Directly update the internal pointer before drawing
        _profileDialog->_poolradPc = _character;
        _profileDialog->draw();
    }
    if (_horizontalMenu) _horizontalMenu->draw();
}

bool ViewCharacterView::msgKeypress(const KeypressMessage &msg) {
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

    // Map menu index to actual menu item by checking the dynamic menu list
    if (index < 0 || index >= static_cast<int>(_menuList.items.size()))
        return;

    const Common::String &menuItem = _menuList.items[index].text;

    if (menuItem == "Items") {
        handleViewItems();
    } else if (menuItem == "Spells") {
        handleViewSpells();
    } else if (menuItem == "Trade") {
        handleTradeValuables();
    } else if (menuItem == "Drop") {
        handleDropValuables();
    } else if (menuItem == "Rename") {
        handleRenameCharacter();
    } else if (menuItem == "Exit") {
        handleExit();
    }
}

void ViewCharacterView::handleViewItems() {
    // TODO: Implement VIEW_Items equivalent
    // This should show the character's inventory
    // Redraw profile after items dialog
    if (_profileDialog) {
        _profileDialog->draw();
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
