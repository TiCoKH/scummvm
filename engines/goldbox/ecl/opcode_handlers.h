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
#include "common/scummsys.h"
#include "goldbox/ecl/ecl_decoder.h"
#include "goldbox/ecl/ecl_memory.h"
#include "goldbox/ecl/syscall_handler.h"

namespace Goldbox {
namespace ECL {

/**
 * Helper to read operand values from a decoded instruction.
 */
class OperandReader {
public:
    explicit OperandReader(const EclInstruction &insn) : _insn(insn), _index(0) {}

    uint8 readU8() {
        if (_index < (int)_insn.operands.size() && _insn.operands[_index].type == OperandType::VAL8) {
            return _insn.operands[_index++].u8;
        }
        return 0;
    }

    uint16 readU16() {
        if (_index < (int)_insn.operands.size() &&
            (_insn.operands[_index].type == OperandType::VAL16 ||
             _insn.operands[_index].type == OperandType::ADDR16)) {
            return _insn.operands[_index++].u16;
        }
        return 0;
    }

    uint16 readAddr() {
        return readU16();
    }

private:
    const EclInstruction &_insn;
    int _index;
};

/**
 * Handler function for an opcode.
 * Returns int (cast from VmResult): VM_OK to continue, VM_YIELD for async, VM_HALTED to exit.
 */
typedef int (*OpcodeHandler)(AddressSpace &mem, const EclInstruction &insn,
        uint16 &nextPc, Common::Array<uint16> &callStack, SyscallHandler *syscalls);

/**
 * Register all opcode handlers.
 * Called once at VM startup.
 */
void registerOpcodeHandlers();

/**
 * Get handler for an opcode.
 */
OpcodeHandler getOpcodeHandler(uint8 opcode);

/**
 * Helper to resolve a variable value (dereference if address).
 */
uint16 resolveVar(AddressSpace &mem, const EclOperand &operand);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_OPCODE_HANDLERS_H
