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

#ifndef GOLDBOX_DATA_EFFECTS_EFFECT_H
#define GOLDBOX_DATA_EFFECTS_EFFECT_H

#include "common/str.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Data {
namespace Effects {

// Effect identifier the spell applies
enum Effects : uint8 {
    E_NONE = 0,
    E_BLESSED = 1,
    E_CURSED = 2,
    E_DETECT_MAGIC = 3,
    E_IMMUNE_TO_ELECTRICITY = 4,
    E_READ_MAGIC = 5,
    E_PROTECTION_FROM_EVIL = 6,
    E_PROTECTION_FROM_GOOD = 7,
    E_RESIST_COLD = 8,
    E_CHARM_PERSON = 9,
    E_SUFFOCATE = 11,
    E_REDUCE = 12,
    E_FRIENDS = 13,
    E_POISON_DAMAGE = 14,
    E_SHIELD = 15,
    E_HUMAN_VS_SMALL = 16,
    E_SLEEP = 17,
    E_FIND_TRAPS = 18,
    E_PARALYZE = 19,
    E_RESIST_FIRE = 20,
    E_SILENCE_15_RADIUS = 21,
    E_SLOW_POISON = 22,
    E_SNAKE_CHARM = 23,
    E_SWORD_VS_UNDEAD = 24,
    E_SPIRITUAL_HAMMER = 25,
    E_DISPEL_EVIL = 26,
    E_SP_DISPEL_EVIL = 27,
    E_DETECT_INVISIBILITY = 28,
    E_INVISIBILITY = 29,
    E_DWARF_VS_ORC = 30,
    E_MIRROR_IMAGE = 31,
    E_RAY_OF_ENFEEBLEMENT = 32,
    E_STINKING_CLOUD = 33,
    E_HELPLESS = 34,
    E_PRAYER = 36,
    E_BESTOW_CURSE = 37,    // TODO: verify raw ID from decompile
    E_BLINK = 37,           // raw 0x25: alias of E_BESTOW_CURSE
    E_STRENGTH = 38,        // raw 0x26
    E_ENLARGE = 38,         // raw 0x26: alias of E_STRENGTH
    E_HASTE = 39,
    E_STINKING_CLOUD_EXPAIR = 40,  // per-character marker: currently inside an active Stinking Cloud area
    E_PROT_FROM_EVIL_10_RADIUS = 41,
    E_PROT_FROM_GOOD_10_RADIUS = 42,
    E_WEAKENED = 43,
    E_PROT_FROM_NORMAL_MISSILES = 43, // Poolrad-specific alias
    E_CAUSE_WOUND = 44,
    E_SLOW = 44,            // alias of E_CAUSE_WOUND
    E_WEAKEN = 45,
    E_DISEASE_CONFUSED = 46,
    E_DWARF_AND_GNOME_VS_GIANTS = 47,
    E_GNOME_LARGE_MONSTER = 48,
    E_CHANT = 49,
    E_HOT_FIRE_SHIELD = 50,
    E_BLINDED = 51,
    E_CAUSE_DISEASE_1 = 52,
    E_CONFUSE = 53,
    E_COLD_FIRE_SHIELD = 54,
    E_POISONED = 55,
    E_ITEM_INVISIBILITY = 56,
    E_ENGULFS = 57,
    E_CLEAR_MOVEMENT = 58,
    E_REGENERATE_3_HPS = 59,
    E_REGENERATE_1_HPS = 60,
    E_RAKSHASA_RESIST_NORMAL_WEAPONS = 61,
    E_FIRE_RESIST = 62,
    E_HIGH_CON_REGEN = 63,
    E_FEAR = 64,
    E_FUMBLING = 65,
    E_MINOR_GLOBE_OF_INVULNERABILITY = 66,
    E_POISON_PLUS_0 = 67,
    E_POISON_PLUS_4 = 68,
    E_POISON_PLUS_2 = 69,
    E_THRI_KREEN_PARALYZE = 70,
    E_ANIMATE_DEAD = 71,
    E_FEEBLEMIND = 72,
    E_INVISIBLE_TO_ANIMALS = 73,
    E_POISON_NEG_2 = 74,
    E_INVISIBLE = 75,
    E_CAMOUFLAGE = 76,
    E_PROT_DRAG_BREATH = 77,
    E_AFFECT_4A = 78,
    E_WEAP_DRAGON_SLAYER = 79,
    E_WEAP_FROST_BRAND = 80,
    E_BERSERK = 81,
    E_AFFECT_4E = 82,
    E_FIRE_ATTACK_2D10 = 83,
    E_ANKHEG_ACID_ATTACK = 84,
    E_HALF_DAMAGE = 85,
    E_RESIST_FIRE_AND_COLD = 86,
    E_PETRIFYING_GAZE = 87,
    E_SHAMBLING_ABSORB_LIGHTNING = 88,
    E_REDUCE_DAMAGE_TO_ONE_IF_ITEM_FIELD7_AFFECT_55 = 89,
    E_GIANT_SLUG_SPIT_ACID = 90,
    E_BEHOLDER_RAYS_AFFECT_57 = 91,
    E_BREATH_ELEC = 92,
    E_DISPLACE = 93,
    E_BREATH_ACID = 94,
    E_CLOUD_KILL = 95,
    E_FEAR_IMMUNITY = 96,
    E_HALF_FIRE_DAMAGE = 97,
    E_DAMAGE_REDUCTION = 98,
    E_WILD_BOAR_DIE_AFTER_EXTRA_FIGHT_TIME_AFFECT_5F = 99,
    E_OWLBEAR_HUG_CHECK = 100,
    E_CON_SAVING_BONUS = 101,
    E_REGEN_3_HP = 102,
    E_WILD_BOAR_AND_BULLETTE_AFFECT_63 = 103,
    E_TROLL_FIRE_OR_ACID = 104,
    E_UNKNOWN_101 = 105,
    E_UNKNOWN_102 = 106,
    E_THRI_KREEN_MISSILE_EVASION = 107,
    E_RESIST_MAGIC_50 = 108,
    E_RESIST_MAGIC_15 = 109,
    E_RESIST_SLEEP_CHARM_90 = 110,
    E_IMMUNITY_SLEEP_CHARM = 111,
    E_IMMUNITY_PARALYSIS = 112,
    E_IMMUNITY_COLD = 113,
    E_IMMUNITY_PARALYSIS_POISON = 114,
    E_IMMUNITY_FIRE = 115,
    E_EFREETI_FIRE_RESISTANCE = 116,
    E_HALF_DAMAGE_ELECTRICITY = 117,
    E_HALF_DAMAGE_PIERCING_SLASHING = 118,
    E_HALF_DAMAGE_MAGICAL_WEAPONS = 119,
    E_VULNERABILITY_HOLY_WATER = 120,
    E_HALF_DAMAGE_COLD = 121,
    E_IMMUNITY_NONMAGICAL_WEAPONS = 122,
    E_BOULDER_EVASION = 123,
    E_ANKHEG_ACID_SQUIRT_ATTACK = 124,
    E_VULNERABILITY_FIRE = 125,
    E_IMMUNITY_NONMAGICAL_HALF_SILVER = 126,
    E_RESIST_SLEEP_CHARM_30 = 127,
    E_IMMUNITY_SLEEP_CHARM_PARALYSIS_POISON = 128,
    E_IMMUNITY_GAZE_ATTACKS = 129,
    E_UNIMPLEMENTED_126 = 130,
    E_ITEM_EFFECT_127 = 131,
    E_ITEM_EFFECT_128 = 132,
    E_ITEM_EFFECT_129 = 133,
    E_EXTRA_STRENGTH_130 = 134,
    E_ITEM_131 = 135,
    E_UNKNOWN_132 = 136,
    E_UNKNOWN_133 = 137,
    E_UNKNOWN_134 = 138,
    E_UNKNOWN_135 = 139,
    E_UNKNOWN_136 = 140,
    E_RESIST_PARALYZE = 141,
    E_ENTANGLE = 142,
    E_FAERIE_FIRE = 143,
    E_MISC = 255,
};

// Pool of Radiance uses a separate raw effect table. These values must not be
// treated as aliases of Effects: the same raw ID can have a different meaning.
enum PoolradEffects : uint8 {
    E_POOLRAD_VAMPIRIC = 0x00,
    E_POOLRAD_BLESSED = 0x01,
    E_POOLRAD_CURSED = 0x02,
    E_POOLRAD_SWORD_VS_UNDEAD = 0x03,
    E_POOLRAD_STUDY_MANUAL_BODILY_HEALTH = 0x04,
    E_POOLRAD_DETECT_MAGIC = 0x05,          // UNIMPLEMENTED in handler table
    E_POOLRAD_FLAME_TONGUE_WEAPON = 0x06,
    E_POOLRAD_TRAIN_MANUAL_BODILY_HEALTH = 0x07,
    E_POOLRAD_PROTECTION_FROM_EVIL = 0x08,
    E_POOLRAD_PROTECTION_FROM_GOOD = 0x09,
    E_POOLRAD_RESIST_COLD = 0x0A,
    E_POOLRAD_CHARM_PERSON = 0x0B,
    E_POOLRAD_ENLARGE_STRENGTHEN = 0x0C,
    E_POOLRAD_UNIMPLEMENTED_0D = 0x0D,
    E_POOLRAD_FRIENDLY = 0x0E,
    E_POOLRAD_POISON_DAMAGE = 0x0F,
    E_POOLRAD_READ_MAGIC = 0x10,
    E_POOLRAD_SHIELD = 0x11,
    E_POOLRAD_SMALL_VS_NORMAL = 0x12,
    E_POOLRAD_UNIMPLEMENTED_13 = 0x13,
    E_POOLRAD_FIRE_RESISTANT = 0x14,        // identity maps to E_RESIST_FIRE=20 ✓
    E_POOLRAD_SILENCED = 0x15,              // identity maps to E_SILENCE_15_RADIUS=21 ✓
    E_POOLRAD_SLOW_POISON = 0x16,           // identity maps to E_SLOW_POISON=22 ✓
    E_POOLRAD_SPIRITUAL_HAMMER = 0x17,
    E_POOLRAD_TRUE_SEEING = 0x18,
    E_POOLRAD_BLUR = 0x19,
    E_POOLRAD_DWARF_TARGET_BONUS = 0x1A,
    E_POOLRAD_DUPLICATED = 0x1C,
    E_POOLRAD_ENFEEBLED = 0x1D,
    E_POOLRAD_NAUSEATED = 0x1E,
    E_POOLRAD_HELPLESS = 0x1F,
    E_POOLRAD_ANIMATING_DEAD = 0x20,
    E_POOLRAD_BLINDED = 0x21,
    E_POOLRAD_DISEASED = 0x22,
    E_POOLRAD_PRAYER = 0x23,
    E_POOLRAD_ACCURSED = 0x24,
    E_POOLRAD_PROT_NORMAL_WEAPONS = 0x29,
    E_POOLRAD_SLOWED = 0x2A,
    E_POOLRAD_PROT_FROM_EVIL_10 = 0x2D,
    E_POOLRAD_PROT_FROM_GOOD_10 = 0x2E,
    E_POOLRAD_DWARF_VS_GIANT = 0x2F,
    E_POOLRAD_GNOME_VS_LARGE = 0x30,
    E_POOLRAD_PRAYER_2 = 0x31,
    E_POOLRAD_ENDLESS_REGEN = 0x32,
    E_POOLRAD_HELPLESS_33 = 0x33,
    E_POOLRAD_HELPLESS_34 = 0x34,
    E_POOLRAD_HELPLESS_35 = 0x35,
    E_POOLRAD_PARALYZED = 0x3A,
    E_POOLRAD_REGEN_3_HP = 0x3E,
};

struct Effect {
    uint8  id;           // Effect ID (original e_id)
    uint16 durationMin;  // Duration in minutes (original duration word)
    uint8  power;        // Power (0xFF = permanent)
    uint8  immediate;

    // Serialization-only compatibility tail (DOS x86 .SPC node bytes 5..8).
    // Runtime effect logic must NOT model a linked list pointer.
    // We still read/write these bytes to preserve on-disk format compatibility.
    enum {
        kLegacySerializedNextAddress = 0
    };

    // Historical layout notes (x86 Pool of Radiance):
    //  Allocation size: 9 bytes
    //    0: e_id
    //    1-2: duration (word, LE)
    //    3: power      (param_2 in Pascal stdcall decompile)
    //    4: unknown    (param_1)
    //    5-8: next_ptr (far pointer, both word & segment zeroed by creator)
    //  The m68k variant reportedly used a 10-byte node (e_id, duration, unknown, power, next_ptr[4]).
    //  Our on-disk representation in .SPC mirrors the x86 layout exactly.

    void load(Common::SeekableReadStream &s) {
        id = s.readByte();
        durationMin = s.readUint16LE();
        power = s.readByte();
        immediate = s.readByte();
        // Legacy next-pointer field (far ptr) is ignored at runtime.
        s.readUint32LE();
    }

    void save(Common::WriteStream &s) const {
        s.writeByte(id);
        s.writeUint16LE(durationMin);
        s.writeByte(power);
        s.writeByte(immediate);
        // Keep DOS save layout compatible: always serialize null next-pointer.
        s.writeUint32LE(kLegacySerializedNextAddress);
    }
};

// Pack str/ext_str into an effect power byte.
// str == 18 : power = ext_str + 1   (values 1-101, ext_str 0-100)
// str != 18 : power = str + 100     (values 102+)
inline uint8 strengthEncode(uint8 str, uint8 extStr) {
    return (str == 18) ? (uint8)(extStr + 1) : (uint8)(str + 100);
}

// Unpack an effect power byte back into str / ext_str.
inline void strengthDecode(uint8 power, uint8 &outStr, uint8 &outExtStr) {
    outExtStr = 0;
    uint8 v = power & 0x7f;
    if (v < 102) {
        outStr    = 18;
        outExtStr = (uint8)(v - 1);
    } else {
        outStr = (uint8)(v + 156); // equivalent to v - 100 in uint8 arithmetic
    }
}

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_H