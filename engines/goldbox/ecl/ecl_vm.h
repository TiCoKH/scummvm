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
#include "goldbox/ecl/ecl_types.h"
#include "goldbox/ecl/ecl_decoder.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/ecl/game_config.h"
#include "goldbox/ecl/syscall_handler.h"

namespace Goldbox {
namespace ECL {

/**
 * ECL Script VM: Bytecode interpreter with entry-point dispatch.
 * Manages parsed programs, memory, stack, and execution flow.
 */
class EclVM {
public:
    explicit EclVM(GameConfig *config, SyscallHandler *syscalls = nullptr);
    ~EclVM();

    static const uint16 kEclHeaderWordCount = 5;
    static const uint16 kEclHeaderSize = kEclHeaderWordCount * 2;

    /**
     * Load and parse an ECL program.
     * Extracts entry points and prepares for execution.
     * @param program Raw ECL bytecode
     * @param scriptId For caching and debugging
     * @return DECODE_OK on success
     */
    DecodeStatus loadProgram(Common::Span<const uint8> program, uint8 scriptId);

    /**
     * Run script starting at an entry point. Synchronous: all syscalls block
     * internally until resolved; this method returns when the script halts.
     * @param entry Entry point selector
     * @param maxSteps Watchdog limit (guards against infinite loops in bad scripts)
     * @return Result code
     */
    VmResult runAtEntryPoint(ECLEntryPoint entry, uint32 maxSteps = 1000000);

    /**
     * Run script starting at an arbitrary bytecode address (original ENGINE_Execute behavior).
     * Synchronous: all syscalls block internally until resolved.
     * @param scriptPc Bytecode PC in VM address space (e.g. 0x9900+offset)
     * @param maxSteps Watchdog limit
     * @return Result code
     */
    VmResult runAtScriptAddress(uint16 scriptPc, uint32 maxSteps = 1000000);

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
    void setPC(uint16 pc);

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

    /**
     * Decode N operands from VM flat memory at the current instruction PC.
     * Mirrors original VM_GetOprand(N): reads type tag + bytes from script stream,
     * builds _opValues[1..N] (decoded words) and _opTypes[1..N] (type tags).
     * _opValues[0] stores N as a count sentinel.
     * Uses MemorySeekableReadWriteStream for LE word reads.
     */
    void getOperand(uint8 opCount);

    /**
     * Return decoded word at 1-based index from the last getOperand call.
     * For ADDR16 (0x01/0x03/0x81) this is the raw address.
     * For VAL8/VAL16 (0x00/0x02) this is the immediate value.
     * Index 0 returns the count sentinel.
     */
    uint16 getOpWord(uint8 index) const;

    /**
     * Return the operand type tag at 1-based index from the last getOperand call.
     * Matches original tags: 0x00=VAL8, 0x01/0x03=ADDR16, 0x02=VAL16,
     * 0x80=STRING_INLINE, 0x81=STRING_PTR.
     */
    uint8 getOpType(uint8 index) const;

    /**
     * Resolve operand N as a numeric value.
     * ADDR16 (0x01/0x03): dereferences VM memory (read16LE at the address).
     * VAL8/VAL16 (0x00/0x02): returns the immediate value directly.
     * Mirrors original toWord(HI_SAVE[i], LOW_SAVE[i]) + optional dereference.
     */
    uint16 readVar(uint8 index) const;

    /**
     * Read operand N as a string.
     * 0x80 (compressed inline): decompresses 6-bit packed data from VM bytes.
     * 0x81/0x03/0x01 (pointer): reads null-terminated string from VM flat memory.
     */
    Common::String readString(uint8 index) const;

private:
    GameConfig *_config;
    SyscallHandler *_syscalls;
    AddressSpace _memory;
    Common::Array<EclInstruction> _program;
    uint16 _pc;
    uint8 _scriptId;
    Common::Array<uint16> _callStack;
    Common::Array<uint16> _entryPoints;

    // Operand decode buffer — populated by getOperand().
    // _opValues[0] = count, _opValues[1..N] = decoded word per operand.
    // _opTypes[0]  = 0,     _opTypes[1..N]  = type tag per operand.
    Common::Array<uint16> _opValues;
    Common::Array<uint8>  _opTypes;
    uint16 _opStartPc;

    /**
     * Find decoded instruction index by bytecode PC.
     * @param scriptPc Bytecode PC in VM address space
     * @return instruction index or -1 if not found
     */
    int findInstructionIndexByPc(uint16 scriptPc) const;

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

    /**
     * Keep the runtime WORD_ECL_PC mirror synchronized with VM state.
     */
    void syncRuntimePc(uint16 pc);

    VmResult executeInstruction(const EclInstruction &insn, uint16 defaultNextPc);
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ECL_VM_H
