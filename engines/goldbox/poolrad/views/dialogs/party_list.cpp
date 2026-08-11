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

namespace {

static int acColumnOffset(int acCurrent) {
    // Right-align AC values: single digit gets +1 offset.
    if (acCurrent >= 0 && acCurrent <= 9)
        return 1;
    return 0;
}

static int hpColumnOffset(int hpCurrent) {
    // Right-align HP values to 3 chars at columns 36..38.
    if (hpCurrent < 10)
        return 2;
    if (hpCurrent <= 99)
        return 1;
    return 0;
}

} // namespace

void PartyList::syncLayoutForGameState() {
    if (_hasCustomLayout)
        return;

    const GameState state = Goldbox::VmInterface::getGameStatus();
    if (state == GS_START_MENU)
        _xName = 1;
    else
        _xName = 0x11;

    // Original DIALOG_ShowParty constants.
    _xAC = 0x21;
    _yStart = 2;
}

void PartyList::activate() {
    _party = Goldbox::VmInterface::getParty();
    if (_syncVmSelection && _party && !_party->empty()) {
        Goldbox::Data::PlayerCharacter *vmSelected = Goldbox::VmInterface::getSelectedCharacter();
        if (vmSelected) {
            uint i = 1;
            for (Goldbox::Data::PlayerCharacter *member : *_party) {
                if (member == vmSelected) {
                    _selectedCharIndex = i;
                    break;
                }
                ++i;
            }
        }
    }
    Dialog::activate();
}

bool PartyList::isSelectableIndex(uint index) const {
    if (!_party || index < 1 || index > (uint)_party->size())
        return false;

    Goldbox::Data::PlayerCharacter *candidate = nullptr;
    uint i = 1;
    for (Goldbox::Data::PlayerCharacter *member : *_party) {
        if (i == index) {
            candidate = member;
            break;
        }
        ++i;
    }
    if (!candidate)
        return false;

    if (_excludedCharacter && candidate == _excludedCharacter)
        return false;

    return true;
}

bool PartyList::findNextSelectableFrom(uint startIndex, int direction, uint &outIndex) const {
    if (!_party || _party->empty())
        return false;

    const uint partySize = (uint)_party->size();
    uint idx = (startIndex < 1) ? 1 : ((startIndex > partySize) ? partySize : startIndex);

    for (uint i = 0; i < partySize; ++i) {
        if (isSelectableIndex(idx)) {
            outIndex = idx;
            return true;
        }

        if (direction >= 0)
            idx = (idx < partySize) ? (idx + 1) : 1;
        else
            idx = (idx > 1) ? (idx - 1) : partySize;
    }

    return false;
}

void PartyList::updateSelectedCharacter() {
    if (!_party || _party->empty())
        return;

    // No VM sync here; only update _selectedCharIndex based on navigation

    const uint partySize = (uint)_party->size();
    if (_selectedCharIndex < 1)
        _selectedCharIndex = 1;
    if (_selectedCharIndex > partySize)
        _selectedCharIndex = partySize;

    uint resolvedIndex = _selectedCharIndex;
    if (findNextSelectableFrom(_selectedCharIndex, 1, resolvedIndex))
        _selectedCharIndex = resolvedIndex;

    if (_syncVmSelection && isSelectableIndex(_selectedCharIndex)) {
        Goldbox::Data::PlayerCharacter *selected = nullptr;
        uint i = 1;
        for (Goldbox::Data::PlayerCharacter *member : *_party) {
            if (i == _selectedCharIndex) {
                selected = member;
                break;
            }
            ++i;
        }
        Goldbox::VmInterface::setSelectedCharacter(selected);
    }
}

void PartyList::draw() {
    if (!_party || _party->size() == 0)
        return;

    syncLayoutForGameState();
    updateSelectedCharacter();

    Surface s = getSurface();
    // Legacy dialog occupies the right panel up to column 0x26 (38).
    // Keep clear bounds data-driven by layout start row and current party size.
    const int maxY = _yStart + 2 + (int)_party->size();
    s.clearBox(_xName, _yStart, 0x26, maxY, 0);

    int y = _yStart;
	s.writeStringC(_xName, y, 15, "Name");
	s.writeStringC(_xAC,   y, 15, "AC  HP");
    y += 2;

    uint _partyIndex = 0;
    for (Data::PlayerCharacter *pc : *_party) {
        if (pc) {
            // Clear row before redrawing to avoid stale wider values.
            s.clearBox(_xName, y, 0x26, y, 0);

            const bool isSelected = (_partyIndex == _selectedCharIndex - 1);
            int color = isSelected ? 15 : 11;
			s.writeStringC(_xName, y, color, pc->name);

            // AC/HP column alignment matches original routine behavior.
            const int ac = pc->armorClass.getCurrent();
            const int hp = (int)pc->hitPoints.current;
            const int acCol = 0x20 + acColumnOffset(ac);
            const int hpCol = 0x24 + hpColumnOffset(hp);

            color = (pc->hitPoints.max > 0) ? 10 : 12;
            s.writeStringC(acCol, y, color, Common::String::format("%d", ac));
            s.writeStringC(hpCol, y, color, Common::String::format("%d", hp));
            y ++;
        }
        ++_partyIndex;
    }

    // Clear one extra line after the list, matching original trailing clear.
    s.clearBox(_xName, y, 0x26, y, 0);
}

void PartyList::nextChar() {
    if (!_party || _party->size() == 0)
        return;

    const uint partySize = (uint)_party->size();
    const uint start = (_selectedCharIndex < partySize) ? (_selectedCharIndex + 1) : 1;
    uint nextIndex = _selectedCharIndex;
    if (findNextSelectableFrom(start, 1, nextIndex)) {
        _selectedCharIndex = nextIndex;
        updateSelectedCharacter();
    }
}

void PartyList::prevChar() {
    if (!_party || _party->size() == 0)
        return;

    const uint partySize = (uint)_party->size();
    const uint start = (_selectedCharIndex > 1) ? (_selectedCharIndex - 1) : partySize;
    uint prevIndex = _selectedCharIndex;
    if (findNextSelectableFrom(start, -1, prevIndex)) {
        _selectedCharIndex = prevIndex;
        updateSelectedCharacter();
    }
}

bool PartyList::handleKeypressDirect(const KeypressMessage &msg) {
    if (!_party || _party->empty())
        return false;

    switch (msg.keycode) {
        case Common::KEYCODE_END:
        case Common::KEYCODE_KP1:
            nextChar();
            draw();
            return true;

        case Common::KEYCODE_HOME:
        case Common::KEYCODE_KP7:
            prevChar();
            draw();
            return true;

        default:
            break;
    }

    return false;
}

bool PartyList::msgKeypress(const KeypressMessage &msg) {
    return handleKeypressDirect(msg);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
