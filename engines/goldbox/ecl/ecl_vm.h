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

#ifndef GOLDBOX_ECL_ECL_VM_H
#define GOLDBOX_ECL_ECL_VM_H

#include "common/array.h"
#include "common/ptr.h"
#include "common/scummsys.h"
#include "common/span.h"
#include "goldbox/vm_interface.h"
#include "goldbox/ecl/ecl_decoder.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/ecl/game_config.h"
#include "goldbox/ecl/syscall_handler.h"

namespace Goldbox {
namespace ECL {

/**
 * ECL Block Entry Points (from 10-byte header).
 * Each ECL block starts with 5 word addresses (little-endian):
 *   [0-1]:  vm_run_addr_1 - Main execution entry point
 *   [2-3]:  SearchLocationAddr - Location search handler
 *   [4-5]:  PreCampCheckAddr - Pre-camp validation
 *   [6-7]:  CampInterruptedAddr - Camp interruption handler
 *   [8-9]:  ecl_initial_entryPoint - Initial startup entry
 *
 * Event System Integration:
 *   - Maps reference ECL blocks via 0x21 (LOAD FILES) with geoID and block number
 *   - Event numbers (0-127) stored in map cells trigger ECL execution
 *   - ON GOTO/GOSUB (0x25/0x26) dispatch to event subroutines
 *   - Annotations in disassembly track event entry points for debugging
 */
enum class ECLEntryPoint {
    ON_MOVE           = 0, // Executed during party movement
    ON_SEARCH         = 1, // When searching a location
    ON_REST           = 2, // When party rests
    ON_REST_INTERRUPT = 3, // When rest is interrupted
    ON_INIT           = 4  // Initialization event on map
};

/**
 * ECL Script VM: Bytecode interpreter with entry-point dispatch.
 * Manages parsed programs, memory, stack, and execution flow.
 */
class EclVM {
public:
    explicit EclVM(GameConfig *config, SyscallHandler *syscalls = nullptr);
    ~EclVM();

    /**
     * Load and parse an ECL program.
     * Extracts entry points and prepares for execution.
     * @param program Raw ECL bytecode
     * @param scriptId For caching and debugging
     * @return DECODE_OK on success
     */
    DecodeStatus loadProgram(Common::Span<const uint8> program, uint8 scriptId);

    /**
     * Run script starting at an entry point.
     * Non-blocking; yields on async calls (I/O, combat, etc.).
     * @param entry Entry point selector
     * @param maxSteps Maximum instructions before timeout
     * @return Result code
     */
    VmResult runAtEntryPoint(EntryPointSelector entry, uint32 maxSteps = 10000);

    /**
     * Resume script execution from current PC.
     * Used after async operations complete.
     * @param maxSteps Maximum instructions
     * @return Result code
     */
    VmResult resume(uint32 maxSteps = 10000);

    /**
     * Step a single instruction.
     * @return Result code
     */
    VmResult step();

    /**
     * Get current PC.
     */
    uint16 getPC() const { return _pc; }

    /**
     * Set PC (for debugging/save/load).
     */
    void setPC(uint16 pc) { _pc = pc; }

    /**
     * Access address space for direct memory operations.
     */
    AddressSpace &getMemory() { return _memory; }

    /**
     * Get decoded program (for inspection).
     */
    const Common::Array<EclInstruction> &getProgram() const { return _program; }

    /**
     * Get current script ID.
     */
    uint8 getScriptId() const { return _scriptId; }

    /**
     * Dump memory region for debugging.
     */
    Common::String dumpMemory(uint16 startAddr, uint16 length) const;

    /**
     * Set syscall handler (for I/O, menus, combat).
     */
    void setSyscallHandler(SyscallHandler *handler) { _syscalls = handler; }

private:
    GameConfig *_config;
    SyscallHandler *_syscalls;
    AddressSpace _memory;
    Common::Array<EclInstruction> _program;
    uint16 _pc;
    uint8 _scriptId;
    Common::Array<uint16> _callStack;
    Common::Array<uint16> _entryPoints;

    /**
     * Parse ECL header (first 10 bytes) to extract entry point offsets.
     * @param program Raw bytecode starting with header
     * @return True if header parsed successfully
     */
    bool parseECLHeader(Common::Span<const uint8> program);

    /**
     * Initialize ECL execution state.
     * Clears transient flags, sets default state, prepares memory.
     */
    void initializeECLState();

    VmResult executeInstruction(const EclInstruction &insn);
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ECL_VM_H
