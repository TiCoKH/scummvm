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