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

#include "common/file.h"
#include "goldbox/data/pascal_string_buffer.h"
#include "goldbox/data/items/character_inventory.h"



namespace Goldbox {
namespace Data {
namespace Items {

using Goldbox::Data::PascalStringBuffer;

bool CharacterInventory::load(const Common::String &filename) {
    clear();

    Common::File file;
    if (!file.open(filename.c_str()))
        return false;  // no .ITM → empty inventory

    auto &s = file;  // File implements SeekableReadStream

    const int recSize = 63;
    int    total   = s.size();

    while (s.pos() + recSize <= total) {
        CharacterItem it;

        it.name = PascalStringBuffer<42>::read(s);
        // --- dynamic fields at offsets 0x2A–0x3E ---
        it.nextAddress = s.readUint32LE();
        it.typeIndex   = s.readByte();
        it.nameCode1   = s.readByte();
        it.nameCode2   = s.readByte();
        it.nameCode3   = s.readByte();
        it.bonus       = s.readByte();
        it.saveBonus   = s.readByte();
        it.readied     = s.readByte();
        it.hidden      = s.readByte();
        it.cursed      = s.readByte();

        it.weight      = s.readUint16LE();
        it.stackSize   = s.readByte();
        it.value       = s.readUint16LE();

        it.effect1     = s.readByte();
        it.effect2     = s.readByte();
        it.effect3     = s.readByte();

        s.skip(1);

        _items.push_back(it);
    }

    file.close();
    return true;
}

bool CharacterInventory::save(const Common::String &filename) const {
    Common::DumpFile file;
    if (!file.open(filename.c_str()))
        return false;

    auto &s = file;  // DumpFile supports WriteStream

    for (const auto &it : _items) {
        PascalStringBuffer<44>::write(s, it.name);

        s.writeByte(it.typeIndex);
        s.writeByte(it.nameCode1);
        s.writeByte(it.nameCode2);
        s.writeByte(it.nameCode3);
        s.writeByte(it.bonus);
        s.writeByte(it.saveBonus);
        s.writeByte(it.readied);
        s.writeByte(it.hidden);
        s.writeByte(it.cursed);

        s.writeUint16LE(it.weight);
        s.writeByte(it.stackSize);
        s.writeUint16LE(it.value);

        s.writeByte(it.effect1);
        s.writeByte(it.effect2);
        s.writeByte(it.effect3);

        // write 4 bytes of zero to reserve far pointer space
        uint8 zeroes[4] = {0, 0, 0, 0};
        s.write(zeroes, 4);
    }

    file.flush();
    file.close();
    return true;
}

void CharacterInventory::debugPrint() const {
    for (size_t i = 0; i < _items.size(); ++i)
        _items[i].debugPrint(i);
}

} // namespace Items
} // namespace Data
} // namespace Goldbox
