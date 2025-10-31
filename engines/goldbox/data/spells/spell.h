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

#ifndef GOLDBOX_DATA_SPELLS_SPELL_H
#define GOLDBOX_DATA_SPELLS_SPELL_H

#include "goldbox/data/effects/effect.h"
#include "common/types.h"

namespace Goldbox {
namespace Data {
namespace Spells {

// Enum for spell class/type (Offset 0x00)
enum SpellClass {
    SC_CLERIC    = 0,
    SC_MAGICUSER = 1,
    SC_ITEM      = 2,
    SC_UNKNOWN   = 0xFF
};

enum Spells : uint8 {
    SP_NONE                 = 0,
    SP_CL1_BLESS            = 1,
    SP_CL1_CURSE            = 2,
    SP_CL1_CURE_LT_WOUNDS   = 3,
    SP_CL1_CAUSE_LT_WOUNDS  = 4,
    SP_CL1_DETECT_MAGIC     = 5,
    SP_CL1_PROT_FROM_EVIL   = 6,
    SP_CL1_PROT_FROM_GOOD   = 7,
    SP_CL1_RESIST_COLD      = 8,
    SP_MUL1_BURNING_HANDS   = 9,
    SP_MUL1_CHARM_PERSON    = 10,
    SP_MUL1_DETECT_MAGIC    = 11,
    SP_MUL1_ENLARGE         = 12,
    SP_MUL1_REDUCE          = 13,
    SP_MUL1_FRIENDS         = 14,
    SP_MUL1_MAGIC_MISSILE   = 15,
    SP_MUL1_PROT_FROM_EVIL  = 16,
    SP_MUL1_PROT_FROM_GOOD  = 17,
    SP_MUL1_READ_MAGIC      = 18,
    SP_MUL1_SHIELD          = 19,
    SP_MUL1_SHOCKING_GRASP  = 20,
    SP_MUL1_SLEEP           = 21,
    SP_CL2_FIND_TRAPS       = 22,
    SP_CL2_HOLD_PERSON      = 23,
    SP_CL2_RESIST_FIRE      = 24,
    SP_CL2_SILENCE_15R      = 25,
    SP_CL2_SLOW_POISON      = 26,
    SP_CL2_SNAKE_CHARM      = 27,
    SP_CL2_SPIRIT_HAMMER    = 28,
    SP_MUL2_DETECT_INVISIB  = 29,
    SP_MUL2_INVISIBILITY    = 30,
    SP_MUL2_KNOCK           = 31,
    SP_MUL2_MIRROR_IMAGE    = 32,
    SP_MUL2_RAY_ENFEEBLE    = 33,
    SP_MUL2_STINKING_CLOUD  = 34,
    SP_MUL2_STRENGTH        = 35,
    SP_CL3_ANIMATE_DEAD     = 36,
    SP_CL3_CURE_BLINDNESS   = 37,
    SP_CL3_CAUSE_BLINDNESS  = 38,
    SP_CL3_CURE_DISEASE     = 39,
    SP_CL3_CAUSE_DISEASE    = 40,
    SP_CL3_DISPEL_MAGIC     = 41,
    SP_CL3_PRAYER           = 42,
    SP_CL3_REMOVE_CURSE     = 43,
    SP_CL3_BESTOW_CURSE     = 44,
    SP_MUL3_BLINK           = 45,
    SP_MUL3_DISPEL_MAGIC    = 46,
    SP_MUL3_FIREBALL        = 47,
    SP_MUL3_HASTE           = 48,
    SP_MUL3_HOLD_PERSON     = 49,
    SP_MUL3_INVIS_10R       = 50,
    SP_MUL3_LIGHTNING_BOLT  = 51,
    SP_MUL3_PROT_F_EVIL_10R = 52,
    SP_MUL3_PROT_F_GOOD_10R = 53,
    SP_MUL3_PROT_F_NORM_MSL = 54,
    SP_MUL3_SLOW            = 55, // Until this possible set in known spell table
    SP_CL7_RESTORATION      = 56,
    SP_MI1                  = 57, // Magic Item spells start here
    SP_MI2                  = 58,
    SP_MI3                  = 59,
    SP_MI4                  = 60,
    SP_MI5                  = 61,
    SP_MI6                  = 62,
    SP_MI7                  = 63,
    SP_MI8                  = 64,
    SP_MI9                  = 65,
    SP_MI10                 = 66,
    SP_MI11                 = 67
};

// Area of effect (Offset 0x06):
// 0 caster, 4 1 target, 5 up to 2, 6 up to 3, 7 up to 4,
// 8 any tile within Line of sight, 9 special, 10 diameter 5, 11 diameter 7,
// 240 scales to caster level
enum AreaOfEffect : uint8 {
    AREA_CASTER     = 0,
    AREA_TARGET_1   = 4,
    AREA_TARGET_2   = 5,
    AREA_TARGET_3   = 6,
    AREA_TARGET_4   = 7,
    AREA_LOS_TILE   = 8,
    AREA_SPECIAL    = 9,
    AREA_DIAMETER_5 = 10,
    AREA_DIAMETER_7 = 11,
    AREA_DIAMETER_U = 31,
    AREA_LVL_SCALE  = 240
};

// Target (Offset 0x07):
// 0 combat, 1 caster, 2 any party member, 4 whole party
// Backed by uint8 for save/load stability; values match engine format.
enum SpellTargets : uint8 {
    ST_COMBAT       = 0,
    ST_CASTER       = 1,
    ST_PARTY_MEMBER = 2,
    ST_WHOLE_PARTY  = 4
};

// Saving throw effect (Offset 0x08)
// 0 no save allowed, 1 negate, 2 half damage
// Some games may use other values; keep explicit codes when needed.
enum DamageOnSave : uint8 {
    DMG_NO_SAVE         = 0,
    DMG_NEGATES         = 1,
    DMG_HALF            = 2,
    DMG_UNKNOWN_3       = 3,
    DMG_UNKNOWN_1E      = 0x1E
};

// Saving throw used (Offset 0x09)
// 0 paralyzation/poison/death, 1 petrification/polymorph,
// 2 rod/staff/wand, 3 breath, 4 spell
enum SaveVerseType : uint8 {
    SVS_POISON         = 0,
    SVS_PETRIFICATION  = 1,
    SVS_ROD_STAFF_WAND = 2,
    SVS_BREATH         = 3,
    SVS_SPELL          = 4
};

// When spell can be cast (Offset 0x0B): 0 non-combat only, 1 combat only, 2 either
enum SpellWhen : uint8 {
    IN_CAMP   = 0,
    IN_COMBAT = 1,
    IN_BOTH   = 2
};

// Detailed spell entry as found in Pool of Radiance data tables
struct SpellEntry {
    SpellClass    spellClass;     // 0x00 class/type
    uint8         spellLevel;     // 0x01 class spell level
    uint8         fixedRange;     // 0x02 fixed range (255 means attack roll)
    uint8         perLvlRange;    // 0x03 variable range per caster level
    uint8         fixedDuration;  // 0x04 fixed duration
    uint8         perLvlDuration; // 0x05 per level duration
    AreaOfEffect  areaOfEffect;   // 0x06 area of effect code
    SpellTargets  targetType;     // 0x07 target code
    DamageOnSave  damageOnSave;   // 0x08 saving throw effect
    SaveVerseType saveVerse;      // 0x09 saving throw type
    uint8         effectId;       // 0x0A effect identifier (0: no temp or perament effect from effect tabel)
    SpellWhen     whenCast;       // 0x0B when spell can be cast
    uint8         castTime;       // 0x0C spellcasting time
    uint8         priority;       // 0x0D AI priority
    uint8         isOffensive;    // 0x0E 1 offensive, else 0
    uint8         minAITargets;   // 0x0F min number of targets for AI

    SpellEntry()
            : spellClass(SC_UNKNOWN), spellLevel(0),
        fixedRange(0), perLvlRange(0), fixedDuration(0), perLvlDuration(0),
        areaOfEffect(AREA_CASTER), targetType(ST_COMBAT), damageOnSave(DMG_NO_SAVE), saveVerse(SVS_POISON),
        effectId(0), whenCast(IN_BOTH),
        castTime(0), priority(0), isOffensive(0), minAITargets(0) {}

    SpellEntry(SpellClass _spellClass, uint8 _spellLevel,
            uint8 _fixedRange, uint8 _perLvlRange,
            uint8 _fixedDuration, uint8 _perLvlDuration,
        AreaOfEffect _area, SpellTargets _targets,
        DamageOnSave _damageOnSave, SaveVerseType _saveVerse,
            uint8 _affectId, SpellWhen _whenCast,
            uint8 _castingDelay, uint8 _priority, uint8 fe, uint8 ff)
            : spellClass(_spellClass), spellLevel(_spellLevel),
        fixedRange(_fixedRange), perLvlRange(_perLvlRange),
    fixedDuration(_fixedDuration), perLvlDuration(_perLvlDuration),
    areaOfEffect(_area), targetType(_targets),
        damageOnSave(_damageOnSave), saveVerse(_saveVerse),
        effectId(_affectId), whenCast(_whenCast),
    castTime(_castingDelay), priority(_priority),
    isOffensive(fe), minAITargets(ff) {}
};

} // namespace Spells
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_SPELLS_SPELL_H
