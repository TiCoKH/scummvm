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

#ifndef GOLDBOX_ECL_ECL_MEMORY_H
#define GOLDBOX_ECL_ECL_MEMORY_H

#include "common/scummsys.h"
#include "goldbox/core/vm_bank.h"

namespace Goldbox {
namespace ECL {

/**
 * Flat 16-bit ECL virtual address space.
 *
 * VM/layout addresses are canonical word-address values used throughout the
 * engine. Bank identity is retained only as debug metadata.
 */
class AddressSpace {
public:
    static const uint32 MEMORY_SIZE = 65536; // 16-bit virtual address space

    AddressSpace();

    /**
     * Read 8-bit value at address.
     * @param addr 16-bit address
     * @return 8-bit unsigned value
     */
    uint8 read8(uint16 addr) const;

    /**
     * Write 8-bit value at address.
     * @param addr 16-bit address
     * @param val 8-bit unsigned value
     */
    void write8(uint16 addr, uint8 val);

    /**
     * Read 16-bit little-endian value at address.
     * @param addr 16-bit address
     * @return 16-bit unsigned value
     */
    uint16 read16LE(uint16 addr) const;

    /**
     * Write 16-bit little-endian value at address.
     * @param addr 16-bit address
     * @param val 16-bit unsigned value
     */
    void write16LE(uint16 addr, uint16 val);

    /**
     * Clear all memory (reset to 0).
     */
    void clear();

    /**
     * Clear script flags ($4B00 to $4B3F for persistent flags area).
     * Called when NEWECL loads a new script.
     */
    void clearTransientFlags(uint16 flagBase, uint16 transientCount);

    /**
     * Dump memory region for debugging.
     * @param startAddr Start address
     * @param length Number of bytes to dump
     * @return String representation of memory
     */
    Common::String dumpRegion(uint16 startAddr, uint16 length) const;

private:
    VmFlatMemory _memory;
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ECL_MEMORY_H
