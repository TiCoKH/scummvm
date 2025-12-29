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

#include "goldbox/ecl/ecl_memory.h"
#include "common/str.h"
#include "common/util.h"

namespace Goldbox {
namespace ECL {

AddressSpace::AddressSpace() : _memory(MEMORY_SIZE, 0) {}

uint8 AddressSpace::read8(uint16 addr) const {
    if (addr < MEMORY_SIZE) {
        return _memory[addr];
    }
    return 0;
}

void AddressSpace::write8(uint16 addr, uint8 val) {
    if (addr < MEMORY_SIZE) {
        _memory[addr] = val;
    }
}

uint16 AddressSpace::read16LE(uint16 addr) const {
    if (addr + 1 < MEMORY_SIZE) {
        return _memory[addr] | (_memory[addr + 1] << 8);
    }
    return 0;
}

void AddressSpace::write16LE(uint16 addr, uint16 val) {
    if (addr + 1 < MEMORY_SIZE) {
        _memory[addr] = val & 0xFF;
        _memory[addr + 1] = (val >> 8) & 0xFF;
    }
}

void AddressSpace::clear() {
    Common::fill(_memory.begin(), _memory.end(), 0);
}

void AddressSpace::clearTransientFlags(uint16 flagBase, uint16 transientCount) {
    for (uint16 i = 0; i < transientCount; ++i) {
        if (flagBase + i < MEMORY_SIZE) {
            _memory[flagBase + i] = 0;
        }
    }
}

Common::String AddressSpace::dumpRegion(uint16 startAddr, uint16 length) const {
    Common::String result;
    for (uint16 i = 0; i < length && startAddr + i < MEMORY_SIZE; ++i) {
        if (i % 16 == 0) {
            result += Common::String::format("\n$%04X: ", startAddr + i);
        }
        result += Common::String::format("%02X ", _memory[startAddr + i]);
    }
    return result;
}

} // namespace ECL
} // namespace Goldbox
