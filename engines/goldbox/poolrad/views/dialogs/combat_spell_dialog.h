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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_SPELL_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_SPELL_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/spells_menu.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * DIALOG_CastSpell — combat spell-selection and scheduling interaction.
 *
 * Mirrors COMBAT_CastSpell:
 *   1. Opens SpellsMenu (SL_IN_MEMORY / SA_CAST) for spell selection.
 *   2. On selection, stores the spell id; CombatView then calls
 *      Combat::castSpell() (combat_turn.h) outside the event handler so
 *      the cast can aim interactively (targeting blocks on the view).
 *   3. Posts MenuResultMessage to parent: _success=true with the selection,
 *      or _success=false on cancel.
 */
class CombatSpellDialog : public Dialog {
public:
    CombatSpellDialog();
    ~CombatSpellDialog() override;

    /** Called by CombatView before attaching. */
    void beginCast(Goldbox::Data::PlayerCharacter *actor);

    /** Spell chosen by the player (0 when cancelled). */
    uint8 getSelectedSpell() const { return _selectedSpell; }

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    Goldbox::Data::PlayerCharacter *_actor;
    uint8 _selectedSpell;

    SpellsMenu *_spellsMenu;

    void finishCast(bool actionComplete);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_SPELL_DIALOG_H
