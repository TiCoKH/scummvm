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

#ifndef GOLDBOX_ECL_OPERAND_ROLES_H
#define GOLDBOX_ECL_OPERAND_ROLES_H

#include "common/scummsys.h"

namespace Goldbox {
namespace ECL {

/**
 * Semantic role of an opcode operand (GB13-001 / GB13-008).
 *
 * The role disambiguates how the handler must treat the operand: a VALUE is
 * evaluated through GetEclVar semantics (readVar), an ADDRESS is a literal VM
 * destination (getOpWord), a TARGET is a branch/next-PC target and a STRING is
 * decoded text. VARARGS marks a trailing variable-length group.
 */
enum class OperandRole : uint8 {
    ROLE_NONE = 0,
    ROLE_VALUE,
    ROLE_ADDRESS,
    ROLE_TARGET,
    ROLE_STRING,
    ROLE_VARARGS
};

/**
 * Semantic role of a specific 1-based operand of an opcode (GB13-008).
 *
 * The role records how the handler must treat that operand so decode is
 * unambiguous and auditable: ROLE_VALUE is evaluated through GetEclVar
 * semantics (EclVM::readVar), ROLE_ADDRESS is a literal VM destination
 * (EclVM::getOpWord, never dereferenced), ROLE_TARGET is a branch/next-PC
 * target and ROLE_STRING is decoded text (EclVM::readString). ROLE_VARARGS
 * marks the start of a trailing variable-length group.
 *
 * Only the opcodes whose operand classification was audited under GB13-001 and
 * GB13-008 are described; everything else returns ROLE_NONE. The table is data,
 * consumed by both the handlers and the debugger, and is intentionally NOT
 * regenerated from the execution or skip counts.
 *
 * @param opcode opcode byte.
 * @param index  1-based operand index.
 * @return the operand's role, or ROLE_NONE when not classified.
 */
OperandRole getOperandRole(uint8 opcode, uint8 index);

/**
 * Number of operands the interpreter *executes* for an opcode, i.e. the count
 * passed to VM_GetOprand in the original EclRob/EclEncounter/EclClock routines.
 *
 * This is the framing count the handler decodes before dispatch so the VM
 * resumes at the decoder-produced next PC. It is deliberately NOT the same
 * table as the conditional-skip count (see getConditionalSkipOperandCount).
 *
 * @return execution operand count, or -1 for opcodes with variable operands
 *         or opcodes not covered by this metadata.
 */
int getExecutionOperandCount(uint8 opcode);

/**
 * Conditional-skip operand count for an opcode (GB13-001.5).
 *
 * This is a SEPARATE, documented compatibility table mirroring the original
 * SkipEclCmd / ECL_SkipNextInstruction routine. It is intentionally NOT derived
 * from the execution operand counts: the original skip routine records historical
 * anomalies where the skip count differs from the execution count, notably
 *   - ADD NPC (0x36)     executes 2 operands but is skipped as 1
 *   - SETUP MONSTER(0x0C) executes 3 operands but is skipped as 2
 * Keeping the two tables apart lets an IF-skipped instruction land at the exact
 * original PC without silently regenerating the anomaly from execution counts.
 *
 * @return skip operand count for the opcode, or -1 if the opcode is not listed
 *         (callers then advance by a single opcode byte).
 */
int getConditionalSkipOperandCount(uint8 opcode);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_OPERAND_ROLES_H
