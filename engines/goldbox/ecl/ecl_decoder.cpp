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

#include "goldbox/ecl/ecl_decoder.h"

namespace Goldbox {
namespace ECL {

static uint32 getPackedStringSize(uint8 decodedLength) {
    return (decodedLength * 3 + 3) / 4;
}

Common::String decompress6BitString(const uint8 *compressedData, uint8 length) {
    Common::String result;
    if (!compressedData || length == 0) {
        return result;
    }

    // 6-bit decompression: 4 chars packed into 3 bytes
    // 3-state decoder matching ECL specification
    int state = 1;
    uint8 lastByte = 0;
    uint32 byteIndex = 0;
    uint8 outputCount = 0;

    while (outputCount < length) {
        if (byteIndex >= length * 3 / 4 + 1) {
            break; // Safety check: compressed size ~= length * 3 / 4
        }

        uint8 thisByte = compressedData[byteIndex++];
        uint8 curr = 0;

        switch (state) {
        case 1:
            curr = (thisByte >> 2) & 0x3F;
            break;
        case 2:
            curr = ((lastByte << 4) | (thisByte >> 4)) & 0x3F;
            break;
        case 3:
            curr = ((lastByte << 2) | (thisByte >> 6)) & 0x3F;
            break;
        }

        if (curr <= 0x1F) {
            curr += 0x40;
        }

        result += (char)curr;
        outputCount++;

        lastByte = thisByte;
        state = (state % 3) + 1;

        if (state == 1 && outputCount < length) {
            curr = thisByte & 0x3F;
            if (curr <= 0x1F) {
                curr += 0x40;
            }
            result += (char)curr;
            outputCount++;
        }
    }

    return result;
}

} // namespace ECL
} // namespace Goldbox
