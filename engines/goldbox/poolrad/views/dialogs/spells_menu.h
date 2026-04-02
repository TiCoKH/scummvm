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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_SPELLS_MENU_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_SPELLS_MENU_H

#include "common/array.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {
class PoolradCharacter;
}
namespace Views {
namespace Dialogs {

/**
 * Spell selection dialog modeled after the original DIALOG_Spells /
 * DIALOG_SpellSelector pair.
 *
 * Legacy naming alignment:
 * - first DIALOG_Spells parameter  -> SpellLocation
 * - second DIALOG_Spells parameter -> SpellAction
 * - spell_dat buffer               -> selection state / legacy index buffer
 * - return value                   -> legacy spell table index (0..54)
 */
class SpellsMenu : public Dialog {
public:
    enum SpellLocation {
        SL_IN_MEMORY = 0,
        SL_IN_SPELL_BOOK = 1,
        SL_ON_SCROLL = 2,
        SL_ON_SCROLLS = 3,
        SL_TO_CHOOSE_FROM = 4,
        SL_TO_BE_MEMORIZED = 5,
        SL_TO_BE_SCRIBED = 6
    };

    enum SpellAction {
        SA_NONE = 0,
        SA_CAST = 1,
        SA_MEMORIZE = 2,
        SA_SCRIBE = 3,
        SA_LEARN = 4
    };

    struct SpellListEntry {
        int legacyIndex;
        Goldbox::Data::Spells::Spells spellId;
        uint8 count;

        SpellListEntry()
            : legacyIndex(-1), spellId(Goldbox::Data::Spells::SP_NONE),
              count(0) {
        }

        SpellListEntry(int index, Goldbox::Data::Spells::Spells spell,
                uint8 spellCount)
            : legacyIndex(index), spellId(spell), count(spellCount) {
        }
    };

    SpellsMenu(const Common::String &name = "SpellsMenu");
    ~SpellsMenu() override;

    void configure(SpellLocation location, SpellAction action);

    void activate() override;
    void deactivate() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void handleMenuResult(const MenuResultMessage &result) override;

    bool hasAvailableSpells() const { return !_spellEntries.empty(); }
    int getSelectedLegacyIndex() const { return _selectedLegacyIndex; }
    Goldbox::Data::Spells::Spells getSelectedSpell() const {
        return _selectedSpell;
    }
    Common::String getSelectedSpellName() const { return _selectedSpellName; }

private:
    SpellLocation _location;
    SpellAction _action;
    Goldbox::Poolrad::Data::PoolradCharacter *_character;
    Goldbox::MenuItemList _spellMenuList;
    Common::Array<Common::String> _horizontalMenuLabels;
    Common::Array<SpellListEntry> _spellEntries;
    VerticalMenu *_verticalMenu;
    int _lastSelection;
    int _selectedLegacyIndex;
    Goldbox::Data::Spells::Spells _selectedSpell;
    Common::String _selectedSpellName;
    int _windowBottom;

    void rebuildVerticalMenu();
    void buildSpellList();
    void buildPromptOptions();
    void appendSpellEntry(int legacyIndex,
        Goldbox::Data::Spells::Spells spellId, uint8 count = 0);
    void handleExit();

    int getWindowBottom() const;
    uint8 getAvailableSlotsForSpell(
        const Goldbox::Data::Spells::SpellEntry &entry) const;
    Common::String getLocationSuffix() const;
    Common::String getActionLabel() const;
    Common::String formatSpellLine(const SpellListEntry &entry) const;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SPELLS_MENU_H
