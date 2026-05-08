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
#include "goldbox/ecl/game_config.h"

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
 * The decoder only needs to skip operand bytes to find instruction boundaries.
 * All runtime operand access is done by EclVM::getOperand() reading from flat memory.
 */

static bool read8(Common::Span<const uint8> program, uint32 offset, uint8 &val) {
    if (offset >= program.size()) {
        return false;
    }
    val = program[offset];
    return true;
}

static uint32 getPackedStringSize(uint8 decodedLength) {
    return (decodedLength * 3 + 3) / 4;
}

enum OperandCountMode {
    kOperandCountFixed = 0,
    kOperandCountVerticalMenu,
    kOperandCountOnJump,
    kOperandCountHorizontalMenu
};

static bool getOperandCountPolicy(uint8 opcode, int &baseCount,
        OperandCountMode &mode, const GameConfig *config) {
    mode = kOperandCountFixed;

    switch (opcode) {
    case 0x00: baseCount = 0; return true;
    case 0x01: baseCount = 1; return true;
    case 0x02: baseCount = 1; return true;
    case 0x03: baseCount = 2; return true;
    case 0x04: baseCount = 3; return true;
    case 0x05: baseCount = 3; return true;
    case 0x06: baseCount = 3; return true;
    case 0x07: baseCount = 3; return true;
    case 0x08: baseCount = 2; return true;
    case 0x09: baseCount = 2; return true;
    case 0x0A: baseCount = 1; return true;
    case 0x0B: baseCount = 3; return true;
    case 0x0C: baseCount = 3; return true;
    case 0x0D: baseCount = 0; return true;
    case 0x0E: baseCount = 1; return true;
    case 0x0F: baseCount = 2; return true;
    case 0x10: baseCount = 2; return true;
    case 0x11: baseCount = 1; return true;
    case 0x12: baseCount = 1; return true;
    case 0x13: baseCount = 0; return true;
    case 0x14: baseCount = 4; return true;
    case 0x15: baseCount = 3; mode = kOperandCountVerticalMenu; return true;
    case 0x16: baseCount = 0; return true;
    case 0x17: baseCount = 0; return true;
    case 0x18: baseCount = 0; return true;
    case 0x19: baseCount = 0; return true;
    case 0x1A: baseCount = 0; return true;
    case 0x1B: baseCount = 0; return true;
    case 0x1C: baseCount = 0; return true;
    case 0x1D: baseCount = 1; return true;
    case 0x1E: baseCount = 6; return true;
    case 0x1F: baseCount = 0; return true;
    case 0x20: baseCount = 1; return true;
    case 0x21: baseCount = 3; return true;
    case 0x22: baseCount = 2; return true;
    case 0x23: baseCount = 4; return true;
    case 0x24: baseCount = 0; return true;
    case 0x25: baseCount = 2; mode = kOperandCountOnJump; return true;
    case 0x26: baseCount = 2; mode = kOperandCountOnJump; return true;
    case 0x27: baseCount = 8; return true;
    case 0x28: baseCount = 3; return true;
    case 0x29: baseCount = 14; return true;
    case 0x2A: baseCount = 3; return true;
    case 0x2B: baseCount = 2; mode = kOperandCountHorizontalMenu; return true;
    case 0x2C: baseCount = 6; return true;
    case 0x2D: baseCount = 1; return true;
    case 0x2E: baseCount = 5; return true;
    case 0x2F: baseCount = 3; return true;
    case 0x30: baseCount = 3; return true;
    case 0x31: baseCount = 0; return true;
    case 0x32: baseCount = 1; return true;
    case 0x33: baseCount = 0; return true;
    case 0x34: baseCount = 2; return true;
    case 0x35: baseCount = 3; return true;
    case 0x36: baseCount = 2; return true;
    case 0x37: baseCount = 3; return true;
    case 0x38: baseCount = 1; return true;
    case 0x39: baseCount = 1; return true;
    case 0x3A: baseCount = 0; return true;
    case 0x3B: baseCount = 3; return true;
    case 0x3C: baseCount = 1; return true;
    case 0x3D: baseCount = 0; return true;
    case 0x3E: baseCount = 0; return true;
    case 0x3F: baseCount = 1; return true;
    case 0x40: baseCount = 1; return true;
    case 0x41: baseCount = 2; return true;
    case 0x42: baseCount = 0; return true;
    case 0x43: baseCount = 1; return true;
    case 0x44: baseCount = 0; return true;
    case 0x45: baseCount = 2; return true;
    case 0x46: baseCount = 2; return true;
    case 0x47: baseCount = 0; return true;
    case 0x48: baseCount = 1; return true;
    case 0x49: baseCount = 6; return true;
    case 0x4A: baseCount = 0; return true;
    case 0x4B: baseCount = 1; return true;
    case 0x4C: baseCount = 2; return true;
    default:
        break;
    }

    if (config && config->getOpcodeOperandCount(opcode, baseCount)) {
        return true;
    }

    return false;
}

/**
 * Skip one tagged operand in the bytecode stream, advancing pc.
 * Also reads the immediate value for varargs count detection.
 * @param program Raw bytecode
 * @param pc Current position (updated on success)
 * @param outValue The decoded u16 value of this operand (for varargs count)
 * @return DECODE_OK on success
 */
static DecodeStatus skipTaggedOperand(Common::Span<const uint8> program,
        uint32 &pc, uint16 &outValue) {
    uint8 typeTag = 0;
    uint8 low = 0;
    if (!read8(program, pc, typeTag) || !read8(program, pc + 1, low)) {
        return DECODE_OUT_OF_BOUNDS;
    }

    outValue = 0;

    switch (typeTag) {
    case 0x00:
        outValue = low;
        pc += 2;
        return DECODE_OK;

    case 0x01:
    case 0x03: {
        uint8 high = 0;
        if (!read8(program, pc + 2, high))
            return DECODE_OUT_OF_BOUNDS;
        outValue = (uint16)(low | (high << 8));
        pc += 3;
        return DECODE_OK;
    }

    case 0x02: {
        uint8 high = 0;
        if (!read8(program, pc + 2, high))
            return DECODE_OUT_OF_BOUNDS;
        outValue = (uint16)(low | (high << 8));
        pc += 3;
        return DECODE_OK;
    }

    case 0x80: {
        uint32 packedSize = getPackedStringSize(low);
        if (pc + 2 + packedSize > program.size())
            return DECODE_OUT_OF_BOUNDS;
        outValue = low;
        pc += 2 + packedSize;
        return DECODE_OK;
    }

    case 0x81: {
        uint8 high = 0;
        if (!read8(program, pc + 2, high))
            return DECODE_OUT_OF_BOUNDS;
        outValue = (uint16)(low | (high << 8));
        pc += 3;
        return DECODE_OK;
    }

    default:
        return DECODE_MALFORMED_OPERAND;
    }
}

DecodeStatus decodeProgram(Common::Span<const uint8> program, uint16 startPc,
    Common::Array<EclInstruction> &outInstructions,
    const GameConfig *config) {
    outInstructions.clear();

    uint32 pc = startPc;
    while (pc < program.size()) {
        uint8 opcode = 0;
        if (!read8(program, pc, opcode)) {
            return DECODE_OUT_OF_BOUNDS;
        }

        const OpcodeInfo *info = getOpcodeInfo(opcode);
        if (!info) {
            return DECODE_UNKNOWN_OPCODE;
        }

        EclInstruction insn;
        insn.pc = (uint16)pc;
        insn.opcode = opcode;

        pc += 1;

        int targetOperandCount = 0;
        OperandCountMode countMode = kOperandCountFixed;
        if (!getOperandCountPolicy(opcode, targetOperandCount, countMode,
                config)) {
            return DECODE_UNKNOWN_OPCODE;
        }

        // Skip operands to find next instruction boundary.
        // For varargs opcodes, read the count operand to determine total.
        int operandsSkipped = 0;
        while (operandsSkipped < targetOperandCount) {
            uint16 opValue = 0;
            DecodeStatus opStatus = skipTaggedOperand(program, pc, opValue);
            if (opStatus != DECODE_OK) {
                return opStatus;
            }
            operandsSkipped++;

            // Vararg-style: use the count operand to extend target count.
            if (countMode == kOperandCountVerticalMenu &&
                    operandsSkipped == 3) {
                targetOperandCount = 3 + opValue;
            } else if (countMode == kOperandCountOnJump &&
                    operandsSkipped == 2) {
                targetOperandCount = 2 + opValue;
            } else if (countMode == kOperandCountHorizontalMenu &&
                    operandsSkipped == 2) {
                targetOperandCount = 2 + opValue;
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
            curr = (thisByte >> 2) & 0x3F;
            break;
        case 2:
            curr = ((lastByte << 4) | (thisByte >> 4)) & 0x3F;
            break;
        case 3:
            curr = ((lastByte << 2) | (thisByte >> 6)) & 0x3F;
            break;
        }

        if (curr <= 0x1F) {
            curr += 0x40;
        }

        result += (char)curr;
        outputCount++;

        lastByte = thisByte;
        state = (state % 3) + 1;

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
