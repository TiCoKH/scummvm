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

#include "goldbox/ecl/rune_presentation.h"
#include "goldbox/ecl/ecl_memory.h"

namespace Goldbox {
namespace ECL {

void RunePresentationContext::reset() {
	_curRow = kTextTopRow;
	_curCol = kTextLeftCol;
	_protNewLine = 0;
	_ecl3dClearFlag = 0;
	_protTextShown = 0;
}

Common::String RunePresentationContext::decodeRunes(const AddressSpace &mem, uint16 addr) {
	// EclProtection: a prior CLEAR BOX that pushed the cursor past the box
	// bottom resets it to the box top and marks protection text as shown.
	if (_ecl3dClearFlag != 0 && _curRow > 13) {
		_curRow = kTextTopRow;
		_curCol = kTextLeftCol;
		_protTextShown = 1;
	}
	_protNewLine = 1;

	Common::String decoded;
	uint8 i = 0;
	for (;;) {
		const uint16 cell = static_cast<uint16>(addr + i);
		// Advance each encoded glyph by one; the byte width (mod 256) means a
		// raw 0x00 wraps to 1 and a raw 0xFF wraps to 0. A decoded value of 1
		// or less terminates the run, matching the source termination test.
		const uint8 c = static_cast<uint8>(mem.read8(cell) + 1);
		if (c <= 1)
			break;
		decoded += static_cast<char>(c);
		++_curCol;
		++i;
	}
	++_curRow;
	return decoded;
}

void RunePresentationContext::onClearBox() {
	// EclClearBox: clear the text rectangle and arm the box-clear flag so the
	// next PROTECTION run repositions the cursor if it overflowed.
	_ecl3dClearFlag = 1;
}

void RunePresentationContext::onPrintReturn() {
	// EclPrintRet: carriage return to the text-left column and advance one row.
	// Both branches of the original perform the same move; the pending-newline
	// flag is cleared so a following run starts fresh.
	_curCol = kTextLeftCol;
	++_curRow;
	_protNewLine = 0;
}

} // namespace ECL
} // namespace Goldbox
