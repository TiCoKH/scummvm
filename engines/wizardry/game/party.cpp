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

#include "wizardry/game/party.h"

namespace Wizardry {

bool Party::loadRoster(const ScenarioFile &scenario) {
	_roster.clear();
	_members.clear();

	const ScenarioFile::Type &type = scenario.type(kRecordCharacter);
	if (type.recordSize != Character::kRecordSize)
		return false;

	for (uint i = 0; i < type.count; ++i) {
		const byte *record = scenario.record(kRecordCharacter, i);
		if (!record)
			break;

		Character c;
		c.load(record);
		// Nobody is out with a party when a game starts from the file
		c.inMaze = false;
		_roster.push_back(c);
	}

	return true;
}

int Party::alignment() const {
	int result = kAlignNeutral;

	for (uint i = 0; i < size(); ++i) {
		if (member(i).alignment != kAlignNeutral)
			result = member(i).alignment;
	}

	return result;
}

bool Party::alignmentClashes(uint rosterIndex) const {
	const int party = alignment();
	const int own = _roster[rosterIndex].alignment;
	return party != kAlignNeutral && own != kAlignNeutral && own != party;
}

bool Party::isAvailable(uint rosterIndex) const {
	const Character &c = _roster[rosterIndex];
	return c.status != kStatusLost && !c.inMaze && c.lostLevel == 0 &&
		!alignmentClashes(rosterIndex);
}

Common::Array<uint> Party::available() const {
	Common::Array<uint> result;

	for (uint i = 0; i < _roster.size() && result.size() < kMaxListed; ++i) {
		if (isAvailable(i))
			result.push_back(i);
	}

	return result;
}

void Party::add(uint rosterIndex) {
	if (isFull())
		return;

	_roster[rosterIndex].inMaze = true;
	_members.push_back(rosterIndex);
}

void Party::remove(uint memberIndex) {
	if (memberIndex >= size())
		return;

	member(memberIndex).inMaze = false;
	_members.remove_at(memberIndex);
}

void Party::divvyGold() {
	if (!size())
		return;

	int64 total = 0;
	for (uint i = 0; i < size(); ++i)
		total += member(i).gold;

	const int64 share = total / size();
	for (uint i = 0; i + 1 < size(); ++i) {
		member(i).gold = share;
		total -= share;
	}
	member(size() - 1).gold = total;
}

} // End of namespace Wizardry
