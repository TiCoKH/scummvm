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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_PARTY_LIST_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_PARTY_LIST_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class PartyList : public Dialog {
public:
    PartyList(const Common::String &name = "PartyList")
        : Dialog(name), _xName(1), _xAC(33), _yStart(2) {
            _party = Goldbox::VmInterface::getParty();
        }
    virtual ~PartyList() {}

    void draw() override;
    void setLayout(uint xName, uint xAC, uint yStart) {
        _xName = xName; _xAC = xAC; _yStart = yStart;
    }
    void setSelectedCharIndex(uint idx) { _selectedCharIndex = idx; }
    uint getSelectedCharIndex() const { return _selectedCharIndex; }
    void nextChar();
    void prevChar();

private:
    uint _xName, _xAC, _yStart;
    uint _selectedCharIndex;
    Common::Array<Goldbox::Data::PlayerCharacter *> *_party;

};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
