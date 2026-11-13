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

bool Party::synchronize(Common::Serializer &s) {
	uint16 count = _roster.size();
	s.syncAsUint16LE(count);
	if (s.isLoading()) {
		if (count > 64)
			return false;
		_roster.resize(count);
	}

	for (uint i = 0; i < count; ++i) {
		byte record[Character::kRecordSize];
		if (s.isSaving())
			_roster[i].save(record);
		s.syncBytes(record, Character::kRecordSize);
		if (s.isLoading())
			_roster[i].load(record);
	}

	uint16 members = _members.size();
	s.syncAsUint16LE(members);
	if (s.isLoading()) {
		if (members > kMaxMembers)
			return false;
		_members.resize(members);
	}

	for (uint i = 0; i < members; ++i) {
		uint16 index = _members[i];
		s.syncAsUint16LE(index);
		if (s.isLoading()) {
			if (index >= count)
				return false;
			_members[i] = index;
		}
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

void Party::reorder(const Common::Array<uint> &order) {
	if (order.size() != _members.size())
		return;

	Common::Array<uint> members;
	for (uint i = 0; i < order.size(); ++i)
		members.push_back(_members[order[i]]);
	_members = members;
}

void Party::disband() {
	for (uint i = 0; i < size(); ++i) {
		Character &c = member(i);
		c.inMaze = false;
		c.lostPosition = 0;
		c.lostLevel = 0;
	}

	_members.clear();
}

int Party::freeSlot() const {
	for (uint i = 0; i < _roster.size(); ++i) {
		if (_roster[i].isFree())
			return i;
	}

	return -1;
}

bool Party::nameInUse(const Common::String &name) const {
	for (uint i = 0; i < _roster.size(); ++i) {
		if (!_roster[i].isFree() && _roster[i].name == name)
			return true;
	}

	return false;
}

int Party::memberIndexOf(uint rosterIndex) const {
	for (uint i = 0; i < _members.size(); ++i) {
		if (_members[i] == rosterIndex)
			return i;
	}

	return -1;
}

void Party::deleteCharacter(uint rosterIndex) {
	const int memberIndex = memberIndexOf(rosterIndex);
	if (memberIndex >= 0)
		_members.remove_at(memberIndex);

	_roster[rosterIndex].status = kStatusLost;
	_roster[rosterIndex].inMaze = false;
}

Party::TradeAnswer Party::giveGold(Character &from, Character &to, const Common::String &amount) {
	if (amount.size() > 12)
		return kTradeBadAmount;

	int64 value = 0;
	for (uint i = 0; i < amount.size(); ++i) {
		if (amount[i] < '0' || amount[i] > '9')
			return kTradeBadAmount;
		value = value * 10 + (amount[i] - '0');
	}

	if (from.gold < value)
		return kTradeTooMuch;

	to.gold = MIN<int64>(to.gold + value, 999999999999LL);
	from.gold -= value;
	return kTradeDone;
}

Party::TradeAnswer Party::giveItem(Character &from, Character &to, uint item) {
	if (to.itemCount >= Character::kMaxItems)
		return kTradeFull;
	if (from.items[item].cursed)
		return kTradeCursed;
	if (from.items[item].equipped)
		return kTradeEquipped;

	to.items[to.itemCount++] = from.items[item];
	from.removeItem(item);
	return kTradeDone;
}

Party::TradeAnswer Party::dropItem(Character &c, uint item) {
	if (c.items[item].cursed)
		return kTradeCursed;
	if (c.items[item].equipped)
		return kTradeEquipped;

	c.removeItem(item);
	return kTradeDone;
}

void Party::poolGold(uint memberIndex) {
	int64 total = 0;
	for (uint i = 0; i < size(); ++i) {
		total += member(i).gold;
		member(i).gold = 0;
	}

	member(memberIndex).gold = MIN<int64>(total, 999999999999LL);
}

} // End of namespace Wizardry
