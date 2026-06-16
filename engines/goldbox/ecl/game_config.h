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

#ifndef GOLDBOX_ECL_GAME_CONFIG_H
#define GOLDBOX_ECL_GAME_CONFIG_H

#include "common/scummsys.h"
#include "common/str.h"
#include "common/array.h"
#include "goldbox/core/vm_layout.h"
#include "goldbox/ecl/runtime_layout.h"

namespace Goldbox {
namespace ECL {

class AddressSpace;
class SyscallHandler;

struct MemoryRegionRange {
    uint16 _startAddr;
    uint16 _endAddr;
};

/**
 * Legacy ECL constants retained for configuration compatibility.
 * New code should use VmLayout, VmGlobalLayout, and EclRuntimeLayout.
 */
namespace ECLMemoryLayout {
    // Memory Base Configuration
    constexpr uint16 ECL_START_POOLRAD = 0x9900;        // Pool of Radiance specific
    constexpr uint16 ECL_START_DEFAULT = 0x8000;        // Default for other games
    constexpr uint16 MEM_BASE_POOLRAD = 0x10000 - ECL_START_POOLRAD; // = 0x6700

    // Transitional constants kept for compatibility with GameConfig and
    // opcode decoding helpers. Prefer layout-based access for VM addresses.

    // Script Variables - Transient (Cleared per NEWECL)
    constexpr uint16 OFFSET_TRANSIENT_FLAGS = 0x4a00;    // Start of transient flags
    constexpr uint16 TRANSIENT_FLAG_COUNT = 0x20;        // Size: $4A00-$4A1F (32 bytes)

    // Script Variables - Persistent (Survive transitions)
    constexpr uint16 OFFSET_PERSISTENT_FLAGS = 0x4a20;   // Start of persistent flags
    constexpr uint16 PERSISTENT_FLAG_COUNT = 0xe0;       // Size: $4A20-$4AFF (224 bytes)

    // Party/Character Data
    constexpr uint16 OFFSET_CHARACTER_DATA = 0x6b00;     // Character stats/inventory base
    constexpr uint16 CHARACTER_DATA_SIZE = 0x11b;        // Size: $6B00-$6C1A (283 bytes)
    constexpr uint16 CHAR_OFFSET_NAME = 0x00;            // Name (6-bit encoded)
    constexpr uint16 CHAR_OFFSET_STATS = 0x15;           // Ability scores
    constexpr uint16 CHAR_OFFSET_HP = 0x119;             // Current HP (within character block)
    constexpr uint16 CHAR_OFFSET_STATUS = 0x100;         // Status/conditions

    // VM decode limits
    constexpr uint16 MAX_OPERANDS = 16;

    // String Compression Markers (6-bit packed text)
    constexpr uint8 STRING_COMPRESSED = 0x80;            // Marker: <0x80> <length> <data>
    constexpr uint8 STRING_FROM_MEMORY = 0x81;           // Marker: string pointer

}

/** Game-specific ECL configuration. */
class GameConfig {
public:
    virtual ~GameConfig() = default;

    /**
     * Register this game's ECL dialect metadata and handlers.
     *
     * This keeps game-specific opcode sets out of generic VM control flow.
     */
    virtual void registerDialect() const = 0;

    /**
     * Get the maximum opcode byte for this game's ECL dialect.
     *
     * The baseline (Pool of Radiance) uses 0x00-0x3D.
     * Future games may extend this range by overriding this method.
     */
    virtual uint8 getMaxOpcodeTableByte() const { return 0x3D; }

    // Concrete VM/global/runtime layout tables for this game profile.
    virtual const Goldbox::VmLayout &getVmLayout() const = 0;
    virtual const Goldbox::VmGlobalLayout &getVmGlobalLayout() const = 0;
    virtual const EclRuntimeLayout &getEclRuntimeLayout() const = 0;

    EclLayoutAccess getLayoutAccess() const {
        return EclLayoutAccess(getVmLayout(), getVmGlobalLayout(),
            getEclRuntimeLayout());
    }

    // VM address layout
    virtual uint16 getScriptVmStart() const { return ECLMemoryLayout::ECL_START_DEFAULT; }
        virtual bool getVmBankRange(Goldbox::VmBankId /*bankId*/,
            uint16 & /*firstAddr*/,
            uint16 & /*lastAddr*/) const { return false; }

    // Flag storage
    virtual uint16 getFlagBase() const { return ECLMemoryLayout::OFFSET_TRANSIENT_FLAGS; }
    virtual uint16 getTransientFlagCount() const { return ECLMemoryLayout::TRANSIENT_FLAG_COUNT; }
    virtual uint16 getPersistentFlagCount() const { return ECLMemoryLayout::PERSISTENT_FLAG_COUNT; }

    // Character data (loaded by LOAD CHARACTER)
    virtual uint16 getCharacterBase() const { return ECLMemoryLayout::OFFSET_CHARACTER_DATA; }
    virtual uint16 getCharacterSize() const { return ECLMemoryLayout::CHARACTER_DATA_SIZE; }

    // Scene/map state
    virtual uint16 getSceneStateBase() const { return 0xC04B; }

    /**
     * Memory-region table used by opcode handler write routing.
     *
     * Regions are checked in-order. The first matching inclusive range
     * [start, end] determines the region ID (its index in the array).
     * Addresses not matching any range map to fallback region ID
     * `ranges.size()`.
     */
    virtual Common::Array<MemoryRegionRange> getMemoryRegions() const {
        Common::Array<MemoryRegionRange> ranges;

        const Goldbox::VmBankId banks[] = {
            Goldbox::kVmBankWorld,
            Goldbox::kVmBankDat,
            Goldbox::kVmBankHeap,
            Goldbox::kVmBankEcl
        };

        for (uint i = 0; i < ARRAYSIZE(banks); ++i) {
            uint16 firstAddr = 0;
            uint16 lastAddr = 0;
            if (!getVmBankRange(banks[i], firstAddr, lastAddr)) {
                continue;
            }
            if (firstAddr > lastAddr) {
                continue;
            }
            ranges.push_back({firstAddr, lastAddr});
        }

        return ranges;
    }

    /**
     * Optional game-specific VM read interception.
     * Return true if handled and set outValue; false to use base behavior.
     */
    virtual bool onReadVmMemory(const AddressSpace & /*memory*/,
            uint16 /*vmAddr*/, uint16 & /*outValue*/) const {
        return false;
    }

    /**
     * Optional game-specific VM write interception.
     * Return true if handled; false to use base behavior.
     */
    virtual bool onWriteVmMemory(AddressSpace & /*memory*/, uint16 /*vmAddr*/,
            uint16 & /*ioValue*/, uint8 /*region*/,
            SyscallHandler * /*syscalls*/) const {
        return false;
    }

    // Specific character attribute offsets (relative to character base)
    virtual uint16 getCharNameOffset() const { return ECLMemoryLayout::CHAR_OFFSET_NAME; }
    virtual uint16 getCharStatBase() const { return ECLMemoryLayout::CHAR_OFFSET_STATS; }
    virtual uint16 getCharHPOffset() const { return ECLMemoryLayout::CHAR_OFFSET_HP; }
    virtual uint16 getCharStatusOffset() const { return ECLMemoryLayout::CHAR_OFFSET_STATUS; }

    // String encoding (Pool of Radiance uses 6-bit encoding for strings)
    virtual bool usesSixBitStringEncoding() const { return true; }

    /**
     * Optional opcode-variant override for operand count.
     *
     * Multi-variant opcode bytes (for example 0x0C, 0x22, 0x29, 0x42) can
     * represent different logical instructions in different game profiles.
     * Return true when this config defines an explicit operand count for
     * the given opcode byte.
     */
    virtual bool getOpcodeOperandCount(uint8 /*opcode*/, int & /*count*/) const {
        return false;
    }

    /**
     * Get a human-readable name for this config (for debugging).
     */
    virtual Common::String getGameName() const { return "Unknown"; }
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_GAME_CONFIG_H
