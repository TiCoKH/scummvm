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

#define MAX_CLASS_RACE 11
#define MAX_LEVEL 11
#define ALIGNMENTS_COUNT 9
#define MAX_ALIGN_CLASS 17

namespace Goldbox {
namespace Data {

namespace Rules {

struct AgeDefEntry {
    uint16 base;
    uint8  sides;
    uint8  dices;
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

// Age calculation; baseClassIndex is a base-class selector (0..6). Multiclass handling TBD.
uint16 computeStartingAge(uint8 race, uint8 baseClassIndex,
                          uint16 (*rng)(uint8 dice, uint8 sides));

// Low-level table hooks (for diagnostics/tools)
const ClassAlignmentDef *getAlignmentTable();
const RaceClassDef *getRaceClassTable();
const thac0Bases *getThac0Table();
const Common::Array< Common::Array<AgeDefEntry> > &getAgeDefs();

// Enum counts (UI helpers)
uint8 classEnumCount();
uint8 alignmentEnumCount();

} // namespace Rules
} // namespace Data
} // namespace Goldbox

#endif // ENGINES_GOLDBOX_DATA_RULES_RULES_H
