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

#include "goldbox/ecl/operand_roles.h"

namespace Goldbox {
namespace ECL {

// Per-opcode operand roles for the handlers audited under GB13-001 / GB13-008.
// VALUE operands are evaluated through readVar (GetEclVar semantics); ADDRESS
// operands stay literal VM addresses via getOpWord; STRING operands are decoded
// text; VARARGS marks a trailing variable-length group. The classification is
// recorded here rather than inferred so the getOpWord/readVar decision at each
// call site is auditable.
OperandRole getOperandRole(uint8 opcode, uint8 index) {
	if (index == 0)
		return OperandRole::ROLE_NONE;

	switch (opcode) {
	case 0x0C: // SETUP MONSTER (Pascal EclSetupMon): id, distance, graphic — all values.
		if (index <= 3)
			return OperandRole::ROLE_VALUE;
		return OperandRole::ROLE_NONE;
	case 0x0E: // PICTURE: picture id is a value.
		if (index == 1)
			return OperandRole::ROLE_VALUE;
		return OperandRole::ROLE_NONE;
	case 0x38: // PROGRAM: program id is a value.
		if (index == 1)
			return OperandRole::ROLE_VALUE;
		return OperandRole::ROLE_NONE;
	case 0x3C: // PROTECTION / PRINT RUNES: operand 1 is the literal glyph address.
		if (index == 1)
			return OperandRole::ROLE_ADDRESS;
		return OperandRole::ROLE_NONE;
	case 0x28: // ROB: wholeParty, percent/item chance, framing-only — all values.
		if (index <= 3)
			return OperandRole::ROLE_VALUE;
		return OperandRole::ROLE_NONE;
	case 0x29: // ENCOUNTER MENU: operand 4 is the result destination address,
	           // operands 10..12 are strings, the rest are values.
		if (index == 4)
			return OperandRole::ROLE_ADDRESS;
		if (index >= 10 && index <= 12)
			return OperandRole::ROLE_STRING;
		if (index <= 14)
			return OperandRole::ROLE_VALUE;
		return OperandRole::ROLE_NONE;
	case 0x34: // CLOCK: single value operand.
		if (index == 1)
			return OperandRole::ROLE_VALUE;
		return OperandRole::ROLE_NONE;
	default:
		return OperandRole::ROLE_NONE;
	}
}

// Execution operand counts for the opcodes whose framing was corrected in
// GB13-001. These mirror the VM_GetOprand(N) count in the original
// EclRob (3), EclEncounter (14) and EclClock (1) routines.
int getExecutionOperandCount(uint8 opcode) {
    switch (opcode) {
    case 0x28: return 3;    // ROB
    case 0x29: return 0x0E; // ENCOUNTER MENU (14 operands)
    case 0x34: return 1;    // CLOCK
    default:   return -1;   // not covered by this framing metadata
    }
}

// Separate, documented conditional-skip table (SkipEclCmd / ECL_SkipNextInstruction).
// Each group records the number of operands the ORIGINAL skip routine consumes.
// Where this differs from the execution count the divergence is an original
// anomaly and is kept deliberately:
//   - ADD NPC (0x36) is skipped as 1 operand (executes 2).
//   - SETUP MONSTER (0x0C) is skipped as 2 operands (executes 3).
int getConditionalSkipOperandCount(uint8 opcode) {
    // 1-operand skip group. Includes ADD NPC (0x36) as the documented anomaly.
    if (opcode == 0x01 || opcode == 0x02 || opcode == 0x0A
            || opcode == 0x0E || opcode == 0x11 || opcode == 0x12
            || opcode == 0x1D || opcode == 0x20 || opcode == 0x2D
            || opcode == 0x32 || opcode == 0x34 || opcode == 0x36
            || opcode == 0x38 || opcode == 0x39 || opcode == 0x3C)
        return 1;

    // 2-operand skip group. Includes SETUP MONSTER (0x0C) as the documented
    // anomaly (execution count is 3).
    if (opcode == 0x03 || opcode == 0x08 || opcode == 0x09
            || opcode == 0x0C || opcode == 0x0F || opcode == 0x10
            || opcode == 0x1F || opcode == 0x22)
        return 2;

    // 3-operand skip group. Includes ROB (0x28).
    if ((opcode >= 0x04 && opcode <= 0x07)
            || opcode == 0x0B || opcode == 0x21 || opcode == 0x28
            || opcode == 0x2A || opcode == 0x2F || opcode == 0x30
            || opcode == 0x35 || opcode == 0x37 || opcode == 0x3B)
        return 3;

    if (opcode == 0x14 || opcode == 0x23)
        return 4;
    if (opcode == 0x2E)
        return 5;
    if (opcode == 0x1E || opcode == 0x2C)
        return 6;
    if (opcode == 0x27)
        return 8;
    if (opcode == 0x29)
        return 0x0E; // ENCOUNTER MENU (14 operands)

    return -1; // single opcode byte, no operands skipped
}

} // namespace ECL
} // namespace Goldbox
