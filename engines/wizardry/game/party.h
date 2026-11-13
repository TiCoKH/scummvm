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

#ifndef WIZARDRY_GAME_PARTY_H
#define WIZARDRY_GAME_PARTY_H

#include "common/array.h"
#include "common/serializer.h"
#include "wizardry/game/character.h"
#include "wizardry/resources/scenario_file.h"

namespace Wizardry {

/**
 * The roster (all characters of the scenario, at most 20) and the party
 * (up to six of them, in marching order).
 */
class Party {
public:
	enum {
		kMaxMembers = 6,
		kMaxListed = 20
	};

private:
	Common::Array<Character> _roster;
	Common::Array<uint> _members;   ///< roster numbers

public:
	/**
	 * Reads the roster from the scenario file. The party starts empty.
	 * @returns false if the character records are not the kind this
	 * class knows (IBM-PC Wizardry I)
	 */
	bool loadRoster(const ScenarioFile &scenario);

	/**
	 * Saves or loads the roster and the party. The characters are kept
	 * as the records of the scenario file, so a saved roster can be told
	 * apart from the original's only by where it is stored.
	 * @returns false if what was loaded cannot be a roster
	 */
	bool synchronize(Common::Serializer &s);

	uint rosterSize() const { return _roster.size(); }
	Character &rosterEntry(uint index) { return _roster[index]; }
	const Character &rosterEntry(uint index) const { return _roster[index]; }

	uint size() const { return _members.size(); }
	bool isFull() const { return _members.size() >= kMaxMembers; }
	Character &member(uint index) { return _roster[_members[index]]; }
	const Character &member(uint index) const { return _roster[_members[index]]; }

	/**
	 * Alignment of the party: neutral unless a member is good or evil;
	 * the last such member decides
	 */
	int alignment() const;

	/**
	 * True if the tavern offers this character: not lost, not out with a
	 * party, not lying in the maze, and of an alignment that goes with
	 * the party's. The dead may join, to be carried to the temple.
	 */
	bool isAvailable(uint rosterIndex) const;

	/**
	 * The roster numbers the tavern offers, at most 20
	 */
	Common::Array<uint> available() const;

	/**
	 * True if good and evil would meet
	 */
	bool alignmentClashes(uint rosterIndex) const;

	void add(uint rosterIndex);
	void remove(uint memberIndex);

	/**
	 * Shares the party's gold equally; the last member gets the remainder
	 */
	void divvyGold();

	/**
	 * Gives all the party's gold to one member
	 */
	void poolGold(uint memberIndex);

	/**
	 * Sends everyone home: the party is empty afterwards
	 */
	void disband();

	/**
	 * Returns a free place in the roster, or -1 if it is full
	 */
	int freeSlot() const;

	bool nameInUse(const Common::String &name) const;

	/**
	 * Returns the place of a roster entry in the party, or -1
	 */
	int memberIndexOf(uint rosterIndex) const;

	/**
	 * Removes a character from the roster: its place is free afterwards
	 */
	void deleteCharacter(uint rosterIndex);

	/**
	 * Why a trade cannot be made
	 */
	enum TradeAnswer {
		kTradeDone,
		kTradeBadAmount,   ///< not a number of at most twelve digits
		kTradeTooMuch,     ///< more gold than the giver has
		kTradeFull,        ///< the receiver carries eight items
		kTradeCursed,
		kTradeEquipped
	};

	/**
	 * Gives gold from one character to another
	 * @param amount  What was typed
	 */
	static TradeAnswer giveGold(Character &from, Character &to, const Common::String &amount);

	/**
	 * Gives an item from one character to another
	 */
	static TradeAnswer giveItem(Character &from, Character &to, uint item);

	/**
	 * Drops an item, unless it is cursed or equipped
	 */
	static TradeAnswer dropItem(Character &c, uint item);
};

} // End of namespace Wizardry

#endif
