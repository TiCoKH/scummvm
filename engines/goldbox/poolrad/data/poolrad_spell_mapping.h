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

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_SPELL_MAPPING_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_SPELL_MAPPING_H

#include "goldbox/data/spells/spell.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {

// Pool of Radiance spell array sizes
constexpr int POOLRAD_MEMORIZED_SIZE = 21;
constexpr int POOLRAD_KNOWN_SIZE = 55;

// Mapping from legacy character file array index to Spells enum
// Based on Pool of Radiance character format offsets 0x033-0x069 (55 spells total)
// The Spells enum values (1-55) directly correspond to the PoR spell IDs
static const Goldbox::Data::Spells::Spells kPoolradSpellMapping[POOLRAD_KNOWN_SIZE] = {
    // Cleric Level 1 (0x033-0x03A) - indices 0-7
    Goldbox::Data::Spells::SP_CL1_BLESS,                    // 0  - 0x033 - cleric 1 01
    Goldbox::Data::Spells::SP_CL1_CURSE,                    // 1  - 0x034 - cleric 1 02
    Goldbox::Data::Spells::SP_CL1_CURE_LT_WOUNDS,           // 2  - 0x035 - cleric 1 03
    Goldbox::Data::Spells::SP_CL1_CAUSE_LT_WOUNDS,          // 3  - 0x036 - cleric 1 04
    Goldbox::Data::Spells::SP_CL1_DETECT_MAGIC,             // 4  - 0x037 - cleric 1 05
    Goldbox::Data::Spells::SP_CL1_PROT_FROM_EVIL,           // 5  - 0x038 - cleric 1 06
    Goldbox::Data::Spells::SP_CL1_PROT_FROM_GOOD,           // 6  - 0x039 - cleric 1 07
    Goldbox::Data::Spells::SP_CL1_RESIST_COLD,              // 7  - 0x03A - cleric 1 08
    
    // Mage Level 1 (0x03B-0x047) - indices 8-20
    Goldbox::Data::Spells::SP_MUL1_BURNING_HANDS,           // 8  - 0x03B - mage 1 01
    Goldbox::Data::Spells::SP_MUL1_CHARM_PERSON,            // 9  - 0x03C - mage 1 02
    Goldbox::Data::Spells::SP_MUL1_DETECT_MAGIC,            // 10 - 0x03D - mage 1 03
    Goldbox::Data::Spells::SP_MUL1_ENLARGE,                 // 11 - 0x03E - mage 1 04
    Goldbox::Data::Spells::SP_MUL1_REDUCE,                  // 12 - 0x03F - mage 1 05
    Goldbox::Data::Spells::SP_MUL1_FRIENDS,                 // 13 - 0x040 - mage 1 06
    Goldbox::Data::Spells::SP_MUL1_MAGIC_MISSILE,           // 14 - 0x041 - mage 1 07
    Goldbox::Data::Spells::SP_MUL1_PROT_FROM_EVIL,          // 15 - 0x042 - mage 1 08
    Goldbox::Data::Spells::SP_MUL1_PROT_FROM_GOOD,          // 16 - 0x043 - mage 1 09
    Goldbox::Data::Spells::SP_MUL1_READ_MAGIC,              // 17 - 0x044 - mage 1 10
    Goldbox::Data::Spells::SP_MUL1_SHIELD,                  // 18 - 0x045 - mage 1 11
    Goldbox::Data::Spells::SP_MUL1_SHOCKING_GRASP,          // 19 - 0x046 - mage 1 12
    Goldbox::Data::Spells::SP_MUL1_SLEEP,                   // 20 - 0x047 - mage 1 13
    
    // Cleric Level 2 (0x048-0x04E) - indices 21-27
    Goldbox::Data::Spells::SP_CL2_FIND_TRAPS,               // 21 - 0x048 - cleric 2 01
    Goldbox::Data::Spells::SP_CL2_HOLD_PERSON,              // 22 - 0x049 - cleric 2 02
    Goldbox::Data::Spells::SP_CL2_RESIST_FIRE,              // 23 - 0x04A - cleric 2 03
    Goldbox::Data::Spells::SP_CL2_SILENCE_15R,              // 24 - 0x04B - cleric 2 04
    Goldbox::Data::Spells::SP_CL2_SLOW_POISON,              // 25 - 0x04C - cleric 2 05
    Goldbox::Data::Spells::SP_CL2_SNAKE_CHARM,              // 26 - 0x04D - cleric 2 06
    Goldbox::Data::Spells::SP_CL2_SPIRIT_HAMMER,            // 27 - 0x04E - cleric 2 07
    
    // Mage Level 2 (0x04F-0x055) - indices 28-34
    Goldbox::Data::Spells::SP_MUL2_DETECT_INVISIB,          // 28 - 0x04F - mage 2 01
    Goldbox::Data::Spells::SP_MUL2_INVISIBILITY,            // 29 - 0x050 - mage 2 02
    Goldbox::Data::Spells::SP_MUL2_KNOCK,                   // 30 - 0x051 - mage 2 03
    Goldbox::Data::Spells::SP_MUL2_MIRROR_IMAGE,            // 31 - 0x052 - mage 2 04
    Goldbox::Data::Spells::SP_MUL2_RAY_ENFEEBLE,            // 32 - 0x053 - mage 2 05
    Goldbox::Data::Spells::SP_MUL2_STINKING_CLOUD,          // 33 - 0x054 - mage 2 06
    Goldbox::Data::Spells::SP_MUL2_STRENGTH,                // 34 - 0x055 - mage 2 07
    
    // Cleric Level 3 (0x056-0x05E) - indices 35-43
    Goldbox::Data::Spells::SP_CL3_ANIMATE_DEAD,             // 35 - 0x056 - cleric 3 01
    Goldbox::Data::Spells::SP_CL3_CURE_BLINDNESS,           // 36 - 0x057 - cleric 3 02
    Goldbox::Data::Spells::SP_CL3_CAUSE_BLINDNESS,          // 37 - 0x058 - cleric 3 03
    Goldbox::Data::Spells::SP_CL3_CURE_DISEASE,             // 38 - 0x059 - cleric 3 04
    Goldbox::Data::Spells::SP_CL3_CAUSE_DISEASE,            // 39 - 0x05A - cleric 3 05
    Goldbox::Data::Spells::SP_CL3_DISPEL_MAGIC,             // 40 - 0x05B - cleric 3 06
    Goldbox::Data::Spells::SP_CL3_PRAYER,                   // 41 - 0x05C - cleric 3 07
    Goldbox::Data::Spells::SP_CL3_REMOVE_CURSE,             // 42 - 0x05D - cleric 3 08
    Goldbox::Data::Spells::SP_CL3_BESTOW_CURSE,             // 43 - 0x05E - cleric 3 09
    
    // Mage Level 3 (0x05F-0x069) - indices 44-54
    Goldbox::Data::Spells::SP_MUL3_BLINK,                   // 44 - 0x05F - mage 3 01
    Goldbox::Data::Spells::SP_MUL3_DISPEL_MAGIC,            // 45 - 0x060 - mage 3 02
    Goldbox::Data::Spells::SP_MUL3_FIREBALL,                // 46 - 0x061 - mage 3 03
    Goldbox::Data::Spells::SP_MUL3_HASTE,                   // 47 - 0x062 - mage 3 04
    Goldbox::Data::Spells::SP_MUL3_HOLD_PERSON,             // 48 - 0x063 - mage 3 05
    Goldbox::Data::Spells::SP_MUL3_INVIS_10R,               // 49 - 0x064 - mage 3 06
    Goldbox::Data::Spells::SP_MUL3_LIGHTNING_BOLT,          // 50 - 0x065 - mage 3 07
    Goldbox::Data::Spells::SP_MUL3_PROT_F_EVIL_10R,         // 51 - 0x066 - mage 3 08
    Goldbox::Data::Spells::SP_MUL3_PROT_F_GOOD_10R,         // 52 - 0x067 - mage 3 09
    Goldbox::Data::Spells::SP_MUL3_PROT_F_NORM_MSL,         // 53 - 0x068 - mage 3 10
    Goldbox::Data::Spells::SP_MUL3_SLOW                     // 54 - 0x069 - mage 3 11
};

} // namespace Data
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_POOLRAD_SPELL_MAPPING_H
