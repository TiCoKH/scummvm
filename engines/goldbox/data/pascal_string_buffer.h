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

#ifndef GOLDBOX_DATA_PASCAL_STRING_BUFFER_H
#define GOLDBOX_DATA_PASCAL_STRING_BUFFER_H

#include "common/stream.h"
#include "common/str.h"

namespace Goldbox {
namespace Data {

template <size_t MaxLen>
struct PascalStringBuffer {
    // Parse a fixed-size Pascal-string record from an in-memory buffer.
    // Layout: [len:1][data:MaxLen], where data is zero-padded.
    static Common::String readFromBuffer(const byte *record,
            size_t recordSize = MaxLen + 1) {
        if (!record || recordSize == 0)
            return Common::String();

        const uint8 len = record[0];
        const size_t dataSize = (recordSize > 1) ? (recordSize - 1) : 0;
        const uint8 clampedLen = MIN<uint8>(len,
            static_cast<uint8>(MIN<size_t>(MaxLen, dataSize)));
        return Common::String(reinterpret_cast<const char *>(&record[1]),
            clampedLen);
    }

    // Write a fixed-size Pascal-string record into an in-memory buffer.
    // Layout: [len:1][data:MaxLen], zero-filling all trailing bytes.
    static void writeToBuffer(byte *record, const Common::String &str,
            size_t recordSize = MaxLen + 1) {
        if (!record || recordSize == 0)
            return;

        for (size_t i = 0; i < recordSize; ++i)
            record[i] = 0;

        const size_t dataSize = (recordSize > 1) ? (recordSize - 1) : 0;
        const uint8 len = static_cast<uint8>(MIN<size_t>(str.size(),
            MIN<size_t>(MaxLen, dataSize)));
        record[0] = len;
        if (len > 0)
            memcpy(&record[1], str.c_str(), len);
    }

    static Common::String read(Common::SeekableReadStream &s) {
        byte record[MaxLen + 1] = {};
        s.read(record, MaxLen + 1);
        return readFromBuffer(record, MaxLen + 1);
    }

    static void write(Common::WriteStream &s, const Common::String &str) {
        byte record[MaxLen + 1] = {};
        writeToBuffer(record, str, MaxLen + 1);
        s.write(record, MaxLen + 1);
    }
};
    

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_PASCAL_STRING_BUFFER_H
