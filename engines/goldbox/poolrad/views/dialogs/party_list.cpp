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

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

void PartyList::draw() {
    if (!_party)
        return;
    Surface s = getSurface();
    s.clearBox(1, 1, 28, 11, 0); // Clear the party area
    int y = _yStart;
    s.writeStringC("Name", 15, _xName, y);
    s.writeStringC("AC  HP", 15, _xAC, y);
    y += 2;
    for (uint _partyIndex = 0; _partyIndex < _party->size(); _partyIndex++) {
        Data::PlayerCharacter *pc = (*_party)[_partyIndex];
        if (pc) {
            int color = (_partyIndex == _selectedCharIndex-1) ? 15 : 11;
            s.writeStringC(pc->name, color, _xName, y);
            color = (pc->hitPoints.max > 0) ? 10 : 12; 
            s.writeStringC(Common::String::format("%d", pc->armorClass.current), color, _xAC, y);
            s.writeStringC(Common::String::format("%d", pc->hitPoints.max), color, _xAC + 4, y);
            y ++;
        }
    }
}

void PartyList::nextChar() {
    if (_selectedCharIndex < _party->size()) {
        _selectedCharIndex++;
    } else {
        _selectedCharIndex = 1;
    }
    draw();
}

void PartyList::prevChar() {
    if (_selectedCharIndex > 1) {
        _selectedCharIndex--;
    } else {
        _selectedCharIndex = _party->size();
    }
    draw(); 
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
