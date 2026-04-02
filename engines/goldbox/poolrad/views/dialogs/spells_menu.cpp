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

#include "goldbox/poolrad/views/dialogs/spells_menu.h"

#include "goldbox/events.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/poolrad_spell_mapping.h"
#include "goldbox/spells/spell_metadata.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

SpellsMenu::SpellsMenu(const Common::String &name)
    : Dialog(name),
      _location(SL_IN_MEMORY),
      _action(SA_CAST),
      _character(nullptr),
      _verticalMenu(nullptr),
      _lastSelection(0),
      _selectedLegacyIndex(-1),
      _selectedSpell(Goldbox::Data::Spells::SP_NONE),
      _windowBottom(22) {
}

SpellsMenu::~SpellsMenu() {
    if (_verticalMenu) {
        _verticalMenu->setParent(nullptr);
        delete _verticalMenu;
        _verticalMenu = nullptr;
    }
}

void SpellsMenu::configure(SpellLocation location, SpellAction action) {
    _location = location;
    _action = action;
}

void SpellsMenu::activate() {
    Dialog::activate();

    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
        VmInterface::getSelectedCharacter()
    );
    _selectedLegacyIndex = -1;
    _selectedSpell = Goldbox::Data::Spells::SP_NONE;
    _selectedSpellName.clear();

    buildSpellList();
    buildPromptOptions();
    rebuildVerticalMenu();

    if (_verticalMenu) {
        _verticalMenu->activate();
    }
}

void SpellsMenu::deactivate() {
    if (_verticalMenu) {
        _verticalMenu->deactivate();
    }

    Dialog::deactivate();
}

bool SpellsMenu::msgKeypress(const KeypressMessage &msg) {
    if (!isActive()) {
        return false;
    }

    return View::msgKeypress(msg);
}

void SpellsMenu::draw() {
    if (!isActive() || !_character) {
        return;
    }

    const int bottom = getWindowBottom();
    Surface s = getSurface();

    drawWindow(1, 1, 38, 1);
    Goldbox::Poolrad::Data::PoolradCharacter *pc = _character;
    s.writeStringC(1, 1, 11, pc->name);
    s.writeString("'s");

    Common::String suffix = getLocationSuffix();
    Common::String heading = suffix.empty()
        ? "Spells"
        : Common::String::format("Spells %s", suffix.c_str());
    s.writeStringC(1 + pc->name.size() + 3, 1, 10, heading);

    drawWindow(1, 3, 38, bottom);

    if (_spellEntries.empty()) {
        s.writeStringC(2, 5, 10, "No spells available");
    }

    if (_verticalMenu) {
        _verticalMenu->draw();
    }
}

void SpellsMenu::handleMenuResult(const MenuResultMessage &result) {
    debug("SpellsMenu::handleMenuResult() success=%d key=%d hasInt=%d int=%d",
        (int)result._success, (int)result._keyCode, (int)result._hasIntValue,
        (int)(result._hasIntValue ? result._intValue : -1));

    if (!result._success) {
        if (result._keyCode == Common::KEYCODE_ESCAPE ||
                result._keyCode == Common::KEYCODE_e) {
            handleExit();
        }
        return;
    }

    if (_spellEntries.empty()) {
        handleExit();
        return;
    }

    const int selection = _spellMenuList.currentSelection;
    if (selection < 0 || selection >= (int)_spellEntries.size()) {
        return;
    }

    _lastSelection = selection;
    _selectedLegacyIndex = _spellEntries[selection].legacyIndex;
    _selectedSpell = _spellEntries[selection].spellId;
    _selectedSpellName = Goldbox::Spells::getSpellName(_selectedSpell);

    deactivate();

    if (_parent) {
        g_events->postMenuResult(_parent->getName(), true,
            result._keyCode, _selectedLegacyIndex, _selectedSpellName,
            true, true);
    }
}

void SpellsMenu::rebuildVerticalMenu() {
    _windowBottom = getWindowBottom();

    if (_verticalMenu) {
        detachDialog(_verticalMenu);
        delete _verticalMenu;
        _verticalMenu = nullptr;
    }

    VerticalMenuConfig menuConfig = {
        "Choose Spell:",
        &_horizontalMenuLabels,
        &_spellMenuList,
        13,
        10,
        15,
        1,
        5,
        38,
        _windowBottom,
        "",
        true
    };

    _verticalMenu = new VerticalMenu(getName() + "_Vertical", menuConfig);
    attachDialog(_verticalMenu);

    if (!_spellMenuList.items.empty()) {
        _spellMenuList.currentSelection = CLIP<int>(
            _lastSelection, 0, (int)_spellMenuList.items.size() - 1);
    } else {
        _spellMenuList.currentSelection = 0;
    }
}

void SpellsMenu::buildSpellList() {
    _spellEntries.clear();
    _spellMenuList.items.clear();
    _spellMenuList.currentSelection = 0;

    if (!_character) {
        return;
    }

    switch (_location) {
    case SL_IN_MEMORY:
        for (int i = 0; i < Goldbox::Poolrad::Data::POOLRAD_MEMORIZED_SIZE; ++i) {
            const uint8 value = _character->spells.memorizedSpells[i];
            if (value != 0 && value < 0x80) {
                appendSpellEntry(i,
                    Goldbox::Poolrad::Data::kPoolradSpellMapping[i], value);
            }
        }
        break;

    case SL_IN_SPELL_BOOK:
        for (int i = 0; i < Goldbox::Poolrad::Data::POOLRAD_KNOWN_SIZE; ++i) {
            if (_character->spells.knownSpells[i] != 0) {
                appendSpellEntry(i,
                    Goldbox::Poolrad::Data::kPoolradSpellMapping[i]);
            }
        }
        break;

    case SL_TO_CHOOSE_FROM: {
        const Common::Array<Goldbox::Data::Spells::SpellEntry> &entries =
            Goldbox::Data::Rules::getSpellEntries();
        for (int i = 0; i < Goldbox::Poolrad::Data::POOLRAD_KNOWN_SIZE; ++i) {
            if (_character->spells.knownSpells[i] != 0) {
                continue;
            }

            const Goldbox::Data::Spells::Spells spellId =
                Goldbox::Poolrad::Data::kPoolradSpellMapping[i];
            const uint spellIndex = (uint)spellId;
            if (spellIndex >= entries.size()) {
                continue;
            }

            if (getAvailableSlotsForSpell(entries[spellIndex]) != 0) {
                appendSpellEntry(i, spellId);
            }
        }
        break;
    }

    case SL_TO_BE_MEMORIZED:
        for (int i = 0; i < Goldbox::Poolrad::Data::POOLRAD_MEMORIZED_SIZE; ++i) {
            const uint8 value = _character->spells.memorizedSpells[i];
            if (value > 0x7f) {
                appendSpellEntry(i,
                    Goldbox::Poolrad::Data::kPoolradSpellMapping[i],
                    (uint8)(value & 0x7f));
            }
        }
        break;

    case SL_ON_SCROLL:
    case SL_ON_SCROLLS:
    case SL_TO_BE_SCRIBED:
        // TODO: Build these from item spell payloads once scroll spell mapping
        // is wired into the inventory system.
        break;
    }

    Common::Array<Common::String> labels;
    for (uint i = 0; i < _spellEntries.size(); ++i) {
        labels.push_back(formatSpellLine(_spellEntries[i]));
    }
    _spellMenuList.generateMenuItems(labels, false);
}

void SpellsMenu::buildPromptOptions() {
    _horizontalMenuLabels.clear();

    if (!_spellEntries.empty()) {
        Common::String actionLabel = getActionLabel();
        if (!actionLabel.empty()) {
            _horizontalMenuLabels.push_back(actionLabel);
        }
    }

    _horizontalMenuLabels.push_back("Exit");
}

void SpellsMenu::appendSpellEntry(int legacyIndex,
        Goldbox::Data::Spells::Spells spellId, uint8 count) {
    _spellEntries.push_back(SpellListEntry(legacyIndex, spellId, count));
}

void SpellsMenu::handleExit() {
    _selectedLegacyIndex = -1;
    _selectedSpell = Goldbox::Data::Spells::SP_NONE;
    _selectedSpellName.clear();

    deactivate();

    if (_parent) {
        g_events->postMenuResult(_parent->getName(), false,
            Common::KEYCODE_ESCAPE, 0, Common::String(), true, false);
    }
}

int SpellsMenu::getWindowBottom() const {
    return (_action == SA_MEMORIZE) ? 15 : 22;
}

uint8 SpellsMenu::getAvailableSlotsForSpell(
        const Goldbox::Data::Spells::SpellEntry &entry) const {
    if (!_character || entry.spellLevel == 0 || entry.spellLevel > 3) {
        return 0;
    }

    switch (entry.spellClass) {
    case Goldbox::Data::Spells::SC_CLERIC:
        switch (entry.spellLevel) {
        case 1:
            return _character->spellSlots.cleric.level1;
        case 2:
            return _character->spellSlots.cleric.level2;
        case 3:
            return _character->spellSlots.cleric.level3;
        default:
            return 0;
        }

    case Goldbox::Data::Spells::SC_MAGICUSER:
        switch (entry.spellLevel) {
        case 1:
            return _character->spellSlots.magicUser.level1;
        case 2:
            return _character->spellSlots.magicUser.level2;
        case 3:
            return _character->spellSlots.magicUser.level3;
        default:
            return 0;
        }

    default:
        return 0;
    }
}

Common::String SpellsMenu::getLocationSuffix() const {
    switch (_location) {
    case SL_IN_MEMORY:
        return "in Memory";
    case SL_IN_SPELL_BOOK:
        return "in Spell Book";
    case SL_ON_SCROLL:
        return "on Scroll";
    case SL_ON_SCROLLS:
        return "on Scrolls";
    case SL_TO_CHOOSE_FROM:
        return "to choose from";
    case SL_TO_BE_MEMORIZED:
        return "to be memorized";
    case SL_TO_BE_SCRIBED:
        return "to be scribed";
    default:
        return Common::String();
    }
}

Common::String SpellsMenu::getActionLabel() const {
    switch (_action) {
    case SA_CAST:
        return "Cast";
    case SA_MEMORIZE:
        return "Memorize";
    case SA_SCRIBE:
        return "Scribe";
    case SA_LEARN:
        return "Learn";
    default:
        return Common::String();
    }
}

Common::String SpellsMenu::formatSpellLine(const SpellListEntry &entry) const {
    Common::String spellName = Goldbox::Spells::getSpellName(entry.spellId);

    if (_location == SL_IN_MEMORY && entry.count > 1) {
        return Common::String::format("%s x%u", spellName.c_str(),
            (unsigned)entry.count);
    }

    if (_location == SL_TO_BE_MEMORIZED && entry.count > 0) {
        return Common::String::format("%s (%u)", spellName.c_str(),
            (unsigned)entry.count);
    }

    return spellName;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
