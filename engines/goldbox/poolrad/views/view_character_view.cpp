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
    // Build initial menu options before constructing horizontal menu so items appear first frame
    buildMenu();

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
    // Activate subviews so they can receive input/draw immediately
    _profileDialog->activate();
    _horizontalMenu->activate();
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
    // Order: conditional feature panels first, then actions, then Exit
    if (_character && _character->getInventory().count() != 0)
        _menuList.push_back("Items");
    if (_character && _character->haveMemorizedSpell())
        _menuList.push_back("Spells");
    _menuList.push_back("Trade");
    _menuList.push_back("Drop");
    _menuList.push_back("Exit");
}

void ViewCharacterView::draw() {
    // Window frame similar to other views
    drawWindow(1, 1, 38, 22);
    // Refresh selected character pointer each frame
    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(Goldbox::VmInterface::getSelectedCharacter());
    buildMenu(); // rebuild menu options (inventory/spells may have changed)
    // Force horizontal menu to redraw if item count changed
    if (_horizontalMenu)
        _horizontalMenu->setRedraw();
    if (_profileDialog) {
        _profileDialog->_poolradPc = _character; // update displayed character
        _profileDialog->draw();
    }
    if (_horizontalMenu)
        _horizontalMenu->draw();
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
    if (!accepted || index < 0 || index >= (int)_menuList.items.size())
        return;
    const Common::String &label = _menuList.items[index].text;
    if (label == "Exit") {
        replaceView("Mainmenu");
        return;
    } else if (label == "Items") {
        // TODO: Show items screen / toggle inventory panel
    } else if (label == "Spells") {
        // TODO: Show spells / memorized list
    } else if (label == "Trade") {
        // TODO: Initiate trade sequence
    } else if (label == "Drop") {
        // TODO: Initiate drop item dialog
    }
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
