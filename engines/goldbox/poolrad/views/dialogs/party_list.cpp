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

#include "goldbox/poolrad/views/dialogs/party_list.h"
#include "common/keyboard.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

void PartyList::activate() {
    _party = Goldbox::VmInterface::getParty();
    Dialog::activate();
}

void PartyList::updateSelectedCharacter() {
    if (_party && _party->size() > 0) {
        if (_selectedCharIndex < 1)
            _selectedCharIndex = 1;
        if (_selectedCharIndex > _party->size())
            _selectedCharIndex = _party->size();

        Goldbox::VmInterface::setSelectedCharacter((*_party)[_selectedCharIndex - 1]);
    }
}

void PartyList::draw() {
    if (!_party || _party->size() == 0)
        return;

    updateSelectedCharacter();

    Surface s = getSurface();
    s.clearBox(1, 1, 28, 11, 0);
    int y = _yStart;
	s.writeStringC(_xName, y, 15, "Name");
	s.writeStringC(_xAC,   y, 15, "AC  HP");
    y += 2;
    for (uint _partyIndex = 0; _partyIndex < _party->size(); _partyIndex++) {
        Data::PlayerCharacter *pc = (*_party)[_partyIndex];
        if (pc) {
            int color = (_partyIndex == _selectedCharIndex-1) ? 15 : 11;
			s.writeStringC(_xName, y, color, pc->name);
            color = (pc->hitPoints.max > 0) ? 10 : 12;
            s.writeStringC(_xAC, y, color, Common::String::format("%d", pc->armorClass.getCurrent()));
			s.writeStringC(_xAC + 4, y, color, Common::String::format("%d", pc->hitPoints.max));
            y ++;
        }
    }
}

void PartyList::nextChar() {
    if (!_party || _party->size() == 0)
        return;

    if (_selectedCharIndex < _party->size()) {
        _selectedCharIndex++;
    } else {
        _selectedCharIndex = 1;
    }
    updateSelectedCharacter();
}

void PartyList::prevChar() {
    if (!_party || _party->size() == 0)
        return;

    if (_selectedCharIndex > 1) {
        _selectedCharIndex--;
    } else {
        _selectedCharIndex = _party->size();
    }
    updateSelectedCharacter();
}

bool PartyList::msgKeypress(const KeypressMessage &msg) {
    // Only handle navigation if there's more than one party member
    if (!_party || _party->size() <= 1)
        return false;

    switch (msg.keycode) {
        case Common::KEYCODE_END:
        case Common::KEYCODE_KP1:
            nextChar();
            redraw();
            return true;

        case Common::KEYCODE_HOME:
        case Common::KEYCODE_KP7:
            prevChar();
            redraw();
            return true;

        default:
            break;
    }

    return Dialog::msgKeypress(msg);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
