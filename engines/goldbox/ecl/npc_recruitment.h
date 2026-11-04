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

#ifndef GOLDBOX_ECL_NPC_RECRUITMENT_H
#define GOLDBOX_ECL_NPC_RECRUITMENT_H

#include "common/list.h"
#include "common/scummsys.h"

namespace Goldbox {

namespace Data {
class PlayerCharacter;
} // namespace Data

namespace ECL {

// GB13-006: testable core of NPC recruitment (ADD NPC, 0x36) and scripted
// party-member removal (LOAD CHARACTER bit 7, 0x0A).
//
// Ports the field arithmetic of Inst_NPC_ADD (eclvm.pas:1455 EclAddNpc) and the
// stable list surgery of PARTY_removeMember / RemoveCreature (chargen.pas:1076)
// as pure functions over the party list, so the npc byte, the hostility-by-id
// rule and the capture-next-before-erase removal can be proven without a live
// engine. The host wires construction (reuse of the decoded monster loader),
// icon ownership, selection bookkeeping and the VM character window around
// these cores.

/** Monster id that recruits as HOSTILE (CS_ENEMY); all others are friendly. */
static const uint8 kHostileRecruitMonsterId = 24; // 0x18

/**
 * Apply the two original NPC fields to a freshly loaded recruit
 * (Inst_NPC_ADD lines 41397-41404).
 *
 *   npc = (morale div 2) | 0x80      ; bit 7 marks an NPC, bits 6..0 hold
 *                                      half the morale (0..127, never overflows)
 *   combatSide = (monsterId == 24) ? CS_ENEMY : CS_PARTY
 *
 * Does NOT recalc stats or touch selection; the caller runs
 * recalcCombatStats() afterward exactly as the original calls
 * CHARACTER_RecalcCombatStats. @p morale is the raw operand-2 value (0..255);
 * the >> 1 keeps the stored value inside the 0..127 field with no signed
 * overflow.
 */
void applyNpcRecruitFields(Data::PlayerCharacter &recruit, uint8 monsterId,
		uint8 morale);

/** Result of a stable party-member removal. */
struct PartyRemovalResult {
	// True when a member was actually removed.
	bool removed;
	// 0-based list index the removed member occupied (its stable id for
	// pending-operation invalidation). Only meaningful when removed is true.
	uint16 departedIndex;
	// The character pointer that was unlinked from the list. OWNERSHIP of this
	// pointer passes back to the caller, which frees it through the same
	// lifetime owner that created it. nullptr when nothing was removed.
	Data::PlayerCharacter *departed;
	// The selection the original leaves behind (GDVars.curChar := prev): the
	// predecessor of the removed member, or the new list head when the removed
	// member was the head, or nullptr when the list becomes empty.
	Data::PlayerCharacter *newSelection;

	PartyRemovalResult()
		: removed(false), departedIndex(0), departed(nullptr),
		  newSelection(nullptr) {}
};

/**
 * Remove @p selected from @p party using the original stable strategy
 * (RemoveCreature / PARTY_removeMember): walk to the predecessor, capture the
 * successor (the "next" link) BEFORE unlinking so the iteration is never
 * invalidated mid-remove, then splice @p selected out.
 *
 * This is ONLY the party-member removal action (LOAD CHARACTER bit 7). It is
 * deliberately distinct from temporary monster teardown (clearMonsters) and
 * disk-character deletion; this function never touches monster slots or disk
 * records and never frees the character — it returns the unlinked pointer so
 * the caller's owner frees it.
 *
 * @p selected must be a member of @p party. When @p selected is null or not a
 * member, the result reports removed == false and leaves @p party untouched.
 */
PartyRemovalResult removeSelectedStable(
		Common::List<Data::PlayerCharacter *> &party,
		Data::PlayerCharacter *selected);

} // namespace ECL
} // namespace Goldbox

#endif // GOLDBOX_ECL_NPC_RECRUITMENT_H
