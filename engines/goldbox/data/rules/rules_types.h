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

#ifndef GOLDBOX_DATA_RULES_RULES_TYPES_H
#define GOLDBOX_DATA_RULES_RULES_TYPES_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Goldbox {
namespace Data {

// Shared constant: number of base (single) classes used in level arrays.
#define BASE_CLASS_NUM 8

// Race enumeration (numeric values preserved for savegame compatibility).
enum RaceADnD {
    R_MONSTER     = 0,
    R_DWARF       = 1,
    R_ELF         = 2,
    R_GNOME       = 3,
    R_HALF_ELF    = 4,
    R_HALFLING    = 5,
    R_HALF_ORC    = 6,
    R_HUMAN       = 7
};

// Class identifiers (includes multiclass composites; values must stay stable).
enum ClassADnD {
    C_CLERIC                   = 0x00,
    C_DRUID                    = 0x01,
    C_FIGHTER                  = 0x02,
    C_PALADIN                  = 0x03,
    C_RANGER                   = 0x04,
    C_MAGICUSER                = 0x05,
    C_THIEF                    = 0x06,
    C_MONK                     = 0x07,
    C_CLERIC_FIGHTER           = 0x08,
    C_CLERIC_FIGHTER_MAGICUSER = 0x09,
    C_CLERIC_RANGER            = 0x0A,
    C_CLERIC_MAGICUSER         = 0x0B,
    C_CLERIC_THIEF             = 0x0C,
    C_FIGHTER_MAGICUSER        = 0x0D,
    C_FIGHTER_THIEF            = 0x0E,
    C_FIGHTER_MAGICUSER_THIEF  = 0x0F,
    C_MAGICUSER_THIEF          = 0x10,
    C_MONSTER                  = 0x11
};

enum ClassFlag {
    CF_CLERIC       = 0x01,
    CF_DRUID        = 0x02,
    CF_FIGHTER      = 0x04,
    CF_PALADIN      = 0x08,
    CF_RANGER       = 0x10,
    CF_MAGICUSER    = 0x20,
    CF_THIEF        = 0x40,
    CF_MONK         = 0x80,
    CF_ALL          = 0xFF
};

enum AlignmentADnD {
    A_LAWFUL_GOOD       = 0,
    A_LAWFUL_NEUTRAL    = 1,
    A_LAWFUL_EVIL       = 2,
    A_NEUTRAL_GOOD      = 3,
    A_TRUE_NEUTRAL      = 4,
    A_NEUTRAL_EVIL      = 5,
    A_CHAOTIC_GOOD      = 6,
    A_CHAOTIC_NEUTRAL   = 7,
    A_CHAOTIC_EVIL      = 8
};

enum ThiefSkill {
    SK_PICK_POCKETS     = 0,
    SK_OPEN_LOCKS       = 1,
    SK_FIND_REMOVE_TRAPS= 2,
    SK_MOVE_SILENTLY    = 3,
    SK_HIDE_IN_SHADOWS  = 4,
    SK_HEAR_NOISE       = 5,
    SK_CLIMB_WALLS      = 6,
    SK_READ_LANGUAGES   = 7,
    THIEF_SKILL_COUNT   = 8
};

// Valuable types (currency + valuables).
enum ValuableType {
    VAL_COPPER     = 0,
    VAL_SILVER     = 1,
    VAL_ELECTRUM   = 2,
    VAL_GOLD       = 3,
    VAL_PLATINUM   = 4,
    VAL_GEMS       = 5,
    VAL_JEWELRY    = 6,
    VALUABLE_COUNT = 7
};

struct ValuableItems {
    Common::Array<uint16> values;
    ValuableItems() : values(VALUABLE_COUNT) {}
    uint16 &operator[](ValuableType t) { return values[t]; }
    const uint16 &operator[](ValuableType t) const { return values[t]; }

    // Sum of all valuables, assuming each unit (coin/gem/jewelry) weighs 1
    inline uint32 getTotalWeight() const {
        uint32 sum = 0;
        for (uint i = 0; i < values.size(); ++i)
            sum += values[i];
        return sum;
    }
};

// Character status codes (stable for save interpretation).
enum StatusADnD {
    S_OKAY        = 0,
    S_ANIMATED    = 1,
    S_TEMPGONE    = 2,
    S_RUNNING     = 3,
    S_UNCONSCIOUS = 4,
    S_DYING       = 5,
    S_DEAD        = 6,
    S_STONED      = 7,
    S_GONE        = 8
};

// Spell arrays (raw layout varies per game; kept generic here).
struct SpellData {
    uint8 memorizedSpells[22];
    uint8 knownSpells[62];
};

// Saving throw block (order matches original games).
struct SavingThrows {
    uint8 vsParalysis;
    uint8 vsPetrification;
    uint8 vsRodStaffWand;
    uint8 vsBreathWeapon;
    uint8 vsSpell;
};

// Thief skill block (percent-style values).
struct ThiefSkills {
    uint8 pickPockets;
    uint8 openLocks;
    uint8 findRemoveTraps;
    uint8 moveSilently;
    uint8 hideInShadows;
    uint8 hearNoise;
    uint8 climbWalls;
    uint8 readLanguages;

    inline uint8 get(ThiefSkill sk) const {
        switch (sk) {
        case SK_PICK_POCKETS:      return pickPockets;
        case SK_OPEN_LOCKS:        return openLocks;
        case SK_FIND_REMOVE_TRAPS: return findRemoveTraps;
        case SK_MOVE_SILENTLY:     return moveSilently;
        case SK_HIDE_IN_SHADOWS:   return hideInShadows;
        case SK_HEAR_NOISE:        return hearNoise;
        case SK_CLIMB_WALLS:       return climbWalls;
        case SK_READ_LANGUAGES:    return readLanguages;
        default:                   return 0;
        }
    }

    inline void set(ThiefSkill sk, uint8 v) {
        switch (sk) {
        case SK_PICK_POCKETS:      pickPockets     = v; break;
        case SK_OPEN_LOCKS:        openLocks       = v; break;
        case SK_FIND_REMOVE_TRAPS: findRemoveTraps = v; break;
        case SK_MOVE_SILENTLY:     moveSilently    = v; break;
        case SK_HIDE_IN_SHADOWS:   hideInShadows   = v; break;
        case SK_HEAR_NOISE:        hearNoise       = v; break;
        case SK_CLIMB_WALLS:       climbWalls      = v; break;
        case SK_READ_LANGUAGES:    readLanguages   = v; break;
        default: break;
        }
    }
};

struct ThiefRaceAdjustments {
    int8 pickPockets;
    int8 openLocks;
    int8 findRemoveTraps;
    int8 moveSilently;
    int8 hideInShadows;
    int8 hearNoise;
    int8 climbWalls;
    int8 readLanguages;

    inline int8 get(ThiefSkill sk) const {
        switch (sk) {
        case SK_PICK_POCKETS:      return pickPockets;
        case SK_OPEN_LOCKS:        return openLocks;
        case SK_FIND_REMOVE_TRAPS: return findRemoveTraps;
        case SK_MOVE_SILENTLY:     return moveSilently;
        case SK_HIDE_IN_SHADOWS:   return hideInShadows;
        case SK_HEAR_NOISE:        return hearNoise;
        case SK_CLIMB_WALLS:       return climbWalls;
        case SK_READ_LANGUAGES:    return readLanguages;
        default:                   return 0;
        }
    }
};

struct ThiefDexterityAdjustments {
    int8 pickPockets;
    int8 openLocks;
    int8 findRemoveTraps;
    int8 moveSilently;
    int8 hideInShadows;
    int8 climbWalls;

    inline int8 get(ThiefSkill sk) const {
        switch (sk) {
        case SK_PICK_POCKETS:      return pickPockets;
        case SK_OPEN_LOCKS:        return openLocks;
        case SK_FIND_REMOVE_TRAPS: return findRemoveTraps;
        case SK_MOVE_SILENTLY:     return moveSilently;
        case SK_HIDE_IN_SHADOWS:   return hideInShadows;
        case SK_CLIMB_WALLS:       return climbWalls;
        // No Dex adj. for hear noise or read languages
        case SK_HEAR_NOISE:
        case SK_READ_LANGUAGES:
        default:                   return 0;
        }
    }
};

struct AgeCategories {
    uint16 young;
    uint16 adult;
    uint16 middle;
    uint16 old;
    uint16 venitiar;
};

struct AgeingEffects {
    int8 young;
    int8 adult;
    int8 middle;
    int8 old;
    int8 venitiar;
};

struct RaceStatMinMax {
    uint8 strengthMinMale;
    uint8 strengthMinFemale;
    uint8 strengthMaxMale;
    uint8 strengthMaxFemale;
    uint8 extStrengthMaxMale;
    uint8 extStrengthMaxFemale;
    uint8 intelligenceMin;
    uint8 intelligenceMax;
    uint8 wisdomMin;
    uint8 wisdomMax;
    uint8 dexterityMin;
    uint8 dexterityMax;
    uint8 constitutionMin;
    uint8 constitutionMax;
    uint8 charismaMin;
    uint8 charismaMax;
};

struct ClassMinStats {
    uint8 strength;
    uint8 intelligence;
    uint8 wisdom;
    uint8 dexterity;
    uint8 constitution;
    uint8 charisma;
};

struct DiceRoll {
    uint8 numDice;
    uint8 diceSides;
};

struct SpellSlots {
    uint8 level1;
    uint8 level2;
    uint8 level3;
};

// Combined spell slot capacities for Cleric and Magic-User (AD&D 1e, levels 1-3).
// Represents how many spells can be memorized per class and level for a character.
struct ClassSpellSlots {
    SpellSlots cleric;
    SpellSlots magicUser;
};

struct NeededExperience {
    byte fromLevel;
    int32 toLevel[8];
};

struct LevelUpInfo {
    NeededExperience experience;
    SpellSlots SlotsByLevel[8];
};

// Combat primitive set (extend if needed).
struct CombatData {
    uint8 thac0Base;
    uint8 attackLevel;
};

// Attack roll pattern (optional usage).
struct CombatRolls {
    uint8 attacks;
    uint8 rolls;
    uint8 dice;
    int8 modifier;
};

// Grouping for level data; one field per class type supported.
struct LevelData {
    Common::Array<uint8> levels;
    LevelData() : levels(BASE_CLASS_NUM) {}
    uint8 &operator[](ClassADnD c) { return levels[(int)c]; }
    const uint8 &operator[](ClassADnD c) const { return levels[(int)c]; }
};

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_RULES_RULES_TYPES_H