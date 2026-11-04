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

#ifndef GOLDBOX_ECL_RUNE_PRESENTATION_H
#define GOLDBOX_ECL_RUNE_PRESENTATION_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Goldbox {
namespace ECL {

class AddressSpace;

/**
 * Cursor / box / redraw state carried by the PROTECTION (PRINT RUNES) family of
 * opcodes (GB13-008).
 *
 * The original engine keeps rune text cursor, newline and cleared-box state in
 * its global variables (GDVars.curRow/curCol/protNewLine/dsEcl3dClearFlag/
 * protTextShown) and threads them through the CLEAR BOX -> PROTECTION ->
 * PRINT RETURN / EXIT transitions. This is the engine-owned home for that same
 * state so the sequence of transitions is reproduced without relying on a live
 * display surface.
 */
class RunePresentationContext {
public:
	RunePresentationContext() { reset(); }

	// Default message-box text origin (C_Msg.TEXT_LEFT / first text row).
	// Enumerators are prvalues so they never need an out-of-line definition.
	enum {
		kTextLeftCol = 2,
		kTextTopRow = 5
	};

	/**
	 * Decode an encoded-rune string starting at a literal VM address.
	 *
	 * Mirrors EclProtection: each source cell is read as a byte and advanced by
	 * one (modulo the byte width); a decoded value of 1 or less terminates the
	 * run (raw 0x00 wraps to 1, raw 0xFF wraps to 0). The cursor column advances
	 * per rendered glyph and the row is incremented once after the run, exactly
	 * as the original. Box-clear state determined by a preceding CLEAR BOX is
	 * honored before rendering.
	 *
	 * @param mem   VM address space to read encoded cells from
	 * @param addr  literal VM address of the first encoded cell
	 * @return the decoded glyph bytes for presentation
	 */
	Common::String decodeRunes(const AddressSpace &mem, uint16 addr);

	/** CLEAR BOX transition: arm the box-clear flag (EclClearBox). */
	void onClearBox();

	/** PRINT RETURN transition: carriage return + line feed (EclPrintRet). */
	void onPrintReturn();

	/** EXIT / script-boundary reset of the transient rune presentation state. */
	void reset();

	uint8 cursorRow() const { return _curRow; }
	uint8 cursorCol() const { return _curCol; }
	bool boxClearArmed() const { return _ecl3dClearFlag != 0; }
	bool protectionTextShown() const { return _protTextShown != 0; }
	bool newLinePending() const { return _protNewLine != 0; }

private:
	uint8 _curRow;
	uint8 _curCol;
	uint8 _protNewLine;
	uint8 _ecl3dClearFlag;
	uint8 _protTextShown;
};

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_RUNE_PRESENTATION_H
