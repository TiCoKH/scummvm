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

#include "goldbox/poolrad/views/dialogs/combat_spell_dialog.h"
#include "goldbox/data/player_character.h"
#include "goldbox/combat/combat_turn.h"
#include "goldbox/poolrad/views/combat_view.h"
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatSpellDialog::CombatSpellDialog()
    : Dialog("CombatSpell"), _actor(nullptr), _selectedSpell(0),
      _spellsMenu(nullptr) {
    _spellsMenu = new SpellsMenu("CombatSpellsMenu");
    subView(_spellsMenu);
}

CombatSpellDialog::~CombatSpellDialog() {
    delete _spellsMenu;
}

void CombatSpellDialog::beginCast(Goldbox::Data::PlayerCharacter *actor) {
    _actor = actor;
    _selectedSpell = 0;
    debug(0, "[COMBATSPELL] beginCast actor='%s'",
        _actor ? _actor->name.c_str() : "NONE");
}

void CombatSpellDialog::activate() {
    Goldbox::Data::PlayerCharacter *selected =
        VmInterface::getSelectedCharacter();
    debug(0, "[COMBATSPELL] activate actor='%s' selected='%s'",
        _actor ? _actor->name.c_str() : "NONE",
        selected ? selected->name.c_str() : "NONE");
    Dialog::activate();
    _spellsMenu->configure(SpellsMenu::SL_IN_MEMORY, SpellsMenu::SA_CAST);
    _spellsMenu->activate();
}

void CombatSpellDialog::deactivate() {
    _spellsMenu->deactivate();
    Dialog::deactivate();
}

void CombatSpellDialog::draw() {
    _spellsMenu->draw();
}

bool CombatSpellDialog::msgKeypress(const KeypressMessage &msg) {
    return _spellsMenu->send(msg);
}

void CombatSpellDialog::handleMenuResult(const MenuResultMessage &result) {
    _selectedSpell = 0;
    debug(0, "[COMBATSPELL] result success=%d hasInt=%d int=%d actor='%s'",
        (int)result._success, (int)result._hasIntValue,
        result._hasIntValue ? result._intValue : -1,
        _actor ? _actor->name.c_str() : "NONE");
    if (!result._success || !result._hasIntValue || !_actor) {
        finishCast(false);
        return;
    }

    _selectedSpell = static_cast<uint8>(_spellsMenu->getSelectedSpell());
    if (_selectedSpell == Goldbox::Data::Spells::SP_NONE) {
        debug(0, "[COMBATSPELL] selection result had no selected spell");
        finishCast(false);
        return;
    }
    debug(0, "[COMBATSPELL] selected legacyIndex=%d spell=%u actor='%s'",
        result._intValue, (unsigned)_selectedSpell, _actor->name.c_str());
    finishCast(true);
}

void CombatSpellDialog::finishCast(bool actionComplete) {
    debug(0, "[COMBATSPELL] finishCast complete=%d selectedSpell=%u",
        (int)actionComplete, (unsigned)_selectedSpell);
    deactivate();
    if (_parent)
        g_events->postMenuResult(
            _parent->getName(), actionComplete,
            Common::KEYCODE_RETURN,
            (int)Combat::CombatSession::PA_CAST,
            Common::String(), true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
