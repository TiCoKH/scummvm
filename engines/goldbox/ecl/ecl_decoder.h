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

class GameConfig;

enum DecodeStatus {
    DECODE_OK = 0,
    DECODE_OUT_OF_BOUNDS,
    DECODE_UNKNOWN_OPCODE,
    DECODE_VARARGS_UNSUPPORTED,
    DECODE_MALFORMED_OPERAND
};

/**
 * Lightweight instruction record used only for PC-to-index mapping.
 * All operand access at runtime goes through EclVM::getOperand()/readVar()/readString()
 * which read directly from VM flat memory (Path B). No operand data is stored here.
 */
struct EclInstruction {
    uint16 pc;
    uint8 opcode;
};

/**
 * Scan a raw ECL program to build an instruction index (PC + opcode only).
 * Operand bytes are skipped to determine instruction boundaries but not stored.
 * @param program Raw byte span of the ECL block
 * @param startPc Starting PC (usually 0)
 * @param outInstructions Output instruction index
 * @param config Optional game config for extended opcode operand counts
 * @return DecodeStatus
 */
DecodeStatus decodeProgram(Common::Span<const uint8> program, uint16 startPc,
    Common::Array<EclInstruction> &outInstructions,
    const GameConfig *config = nullptr);

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
