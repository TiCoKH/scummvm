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
};

} // End of namespace Wizardry

#endif
