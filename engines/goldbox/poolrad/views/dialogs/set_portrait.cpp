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

#include "goldbox/poolrad/views/dialogs/set_portrait.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Common::String;
using Common::KeyCode;
using Common::Array;
using Goldbox::Poolrad::Data::PoolradCharacter;

namespace {
const byte kPromptColor = 13;
const byte kTextColor = 10;
const byte kSelectColor = 15;
}

SetPortrait::SetPortrait(const String &name,
    PoolradCharacter *pc,
    CharacterProfile *profile) :
    Dialog(name), _pc(pc), _characterProfile(profile),
    _committedHead(kMinHead), _committedBody(kMinBody) {
    if (_pc) {
        _committedHead = _pc->portrait.head ? _pc->portrait.head : kMinHead;
        _committedBody = _pc->portrait.body ? _pc->portrait.body : kMinBody;
    }

    Array<String> labels;
    labels.push_back("Head");
    labels.push_back("Body");
    labels.push_back("Keep");
    _menuItems.generateMenuItems(labels, true);

    HorizontalMenuConfig cfg = {
        "Head Body Keep",
        &_menuItems,
        kTextColor,
        kSelectColor,
        kPromptColor,
        false
    };

    _menu = new HorizontalMenu(name + "_Menu", cfg);
    _menu->setParent(this);
}

SetPortrait::~SetPortrait() {
    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }
}

bool SetPortrait::msgKeypress(const KeypressMessage &msg) {
    if (_menu)
        return _menu->msgKeypress(msg);
    return true;
}

void SetPortrait::draw() {
    if (_menu)
        _menu->draw();
}

void SetPortrait::cycleHead() {
    if (_pc->portrait.head < kMaxHead)
        _pc->portrait.head++;
    else
        _pc->portrait.head = kMinHead;

    if (_characterProfile)
        _characterProfile->drawPortrait();
}

void SetPortrait::cycleBody() {
    if (_pc->portrait.body < kMaxBody)
        _pc->portrait.body++;
    else
        _pc->portrait.body = kMinBody;

    if (_characterProfile)
        _characterProfile->drawPortrait();
}

void SetPortrait::commitSelection() {
    if (!_pc)
        return;
    _committedHead = _pc->portrait.head;
    _committedBody = _pc->portrait.body;
}

void SetPortrait::restoreCommitted() {
    if (!_pc)
        return;
    _pc->portrait.head = _committedHead;
    _pc->portrait.body = _committedBody;
}

void SetPortrait::refresh() {
    if (_menu) {
        _menu->setRedraw();
        _menu->draw();
    }
}

void SetPortrait::handleMenuResult(bool success, Common::KeyCode key, short value) {
    if (!success) {
        restoreCommitted();
        refresh();
        if (_parent)
            _parent->handleMenuResult(false, key, value);
        return;
    }

    switch (value) {
    case 0: // Head
        cycleHead();
        refresh();
        break;
    case 1: // Body
        cycleBody();
        refresh();
        break;
    case 2: // Keep
        commitSelection();
        if (_parent)
            _parent->handleMenuResult(true, Common::KEYCODE_RETURN, 0);
        break;
    default:
        break;
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
