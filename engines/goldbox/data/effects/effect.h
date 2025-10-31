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
    // Specific effects referenced by Poolrad spell table (subset)
    E_BLESS,
    E_CURSED,
    E_DETECT_MAGIC,
    E_READ_MAGIC,
    E_PROTECTION_FROM_EVIL,
    E_PROTECTION_FROM_GOOD,
    E_RESIST_COLD,
    E_CHARM_PERSON,
    E_ENLARGE,
    E_REDUCE,
    E_FRIENDS,
    E_SHIELD,
    E_SLEEP,
    E_FIND_TRAPS,
    E_PARALYZE,
    E_RESIST_FIRE,
    E_SILENCE_15_RADIUS,
    E_SLOW_POISON,
    E_SNAKE_CHARM,
    E_STICKS_TO_SNAKES,
    E_SPIRITUAL_HAMMER,
    E_SP_DISPEL_EVIL,
    E_DETECT_INVISIBILITY,
    E_INVISIBILITY,
    E_MIRROR_IMAGE,
    E_RAY_OF_ENFEEBLEMENT,
    E_STINKING_CLOUD,
    E_STRENGTH,
    E_PRAYER,
    E_BESTOW_CURSE,
    E_BLINK,
    E_HASTE,
    E_PROT_FROM_EVIL_10_RADIUS,
    E_PROT_FROM_GOOD_10_RADIUS,
    E_PROT_FROM_NORMAL_MISSILES,
    E_SLOW,
    E_BLINDED,
    E_CAUSE_DISEASE_1,
    E_CONFUSE,
    E_FEAR,
    E_FUMBLING,
    E_MINOR_GLOBE_OF_INVULNERABILITY,
    E_ANIMATE_DEAD,
    E_FEEBLEMIND,
    E_PROT_DRAG_BREATH,
    E_RESIST_PARALYZE,
    E_INVISIBLE, // distinct tag present in data
    E_ENTANGLE,
    E_FAERIE_FIRE,
    E_INVISIBLE_TO_ANIMALS,
    // Generic placeholders
    E_DAMAGE,
    E_HEAL,
    E_BUFF,
    E_DEBUFF,
    E_SUMMON,
    E_MISC
};

struct Effect {
    uint8 type;          // Effect ID (original e_id)
    uint16 durationMin;  // Duration in minutes (original duration word)
    uint8 power;         // Power (0xFF = permanent)
    uint32 nextAddress;  // Original DOS far ptr to next node (always 0 in saves)
    uint8 immediate;

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
        type = s.readByte();
        durationMin = s.readUint16LE();
        power = s.readByte();
        immediate = s.readByte();
        nextAddress = s.readUint32LE();
    }

    void save(Common::WriteStream &s) const {
        s.writeByte(type);
        s.writeUint16LE(durationMin);
        s.writeByte(power);
        s.writeByte(immediate);
        s.writeUint32LE(nextAddress);
    }
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_H