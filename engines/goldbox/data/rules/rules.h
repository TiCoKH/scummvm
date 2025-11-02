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

#ifndef ENGINES_GOLDBOX_DATA_RULES_RULES_H
#define ENGINES_GOLDBOX_DATA_RULES_RULES_H

#include "common/scummsys.h"
#include "common/array.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/data/rules/rules_types.h"

#define MAX_CLASS_RACE 11
#define MAX_LEVEL 11
#define ALIGNMENTS_COUNT 9
#define MAX_ALIGN_CLASS 17

namespace Goldbox {
namespace Data {
namespace Rules {

struct AgeDefEntry {
    uint16 base;
    uint8  dices;
    uint8  sides;
};

struct thac0Bases {
    uint8 thac0[MAX_LEVEL];
};

struct ClassAlignmentDef {
    uint8 align_count;
    uint8 align_ids[ALIGNMENTS_COUNT];
};

struct RaceClassDef {
    uint8 class_count;
    uint8 class_ids[MAX_CLASS_RACE];
};


// Accessors for the active ruleset (per game). For now, we expose Poolrad directly.

// Race/Class/Alignment constraints
bool isClassAllowed(uint8 race, uint8 classId);
bool isAlignmentAllowed(uint8 classId, uint8 alignmentId);

// Progression and combat
int  thac0AtLevel(uint8 classId, uint8 level); // returns THAC0 for given class/level

// Retrieve the age definition row for a given race and base class index.
// baseClassIndex is a base-class selector (0..7 for Poolrad base classes).
// Returns a reference valid for the lifetime of the process.
const AgeDefEntry &getAgeDef(uint8 race, uint8 baseClassIndex);

// Low-level table hooks (for diagnostics/tools)
const ClassAlignmentDef *getAlignmentTable();
const RaceClassDef *getRaceClassTable();
const thac0Bases *getThac0Table();
const Common::Array< Common::Array<AgeDefEntry> > &getAgeDefs();

// Spells table accessor (Poolrad). Returns the full list of spell entries.
const Common::Array<Spells::SpellEntry> &getSpellEntries();

// Initial gold dice definition for base classes (0..7, see ClassADnD base order).
// Returns a reference to the DiceRoll describing how to roll starting gold
// for the given base class; out-of-range indexes return a zero roll {0,0}.
const DiceRoll &getInitGoldRoll(uint8 baseClassIndex);

// Hit Points dice definition per base class (0..7).
// Returns the DiceRoll describing the HP dice to roll per level for that base class;
// out-of-range indexes return a zero roll {0,0}.
const DiceRoll &getHPRoll(uint8 baseClassIndex);

// Constitution-based HP modifier accessor.
// Returns the base HP modifier for a given Constitution score (3..25). Values outside
// that range are clamped. Fighter classes may apply additional adjustments separately.
int8 conHPModifier(uint8 constitution);

// Ageing helpers
// Returns the age category thresholds for a given race (maps Dwarf..Human as 0..6).
const AgeCategories &getAgeCategoriesForRace(uint8 race);
// Returns the table of per-stat ageing effects (7 rows: Str, ExtStr, Int, Wis, Dex, Con, Cha).
const Common::Array<AgeingEffects> &getStatAgeingEffects();

// Race/gender stat bounds
// Returns the RaceStatMinMax row for a given race (maps Dwarf..Human as 0..6).
// Note: Monster race (0) maps to index 0 as a safe default; callers may choose to skip applying.
const RaceStatMinMax &getRaceStatMinMaxForRace(uint8 race);

// Class minimum stats accessor; returns the row for the given classId.
const ClassMinStats &getClassMinStats(uint8 classId);

// Enum counts (UI helpers)
uint8 classEnumCount();
uint8 alignmentEnumCount();

// Base thief skills table accessor (level is 1-based; clamps to last row)
const ThiefSkills &getThiefSkillsForLevel(uint8 level);

// Compute final thief skills (race + dexterity adjustments applied)
ThiefSkills computeThiefSkills(uint8 race, uint8 dexterity, uint8 thiefLevel);

// Returns the bit assigned to a base class (0..7), or 0 if out of range.
uint8 classItemLimitBit(uint8 baseClassIndex);

// OR of all bits for base classes with level > 0.
uint8 computeItemLimitMask(const Common::Array<uint8> &levels);

// Saving throws accessor: baseClassIndex 0..7, level 1..9 (clamped)
const SavingThrows &savingThrowsAt(uint8 baseClassIndex, uint8 level);

} // namespace Rules
} // namespace Data
} // namespace Goldbox

#endif // ENGINES_GOLDBOX_DATA_RULES_RULES_H
