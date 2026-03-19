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

#ifndef GOLDBOX_ECL_SYSCALL_HANDLER_H
#define GOLDBOX_ECL_SYSCALL_HANDLER_H

#include "common/scummsys.h"
#include "common/str.h"
#include "engines/goldbox/vm_interface.h"

namespace Goldbox {

namespace ECL {

enum class YieldReason {
    None = 0,
    WaitingForInput,
    WaitingForCombat,
    WaitingForPicture,
    WaitingForMenu,
    WaitingForScript,
    WaitingForCharacterLoad
};

/**
 * Interface for ECL syscalls (I/O, combat, menus, etc.).
 * Implemented by the engine to handle async operations.
 */
class SyscallHandler {
public:
    virtual ~SyscallHandler() = default;

    /**
     * Print text to the message/text box.
     * @param text Text to print (may be encoded)
     * @param clearBox If true, clear text box before printing
     */
    virtual void printText(const Common::String &text, bool clearBox = false) = 0;

    /**
     * Input number from player.
     * @param maxDigits Maximum digits to accept
     * @param resultAddr Memory address where input result should be stored
     * @return VmResult (should return VM_YIELD, completion sets result in address)
     */
    virtual VmResult inputNumber(uint8 maxDigits, uint16 resultAddr) = 0;

    /**
     * Input string from player.
     * @param maxLength Maximum characters to accept
     * @param resultAddr Memory address where input result should be stored
     * @return VmResult (should return VM_YIELD)
     */
    virtual VmResult inputString(uint8 maxLength, uint16 resultAddr) = 0;

    /**
     * Display picture/sprite.
     * @param picID Picture ID (255 to end display)
     * @return VmResult
     */
    virtual VmResult displayPicture(uint8 picID) = 0;

    /**
     * Display vertical menu with options.
     * @param message Prompt message
     * @param options Array of option strings
     * @param resultAddr Memory address to store selection (0-based index)
     * @return VmResult (should return VM_YIELD)
     */
    virtual VmResult verticalMenu(const Common::String &message,
            const Common::Array<Common::String> &options, uint16 resultAddr) = 0;

    /**
     * Display horizontal menu (for bottom bar selection).
     * @param options Array of option strings
     * @param resultAddr Memory address to store selection (0-based index)
     * @return VmResult (should return VM_YIELD)
     */
    virtual VmResult horizontalMenu(const Common::Array<Common::String> &options, uint16 resultAddr) = 0;

    /**
     * Start combat with monsters.
     * @return VmResult (should return VM_YIELD)
     */
    virtual VmResult startCombat() = 0;

    /**
     * Execute special program routine.
     * @param programID Program ID (0=training, 8=win, 9=camp)
     * @return VmResult (may return VM_YIELD for async operations)
     */
    virtual VmResult executeProgram(uint8 programID) = 0;

    /**
     * Clear the text/message box.
     * @return VmResult
     */
    virtual VmResult clearTextBox() = 0;

    /**
     * Load a new ECL script.
     * @param scriptID Script ID to load
     * @return VmResult
     */
    virtual VmResult loadScript(uint8 scriptID) = 0;

    /**
     * Load a GEO dungeon map block by block ID.
     * Corresponds to Open_GeoBlock / GEO_LoadMapBlock in the original.
     * @param blockId  DAX GEO block ID to load
     * @return VmResult
     */
    virtual VmResult loadGeoBlock(uint8 blockId) { return VM_OK; }

    /**
     * Load a wallset definition and its associated 8x8 tile graphics into
     * a cache slot. Corresponds to Walldef_LoadBlock / GFX_LoadWallSet.
     * Slots 1-3 are dynamic (set per area); slots 0 and 4 are fixed at init.
     * If blockId is 0xFF the slot should be cleared/invalidated.
     * @param blockId  DAX WALLDEF block ID (0xFF = clear slot)
     * @param setSlot  Cache slot to populate (1, 2, or 3)
     * @return VmResult
     */
    virtual VmResult loadWallSet(uint8 blockId, uint8 setSlot) { return VM_OK; }

    /**
     * Load the outdoor/city icon strip (BACPAC file).
     * Called when the area is not in indoor/dungeon mode.
     * Corresponds to DAX_LoadIconBlock(s_BACPAC).
     * @return VmResult
     */
    virtual VmResult loadIconBlock() { return VM_OK; }

    /**
     * Get the current yield reason (for debugging).
     */
    virtual YieldReason getYieldReason() const = 0;

    /**
     * Set yield reason.
     */
    virtual void setYieldReason(YieldReason reason) = 0;
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_SYSCALL_HANDLER_H
