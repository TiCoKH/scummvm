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

#include "goldbox/vm_interface.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/view_character_view.h"
#include "goldbox/poolrad/views/dialogs/items_menu.h"
#include "goldbox/data/rules/rules_types.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

ViewCharacterView::ViewCharacterView()
        : View("ViewCharacter"),
            _character(nullptr),
            _horizontalMenu(nullptr),
            _profileDialog(nullptr),
            _activeSubView(nullptr)
            // _itemsMenu(nullptr)
{

    Dialogs::HorizontalMenuConfig menuConfig = {
        "View:", // promptTxt
        &_menuList,
        10, // textColor
        15, // selectColor
        13, // promptColor
        false // allowNumPad
    };
    _horizontalMenu = new Dialogs::HorizontalMenu("CharacterHorizontalMenu", menuConfig);
    _profileDialog = new Dialogs::CharacterProfile();
//    _itemsMenu = new Dialogs::ItemsMenu(_character, "ItemsMenu");
    subView(_profileDialog);
    subView(_horizontalMenu);
    // subView(_itemsMenu);

    // Don't call setStage here - defer to activate() when character data is available
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
    // if (_itemsMenu) {
    //     delete _itemsMenu;
    //     _itemsMenu = nullptr;
    // }
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
    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
    }
}

void ViewCharacterView::syncSelectedCharacter(bool forceRefresh) {
    Goldbox::Poolrad::Data::PoolradCharacter *selectedCharacter =
        static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
            VmInterface::getSelectedCharacter()
        );

    if (!forceRefresh && selectedCharacter == _character) {
        return;
    }

    _character = selectedCharacter;
    if (_profileDialog) {
        _profileDialog->activate();
    }
    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
    }
}

void ViewCharacterView::onEnter(Goldbox::GameState state) {
    View::onEnter(state);
    syncSelectedCharacter(true);
    setStage(VC_STATE_PROFILE);
}

void ViewCharacterView::draw() {
	syncSelectedCharacter(false);
    if (!_character) {
		return;
	}

    //if (_stage == VC_STATE_ITEMS && _itemsMenu && !_itemsMenu->isActive()) {
    //    setStage(VC_STATE_PROFILE);
    //}

    //if (_stage == VC_STATE_ITEMS) {
    //    if (_itemsMenu) {
    //        _itemsMenu->setCharacter(_character);
    //        _itemsMenu->draw();
    //    }
    //    return;
    //}

    // Profile stage: rebuild menu and draw both profile and menu
    buildMenu();
    if (_profileDialog) {
        _profileDialog->draw();
    }
    // Only draw horizontal menu in PROFILE stage
    if (_horizontalMenu) {
        _horizontalMenu->draw();
    }
}

bool ViewCharacterView::msgKeypress(const KeypressMessage &msg) {
    if (_activeSubView && _activeSubView->send(msg)) {
        return true;
    }
    return View::msgKeypress(msg);
}

// Skeleton for handling menu results from HorizontalMenu
void ViewCharacterView::handleMenuResult(bool success, Common::KeyCode keyCode, short value) {
    if (!success) {
        if (_parent) {
            _parent->handleMenuResult(false, keyCode, value);
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
        handleExit();
        break;
    default:
        break;
    }
}

void ViewCharacterView::setStage(ViewCharacterState stage) {
    _stage = stage;

    switch (_stage) {
    case VC_STATE_PROFILE:
        // if (_itemsMenu)
        //     _itemsMenu->deactivate();
        if (_profileDialog)
            _profileDialog->activate();
        if (_horizontalMenu)
            _horizontalMenu->activate();
        setActiveSubView(_horizontalMenu);
        break;
    case VC_STATE_ITEMS:
        if (_horizontalMenu)
            _horizontalMenu->deactivate();
        if (_profileDialog)
            _profileDialog->deactivate();
        // if (_itemsMenu)
        //     _itemsMenu->activate();
        // setActiveSubView(_itemsMenu);
        break;
    }
}

void ViewCharacterView::setActiveSubView(Dialogs::Dialog *dlg) {
    if (_activeSubView && _activeSubView != dlg) {
        _activeSubView->deactivate();
    }
    _activeSubView = dlg;
    if (_activeSubView)
        _activeSubView->activate();
}

void ViewCharacterView::handleViewItems() {
    // if (_itemsMenu)
    //     _itemsMenu->setCharacter(_character);
    // setStage(VC_STATE_ITEMS);
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
