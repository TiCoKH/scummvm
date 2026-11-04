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

#ifndef GOLDBOX_ECL_ECL_STRING_WRITER_H
#define GOLDBOX_ECL_ECL_STRING_WRITER_H

#include "common/scummsys.h"
#include "common/str.h"
#include "goldbox/ecl/ecl_memory.h"

namespace Goldbox {
namespace ECL {

/**
 * GB13-003: numeric and string result helpers for INPUT NUMBER / INPUT STRING.
 *
 * These are pure, engine-independent helpers so the bank-aware string writer
 * and the InputUint16 validation can be unit-tested without a live dialog or
 * engine. The opcode handlers and the Poolrad host call through them.
 */

/** Maximum characters INPUT STRING accepts (original EclInputStr limit). */
static const uint8 kInputStringMaxLength = 40;

/**
 * Character-name storage limit for the $6B00 DAT-bank name cell
 * (original WriteGbvmString truncates a name written to $6B00 to 15 chars).
 */
static const uint8 kCharacterNameMaxLength = 15;

/** VM address of the current-character name cell in the DAT bank. */
static const uint16 kCharacterNameVmAddr = 0x6B00;

/**
 * Parsed result of validating numeric input against InputUint16 semantics.
 *
 * The full 0..65535 domain is carried in @c value and is only meaningful when
 * @c valid is true. Cancellation / empty / overflow leave @c valid false so no
 * VM write happens (Requirement 15.2: invalid input must not mutate memory).
 */
struct NumberInputResult {
    bool   valid;
    uint16 value;

    NumberInputResult() : valid(false), value(0) {}
};

/**
 * Validate a typed numeric string the way the original InputUint16 /
 * DIALOG_InputNumber does: accept up to @p maxDigits decimal digits and reject
 * a value that overflows the unsigned 16-bit domain (> 65535).
 *
 * Empty input, a non-digit, more than @p maxDigits digits, or an overflow all
 * yield an invalid result (no value). Leading digits are accepted up to the
 * limit; the full 0..65535 range is representable because the result is
 * carried as uint16, not int16.
 */
NumberInputResult validateNumberInput(const Common::String &typed,
        uint8 maxDigits);

/**
 * Write an INPUT STRING result into VM memory the way WriteGbvmString /
 * VM_WriteStringVar does, honoring bank cell width and the zero terminator.
 *
 * Rules (Requirement 15.3/15.4):
 *  - Empty input is replaced by a single space before writing.
 *  - The text is capped at @p maxChars (40 for a general INPUT STRING; the
 *    caller passes 15 when the destination is the $6B00 character name).
 *  - When @p cellWidth is 1 (ECL script bank, region 3) each character is one
 *    byte at consecutive addresses, terminator byte at addr + len.
 *  - When @p cellWidth is 2 (world/dat/heap word banks) each character is the
 *    low byte of a word cell, terminator word at addr + len * 2.
 *
 * Returns the number of characters written (excluding the terminator).
 */
uint16 writeVmString(AddressSpace &mem, uint16 destAddr,
        const Common::String &text, uint8 cellWidth, uint8 maxChars);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_ECL_STRING_WRITER_H
