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

/**
 * ECL Script VM Implementation
 *
 * DaxEcl File Structure:
 * ----------------------
 * [Offset 0-9]:    10-byte header with 5 entry point addresses (word each, little-endian)
 * [Offset 10+]:    Bytecode stream with opcodes and operands
 *
 * Memory Layout (Pool of Radiance):
 * ---------------------------------
 * MemStart = 0x9900
 * MemBase = 0x10000 - 0x9900 = 0x6700
 *
 * String Compression (6-bit):
 * ---------------------------
 * Pattern: 0x80 <length> <compressed_data>
 * 3-state decoder packs 4 chars into 3 bytes:
 *   State 1: curr = (thisByte >> 2) & 0x3F
 *   State 2: curr = ((lastByte << 4) | (thisByte >> 4)) & 0x3F
 *   State 3: curr = ((lastByte << 2) | (thisByte >> 6)) & 0x3F
 *   Inflate: if (curr <= 0x1F) curr += 0x40
 *
 * Command Table: 60+ opcodes for game logic
 * Key Opcodes: 0x21 (LOAD FILES), 0x37 (LOAD PIECES), 0x25/0x26 (ON GOTO/GOSUB),
 *              0x0E (PICTURE), 0x24 (COMBAT)
 */

#include "goldbox/ecl/ecl_vm.h"
#include "goldbox/ecl/opcode_handlers.h"
#include "goldbox/ecl/game_config.h"
#include "common/memstream.h"

namespace Goldbox {
namespace ECL {

EclVM::EclVM(GameConfig *config, SyscallHandler *syscalls)
    : _config(config), _syscalls(syscalls), _pc(0), _scriptId(0xFF) {
    registerOpcodeHandlers();
}

EclVM::~EclVM() {
}

DecodeStatus EclVM::loadProgram(Common::Span<const uint8> program, uint8 scriptId) {
    _scriptId = scriptId;
    _pc = 0;
    _callStack.clear();
    _program.clear();
    _entryPoints.clear();

    // Parse entry points from header
    if (!parseECLHeader(program)) {
        return DECODE_OUT_OF_BOUNDS;
    }

    // Decode the entire program starting from offset 10 (after header)
    Common::Span<const uint8> programBody = program.subspan(10);
    DecodeStatus status = decodeProgram(programBody, 10, _program);
    if (status != DECODE_OK) {
        return status;
    }

    // Initialize ECL state for new script
    initializeECLState();

    return DECODE_OK;
}

bool EclVM::parseECLHeader(Common::Span<const uint8> program) {
    if (program.size() < 10) {
        return false;
    }

    // Parse 5 entry point offsets (little-endian, 2 bytes each)
    // [0-1]:  vm_run_addr_1 (Main execution)
    // [2-3]:  SearchLocationAddr
    // [4-5]:  PreCampCheckAddr
    // [6-7]:  CampInterruptedAddr
    // [8-9]:  ecl_initial_entryPoint
    for (int i = 0; i < 5; ++i) {
        uint16 offset = program[i * 2] | (program[i * 2 + 1] << 8);
        _entryPoints.push_back(offset);
    }

    return true;
}

void EclVM::initializeECLState() {
    if (!_config) return;

    // Clear transient flags (script-local variables)
    uint16 flagBase = _config->getFlagBase();
    uint16 transientCount = _config->getTransientFlagCount();
    for (uint16 i = 0; i < transientCount; ++i) {
        _memory.write8(flagBase + i, 0);
    }

    // Clear execution control flags
    _memory.write8(ECLMemoryLayout::OFFSET_ECL_HALT, 0);
    _memory.write8(ECLMemoryLayout::OFFSET_BREAK_FLAG, 0);
    _memory.write8(ECLMemoryLayout::OFFSET_EXIT_SCRIPT, 0);
    _memory.write8(ECLMemoryLayout::OFFSET_PROGRAM_STATE, 0);

    // Set default game state (dungeon)
    _memory.write8(ECLMemoryLayout::OFFSET_GAME_STATE, GS_DUNGEON_MAP);

    // Clear character pointers
    _memory.write16LE(ECLMemoryLayout::OFFSET_SEL_CHAR_PTR, 0);
    _memory.write16LE(ECLMemoryLayout::OFFSET_NEXT_CHAR_PTR, 0);

    // Clear menu/combat state
    _memory.write8(ECLMemoryLayout::OFFSET_MENU_COMBAT_STATE, 0);

    // Clear text output flags
    _memory.write8(ECLMemoryLayout::OFFSET_TEXT_PRINT_FLAG, 0);
    _memory.write8(ECLMemoryLayout::OFFSET_TEXT_OUTPUT_FLAG, 0);
}

VmResult EclVM::runAtEntryPoint(EntryPointSelector entry, uint32 maxSteps) {
    int entryIdx = static_cast<int>(entry);
    if (entryIdx < 0 || entryIdx >= 5) {
        return VM_ERROR;
    }

    // Look up entry point offset from parsed header
    if (entryIdx >= (int)_entryPoints.size()) {
        return VM_ERROR;
    }

    uint16 offset = _entryPoints[entryIdx];

    // Find instruction at this offset
    _pc = 0;
    for (uint i = 0; i < _program.size(); ++i) {
        if (_program[i].pc == offset) {
            _pc = i;
            break;
        }
    }

    return resume(maxSteps);
}

VmResult EclVM::resume(uint32 maxSteps) {
    for (uint32 i = 0; i < maxSteps; ++i) {
        VmResult r = step();
        if (r != VM_OK) {
            return r;
        }
    }
    return VM_OK;
}

VmResult EclVM::step() {
    if (_pc >= _program.size()) {
        return VM_HALTED;
    }

    const EclInstruction &insn = _program[_pc];
    _pc += 1;

    return executeInstruction(insn);
}

VmResult EclVM::executeInstruction(const EclInstruction &insn) {
    OpcodeHandler handler = getOpcodeHandler(insn.opcode);
    if (!handler) {
        // No handler registered; stub with yield
        return VM_YIELD;
    }

    uint16 nextPc = _pc;
    VmResult result = static_cast<VmResult>(handler(_memory, insn, nextPc, _callStack, _syscalls));

    if (result == VM_OK) {
        _pc = nextPc;
    }

    return result;
}

Common::String EclVM::dumpMemory(uint16 startAddr, uint16 length) const {
    return _memory.dumpRegion(startAddr, length);
}

} // namespace ECL
} // namespace Goldbox
