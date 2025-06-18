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
    uint8 type;          // Effect ID
    uint16 durationMin;  // Duration in minutes (LE)
    uint8 power;         // 0xFF = permanent, or encoded power level
    uint8 data[5];       // Unused/padding/reserved

    void load(Common::SeekableReadStream &s) {
        type = s.readByte();
        durationMin = s.readUint16LE();
        power = s.readByte();
        s.read(data, 5);
    }

    void save(Common::WriteStream &s) const {
        s.writeByte(type);
        s.writeUint16LE(durationMin);
        s.writeByte(power);
        s.write(data, 5);
    }
};

} // namespace Effects
} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_EFFECTS_EFFECT_H