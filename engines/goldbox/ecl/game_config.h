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

namespace Goldbox {
namespace ECL {

/**
 * Legacy ECL constants retained for configuration compatibility.
 * New code should use VmLayout, VmGlobalLayout, and EclRuntimeLayout.
 */
namespace ECLMemoryLayout {
    // Memory Base Configuration
    constexpr uint16 MEM_START_POOLRAD = 0x9900;        // Pool of Radiance specific
    constexpr uint16 MEM_START_DEFAULT = 0x8000;        // Default for other games
    constexpr uint16 MEM_BASE_POOLRAD = 0x10000 - MEM_START_POOLRAD; // = 0x6700

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

/** Game state values used by ECL scripts. */
enum GameStateValue {
    GS_DUNGEON_MAP = 0,
    GS_WILDERNESS_MAP = 1,
    GS_COMBAT = 2
};

/** Game-specific ECL configuration. */
class GameConfig {
public:
    virtual ~GameConfig() = default;

    // Flag storage
    virtual uint16 getFlagBase() const { return ECLMemoryLayout::OFFSET_TRANSIENT_FLAGS; }
    virtual uint16 getTransientFlagCount() const { return ECLMemoryLayout::TRANSIENT_FLAG_COUNT; }
    virtual uint16 getPersistentFlagCount() const { return ECLMemoryLayout::PERSISTENT_FLAG_COUNT; }

    // Character data (loaded by LOAD CHARACTER)
    virtual uint16 getCharacterBase() const { return ECLMemoryLayout::OFFSET_CHARACTER_DATA; }
    virtual uint16 getCharacterSize() const { return ECLMemoryLayout::CHARACTER_DATA_SIZE; }

    // Scene/map state
    virtual uint16 getSceneStateBase() const { return 0xC04B; }

    // Specific character attribute offsets (relative to character base)
    virtual uint16 getCharNameOffset() const { return ECLMemoryLayout::CHAR_OFFSET_NAME; }
    virtual uint16 getCharStatBase() const { return ECLMemoryLayout::CHAR_OFFSET_STATS; }
    virtual uint16 getCharHPOffset() const { return ECLMemoryLayout::CHAR_OFFSET_HP; }
    virtual uint16 getCharStatusOffset() const { return ECLMemoryLayout::CHAR_OFFSET_STATUS; }

    // String encoding (Pool of Radiance uses 6-bit encoding for strings)
    virtual bool usesSixBitStringEncoding() const { return true; }

    /**
     * Get a human-readable name for this config (for debugging).
     */
    virtual Common::String getGameName() const { return "Unknown"; }
};

/** Pool of Radiance configuration. */
class PoolradConfig : public GameConfig {
public:
    Common::String getGameName() const override { return "Pool of Radiance"; }
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_GAME_CONFIG_H
