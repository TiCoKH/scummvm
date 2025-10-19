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
    // Add Items if character has any
    if (_character && _character->getInventory().count() != 0)
        _menuList.push_back("Items");
    // Add Spells if character has any
    if (_character && _character->haveMemorizedSpell())
        _menuList.push_back("Spells");
    _menuList.push_back("Trade");
    _menuList.push_back("Drop");
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
void ViewCharacterView::handleMenuResult(bool accepted, int keyCode, int index) {
    if (!accepted)
        return;
    // Handle menu selection by index
    switch (index) {
        case 0: // Trade
            // TODO: Implement Trade action
            break;
        case 1: // Drop
            // TODO: Implement Drop action
            break;
        case 2: // Items (if present)
            // TODO: Implement Items action
            break;
        case 3: // Spells (if present)
            // TODO: Implement Spells action
            break;
        case 4: // Exit
            // TODO: Implement Exit action
            break;
        default:
            break;
    }
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
