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
#include "goldbox/data/items/base_items.h"

namespace Goldbox {
namespace Data {
namespace Items {

void Storage::load(Common::SeekableReadStream &stream) {
    
    _items.clear();

    // Skip the 2-byte header (not a count)
    stream.skip(2);
    // First entry is empty (No Item)
    _items.push_back(ItemProperty{});

    const int recordSize = 16;
    int total = stream.size(); 

    while (stream.pos() + recordSize <= total) {
        ItemProperty ip;

        ip.slot           = static_cast<Slot>(stream.readByte());
        ip.hands          = stream.readByte();

        // Damage vs. Large creatures
        ip.dmgLarge.dices = stream.readByte();
        ip.dmgLarge.sides = stream.readByte();
        ip.dmgLarge.bonus = stream.readSByte();

        ip.fireRate       = stream.readByte();
        ip.protect        = stream.readByte();
        ip.wpnType        = stream.readByte();
        ip.melType        = stream.readByte();

        // Damage vs. Small/Medium creatures
        ip.dmgSmallMed.dices = stream.readByte();
        ip.dmgSmallMed.sides = stream.readByte();
        ip.dmgSmallMed.bonus = stream.readSByte();

        ip.range          = stream.readByte();
        ip.classMask      = stream.readByte();
        ip.missileType    = stream.readByte();
        stream.skip(1);

        _items.push_back(ip);
    }
}

} // namespace Items
} // namespace Data
} // namespace Goldbox
