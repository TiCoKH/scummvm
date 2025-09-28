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

#include "common/array.h"
#include "engines/goldbox/data/rules/rules.h"

namespace Goldbox {
namespace Data {
namespace Rules {

// Alignment table per class: which alignments are allowed for each class.
// Index by ClassADnD (0..N). Alignments are enum ids (0..8) in align_ids.
static const Common::Array<ClassAlignmentDef> kClassAlignment = {
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Cleric
    {5, {1, 3, 4, 5, 7, 0, 0, 0, 0}}, // Druid
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Fighter
    {1, {0, 0, 0, 0, 0, 0, 0, 0, 0}}, // Paladin
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Ranger
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Magic-User
    {7, {1, 2, 3, 4, 5, 7, 8, 0, 0}}, // Thief
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Monk
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Cleric/Fighter
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Cleric/Fighter/Magic-User
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Cleric/Ranger
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Cleric/Magic-User
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Cleric/Thief
    {9, {0, 1, 2, 3, 4, 5, 6, 7, 8}}, // Fighter/Magic-User
    {7, {1, 2, 3, 4, 5, 7, 8, 0, 0}}, // Fighter/Thief
    {7, {1, 2, 3, 4, 5, 7, 8, 0, 0}}, // Fighter/Magic-User/Thief
    {7, {1, 2, 3, 4, 5, 7, 8, 0, 0}}  // Magic-User/Thief
};

// Race -> allowed classes.
static const Common::Array<RaceClassDef> kRaceClasses = {
    {3, {2, 6, 14}},                              // race Dwarf
    {7, {2, 5, 6, 13, 14, 15, 16}},               // race Elf
    {3, {2, 6, 14}},                              // race Gnome
    {11, {0, 2, 5, 6, 8, 9, 11, 13, 14, 15, 16}}, // race Half-Elf
    {3, {2, 6, 14}},                              // race Halfling
    {0, { }},                                     // race Half-Orc none playable race in Poolrad
    {4, {0, 2, 5, 6}}                             // race Human can't be multiclassed in Poolrad
};

// THAC0 by level per class; 1..10 stored, index 0 unused.
static const Common::Array<thac0Bases> kThac0ByClass = {
    { {40, 40, 40, 40, 42, 42, 42, 44, 44, 44, 46} }, // Class 0 - Cleric
    { {40, 40, 40, 40, 42, 42, 42, 44, 44, 44, 46} }, // class 1 - Druid
    { {40, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49} }, // class 2 - Fighter
    { {40, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49} }, // class 3 - Paladin
    { {40, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49} }, // class 4 - Rangrer
    { {40, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41} }, // class 5 - Magic-User
    { {40, 40, 40, 40, 40, 41, 41, 41, 41, 44, 44} }, // class 6 - Thief
    { {40, 40, 40, 40, 42, 42, 42, 44, 44, 44, 46} }  // class 7 - Monk
};

// Age definitions
static const Common::Array< Common::Array<AgeDefEntry> > kAgeDefs = {
    // race 0..N, base classes 0..6
    // Cleric, Druid, Fighter, Paladin, Ranger, Magic-User, Thief 
    // Dwarf
    { {250, 2, 20}, {0, 0, 0}, {40, 5, 4}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {75, 3, 6} },
    // Elf
    { {650, 10, 10}, {0, 0, 0}, {130, 5, 6}, {0, 0, 0}, {0, 0, 0}, {150, 5, 6}, {100, 5, 6} },
    // Gnome
    { {300, 3, 12}, {0, 0, 0}, {60, 5, 4}, {0, 0, 0},  {0, 0, 0}, {100, 2, 12}, {80, 5, 4} },
    // Half-Elf
    { {40, 2, 4}, {0, 0, 0}, {22, 3, 4}, {0, 0, 0}, {0, 0, 0}, {30, 2, 8}, {22, 3, 8} },
    // Halfling
    { {0, 0, 0}, {0, 0, 0}, {20, 3, 4}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {40, 2, 4} },
    // Half-Orc none playable
    { {20, 1, 4}, {0, 0, 0}, {13, 1, 4}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {20, 2, 4} },
    // Human
    { {18, 1, 4}, {18, 1, 4}, {15, 1, 4}, {17, 1, 4}, {20, 1, 4}, {24, 2, 4}, {18, 1, 4} }
};

bool isClassAllowed(uint8 race, uint8 classId) {
    if (race >= kRaceClasses.size())
        return false;
    const RaceClassDef &rc = kRaceClasses[race];
    for (uint8 i = 0; i < rc.class_count && i < ARRAYSIZE(rc.class_ids); ++i) {
        if (rc.class_ids[i] == classId)
            return true;
    }
    return false;
}

bool isAlignmentAllowed(uint8 classId, uint8 alignmentId) {
    if (classId >= kClassAlignment.size())
        return false;
    const ClassAlignmentDef &ca = kClassAlignment[classId];
    for (uint8 i = 0; i < ca.align_count && i < ARRAYSIZE(ca.align_ids); ++i) {
        if (ca.align_ids[i] == alignmentId)
            return true;
    }
    return false;
}

int thac0AtLevel(uint8 classId, uint8 level) {
    if (classId >= kThac0ByClass.size())
        return 20;
    if (level > 10)
        level = 10;
    return kThac0ByClass[classId].thac0[level];
}

uint16 computeStartingAge(uint8 race, uint8 baseClassIndex,
                          uint16 (*rng)(uint8 dice, uint8 sides)) {
    if (race >= kAgeDefs.size())
        race = 0;
    if (baseClassIndex >= kAgeDefs[race].size())
        baseClassIndex = 0;
    const AgeDefEntry &def = kAgeDefs[race][baseClassIndex];
    uint16 extra = 0;
    if (rng) extra = rng(def.dices, def.sides);
    return def.base + extra;
}

const ClassAlignmentDef *getAlignmentTable() { return kClassAlignment.data(); }
const RaceClassDef *getRaceClassTable() { return kRaceClasses.data(); }
const thac0Bases *getThac0Table() { return kThac0ByClass.data(); }
const Common::Array< Common::Array<AgeDefEntry> > &getAgeDefs() { return kAgeDefs; }

uint8 classEnumCount() {
    // For now, return the larger of the local tables or a conservative default the UI uses (18)
    uint8 sz = (uint8)kClassAlignment.size();
    if (sz < 18) return 18;
    return sz;
}

uint8 alignmentEnumCount() {
    // Alignments are typically 9 (LG..CE). Keep as 9 unless tables suggest more.
    return 9;
}

} // namespace Rules
} // namespace Data
} // namespace Goldbox
