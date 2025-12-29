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

#ifndef GOLDBOX_ECL_ECL_DECODER_H
#define GOLDBOX_ECL_ECL_DECODER_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/span.h"
#include "common/str.h"
#include "goldbox/ecl/opcode_table.h"

namespace Goldbox {
namespace ECL {

enum DecodeStatus {
    DECODE_OK = 0,
    DECODE_OUT_OF_BOUNDS,
    DECODE_UNKNOWN_OPCODE,
    DECODE_VARARGS_UNSUPPORTED,
    DECODE_MALFORMED_OPERAND
};

struct EclOperand {
    OperandType type;
    uint8 u8;
    uint16 u16;
    Common::String str;
    Common::Array<uint8> bytes;
};

struct EclInstruction {
    uint16 pc;
    uint8 opcode;
    Common::Array<EclOperand> operands;
};

/**
 * Helper class for reading operands from a decoded instruction.
 * Matches original VM semantics where operands are pre-buffered and accessed by index.
 * Supports both sequential reading and random access.
 */
class OperandBuffer {
public:
    explicit OperandBuffer(const EclInstruction &insn)
        : _insn(insn), _nextIndex(0) {}

    /**
     * Read next operand as 8-bit value (sequential access).
     * Mimics original: uVar1 = thunk_FUN_0022752e('\x01')
     * @return 8-bit unsigned value, 0 if out of bounds
     */
    uint8 readU8() {
        if (_nextIndex < (int)_insn.operands.size()) {
            return _insn.operands[_nextIndex++].u8;
        }
        return 0;
    }

    /**
     * Read next operand as 16-bit value (sequential access).
     * Mimics original: toWord(0, 0)
     * @return 16-bit unsigned value, 0 if out of bounds
     */
    uint16 readU16() {
        if (_nextIndex < (int)_insn.operands.size()) {
            const EclOperand &op = _insn.operands[_nextIndex++];
            if (op.type == OperandType::VAL16 || op.type == OperandType::ADDR16) {
                return op.u16;
            }
            // If only 8-bit available, pad with zero
            return op.u8;
        }
        return 0;
    }

    /**
     * Read operand at specific 1-based index (random access).
     * Mimics original: getOperand(1), getOperand(2), etc.
     * @param index 1-based operand number
     * @return 8-bit value, 0 if out of bounds
     */
    uint8 readByIndex(int index) {
        if (index < 1 || index > (int)_insn.operands.size()) {
            return 0;
        }
        return _insn.operands[index - 1].u8;
    }

    /**
     * Read 16-bit operand at specific 1-based index.
     * @param index 1-based operand number
     * @return 16-bit value, 0 if out of bounds
     */
    uint16 readU16ByIndex(int index) {
        if (index < 1 || index > (int)_insn.operands.size()) {
            return 0;
        }
        const EclOperand &op = _insn.operands[index - 1];
        if (op.type == OperandType::VAL16 || op.type == OperandType::ADDR16) {
            return op.u16;
        }
        return op.u8;
    }

    /**
     * Get total number of operands available.
     * Useful for bounds checking.
     */
    int count() const { return _insn.operands.size(); }

    /**
     * Reset sequential reader to start.
     */
    void reset() { _nextIndex = 0; }

private:
    const EclInstruction &_insn;
    int _nextIndex;
};

/**
 * Decode a raw ECL program into instructions.
 * For now, varargs opcodes are flagged as unsupported.
 * @param program Raw byte span of the ECL block
 * @param startPc Starting PC (usually 0)
 * @param outInstructions Output decoded instruction list
 * @return DecodeStatus
 */
DecodeStatus decodeProgram(Common::Span<const uint8> program, uint16 startPc,
        Common::Array<EclInstruction> &outInstructions);

/**
 * Decompress 6-bit packed string.
 * ECL uses 3-state decoder: 4 chars packed into 3 bytes.
 * Pattern: 0x80 <length> <compressed_data>
 *
 * State machine:
 *   State 1: curr = (thisByte >> 2) & 0x3F
 *   State 2: curr = ((lastByte << 4) | (thisByte >> 4)) & 0x3F
 *   State 3: curr = ((lastByte << 2) | (thisByte >> 6)) & 0x3F
 *   Inflate: if (curr <= 0x1F) curr += 0x40
 *
 * @param compressedData Compressed byte sequence
 * @param length Expected output character count
 * @return Decompressed string
 */
Common::String decompress6BitString(const uint8 *compressedData, uint8 length);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ECL_DECODER_H
