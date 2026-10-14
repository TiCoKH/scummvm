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
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatSpellDialog::CombatSpellDialog()
    : Dialog("CombatSpell"), _actor(nullptr),
      _spellsMenu(nullptr) {
    _spellsMenu = new SpellsMenu("CombatSpellsMenu");
    subView(_spellsMenu);
}

CombatSpellDialog::~CombatSpellDialog() {
    delete _spellsMenu;
}

void CombatSpellDialog::beginCast(Goldbox::Data::PlayerCharacter *actor) {
    _actor = actor;
}

void CombatSpellDialog::activate() {
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
    if (!result._success) {
        finishCast(false);
        return;
    }

    if (!result._hasIntValue || !_actor) {
        finishCast(false);
        return;
    }

    const uint8 spellId = static_cast<uint8>(result._intValue);
    CombatView *view = dynamic_cast<CombatView *>(_parent);

    const bool consumed = Combat::castSpell(_actor, spellId, false, view);
    debug(0, "[SPELL] castSpell: spell %d consumed=%d", (int)spellId, (int)consumed);
    finishCast(consumed);
}

void CombatSpellDialog::finishCast(bool actionComplete) {
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
