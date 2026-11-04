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

#ifndef GOLDBOX_ECL_OPCODE_HANDLERS_H
#define GOLDBOX_ECL_OPCODE_HANDLERS_H

#include "common/array.h"
#include "common/random.h"
#include "common/scummsys.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/ecl/runtime_layout.h"
#include "goldbox/ecl/syscall_handler.h"

namespace Goldbox {
namespace ECL {

// Forward declaration — full type in ecl_vm.h.
class EclVM;

/**
 * Handler function for an opcode.
 * Returns int (cast from VmResult): VM_OK to continue, VM_YIELD for async, VM_HALTED to exit.
 * EclVM provides getOperand(N)/getOpWord(i)/readVar(i)/readString(i) for operand access.
 *
 * All operand data is accessed at runtime via EclVM methods that read directly
 * from VM flat memory. There is no pre-decoded operand array.
 */
typedef int (*OpcodeHandler)(EclVM &vm, AddressSpace &mem,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls);

/**
 * Register all opcode handlers.
 * Called once at VM startup.
 */
void clearOpcodeHandlers();

/**
 * Register one opcode handler.
 */
void registerOpcodeHandler(uint8 opcode, OpcodeHandler handler);

/**
 * Configure the VM/global/runtime layout tables used by shared core handlers.
 * Must be called by the active game dialect before any opcode execution.
 */
void setOpcodeLayout(const Goldbox::VmLayout &vmLayout,
    const Goldbox::VmGlobalLayout &vmGlobalLayout,
    const EclRuntimeLayout &runtimeLayout,
    uint16 characterBase, uint16 characterSize);

/**
 * Register the baseline opcode handler table.
 *
 * This registers handlers for all baseline opcodes (0x00-0x3D).
 * Game-specific handlers are registered separately by dialect configuration.
 */
void registerBaselineOpcodeHandlers();

/**
 * Get handler for an opcode.
 */
OpcodeHandler getOpcodeHandler(uint8 opcode);

/**
 * Get the shared layout accessor used by opcode handlers.
 * Backed by the Pool of Radiance VM / global / runtime layout tables.
 */
EclLayoutAccess getOpcodeLayout();

/**
 * Get the shared random source used by opcode handlers.
 */
Common::RandomSource &getOpcodeRandom();

/**
 * Run the opcode 0x24 ENCOUNTER post-encounter cleanup tail (GB13-009).
 *
 * The three cleanup steps that originally ran at the bottom of the ENCOUNTER
 * handler after the service/combat/shop/temple call returned:
 *   1. reset the runtime game state from the indoor-mode flag,
 *   2. clear the search flags keeping only bit 0,
 *   3. clear the skybox redraw flag.
 *
 * Factored into one helper so the inline (non-yield, VM_OK) path in
 * handle_0x24_ENCOUNTER and the resume path in the host's finalize seam run
 * byte-identical mutations. The yield path returns early and skips the inline
 * tail, so the host runs this exactly once after the yielded dialog completes,
 * before the VM fetches the recorded resume PC. Reads/writes go through the
 * shared layout (getOpcodeLayout); no VM re-execution of the opcode.
 */
void runEncounterCleanupTail(AddressSpace &mem);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_OPCODE_HANDLERS_H
