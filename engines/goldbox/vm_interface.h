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

    // DAX Container accessors
    static Data::DaxBlockContainer &getDax8x8d() { return g_engine->getDax8x8d(); }
    static Data::DaxBlockContainer &getDaxBacpac() { return g_engine->getDaxBacpac(); }
    static Data::DaxBlockContainer &getDaxDungcom() { return g_engine->getDaxDungcom(); }
    static Data::DaxBlockContainer &getDaxRandcom() { return g_engine->getDaxRandcom(); }
    static Data::DaxBlockContainer &getDaxSqrpaci() { return g_engine->getDaxSqrpaci(); }
    static Data::DaxBlockContainer &getDaxBody() { return g_engine->getDaxBody(); }
    static Data::DaxBlockContainer &getDaxCBody() { return g_engine->getDaxCBody(); }
    static Data::DaxBlockContainer &getDaxCHead() { return g_engine->getDaxCHead(); }
    static Data::DaxBlockContainer &getDaxComSpr() { return g_engine->getDaxComSpr(); }
    static Data::DaxBlockContainer &getDaxEcl() { return g_engine->getDaxEcl(); }
    static Data::DaxBlockContainer &getDaxGeo() { return g_engine->getDaxGeo(); }
    static Data::DaxBlockContainer &getDaxHead() { return g_engine->getDaxHead(); }
    static Data::DaxBlockContainer &getDaxMonCha() { return g_engine->getDaxMonCha(); }
    static Data::DaxBlockContainer &getDaxMonItm() { return g_engine->getDaxMonItm(); }
    static Data::DaxBlockContainer &getDaxMonSpc() { return g_engine->getDaxMonSpc(); }
    static Data::DaxBlockContainer &getDaxPic() { return g_engine->getDaxPic(); }
    static Data::DaxBlockContainer &getDaxCPic() { return g_engine->getDaxCPic(); }
    static Data::DaxBlockContainer &getDaxSprit() { return g_engine->getDaxSprit(); }
    static Data::DaxBlockContainer &getDaxTitle() { return g_engine->getDaxTitle(); }
    static Data::DaxBlockContainer &getDaxWalldef() { return g_engine->getDaxWalldef(); }

    // DAX Manager direct access
    static Data::DaxFileManager &getDaxManager() { return g_engine->getDaxManager(); }
};

} // namespace Goldbox

#endif // GOLDBOX_VM_INTERFACE_H
