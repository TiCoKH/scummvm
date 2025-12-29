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
 * ECL Virtual Memory Layout Constants
 * These offsets discovered through comparative analysis of m68k and x86 decompiles.
 * All offsets are within the 16-bit ECL virtual address space (0x0000-0xFFFF).
 *
 * Memory Base Calculation:
 *   MemStart = 0x9900 (Pool of Radiance)
 *   MemStart = 0x8000 (most other Gold Box games)
 *   MemBase = 0x10000 - MemStart
 */
namespace ECLMemoryLayout {
    // Memory Base Configuration
    constexpr uint16 MEM_START_POOLRAD = 0x9900;        // Pool of Radiance specific
    constexpr uint16 MEM_START_DEFAULT = 0x8000;        // Default for other games
    constexpr uint16 MEM_BASE_POOLRAD = 0x10000 - MEM_START_POOLRAD; // = 0x6700
    // VM Execution Control
    constexpr uint16 OFFSET_MAP_ID = 0x1e4;              // Current map/dungeon ID
    constexpr uint16 OFFSET_MAP_X = 0x1e0;               // X coordinate
    constexpr uint16 OFFSET_MAP_Y = 0x1e2;               // Y coordinate
    constexpr uint16 OFFSET_MAP_DIRECTION = 0x1e6;       // Facing direction

    // Text Output State (UI)
    constexpr uint16 OFFSET_TEXT_BUFFER = 0x84c8;        // Output buffer (128 bytes)
    constexpr uint16 OFFSET_TEXT_BUFFER_SIZE = 128;
    constexpr uint16 OFFSET_TEXT_PRINT_FLAG = 0x84de;    // Output ready (0=waiting, 1=ready)
    constexpr uint16 OFFSET_TEXT_OUTPUT_FLAG = 0x84df;   // Output complete flag

    // Execution State Flags (Critical)
    constexpr uint16 OFFSET_ECL_HALT = 0x49ff;           // Halt execution (1=halt, 0=continue)
    constexpr uint16 OFFSET_BREAK_FLAG = 0x442e;         // Break inner loop
    constexpr uint16 OFFSET_EXIT_SCRIPT = 0x442f;        // Exit main loop
    constexpr uint16 OFFSET_PROGRAM_STATE = 0x4430;      // Program control state
    constexpr uint16 OFFSET_GAME_STATE = 0x4431;         // Game state (dungeon/wilderness/combat)
    constexpr uint16 OFFSET_MENU_COMBAT_STATE = 0x4432;  // Menu or combat active

    // Monster/Combat Data
    constexpr uint16 OFFSET_MONSTER_COUNT = 0x0580;      // Number of active monsters
    constexpr uint16 OFFSET_MONSTER_DATA = 0x0582;       // Monster data start
    constexpr uint16 OFFSET_ENCOUNTER_FLAGS = 0x0594;    // Encounter type flags

    // Character Selection & Iteration
    constexpr uint16 OFFSET_SEL_CHAR_PTR = 0x6000;       // Selected character pointer
    constexpr uint16 OFFSET_NEXT_CHAR_PTR = 0x6002;      // Next character in list

    // UI State
    constexpr uint16 OFFSET_MENU_STATUS = 0x84e4;        // Active menu ID
    constexpr uint16 OFFSET_UI_FLAG = 0x84e9;            // UI rendering flag
    constexpr uint16 OFFSET_SPRITE_STATE = 0x84e3;       // Sprite visible/hidden

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

    // Operand Buffer (for instruction decoding)
    constexpr uint16 OFFSET_OPERAND_BUFFER_LOW = 0x7046; // Operand byte values
    constexpr uint16 OFFSET_OPERAND_BUFFER_HIGH = 0x7086; // Operand high bytes
    constexpr uint16 MAX_OPERANDS = 16;

    // String Compression Markers (6-bit packed text)
    constexpr uint8 STRING_COMPRESSED = 0x80;            // Marker: <0x80> <length> <data>
    constexpr uint8 STRING_FROM_MEMORY = 0x81;           // Marker: string pointer

    // Time & Date (from FAQ 11.4.2)
    constexpr uint16 OFFSET_TIME_MINUTE_ONES = 0x49c7;   // Minutes ones digit
    constexpr uint16 OFFSET_TIME_MINUTE_TENS = 0x49c8;   // Minutes tens digit
    constexpr uint16 OFFSET_TIME_HOUR = 0x49c9;          // Hour of day
    constexpr uint16 OFFSET_TIME_DAY = 0x49ca;           // Day of month
    constexpr uint16 OFFSET_TIME_MONTH = 0x49cb;         // Month of year
    constexpr uint16 OFFSET_TIME_YEAR = 0x49cc;          // Elapsed years

    // Environment & Navigation
    constexpr uint16 OFFSET_INDOOR_OUTDOOR = 0x49e6;     // 0=indoors, 1=outdoors
    constexpr uint16 OFFSET_PREV_X = 0x49f0;             // Previous X coordinate
    constexpr uint16 OFFSET_PREV_Y = 0x49f1;             // Previous Y coordinate
    constexpr uint16 OFFSET_PREV_SCRIPT = 0x49f2;        // Previous script ID
    constexpr uint16 OFFSET_AUTOMAP_DISABLE = 0x49fb;    // 1=automap disabled
    constexpr uint16 OFFSET_GAME_SPEED = 0x49fc;         // Game speed setting
    constexpr uint16 OFFSET_SKY_COLOR = 0x49fd;          // Sky color palette
    constexpr uint16 OFFSET_CEILING_COLOR = 0x49fe;      // Ceiling color palette

    // Combat & Party State
    constexpr uint16 OFFSET_CLASS_TRAINABLE_MASK = 0x6da8; // Trainable classes bitmask
    constexpr uint16 OFFSET_SELECTED_CHAR = 0x6db1;        // Selected character index
    constexpr uint16 OFFSET_MONSTER_DISTANCE = 0x6dc1;     // Distance to monsters
    constexpr uint16 OFFSET_MORALE_THRESHOLD = 0x6dc6;     // Monster morale threshold
    constexpr uint16 OFFSET_COMBAT_RESULT = 0x6dc7;        // 0=victory, 129=ran away
    constexpr uint16 OFFSET_COMBAT_KILLS = 0x6dc8;         // Monsters killed
    constexpr uint16 OFFSET_NO_MOVE_FLAG = 0x6dc9;         // Cannot move forward/backward
    constexpr uint16 OFFSET_SEARCH_MODE = 0x6dca;          // Search mode active
    constexpr uint16 OFFSET_SURPRISE_FLAGS = 0x6dcb;       // Surprise result
    constexpr uint16 OFFSET_CHARISMA_BONUS = 0x6dcf;       // Charisma bonus
    constexpr uint16 OFFSET_REST_INTERRUPT_TIME = 0x6dd2;   // Rest interrupt time
    constexpr uint16 OFFSET_REST_INTERRUPT_CHANCE = 0x6dd3; // Rest interrupt chance %
    constexpr uint16 OFFSET_LEAVE_MAP_FLAG = 0x6dd5;        // Party leaving map
    constexpr uint16 OFFSET_PICTURE_ID = 0x6de1;           // Picture ID (255=none)
    constexpr uint16 OFFSET_TEMPLE_FLAG = 0x6de2;          // Entering temple
    constexpr uint16 OFFSET_NO_LOOT_FLAG = 0x6de3;         // Combat gives no items
    constexpr uint16 OFFSET_PARTY_ATTACKED_ALLIES = 0x6e33; // Attacked allies flag
    constexpr uint16 OFFSET_PARTY_SIZE = 0x6e3e;           // Number of party members
    constexpr uint16 OFFSET_SHOP_FLAG = 0x6e6c;            // Entering shop
    constexpr uint16 OFFSET_SHOP_PRICE_MULT = 0x6e6d;      // Shop price multiplier
    constexpr uint16 OFFSET_COMBAT_MONSTER_THAC0_MOD = 0x6e70; // Monster THAC0 bonus
    constexpr uint16 OFFSET_COMBAT_PARTY_THAC0_MOD = 0x6e71;   // Party THAC0 bonus
    constexpr uint16 OFFSET_COMBAT_PARTY_MOVE_MOD = 0x6e72;    // Party movement bonus
}

/**
 * Game State Enumerations
 */
enum GameStateValue {
    GS_DUNGEON_MAP = 0,
    GS_WILDERNESS_MAP = 1,
    GS_COMBAT = 2
};

/**
 * Game-specific ECL configuration.
 * Pool of Radiance hardcodes certain address ranges for flags, characters, and scene state.
 * Other Gold Box games (Curse of the Azure Bonds, etc.) may use different offsets.
 */
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

    // Monster/NPC list
    virtual uint16 getMonsterListBase() const { return ECLMemoryLayout::OFFSET_MONSTER_COUNT; }

    // String encoding (Pool of Radiance uses 6-bit encoding for strings)
    virtual bool usesSixBitStringEncoding() const { return true; }

    /**
     * Get a human-readable name for this config (for debugging).
     */
    virtual Common::String getGameName() const { return "Unknown"; }
};

/**
 * Pool of Radiance specific configuration.
 */
class PoolradConfig : public GameConfig {
public:
    Common::String getGameName() const override { return "Pool of Radiance"; }
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_GAME_CONFIG_H
