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

#ifndef GOLDBOX_POOLRAD_DATA_POOLRAD_EFFECTS_H
#define GOLDBOX_POOLRAD_DATA_POOLRAD_EFFECTS_H

#include "common/scummsys.h"

namespace Goldbox {
namespace Data {
namespace Effects {

// Pool of Radiance's raw, on-disk effect-id table. These values must not be
// treated as aliases of the generic Effects enum in effect.h: the same raw
// ID can have a different meaning per game. Values mirror the original
// single-byte effect id read/written from .SPC save data, so they cannot be
// widened into a shared "game-specific ID range" the way a synthetic id
// space could; each game instead owns its own raw-id enum in its own header.
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
    E_POOLRAD_ROT = 0x39,
    // 0x3A identity-maps to E_IMMOBILIZED=58; use that directly.
    E_POOLRAD_REGENERATING = 0x3B,
    E_POOLRAD_FIRE_RESISTANCE = 0x3D,
    E_POOLRAD_REGEN_3_HP = 0x3E,
    E_POOLRAD_INVISIBLE_RING = 0x38,
    E_POOLRAD_DRAGON_FEAR_AURA = 0x51,
    E_POOLRAD_MUMMY_FEAR_AURA = 0x52,
    E_POOLRAD_PETRIFYING_GAZE = 0x53,
    E_POOLRAD_CHARMING_GAZE = 0x54,
    E_POOLRAD_REFLECTIVE_GAZE_MARKER = 0x7F,
    E_POOLRAD_CON_SAVING_BONUS = 0x5A,
    E_POOLRAD_HALFLING_POISON_BONUS = 0x5B,
    E_POOLRAD_HALF_DAMAGE_FROM_FIRE = 0x5D,
    E_POOLRAD_HALF_DAMAGE_BLUNT_PIERCING = 0x5E,
    E_POOLRAD_FIGHT_ON_AT_ZERO_HP = 0x5F,
    E_POOLRAD_IMMUNITY_NON_SILVER_NON_MAGICAL = 0x60,
    E_POOLRAD_DWARF_SAVE_BONUS = 0x61,
    E_POOLRAD_REGEN_3_HP_ROUND = 0x62,
    E_POOLRAD_KEEP_FIGHTING_AFTER_UNCONSCIOUS = 0x63,
    E_POOLRAD_TROLL_FIRE_ACID_VULNERABILITY = 0x64,
    E_POOLRAD_APPLY_DEATH_RECOVERY = 0x65,
    E_POOLRAD_RETURN_FROM_DEATH = 0x66,
    E_POOLRAD_GASEOUS_ESCAPE = 0x67,
    E_POOLRAD_THRI_KREEN_MISSILE_EVASION_60 = 0x68,
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_DATA_POOLRAD_EFFECTS_H
