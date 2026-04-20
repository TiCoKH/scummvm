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

    // Map the raw menu-list index (which includes separator rows) back to the
    // corresponding _spellEntries index via _menuIndexToEntry.
    const int menuIdx = _spellMenuList.currentSelection;
    if (menuIdx < 0 || menuIdx >= (int)_menuIndexToEntry.size()) {
        return;
    }
    const int entryIdx = _menuIndexToEntry[menuIdx];
    if (entryIdx < 0 || entryIdx >= (int)_spellEntries.size()) {
        // Separator row was somehow selected; ignore.
        return;
    }

    _lastSelection = entryIdx;
    _selectedLegacyIndex = _spellEntries[entryIdx].legacyIndex;
    _selectedSpell = _spellEntries[entryIdx].spellId;
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

    // Restore the cursor to the previously chosen entry.  _lastSelection is an
    // _spellEntries index; find the corresponding menu-item index (skip
    // separator rows which have _menuIndexToEntry value of -1).
    int startMenuIdx = 0;
    if (!_menuIndexToEntry.empty()) {
        for (int i = 0; i < (int)_menuIndexToEntry.size(); ++i) {
            if (_menuIndexToEntry[i] == _lastSelection) {
                startMenuIdx = i;
                break;
            }
        }
    }
    if (!_spellMenuList.items.empty()) {
        _spellMenuList.currentSelection = CLIP<int>(
            startMenuIdx, 0, (int)_spellMenuList.items.size() - 1);
    } else {
        _spellMenuList.currentSelection = 0;
    }
}

void SpellsMenu::buildSpellList() {
    _spellEntries.clear();
    _spellMenuList.items.clear();
    _menuIndexToEntry.clear();
    _spellMenuList.currentSelection = 0;

    if (!_character) {
        return;
    }

    // Phase 1: collect spell entries (no separators yet).
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

    // Phase 2: build _spellMenuList with interleaved level-separator rows,
    // mirroring the original SPELL_addToList level-change logic.
    // Separators are inactive (non-selectable) and rendered with headColor.
    const Common::Array<Goldbox::Data::Spells::SpellEntry> &spellData =
        Goldbox::Data::Rules::getSpellEntries();

    uint8 lastLevel = 0;
    for (int i = 0; i < (int)_spellEntries.size(); ++i) {
        const SpellListEntry &entry = _spellEntries[i];
        const uint spellIdx = (uint)entry.spellId;
        uint8 currentLevel = 0;
        if (spellIdx < spellData.size()) {
            currentLevel = spellData[spellIdx].spellLevel;
        }

        if (currentLevel != lastLevel) {
            // Insert a level-header separator row (not selectable).
            MenuItem sep;
            sep.text = buildLevelSeparatorLabel(entry.spellId);
            sep.active = false;
            sep.shortcut = 0;
            _spellMenuList.items.push_back(sep);
            _menuIndexToEntry.push_back(-1);
            lastLevel = currentLevel;
        }

        MenuItem item;
        item.text = formatSpellLine(entry);
        item.active = true;
        item.shortcut = 0;
        _spellMenuList.items.push_back(item);
        _menuIndexToEntry.push_back(i);
    }
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

    // Original SPELL_addToList prefixes pending-memorize spells (high-bit set
    // in mem_spells[]) with "*" to signal they are queued, not yet memorized.
    if (_location == SL_TO_BE_MEMORIZED) {
        if (entry.count > 0) {
            return Common::String::format("*%s (%u)", spellName.c_str(),
                (unsigned)entry.count);
        }
        return Common::String::format("*%s", spellName.c_str());
    }

    return spellName;
}

Common::String SpellsMenu::buildLevelSeparatorLabel(
        Goldbox::Data::Spells::Spells spellId) const {
    // Mirror original SPELL_addToList which copies the level label string from
    // SPRITE_ARRAY_5 indexed by sp_level * 41.  We reconstruct an equivalent
    // human-readable header from the spell metadata.
    Common::String levelText = Goldbox::Spells::getSpellLevelText(spellId);
    if (levelText.empty()) {
        return Common::String();
    }
    return Common::String::format("- %s -", levelText.c_str());
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
