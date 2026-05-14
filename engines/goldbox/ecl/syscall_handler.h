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
#include "goldbox/ecl/ecl_types.h"

namespace Goldbox {

namespace ECL {

/**
 * Interface for ECL syscalls (I/O, combat, menus, etc.).
 * All blocking operations are synchronous: the syscall pumps the event loop
 * internally and only returns when the operation is fully resolved. The ECL VM
 * never suspends mid-script; save points are always between ECL invocations.
 */
class SyscallHandler {
public:
    virtual ~SyscallHandler() = default;

    /**
     * Print text to the message/text box.
     * @param text     Text to print (may be encoded)
     * @param clearBox If true, clear text box before printing
     */
    virtual void printText(const Common::String &text, bool clearBox = false) = 0;

    /**
     * Input a number from the player. Blocks until the player confirms.
     * @param maxDigits Maximum digits to accept
     * @return Value entered by the player (non-negative), or -1 on cancel
     */
    virtual int16 inputNumber(uint8 maxDigits) = 0;

    /**
     * Input a string from the player. Blocks until the player confirms.
     * @param maxLength Maximum characters to accept
     * @return String entered by the player (may be empty on cancel)
     */
    virtual Common::String inputString(uint8 maxLength) = 0;

    /**
     * Display picture/sprite. Blocks until display is complete.
     * @param picID Picture ID (0xFF to clear display and redraw 3D view)
     * @return VmResult
     */
    virtual VmResult displayPicture(uint8 picID) = 0;

    /**
     * Draw encounter stage with sprite and portrait (0x0C SPRITE START).
     * Calculates monster distance from party position and draws the 3D encounter view.
     * @param resourceId Sprite resource ID (DAX sprite sheet)
     * @param distanceCap Maximum distance cap (clamps calculated distance)
     * @param variantId Picture/portrait variant ID
     * @return VmResult
     */
    virtual VmResult drawEncounterStage(uint8 resourceId, uint8 distanceCap,
            uint8 variantId) { return VM_OK; }

    /**
     * Redraw encounter stage with new distance (0x0D SPRITE ADVANCE).
     * Host maintains sprite and variant state from last drawEncounterStage call.
     * @param newDistance New monster distance to display
     * @return VmResult
     */
    virtual VmResult redrawEncounterStage(uint8 newDistance) { return VM_OK; }

    /**
     * Disable active encounter sprite overlay and refresh 3D area (0x31 SPRITE OFF).
     * Called only when runtime sprite-load flag is set.
     */
    virtual VmResult spriteOff() { return VM_OK; }

    /**
     * Start an async delay operation (0x3A DELAY).
     * Host reads its own CFG_GAME_SPEED configuration.
     * Delay time = gameSpeed * 5 milliseconds.
     * @return VM_YIELD to suspend VM, VM_OK if delay not supported
     */
    virtual VmResult beginDelay() {
        return VM_OK;
    }

    /**
     * Enable or disable legacy text draw delay pacing.
     * PRINT/PRINTCLEAR set this true for the duration of one text draw.
     */
    virtual void setTextDelayEnabled(bool enabled) {
        (void)enabled;
    }

    /**
     * Start asynchronous PRINT/PRINTCLEAR rendering.
     *
     * Return values:
     * - VM_YIELD: async print started; VM should suspend.
     * - VM_OK: async path not used; caller should fallback to printText().
     * - VM_ERROR: failed to start async operation.
     */
    virtual VmResult beginPrintAsync(const Common::String &text,
            bool clearBox) {
        (void)text;
        (void)clearBox;
        return VM_OK;
    }

    /**
     * Display a vertical (list) menu and wait for selection. Blocks until the
     * player selects an entry.
     * @param message Prompt message shown above the list
     * @param options Array of option strings
     * @return 0-based selection index, or -1 on cancel
     */
    virtual int16 verticalMenu(const Common::String &message,
            const Common::Array<Common::String> &options) = 0;

    /**
     * Display a horizontal (hotkey) menu and wait for selection. Blocks until
     * the player selects an entry.
     * @param options Array of option strings (one per key)
     * @return 0-based selection index, or -1 on cancel
     */
    virtual int16 horizontalMenu(const Common::Array<Common::String> &options) = 0;

    /**
     * Start the main combat loop with currently loaded monsters. Blocks until
     * combat resolution.
     * @return VmResult
     */
    virtual VmResult startCombat() = 0;

    /**
     * Execute a special program routine. Blocks until the program completes.
     * @param programID Program ID (0=training, 8=win, 9=camp)
     * @return VM_OK on completion, VM_HALTED if the program ends the game
     */
    virtual VmResult executeProgram(uint8 programID) = 0;

    /**
     * Clear the text/message box.
     */
    virtual void clearTextBox() = 0;

    /**
     * Load a new ECL script, replacing the current one.
     * @param scriptID Script ID to load
     * @return VM_HALTED to signal that the current script chain has ended
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
     * Called by EclVM when both geo (BOOL_GEO_READY) and wallset
     * (BOOL_WALLSET_READY) data have been loaded for the current area.
     * Mirrors the final redraw check at the end of INSTR_LoadAreaDeco:
     *   GAME_ScreenByState() + DIALOG_ShowParty() in the original binary.
     * Default is a no-op; override in the game-specific handler.
     * @return VmResult
     */
    virtual VmResult onMapDataReady() { return VM_OK; }
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_SYSCALL_HANDLER_H
