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
      _selectedSpell(Goldbox::Data::Spells::SP_NONE) {

    _menuConfig.promptTxt = "Choose Spell: ";
    _menuConfig.promptOptions = &_horizontalMenuLabels;
    _menuConfig.menuItemList = &_spellMenuList;
    _menuConfig.headColor = 13;
    _menuConfig.textColor = 10;
    _menuConfig.selectColor = 15;
    _menuConfig.xStart = 1;
    _menuConfig.yStart = 5;
    _menuConfig.xEnd = 38;
    _menuConfig.yEnd = 22;
    _menuConfig.title = "";
    _menuConfig.asHeader = true;

    _verticalMenu = new VerticalMenu(name + "_Vertical", _menuConfig);
    subView(_verticalMenu);
    _verticalMenu->deactivate();
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
    debug(3, "SpellsMenu::activate() name='%s' location=%d action=%d",
        getName().c_str(), (int)_location, (int)_action);
    Dialog::activate();

    _character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
        VmInterface::getSelectedCharacter()
    );
    _selectedLegacyIndex = -1;
    _selectedSpell = Goldbox::Data::Spells::SP_NONE;
    _selectedSpellName.clear();

    buildSpellList();
    buildPromptOptions();

    if (_verticalMenu) {
        _verticalMenu->_hMenuList.items.clear();
        _verticalMenu->_hMenuList.generateMenuItems(_horizontalMenuLabels, true);
        _verticalMenu->rebuild(&_spellMenuList, "");

        // Restore cursor to previously chosen entry, skipping separators.
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

    // Header window: character name + "Spells <suffix>"
    drawWindow(1, 1, 38, 1);
    Goldbox::Poolrad::Data::PoolradCharacter *pc = _character;
    s.writeStringC(1, 1, 11, pc->name);
    s.writeString("'s");

    Common::String suffix = getLocationSuffix();
    Common::String heading = suffix.empty()
        ? "Spells"
        : Common::String::format("Spells %s", suffix.c_str());
    s.writeStringC(1 + pc->name.size() + 3, 1, 10, heading);

    // Content window
    drawWindow(1, 3, 38, bottom);

    if (_spellEntries.empty()) {
        s.writeStringC(2, 5, 10, "No spells available");
    }

    if (_verticalMenu) {
        _verticalMenu->draw();
    }
}

void SpellsMenu::handleMenuResult(const MenuResultMessage &result) {
    debug(7, "SpellsMenu::handleMenuResult() success=%d key=%d hasInt=%d int=%d",
        (int)result._success, (int)result._keyCode, (int)result._hasIntValue,
        (int)(result._hasIntValue ? result._intValue : -1));

    // Exit when cancelled OR when 'E' was chosen from the horizontal prompt
    // (VerticalMenu posts success=true with key=KEYCODE_e for its Exit option).
    if (!result._success ||
            result._keyCode == Common::KEYCODE_ESCAPE ||
            result._keyCode == Common::KEYCODE_e) {
        handleExit();
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

void SpellsMenu::buildSpellList() {
    _spellEntries.clear();
    _spellMenuList.items.clear();
    _menuIndexToEntry.clear();
    _spellMenuList.currentSelection = 0;

    if (!_character) {
        return;
    }

    // Phase 1: collect spell entries.
    // memorizedSpells[0..20]: each slot holds a spell ID (1-55). Bit 7 set =
    // pending (not yet memorized). Value 0 = empty. One list entry per slot.
    // knownSpells[0..54]: non-zero = spell known. Index i maps to spell enum
    // via kPoolradSpellMapping[i].
    switch (_location) {
    case SL_IN_MEMORY:
        for (int i = 0; i < Goldbox::Poolrad::Data::POOLRAD_MEMORIZED_SIZE; ++i) {
            const uint8 value = _character->spells.memorizedSpells[i];
            debug(7, "SpellsMenu::buildSpellList() SL_IN_MEMORY slot[%d] = 0x%02x",
                i, (unsigned)value);
            if (value != 0 && value < 0x80) {
                // value is 1-based spell ID; map to Spells enum
                const int spellIdx = (value & 0x7f) - 1;
                if (spellIdx >= 0 && spellIdx < Goldbox::Poolrad::Data::POOLRAD_KNOWN_SIZE) {
                    debug(7, "  -> spellIdx=%d enum=%d name=%s", spellIdx,
                        (int)Goldbox::Poolrad::Data::kPoolradSpellMapping[spellIdx],
                        Goldbox::Spells::getSpellName(
                            Goldbox::Poolrad::Data::kPoolradSpellMapping[spellIdx]).c_str());
                    appendSpellEntry(spellIdx,
                        Goldbox::Poolrad::Data::kPoolradSpellMapping[spellIdx]);
                }
            }
        }
        break;

    case SL_TO_BE_MEMORIZED:
        for (int i = 0; i < Goldbox::Poolrad::Data::POOLRAD_MEMORIZED_SIZE; ++i) {
            const uint8 value = _character->spells.memorizedSpells[i];
            debug(7, "SpellsMenu::buildSpellList() SL_TO_BE_MEMORIZED slot[%d] = 0x%02x",
                i, (unsigned)value);
            if (value > 0x7f) {
                // Pending memorize: bit 7 set, spell ID = value & 0x7f
                const int spellIdx = (value & 0x7f) - 1;
                if (spellIdx >= 0 && spellIdx < Goldbox::Poolrad::Data::POOLRAD_KNOWN_SIZE) {
                    debug(7, "  -> spellIdx=%d enum=%d name=%s", spellIdx,
                        (int)Goldbox::Poolrad::Data::kPoolradSpellMapping[spellIdx],
                        Goldbox::Spells::getSpellName(
                            Goldbox::Poolrad::Data::kPoolradSpellMapping[spellIdx]).c_str());
                    appendSpellEntry(spellIdx,
                        Goldbox::Poolrad::Data::kPoolradSpellMapping[spellIdx]);
                }
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

    case SL_ON_SCROLL:
    case SL_ON_SCROLLS:
    case SL_TO_BE_SCRIBED:
        // TODO: Build from item spell payloads once scroll spell mapping
        // is wired into the inventory system.
        break;
    }

    // Phase 2: build _spellMenuList with interleaved level-separator rows.
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
    debug(3, "SpellsMenu::handleExit() name='%s' parent='%s'",
        getName().c_str(),
        _parent ? _parent->getName().c_str() : "(null)");
    _selectedLegacyIndex = -1;
    _selectedSpell = Goldbox::Data::Spells::SP_NONE;
    _selectedSpellName.clear();

    deactivate();

    if (_parent) {
        debug(3, "SpellsMenu::handleExit() -> posting ESCAPE/false to '%s'",
            _parent->getName().c_str());
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

    // Original SPELL_addToList prefixes with " " (space) for memorized or
    // "*" for pending-memorize. All spell names start 2 chars from border.
    if (_location == SL_TO_BE_MEMORIZED) {
        return Common::String::format(" *%s", spellName.c_str());
    }

    return Common::String::format("  %s", spellName.c_str());
}

Common::String SpellsMenu::buildLevelSeparatorLabel(
        Goldbox::Data::Spells::Spells spellId) const {
    // Original uses SPELL_LEVEL_HEADERS[level] which contains strings like
    // "1st LEVEL", "2nd LEVEL", etc. These are stored in YML as stats.levels.N.
    const Goldbox::Data::Spells::SpellEntry *entry = nullptr;
    const Common::Array<Goldbox::Data::Spells::SpellEntry> &entries =
        Goldbox::Data::Rules::getSpellEntries();
    const uint idx = (uint)spellId;
    if (idx < entries.size()) {
        entry = &entries[idx];
    }

    if (!entry || entry->spellLevel == 0) {
        return Common::String();
    }

    Common::String key = Common::String::format("stats.levels.%u",
        (unsigned)entry->spellLevel);
    Common::String text = Goldbox::VmInterface::getString(key);
    if (text.empty()) {
        // Fallback if YML key not found
        text = Common::String::format("Level %u", (unsigned)entry->spellLevel);
    }
    return text;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
