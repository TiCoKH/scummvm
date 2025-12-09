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

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {
namespace {
const byte kPromptColor = 13;
const byte kTextColor = 10;
const byte kSelectColor = 15;

const uint8 kPortraitLeft = 28;
const uint8 kPortraitTop = 1;
const uint8 kPortraitRight = 38;
const uint8 kPortraitBottom = 11;
}

PortraitDialog::PortraitDialog(Goldbox::Poolrad::Data::PoolradCharacter *pc,
    const Common::String &name) :
    Dialog(name), _pc(pc), _left(kPortraitLeft), _top(kPortraitTop),
    _right(kPortraitRight), _bottom(kPortraitBottom) {
}

void PortraitDialog::draw() {
    drawWindow(_left, _top, _right, _bottom);
    if (!_pc)
        return;

    Surface s = getSurface();
    Common::String headText = Common::String::format("Head %d", (int)_pc->portrait.head);
    Common::String bodyText = Common::String::format("Body %d", (int)_pc->portrait.body);
    s.writeStringC(headText, 15, _left + 1, _top + 1);
    s.writeStringC(bodyText, 15, _left + 1, _top + 2);
}

SetPortraitDialog::SetPortraitDialog(const Common::String &name,
    Goldbox::Poolrad::Data::PoolradCharacter *pc) :
    Dialog(name), _pc(pc), _portraitPreview(nullptr), _menu(nullptr),
    _committedHead(kMinHead), _committedBody(kMinBody) {
    if (_pc) {
        _committedHead = _pc->portrait.head ? _pc->portrait.head : kMinHead;
        _committedBody = _pc->portrait.body ? _pc->portrait.body : kMinBody;
    }

    _portraitPreview = new PortraitDialog(pc, name + "_Preview");
    _portraitPreview->setParent(this);

    Common::Array<Common::String> labels;
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

SetPortraitDialog::~SetPortraitDialog() {
    if (_portraitPreview) {
        _portraitPreview->setParent(nullptr);
        delete _portraitPreview;
        _portraitPreview = nullptr;
    }
    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }
}

bool SetPortraitDialog::msgKeypress(const KeypressMessage &msg) {
    if (_menu)
        return _menu->msgKeypress(msg);
    return true;
}

void SetPortraitDialog::draw() {
    if (_portraitPreview)
        _portraitPreview->draw();
    if (_menu)
        _menu->draw();
}

void SetPortraitDialog::cycleHead() {
    if (!_pc)
        return;
    if (_pc->portrait.head < kMaxHead)
        _pc->portrait.head++;
    else
        _pc->portrait.head = kMinHead;
}

void SetPortraitDialog::cycleBody() {
    if (!_pc)
        return;
    if (_pc->portrait.body < kMaxBody)
        _pc->portrait.body++;
    else
        _pc->portrait.body = kMinBody;
}

void SetPortraitDialog::commitSelection() {
    if (!_pc)
        return;
    _committedHead = _pc->portrait.head;
    _committedBody = _pc->portrait.body;
}

void SetPortraitDialog::restoreCommitted() {
    if (!_pc)
        return;
    _pc->portrait.head = _committedHead;
    _pc->portrait.body = _committedBody;
}

void SetPortraitDialog::refresh() {
    if (_portraitPreview)
        _portraitPreview->draw();
    if (_menu) {
        _menu->setRedraw();
        _menu->draw();
    }
}

void SetPortraitDialog::handleMenuResult(bool success, Common::KeyCode key, short value) {
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
