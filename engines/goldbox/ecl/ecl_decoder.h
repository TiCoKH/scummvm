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

#include "common/scummsys.h"
#include "common/str.h"

namespace Goldbox {
namespace ECL {

enum DecodeStatus {
    DECODE_OK = 0,
    DECODE_OUT_OF_BOUNDS
};

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
