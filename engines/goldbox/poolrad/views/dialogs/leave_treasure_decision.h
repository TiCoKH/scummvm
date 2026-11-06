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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_LEAVE_TREASURE_DECISION_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_LEAVE_TREASURE_DECISION_H

#include "goldbox/messages.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * GB13-026 — semantic answer to the leave-treasure confirmation.
 *
 * The exit-confirm prompt asks whether to GO BACK AND RETRIEVE the treasure
 * ("you left some things here. Do you want to go back?"). The legacy menu is
 * zero-based (0=Yes/return, 1=No/leave) and the HorizontalYesNo helper posts
 * it as an int result: YES -> intValue==1, NO -> intValue==0. This enum keeps
 * that legacy numbering confined to the adapter so only No ever leaves.
 */
enum LeaveTreasureDecision {
	// Yes: return to the treasure. Resume the prior stage, pool/items intact.
	RETURN_TO_TREASURE = 0,
	// No: leave the treasure behind. Perform the exit/disposal sequence.
	LEAVE_BEHIND,
	// Escape / unhandled key: the helper posts nothing, so a result without an
	// int value is a non-answer. The prompt stays up; nothing is disposed.
	NO_DECISION
};

/**
 * Map the child HorizontalYesNo answer to a semantic decision.
 *
 * Contract (traced from HorizontalYesNo::msgKeypress):
 *   - YES (selection 0, 'Y', or RETURN-on-YES) -> intValue==1 (KEYCODE_y)
 *   - NO  (selection 1, 'N', or RETURN-on-NO)  -> intValue==0 (KEYCODE_n)
 *   - ESCAPE / other keys post NO result at all (_hasIntValue == false).
 *
 * Keyboard and Enter therefore agree: the posted intValue is the sole
 * authority. Only an explicit NO (intValue==0) leaves; anything without an
 * int value is a non-answer that must NOT dispose treasure.
 */
inline LeaveTreasureDecision decideLeaveTreasure(const MenuResultMessage &result) {
	if (!result._hasIntValue)
		return NO_DECISION;
	return (result._intValue == 0) ? LEAVE_BEHIND : RETURN_TO_TREASURE;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_LEAVE_TREASURE_DECISION_H
