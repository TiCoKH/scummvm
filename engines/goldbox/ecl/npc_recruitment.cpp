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

#include "goldbox/ecl/npc_recruitment.h"
#include "goldbox/data/player_character.h"

namespace Goldbox {
namespace ECL {

void applyNpcRecruitFields(Data::PlayerCharacter &recruit, uint8 monsterId,
		uint8 morale) {
	// npc = (morale div 2) | 0x80. morale is a byte (0..255); >> 1 yields
	// 0..127, so the 0x80 bit never collides with the stored half-morale and
	// the result fits the int8 field without signed overflow.
	recruit.npc = static_cast<int8>((morale >> 1) | 0x80);

	// Monster 24 (0x18) recruits hostile; every other id joins the party side.
	recruit.combatSide =
		(monsterId == kHostileRecruitMonsterId) ? Data::CS_ENEMY : Data::CS_PARTY;
}

PartyRemovalResult removeSelectedStable(
		Common::List<Data::PlayerCharacter *> &party,
		Data::PlayerCharacter *selected) {
	PartyRemovalResult result;
	if (!selected || party.empty())
		return result;

	// Locate the member and its 0-based index without erasing yet. The index
	// becomes the departed member's stable id for pending-operation
	// invalidation; the iterator is used only after we have captured the
	// successor so the erase cannot invalidate a link we still read.
	Common::List<Data::PlayerCharacter *>::iterator it = party.begin();
	uint16 index = 0;
	for (; it != party.end(); ++it, ++index) {
		if (*it == selected)
			break;
	}
	if (it == party.end())
		return result; // not a member: leave the list untouched

	// Capture the successor (the "next" link, GDVars.curChar^.nextCharAddr) and
	// the predecessor BEFORE erasing, mirroring RemoveCreature's prev/next
	// bookkeeping. Common::List::erase returns the following iterator, but we
	// read both neighbours up front so the strategy is explicitly stable and
	// does not depend on post-erase iterator validity.
	Data::PlayerCharacter *successor = nullptr;
	{
		Common::List<Data::PlayerCharacter *>::iterator after = it;
		++after;
		if (after != party.end())
			successor = *after;
	}
	Data::PlayerCharacter *predecessor = nullptr;
	if (index != 0) {
		Common::List<Data::PlayerCharacter *>::iterator prev = party.begin();
		for (uint16 k = 0; k + 1 < index; ++k)
			++prev;
		predecessor = *prev;
	}

	result.removed = true;
	result.departedIndex = index;
	result.departed = selected;

	// GDVars.curChar := prev. When the removed member was the head (index 0),
	// the original advances the list head and selects the new head (the
	// successor); otherwise it selects the predecessor.
	result.newSelection = (index == 0) ? successor : predecessor;

	party.erase(it);
	return result;
}

} // namespace ECL
} // namespace Goldbox
