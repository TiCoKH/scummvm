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

#ifndef GOLDBOX_ECL_OPCODE_TABLE_H
#define GOLDBOX_ECL_OPCODE_TABLE_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Goldbox {
namespace ECL {

/**
 * ECL Operand Types (matching original interpreter).
 * Original encoding in bytecode:
 *   0x00: Immediate byte value
 *   0x01: Memory reference (word)
 *   0x02: Immediate word value
 *   0x03: Memory reference (word, alternate)
 *   0x80: Compressed 6-bit string
 *   0x81: String from memory address
 */
enum class OperandType {
    NONE,     // No operand
    VAL8,     // 8-bit value (0x00 in bytecode)
    ADDR16,   // 16-bit address (0x01 or 0x03 in bytecode)
    VAL16,    // 16-bit value (0x02 in bytecode)
    STRING,   // String (0x80=compressed 6-bit, 0x81=memory reference)
    VARARGS   // Variable number of operands (for menus, ON GOTO, etc.)
};

/**
 * ECL opcode table entry.
 * Maps opcode byte to name, operand info, and routing for syscalls.
 */
struct OpcodeInfo {
    uint8 opcode;
    const char *name;
    OperandType *operands; // Array of operand types (null-terminated)
    const char *description;
};

/**
 * Lookup opcode info by opcode byte.
 * Returns nullptr if opcode is undefined.
 */
OpcodeInfo *getOpcodeInfo(uint8 opcode);

/**
 * Get human-readable opcode name.
 */
const char *getOpcodeName(uint8 opcode);

/**
 * Operand count for an opcode.
 * Returns 0 for opcodes with variable operands.
 */
int getOperandCount(uint8 opcode);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_OPCODE_TABLE_H
