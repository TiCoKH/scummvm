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

#include "goldbox/ecl/ecl_string_writer.h"

namespace Goldbox {
namespace ECL {

NumberInputResult validateNumberInput(const Common::String &typed,
        uint8 maxDigits) {
    NumberInputResult result;

    if (typed.empty() || maxDigits == 0)
        return result;
    if (typed.size() > maxDigits)
        return result;

    // Accumulate in a 32-bit intermediate so overflow past 65535 is detected
    // before narrowing (no C++ unsigned wrap is relied on). Only decimal
    // digits are accepted; anything else invalidates the whole entry.
    uint32 acc = 0;
    for (uint i = 0; i < typed.size(); ++i) {
        const char c = typed[i];
        if (c < '0' || c > '9')
            return result;
        acc = acc * 10 + static_cast<uint32>(c - '0');
        if (acc > 0xFFFF)
            return result;
    }

    result.valid = true;
    result.value = static_cast<uint16>(acc);
    return result;
}

uint16 writeVmString(AddressSpace &mem, uint16 destAddr,
        const Common::String &text, uint8 cellWidth, uint8 maxChars) {
    // Empty input substitutes a single space (original EclInputStr behavior).
    Common::String value = text.empty() ? Common::String(" ") : text;

    uint16 len = static_cast<uint16>(value.size());
    if (len > maxChars)
        len = maxChars;

    const uint8 width = (cellWidth == 0) ? 1 : cellWidth;

    for (uint16 i = 0; i < len; ++i) {
        const uint16 cellAddr = static_cast<uint16>(destAddr + i * width);
        const uint8 ch = static_cast<uint8>(value[i]);
        if (width == 1)
            mem.write8(cellAddr, ch);
        else
            mem.write16LE(cellAddr, ch);
    }

    // Zero terminator in the cell following the last character.
    const uint16 termAddr = static_cast<uint16>(destAddr + len * width);
    if (width == 1)
        mem.write8(termAddr, 0);
    else
        mem.write16LE(termAddr, 0);

    return len;
}

} // namespace ECL
} // namespace Goldbox
