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

#include "goldbox/ecl/ecl_decoder.h"

namespace Goldbox {
namespace ECL {

/**
 * ECL Bytecode Operand Encoding:
 * Original interpreter uses type bytes before each operand:
 *   0x00: Immediate byte value (VAL8)
 *   0x01: Memory reference word (ADDR16)
 *   0x02: Immediate word value (VAL16)
 *   0x03: Memory reference word (ADDR16, alternate form)
 *   0x80: Compressed 6-bit string (length + packed data)
 *   0x81: String from memory address (pointer to null-terminated string)
 *
 * Current decoder uses opcode table to determine operand types statically.
 * Runtime operand type detection would require bytecode pattern matching.
 */

static bool read8(Common::Span<const uint8> program, uint32 offset, uint8 &val) {
    if (offset >= program.size()) {
        return false;
    }
    val = program[offset];
    return true;
}

static bool read16(Common::Span<const uint8> program, uint32 offset, uint16 &val) {
    if (offset + 1 >= program.size()) {
        return false;
    }
    val = program[offset] | (program[offset + 1] << 8);
    return true;
}

static bool readString(Common::Span<const uint8> program, uint32 offset,
        Common::String &outStr, uint32 &bytesConsumed) {
    bytesConsumed = 0;
    uint8 marker = 0;
    if (!read8(program, offset, marker)) {
        return false;
    }

    if (marker == 0x80) {
        // Compressed 6-bit string: 0x80 <length> <compressed_data>
        bytesConsumed = 1;
        uint8 len = 0;
        if (!read8(program, offset + 1, len)) {
            return false;
        }
        bytesConsumed += 1;

        // Compressed size is approximately (len * 3) / 4, round up
        uint32 compressedSize = (len * 3 + 3) / 4;
        if (offset + 2 + compressedSize > program.size()) {
            return false;
        }

        outStr = decompress6BitString(&program[offset + 2], len);
        bytesConsumed += compressedSize;
        return true;
    } else if (marker == 0x81) {
        // String from memory: 0x81 <addr16>
        // Read address, actual string lookup deferred to runtime
        bytesConsumed = 1;
        uint16 addr = 0;
        if (!read16(program, offset + 1, addr)) {
            return false;
        }
        bytesConsumed += 2;
        outStr = Common::String::format("[MEM:0x%04X]", addr);
        return true;
    } else {
        // Legacy format: <length> <raw_bytes>
        uint8 len = marker;
        bytesConsumed = 1;
        if (offset + 1 + len > program.size()) {
            return false;
        }
        outStr.clear();
        for (uint8 i = 0; i < len; ++i) {
            outStr += (char)program[offset + 1 + i];
        }
        bytesConsumed += len;
        return true;
    }
}

DecodeStatus decodeProgram(Common::Span<const uint8> program, uint16 startPc,
        Common::Array<EclInstruction> &outInstructions) {
    outInstructions.clear();

    uint32 pc = startPc;
    while (pc < program.size()) {
        uint8 opcode = 0;
        if (!read8(program, pc, opcode)) {
            return DECODE_OUT_OF_BOUNDS;
        }

        OpcodeInfo *info = getOpcodeInfo(opcode);
        if (!info) {
            return DECODE_UNKNOWN_OPCODE;
        }

        // Varargs instructions need opcode-specific parsing not yet provided.
        if (info->operands && info->operands[0] == OperandType::VARARGS) {
            return DECODE_VARARGS_UNSUPPORTED;
        }

        EclInstruction insn;
        insn.pc = (uint16)pc;
        insn.opcode = opcode;

        pc += 1;

        if (info->operands) {
            for (int i = 0; info->operands[i] != OperandType::NONE; ++i) {
                OperandType t = info->operands[i];
                EclOperand val;
                val.type = t;
                val.u8 = 0;
                val.u16 = 0;

                switch (t) {
                case OperandType::VAL8: {
                    uint8 b = 0;
                    if (!read8(program, pc, b)) {
                        return DECODE_OUT_OF_BOUNDS;
                    }
                    val.u8 = b;
                    pc += 1;
                    break;
                }
                case OperandType::ADDR16:
                case OperandType::VAL16: {
                    uint16 w = 0;
                    if (!read16(program, pc, w)) {
                        return DECODE_OUT_OF_BOUNDS;
                    }
                    val.u16 = w;
                    pc += 2;
                    break;
                }
                case OperandType::STRING: {
                    uint32 consumed = 0;
                    Common::String s;
                    if (!readString(program, pc, s, consumed)) {
                        return DECODE_OUT_OF_BOUNDS;
                    }
                    val.str = s;
                    pc += consumed;
                    break;
                }
                case OperandType::VARARGS:
                    return DECODE_VARARGS_UNSUPPORTED;
                case OperandType::NONE:
                default:
                    break;
                }

                insn.operands.push_back(val);
            }
        }

        outInstructions.push_back(insn);
    }

    return DECODE_OK;
}

Common::String decompress6BitString(const uint8 *compressedData, uint8 length) {
    Common::String result;
    if (!compressedData || length == 0) {
        return result;
    }

    // 6-bit decompression: 4 chars packed into 3 bytes
    // 3-state decoder matching ECL specification
    int state = 1;
    uint8 lastByte = 0;
    uint32 byteIndex = 0;
    uint8 outputCount = 0;

    while (outputCount < length) {
        if (byteIndex >= length * 3 / 4 + 1) {
            break; // Safety check: compressed size ~= length * 3 / 4
        }

        uint8 thisByte = compressedData[byteIndex++];
        uint8 curr = 0;

        switch (state) {
        case 1:
            // State 1: extract bits [7:2]
            curr = (thisByte >> 2) & 0x3F;
            break;
        case 2:
            // State 2: combine lastByte[1:0] << 4 | thisByte[7:4]
            curr = ((lastByte << 4) | (thisByte >> 4)) & 0x3F;
            break;
        case 3:
            // State 3: combine lastByte[3:0] << 2 | thisByte[7:6]
            curr = ((lastByte << 2) | (thisByte >> 6)) & 0x3F;
            break;
        }

        // Inflate to ASCII range
        if (curr <= 0x1F) {
            curr += 0x40;
        }

        result += (char)curr;
        outputCount++;

        // Advance state
        lastByte = thisByte;
        state = (state % 3) + 1;

        // State 3 processes 2 chars from same byte
        if (state == 1 && outputCount < length) {
            curr = thisByte & 0x3F;
            if (curr <= 0x1F) {
                curr += 0x40;
            }
            result += (char)curr;
            outputCount++;
        }
    }

    return result;
}

} // namespace ECL
} // namespace Goldbox
