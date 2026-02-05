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
 */

#include "goldbox/data/spells/spell_book.h"
#include "goldbox/data/rules/rules.h"
#include "common/debug.h"

namespace Goldbox {
namespace Data {
namespace Spells {

SpellBook::SpellBook() {
}

bool SpellBook::isKnown(Spells spell) const {
	if (!_spells.contains(spell))
		return false;
	return _spells[spell].known;
}

uint8 SpellBook::getMemorized(Spells spell) const {
	if (!_spells.contains(spell))
		return 0;
	return _spells[spell].memorized;
}

bool SpellBook::hasAnyMemorized() const {
	for (Common::HashMap<Spells, SpellEntry>::const_iterator it = _spells.begin();
	     it != _spells.end(); ++it) {
		if (it->_value.memorized > 0)
			return true;
	}
	return false;
}

void SpellBook::setKnown(Spells spell, bool known) {
	if (!_spells.contains(spell)) {
		_spells[spell] = SpellEntry();
	}
	_spells[spell].known = known;
}

void SpellBook::setMemorized(Spells spell, uint8 count) {
	if (!_spells.contains(spell)) {
		_spells[spell] = SpellEntry();
	}
	_spells[spell].memorized = count;
}

void SpellBook::clearMemorized(Spells spell) {
	if (_spells.contains(spell)) {
		_spells[spell].memorized = 0;
	}
}

void SpellBook::clearAllMemorized() {
	for (Common::HashMap<Spells, SpellEntry>::iterator it = _spells.begin();
	     it != _spells.end(); ++it) {
		it->_value.memorized = 0;
	}
}

void SpellBook::loadFromLegacyArrays(const uint8 *memorizedArray, int memorizedSize,
                                     const uint8 *knownArray, int knownSize,
                                     const Spells *spellMapping, int mappingSize) {
	_spells.clear();

	debug("SpellBook::loadFromLegacyArrays - memorizedSize=%d knownSize=%d mappingSize=%d",
	      memorizedSize, knownSize, mappingSize);

	// Load known spells from legacy array
	// Format: array index maps to spell ID via spellMapping
	// Value of 1 means known, 0 means unknown
	int knownCount = 0;
	for (int i = 0; i < knownSize && i < mappingSize; i++) {
		if (knownArray[i] != 0) {
			Spells spell = spellMapping[i];
			setKnown(spell, true);
			debug("  Known spell at index %d: spell ID=%u value=%u", i, (unsigned)spell, (unsigned)knownArray[i]);
			knownCount++;
		}
	}

	// Load memorized spells from legacy array
	// Format: array index maps to spell ID via spellMapping
	// Value indicates count of memorized instances
	int memorizedCount = 0;
	for (int i = 0; i < memorizedSize && i < mappingSize; i++) {
		if (memorizedArray[i] != 0) {
			Spells spell = spellMapping[i];
			setMemorized(spell, memorizedArray[i]);
			// Ensure spell is also marked as known
			setKnown(spell, true);
			debug("  Memorized spell at index %d: spell ID=%u count=%u", i, (unsigned)spell, (unsigned)memorizedArray[i]);
			memorizedCount++;
		}
	}

	debug("SpellBook::loadFromLegacyArrays - Loaded %d known spells, %d memorized spells, total unique spells=%u",
	      knownCount, memorizedCount, _spells.size());
}

void SpellBook::saveToLegacyArrays(uint8 *memorizedArray, int memorizedSize,
                                   uint8 *knownArray, int knownSize,
                                   const Spells *spellMapping, int mappingSize) const {
	debug("SpellBook::saveToLegacyArrays - memorizedSize=%d knownSize=%d mappingSize=%d totalSpells=%u",
	      memorizedSize, knownSize, mappingSize, _spells.size());

	// Clear output arrays
	for (int i = 0; i < memorizedSize; i++) {
		memorizedArray[i] = 0;
	}
	for (int i = 0; i < knownSize; i++) {
		knownArray[i] = 0;
	}

	// Write spell data to legacy arrays using reverse mapping
	int knownWritten = 0;
	int memorizedWritten = 0;
	for (int i = 0; i < mappingSize; i++) {
		Spells spell = spellMapping[i];

		if (_spells.contains(spell)) {
			const SpellEntry &entry = _spells[spell];

			// Write known flag
			if (i < knownSize) {
				knownArray[i] = entry.known ? 1 : 0;
				if (entry.known) {
					debug("  Writing known spell at index %d: spell ID=%u", i, (unsigned)spell);
					knownWritten++;
				}
			}

			// Write memorized count
			if (i < memorizedSize) {
				memorizedArray[i] = entry.memorized;
				if (entry.memorized > 0) {
					debug("  Writing memorized spell at index %d: spell ID=%u count=%u", i, (unsigned)spell, (unsigned)entry.memorized);
					memorizedWritten++;
				}
			}
		}
	}

	debug("SpellBook::saveToLegacyArrays - Wrote %d known spells, %d memorized spells",
	      knownWritten, memorizedWritten);
}

uint SpellBook::countKnownByClass(SpellClass spellClass) const {
	uint count = 0;
	const Common::Array<Goldbox::Data::Spells::SpellEntry> &allSpells = Goldbox::Data::Rules::getSpellEntries();

	for (Common::HashMap<Spells, SpellBook::SpellEntry>::const_iterator it = _spells.begin();
	     it != _spells.end(); ++it) {
		if (it->_value.known) {
			// Look up the spell's class from the master spell table
			uint spellId = (uint)it->_key;
			if (spellId < allSpells.size() && allSpells[spellId].spellClass == spellClass) {
				count++;
			}
		}
	}
	return count;
}

uint SpellBook::countMemorizedByClass(SpellClass spellClass) const {
	uint count = 0;
	const Common::Array<Goldbox::Data::Spells::SpellEntry> &allSpells = Goldbox::Data::Rules::getSpellEntries();

	for (Common::HashMap<Spells, SpellBook::SpellEntry>::const_iterator it = _spells.begin();
	     it != _spells.end(); ++it) {
		if (it->_value.memorized > 0) {
			// Look up the spell's class from the master spell table
			uint spellId = (uint)it->_key;
			if (spellId < allSpells.size() && allSpells[spellId].spellClass == spellClass) {
				count += it->_value.memorized;
			}
		}
	}
	return count;
}

bool SpellBook::hasMemorizedOfClass(SpellClass spellClass) const {
	const Common::Array<Goldbox::Data::Spells::SpellEntry> &allSpells = Goldbox::Data::Rules::getSpellEntries();

	for (Common::HashMap<Spells, SpellBook::SpellEntry>::const_iterator it = _spells.begin();
	     it != _spells.end(); ++it) {
		if (it->_value.memorized > 0) {
			// Look up the spell's class from the master spell table
			uint spellId = (uint)it->_key;
			if (spellId < allSpells.size() && allSpells[spellId].spellClass == spellClass) {
				return true;
			}
		}
	}
	return false;
}

} // namespace Spells
} // namespace Data
} // namespace Goldbox
