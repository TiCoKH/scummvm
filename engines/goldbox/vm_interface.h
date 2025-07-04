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

#ifndef GOLDBOX_VM_INTERFACE_H
#define GOLDBOX_VM_INTERFACE_H

#include "goldbox/engine.h"

namespace Goldbox {

class VmInterface {
public:
    // Static utility methods to access Engine functionality

    // Example: Dice rolling
    static int rollDice(int number, int sides) {
        return g_engine->rollDice(number, sides);
    }

    // Add getString method
    static Common::String getString(const Common::String &key) {
        return g_engine->_strings.getVal(key);
    }

    // Add getParty method
    static Common::Array<Data::PlayerCharacter *> *getParty() {
        return &g_engine->getParty();
    }

    // Add getSelectedCharacter accessor
    static Data::PlayerCharacter *getSelectedCharacter() {
        return g_engine->getSelectedCharacter();
    }

    // Add setSelectedCharacter accessor
    static void setSelectedCharacter(Data::PlayerCharacter *character) {
        g_engine->setSelectedCharacter(character);
    }
};

} // namespace Goldbox

#endif // GOLDBOX_VM_INTERFACE_H
