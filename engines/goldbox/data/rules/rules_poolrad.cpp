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
	{0, { }},                                     // race Monster placeholder not used
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

static const Common::Array<ThiefSkills> kThiefSkills = {
	// ThiefSkills in %
	{ 30, 25, 20, 15, 10, 10, 85, 0 }, // Level 1
	{ 35, 29, 25, 21, 15, 10, 86, 0 }, // Level 2
	{ 40, 33, 30, 27, 20, 15, 87, 0 }, // Level 3
	{ 45, 37, 35, 33, 25, 15, 88, 20 }, // Level 4
	{ 50, 42, 40, 40, 31, 20, 90, 25 }, // Level 5
	{ 55, 47, 45, 47, 37, 20, 92, 30 }, // Level 6
	{ 60, 52, 50, 55, 43, 25, 94, 35 }, // Level 7
	{ 65, 57, 55, 62, 49, 25, 96, 40 }, // Level 8
	{ 70, 62, 60, 70, 56, 30, 98, 45 }  // Level 9
};

const ThiefSkills &getThiefSkillsForLevel(uint8 level) {
	// Clamp to last defined progression row.
	if (level >= (uint8)(sizeof(kThiefSkills) / sizeof(kThiefSkills[0])))
		level = (uint8)(sizeof(kThiefSkills) / sizeof(kThiefSkills[0]) - 1);
	return kThiefSkills[level];
}

static const Common::Array<ThiefRaceAdjustments> kThiefRaceAdjustments = {
	{  0, 10, 15,  0,  0,  0, -10,  -5 }, // Dwarf
	{  5, -5,  0,  5, 10,  5,   0,   0 }, // Elf
	{  0,  5, 10,  5,  5, 10, -15,   0 }, // Gnome
	{ 10,  0,  0,  0,  5,  0,   0,   0 }, // Half-Elf
	{  5,  5,  5, 10, 15,  5, -15,  -5 }, // Halfling
	{ -5,  5,  5,  0,  0,  5,   5, -10 }, // Half-Orc
	{  0,  0,  0,  0,  0,  0,   0,   0 }  // Human
};

static const Common::Array<ThiefDexterityAdjustments> kThiefDexterityAdjustments = {
	{ -15, -10, -10, -20, -10 }, // 9
	{ -19,  -5, -10, -15,  -5 }, // 10
	{  -5,   0,  -5, -10,   0 }, // 11
	{   0,   0,   0,  -5,   0 }, // 12
	{   0,   0,   0,   0,   0 }, // 13
	{   0,   0,   0,   0,   0 }, // 14
	{   0,   0,   0,   0,   0 }, // 15
	{   0,  -5,   0,   0,   0 }, // 16
	{   5,  10,   0,   5,   5 }, // 17
	{  10,  15,   5,  10,  10 }, // 18
	{  15,  20,  10,  12,  12 }  // 19
};

static const Common::Array<RaceStatMinMax> kRaceStatMinMax = {
	// RaceStatMinMax
	// strMinM, strMinF, strMaxM, strMaxF, extStrMaxM, extStrMaxF,
	// intMin, intMax, wisMin, wisMax, dexMin, dexMax, conMin, conMax, chaMin, chaMax
	{ 8, 8, 18, 17,  99,  0, 3, 18, 3, 18, 3, 17, 12, 19, 3, 16 }, // Dwarf
	{ 3, 3, 18, 16,  75,  0, 8, 18, 3, 18, 7, 19,  6, 18, 8, 18 }, // Elf
	{ 6, 6, 18, 15,  50,  0, 7, 18, 3, 18, 3, 18,  8, 18, 3, 18 }, // Gnome
	{ 3, 3, 18, 17,  90,  0, 4, 18, 3, 18, 6, 18,  6, 18, 3, 18 }, // Half-Elf
	{ 6, 6, 17, 14,   0,  0, 6, 18, 3, 17, 8, 18, 10, 19, 3, 18 }, // Halfling
	{ 6, 6, 18, 18,  99, 75, 3, 17, 3, 14, 3, 17, 13, 19, 3, 12 }, // Half-Orc none playable
	{ 3, 3, 18, 18, 100, 50, 3, 18, 3, 18, 3, 18,  3, 18, 3, 18 }  // Human
};

static const Common::Array<AgeCategories> kAgeCategories = {
	// AgeCategories
	// young, adult, middle, old, venitiar
	{ 50, 150, 250, 350, 450 },   // Dwarf
	{ 175, 550, 875, 1200, 1600 },// Elf
	{ 90, 300, 450, 600, 750 },   // Gnome
	{ 40, 100, 175, 250, 325 },   // Half-Elf
	{ 33, 68, 101, 144, 199 },    // Halfling
	{ 15, 30, 45, 60, 80 },       // Half-Orc (not playable in Poolrad)
	{ 20, 40, 60, 90, 120 }       // Human
};

static const Common::Array<AgeingEffects> kStatAgeingEffects = 	{
	{  0, 1, -1, -2, -1 }, // Strength
	{  0, 0,  0,  0,  0 }, // Extended Strength
	{  0, 0,  1,  0,  1 }, // Intelligence
	{ -1, 1,  1,  1,  1 }, // Wisdom
	{  0, 0,  0, -2, -1 }, // Dexterity
	{  1, 0, -1, -1, -1 }, // Constitution
	{  0, 0,  0,  0,  0 }  // Charisma
};

static const Common::Array<ClassMinStats> kClassMinStats = {
	// ClassMinStats
	// strMin, intMin, wisMin, dexMin, conMin, chaMin
	{  6,  6,  9,  0,  0,  0 }, // Cleric
	{  0,  0, 12,  0,  0, 15 }, // Druid
	{  9,  0,  6,  6,  7,  0 }, // Fighter
	{ 13,  9, 12,  0,  9, 17 }, // Paladin
	{  0, 13, 14,  0, 14,  0 }, // Ranger
	{  0,  9,  6,  6,  0,  0 }, // Magic-User
	{  6,  6,  0,  9,  0,  0 }, // Thief
	{ 15,  0, 15, 15, 11,  0 }, // Monk
	{  9,  0,  9,  0,  0,  0 }, // Cleric/Fighter
	{  9,  9,  9,  0,  0,  0 }, // Cleric/Fighter/Magic-User
	{  0, 13, 14,  0, 14,  0 }, // Cleric/Ranger
	{  0,  9,  9,  0,  0,  0 }, // Cleric/Magic-User
	{  0,  0,  9,  9,  0,  0 }, // Cleric/Thief
	{  9,  9,  0,  0,  0,  0 }, // Fighter/Magic-User
	{  9,  0,  0,  9,  0,  0 }, // Fighter/Thief
	{  9,  9,  0,  9,  0,  0 }, // Fighter/Magic-User/Thief
	{  0,  9,  0,  9,  0,  0 }  // Magic-User/Thief
};

static const Common::Array<LevelUpInfo> kExperienceByClassAndLevel = {
	{ { 2, { 1501, 3001, 6001, 13001, 27501, -1, -1, -1 } }, {
		{ 2, 0, 0 }, { 2, 1, 0 }, { 3, 2, 0 }, { 3, 3, 1 },
		{ 3, 3, 2 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} }, //Cleric
	{ { 2, { -1, -1, -1, -1, -1, -1, -1, -1 } }, {
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} }, //Druid
	{ { 2, { 2001, 4001, 8001, 18001, 35001, 70001, 125001, -1 } }, {
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} }, //Fighter
	{ { 2, { -1, -1, -1, -1, -1, -1, -1, -1 } }, {
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} }, //Paladin
	{ { 2, { -1, -1, -1, -1, -1, -1, -1, -1 } }, {
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} }, //Ranger
	{ { 2, { 2501, 5001, 10001, 22501, 40001, -1, -1, -1 } }, {
		{ 2, 0, 0 }, { 2, 1, 0 }, { 3, 2, 0 }, { 4, 2, 1 },
		{ 4, 2, 2 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} }, //Magic-User
	{ { 2, { 1251, 2501, 5001, 10001, 20001, 42501, 70001, 110001 } }, {
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} }, //Thief
	{ { 2, { -1, -1, -1, -1, -1, -1, -1, -1 } }, {
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 },
		{ 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
	} } //Monk
};

static const Common::Array<DiceRoll> kSpellRolls = {
	{ 3, 6 }, // Cleric
	{ 3, 6 }, // Druid
	{ 5, 4 }, // Fighter
	{ 5, 4 }, // Paladin
	{ 5, 4 }, // Ranger
	{ 2, 4 }, // Magic-User
	{ 2, 6 }, // Thief
	{ 5, 4 }  // Monk
};

// 8 base classes (0..7) x 9 levels (1..9)
static const SavingThrows kSavingThrows[BASE_CLASS_NUM][9] = {
    // Class 0 - Cleric
    {
        { 10, 13, 14, 16, 15 }, { 10, 13, 14, 16, 15 }, { 10, 13, 14, 16, 15 },
        {  9, 12, 13, 15, 14 }, {  9, 12, 13, 15, 14 }, {  9, 12, 13, 15, 14 },
        {  7, 10, 11, 13, 12 }, {  7, 10, 11, 13, 12 }, {  7, 10, 11, 13, 12 }
    },
    // Class 1 - Druid
    {
        { 10, 13, 14, 16, 15 }, { 10, 13, 14, 16, 15 }, { 10, 13, 14, 16, 15 },
        {  9, 12, 13, 15, 14 }, {  9, 12, 13, 15, 14 }, {  9, 12, 13, 15, 14 },
        {  7, 10, 11, 13, 12 }, {  7, 10, 11, 13, 12 }, {  7, 10, 11, 13, 12 }
    },
    // Class 2 - Fighter
    {
        { 14, 15, 16, 17, 17 }, { 14, 15, 16, 17, 17 }, { 13, 14, 15, 16, 16 },
        { 13, 14, 15, 16, 16 }, { 11, 12, 13, 13, 14 }, { 11, 12, 13, 13, 14 },
        { 10, 11, 12, 12, 13 }, { 10, 11, 12, 12, 13 }, {  8,  9, 10,  9, 11 }
    },
    // Class 3 - Paladin
    {
        { 14, 15, 16, 17, 17 }, { 14, 15, 16, 17, 17 }, { 13, 14, 15, 16, 16 },
        { 13, 14, 15, 16, 16 }, { 11, 12, 13, 13, 14 }, { 11, 12, 13, 13, 14 },
        { 10, 11, 12, 12, 13 }, { 10, 11, 12, 12, 13 }, {  8,  9, 10,  9, 11 }
    },
    // Class 4 - Ranger
    {
        { 14, 15, 16, 17, 17 }, { 14, 15, 16, 17, 17 }, { 13, 14, 15, 16, 16 },
        { 13, 14, 15, 16, 16 }, { 11, 12, 13, 13, 14 }, { 11, 12, 13, 13, 14 },
        { 10, 11, 12, 12, 13 }, { 10, 11, 12, 12, 13 }, {  8,  9, 10,  9, 11 }
    },
    // Class 5 - Magic-User
    {
        { 14, 13, 11, 15, 12 }, { 14, 13, 11, 15, 12 }, { 14, 13, 11, 15, 12 },
        { 14, 13, 11, 15, 12 }, { 14, 13, 11, 15, 12 }, { 13, 11,  9, 13, 10 },
        { 13, 11,  9, 13, 10 }, { 13, 11,  9, 13, 10 }, { 13, 11,  9, 13, 10 }
    },
    // Class 6 - Thief
    {
        { 13, 12, 14, 16, 15 }, { 13, 12, 14, 16, 15 }, { 13, 12, 14, 16, 15 },
        { 13, 12, 14, 16, 15 }, { 12, 11, 12, 15, 13 }, { 12, 11, 12, 15, 13 },
        { 12, 11, 12, 15, 13 }, { 12, 11, 12, 15, 13 }, { 11, 10, 10, 14, 11 }
    },
    // Class 7 - Monk
    {
        { 13, 12, 14, 16, 15 }, { 13, 12, 14, 16, 15 }, { 13, 12, 14, 16, 15 },
        { 13, 12, 14, 16, 15 }, { 12, 11, 12, 15, 13 }, { 12, 11, 12, 15, 13 },
        { 12, 11, 12, 15, 13 }, { 12, 11, 12, 15, 13 }, { 11, 10, 10, 14, 11 }
    }
};

const SavingThrows &savingThrowsAt(uint8 baseClassIndex, uint8 level) {
	if (baseClassIndex >= BASE_CLASS_NUM)
		baseClassIndex = 0;
	if (level == 0)
		level = 1;
	if (level > 9)
		level = 9;
	return kSavingThrows[baseClassIndex][level - 1];
}

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

const AgeDefEntry &getAgeDef(uint8 race, uint8 baseClassIndex) {
	// kAgeDefs is laid out for playable races only, starting at Dwarf.
	// RaceADnD enumerates R_MONSTER=0, then Dwarf=1..Human=7.
	// Map: Dwarf->0, Elf->1, ..., Human->6; Monster falls back to 0.
	uint8 raceIdx = (race > 0) ? (race - 1) : 0;
	if (raceIdx >= kAgeDefs.size())
		raceIdx = 0;
	// Clamp baseClassIndex to available entries for the given race
	if (baseClassIndex >= kAgeDefs[raceIdx].size())
		baseClassIndex = 0;
	return kAgeDefs[raceIdx][baseClassIndex];
}

const ClassAlignmentDef *getAlignmentTable() { return kClassAlignment.data(); }
const RaceClassDef *getRaceClassTable() { return kRaceClasses.data(); }
const thac0Bases *getThac0Table() { return kThac0ByClass.data(); }
const Common::Array< Common::Array<AgeDefEntry> > &getAgeDefs() { return kAgeDefs; }

const AgeCategories &getAgeCategoriesForRace(uint8 race) {
	// Map R_DWARF..R_HUMAN (1..7) -> 0..6; out-of-range maps to 0 (Dwarf) as a safe default.
	uint8 raceIdx = (race > 0) ? (race - 1) : 0;
	if (raceIdx >= kAgeCategories.size())
		raceIdx = 0;
	return kAgeCategories[raceIdx];
}

const Common::Array<AgeingEffects> &getStatAgeingEffects() {
	return kStatAgeingEffects;
}

const RaceStatMinMax &getRaceStatMinMaxForRace(uint8 race) {
	// Map R_DWARF..R_HUMAN (1..7) -> 0..6; out-of-range maps to 0 (Dwarf) as a safe default.
	uint8 raceIdx = (race > 0) ? (race - 1) : 0;
	if (raceIdx >= kRaceStatMinMax.size())
		raceIdx = 0;
	return kRaceStatMinMax[raceIdx];
}

const ClassMinStats &getClassMinStats(uint8 classId) {
	// Clamp to table size; if out of range, use first row (Cleric) as safe default
	uint8 idx = classId;
	if (idx >= kClassMinStats.size())
		idx = 0;
	return kClassMinStats[idx];
}

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

using namespace Goldbox::Data;

// 0 Cleric -> 0x02, 1 Druid -> 0x10, 2 Fighter -> 0x08, 3 Paladin -> 0x40,
// 4 Ranger -> 0x80, 5 Magic-User -> 0x01, 6 Thief -> 0x04, 7 Monk -> 0x20.
static const uint8 kClassItemLimitBits[BASE_CLASS_NUM] = {
    0x02, 0x10, 0x08, 0x40, 0x80, 0x01, 0x04, 0x20
};

uint8 Rules::classItemLimitBit(uint8 baseClassIndex) {
    if (baseClassIndex >= BASE_CLASS_NUM)
        return 0;
    return kClassItemLimitBits[baseClassIndex];
}

uint8 Rules::computeItemLimitMask(const Common::Array<uint8> &levels) {
    uint8 mask = 0;
    const uint sz = MIN<uint>(levels.size(), BASE_CLASS_NUM);
    for (uint i = 0; i < sz; ++i) {
        if (levels[i] > 0)
            mask |= kClassItemLimitBits[i];
    }
    return mask;
}

static inline uint8 clampToU8(int v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return (uint8)v;
}

ThiefSkills Rules::computeThiefSkills(uint8 race, uint8 dexterity, uint8 thiefLevel) {
	ThiefSkills out;
	// Level 0 -> all zero
	if (thiefLevel == 0)
		return out;

	// Base from level (1-based), clamp to last row
	const ThiefSkills &base = Rules::getThiefSkillsForLevel(thiefLevel);

	// Race index in adjustments: Dwarf..Human -> 0..6, default Human
	const ThiefRaceAdjustments *rAdjPtr = &kThiefRaceAdjustments[6];
	if (race >= R_DWARF && race <= R_HUMAN) {
		rAdjPtr = &kThiefRaceAdjustments[race - 1];
	}
	const ThiefRaceAdjustments &rAdj = *rAdjPtr;

	// Dexterity adjustments cover 9..19; clamp accordingly
	uint8 dex = dexterity;
	if (dex < 9) dex = 9;
	if (dex > 19) dex = 19;
	const ThiefDexterityAdjustments &dAdj = kThiefDexterityAdjustments[dex - 9];

	// For skills PICK..HIDE add dex bonus; others do not
	for (int s = 0; s < THIEF_SKILL_COUNT; ++s) {
		ThiefSkill sk = (ThiefSkill)s;

		int baseVal = base.get(sk);
		int raceMod = rAdj.get(sk);
		int dexMod  = (sk <= SK_HIDE_IN_SHADOWS) ? dAdj.get(sk) : 0;

		// If race penalty exceeds base, floor at 0
		if (raceMod < 0 && (-raceMod) > baseVal) {
			out.set(sk, 0);
		} else {
			out.set(sk, clampToU8(baseVal + raceMod + dexMod));
		}
	}

	return out;
}

} // namespace Rules
} // namespace Data
} // namespace Goldbox

