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

namespace Goldbox {
namespace ECL {

namespace {

static const uint16 kGeoFirstVmAddr = 0x4900;
static const uint16 kGeoLastVmAddr = 0x4CFF;

static const uint16 kDatFirstVmAddr = 0x6B00;
static const uint16 kDatLastVmAddr = 0x6EFF;

static const uint16 kHeapFirstVmAddr = 0x9700;
static const uint16 kHeapLastVmAddr = 0x98FF;

static const uint16 kEclFirstVmAddr = 0x9900;
static const uint16 kEclLastVmAddr = 0xB6FF;

static const char *vmBankName(VmBankId bankId) {
    switch (bankId) {
    case kVmBankGeo:
        return "GEO";
    case kVmBankDat:
        return "DAT";
    case kVmBankHeap:
        return "HEAP";
    case kVmBankEcl:
        return "ECL";
    case kVmBankSystem:
        return "SYSTEM";
    default:
        return "UNKNOWN";
    }
}

} // namespace

AddressSpace::AddressSpace() {
    _memory.setDebugRange(kVmBankGeo, kGeoFirstVmAddr, kGeoLastVmAddr);
    _memory.setDebugRange(kVmBankDat, kDatFirstVmAddr, kDatLastVmAddr);
    _memory.setDebugRange(kVmBankHeap, kHeapFirstVmAddr, kHeapLastVmAddr);
    _memory.setDebugRange(kVmBankEcl, kEclFirstVmAddr, kEclLastVmAddr);
    // System bank is the fallback for addresses outside other ranges
}

uint8 AddressSpace::read8(uint16 addr) const {
    return _memory.readByte(addr);
}

void AddressSpace::write8(uint16 addr, uint8 val) {
    _memory.writeByte(addr, val);
}

uint16 AddressSpace::read16LE(uint16 addr) const {
    return _memory.readWordLE(addr);
}

void AddressSpace::write16LE(uint16 addr, uint16 val) {
    _memory.writeWordLE(addr, val);
}

void AddressSpace::clear() {
    _memory.clear(0);
}

void AddressSpace::clearTransientFlags(uint16 flagBase, uint16 transientCount) {
    for (uint16 i = 0; i < transientCount; ++i) {
        _memory.writeWordLE(static_cast<uint16>(flagBase + i * 2), 0);
    }
}

Common::String AddressSpace::dumpRegion(uint16 startAddr, uint16 length) const {
    Common::String result;
    for (uint16 i = 0; i < length && startAddr + i < MEMORY_SIZE; ++i) {
        if (i % 16 == 0) {
            uint16 lineAddr = static_cast<uint16>(startAddr + i);
            VmBankId bankId = kVmBankHeap;
            uint16 bankOffset = 0;
            _memory.classifyAddr(lineAddr, bankId, bankOffset);
            result += Common::String::format("\n$%04X (%s +0x%03X): ",
                lineAddr, vmBankName(bankId), bankOffset);
        }
        result += Common::String::format("%02X ",
            _memory.readByte(static_cast<uint16>(startAddr + i)));
    }

    return result;
}

} // namespace ECL
} // namespace Goldbox
