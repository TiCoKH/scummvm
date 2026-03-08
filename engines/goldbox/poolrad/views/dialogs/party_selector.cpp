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

#include "goldbox/poolrad/views/dialogs/party_selector.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

namespace {
const byte kPromptColor = 13;
const byte kTextColor = 10;
const byte kSelectColor = 15;
}

PartySelector::PartySelector(const Common::String &name,
                             const PartySelectorConfig &config)
    : Dialog(name),
      _config(config),
      _partyList(nullptr),
      _horizontalMenu(nullptr) {
    buildMenuItems();

    _menuPrompt = _config.promptText;
    if (!_menuPrompt.empty() && !_menuPrompt.hasSuffix(" ")) {
        _menuPrompt += " ";
    }

    _partyList = new PartyList(name + "_PartyList");
    subView(_partyList);

    HorizontalMenuConfig menuConfig = {
        _menuPrompt,
        &_menuItems,
        kTextColor,
        kSelectColor,
        kPromptColor,
        true
    };

    _horizontalMenu = new HorizontalMenu(name + "_Menu", menuConfig);
    subView(_horizontalMenu);

    _partyList->deactivate();
    _horizontalMenu->deactivate();
}

PartySelector::~PartySelector() {
    if (_horizontalMenu) {
        _horizontalMenu->setParent(nullptr);
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    if (_partyList) {
        _partyList->setParent(nullptr);
        delete _partyList;
        _partyList = nullptr;
    }
}

void PartySelector::buildMenuItems() {
    Common::Array<Common::String> labels;
    labels.push_back("Select");
    if (_config.allowExit) {
        labels.push_back("Exit");
    }
    _menuItems.generateMenuItems(labels, true);
}

void PartySelector::activate() {
    Dialog::activate();

    if (_partyList) {
        _partyList->setSyncVmSelection(false);
        _partyList->setExcludedCharacter(_config.excludedCharacter);
        _partyList->setSelectedCharIndex(1);
        _partyList->activate();
    }
    if (_horizontalMenu) {
        _horizontalMenu->activate();
    }
}

void PartySelector::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
    }
    if (_partyList) {
        _partyList->deactivate();
    }

    Dialog::deactivate();
}

void PartySelector::draw() {
    if (!isActive()) {
        return;
    }

    if (_partyList) {
        _partyList->draw();
    }

    if (_horizontalMenu) {
        _horizontalMenu->draw();
    }
}

bool PartySelector::msgKeypress(const KeypressMessage &msg) {
    if (!isActive()) {
        return false;
    }

    return View::msgKeypress(msg);
}

bool PartySelector::isNextSelectionKey(Common::KeyCode keyCode) const {
    return keyCode == Common::KEYCODE_END ||
           keyCode == Common::KEYCODE_DOWN;
}

bool PartySelector::isPrevSelectionKey(Common::KeyCode keyCode) const {
    return keyCode == Common::KEYCODE_HOME ||
           keyCode == Common::KEYCODE_UP;
}

void PartySelector::reactivateHorizontalMenu() {
    if (!_horizontalMenu) {
        return;
    }

    _horizontalMenu->activate();
    _horizontalMenu->setRedraw();
    redraw();
}

void PartySelector::postSelectionResult(bool success, Common::KeyCode keyCode) {
    if (!_parent) {
        return;
    }

    // PartyList is 1-based; post 0-based party index to parent handlers.
    int selectedIndex = _partyList ? (int)_partyList->getSelectedCharIndex() : 1;
    if (selectedIndex < 1)
        selectedIndex = 1;
    selectedIndex -= 1;

    g_events->postMenuResult(_parent->getName(), success, keyCode,
        selectedIndex, Common::String(), true, false);
}

void PartySelector::handleMenuResult(const MenuResultMessage &result) {
    const Common::KeyCode keyCode = result._keyCode;

    if (!result._success) {
        postSelectionResult(false, keyCode);
        deactivate();
        return;
    }

    if (isNextSelectionKey(keyCode)) {
        if (_partyList) {
            _partyList->nextChar();
            _partyList->redraw();
        }
        reactivateHorizontalMenu();
        return;
    }

    if (isPrevSelectionKey(keyCode)) {
        if (_partyList) {
            _partyList->prevChar();
            _partyList->redraw();
        }
        reactivateHorizontalMenu();
        return;
    }

    if (keyCode == Common::KEYCODE_s || keyCode == Common::KEYCODE_RETURN) {
        postSelectionResult(true, keyCode);
        deactivate();
        return;
    }

    if (_config.allowExit && keyCode == Common::KEYCODE_e) {
        postSelectionResult(false, keyCode);
        deactivate();
        return;
    }

    reactivateHorizontalMenu();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
