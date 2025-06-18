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
        static Common::String read(Common::SeekableReadStream &s) {
            uint8 len = s.readByte();
    
            // Always read the full buffer
            char buf[MaxLen] = {};
            s.read(buf, MaxLen);
    
            // Ensure the length is within bounds
            len = MIN<uint8>(len, MaxLen);
    
            // Construct only the valid portion of the string
            return Common::String(buf, len);
        }
    
        static void write(Common::WriteStream &s, const Common::String &str) {
            uint8 len = static_cast<uint8>(MIN<size_t>(str.size(), MaxLen));
            s.writeByte(len);
    
            // Write only the valid part
            s.write(str.c_str(), len);
    
            // Pad the rest of the buffer with zeros
            for (size_t i = len; i < MaxLen; ++i)
                s.writeByte(0);
        }
    };
    

} // namespace Data
} // namespace Goldbox

#endif // GOLDBOX_DATA_PASCAL_STRING_BUFFER_H
