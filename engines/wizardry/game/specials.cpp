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

#include "wizardry/game/specials.h"

namespace Wizardry {

Specials::Event Specials::begin(MazeLevel &level, uint index) {
	Event e;
	e.kind = level.aux(2, index);
	e.text = level.aux(1, index);
	e.param = level.aux(0, index);
	e.happens = e.kind != kNothing;

	if (e.kind == kTextOnly || e.kind == kSearch || e.kind == kToCastle) {
		if (e.param == 0) {
			e.happens = false;
			return e;
		}

		if (e.kind == kSearch) {
			// From -1 to -999: found once. Further below: again and again.
			if (e.param < 0 && e.param >= -999)
				level.setAux(0, index, 0);
		} else if (e.param > 0) {
			level.setAux(0, index, e.param - 1);
			if (e.param == 1)
				level.setSquareType(index, kSquareNormal);
		}
	}

	return e;
}

Common::Array<Common::String> Specials::text(const MessageFile &messages, int text) {
	// The lines of a text are messages one after another; the next
	// text starts fifty numbers on
	Common::Array<Common::String> lines;
	if (text < 0)
		return lines;

	for (uint id = 15000 + text * 50; messages.contains(id) && lines.size() < 50; ++id)
		lines.push_back(messages.get(id));

	return lines;
}

int Specials::searchMonster(const Event &event) {
	return event.param > 0 ? event.param : -1;
}

int Specials::searchItem(const Event &event) {
	if (event.param >= 0 || -event.param >= 20000)
		return -1;   // TODO: from 20000 on, one item is taken and another given
	return (-event.param >= 1000) ? -event.param - 1000 : -event.param;
}

int Specials::giveItem(Party &party, int item) {
	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);
		if (c.itemCount >= Character::kMaxItems || c.carries(item))
			continue;

		c.addItem(item, false);
		return i;
	}

	return -1;
}

bool Specials::anyoneCarries(const Party &party, int item) {
	for (uint i = 0; i < party.size(); ++i) {
		if (party.member(i).carries(item))
			return true;
	}
	return false;
}

bool Specials::gateLetsBy(const Party &party, int allowed) {
	for (uint i = 0; i < party.size(); ++i) {
		const int bit = 1 << (party.member(i).alignment - kAlignGood);
		if (!(allowed & bit))
			return false;
	}
	return true;
}

void Specials::pushBack(MazeState &state) {
	int x = state.x, y = state.y;
	Maze::offset(x, y, state.facing, 0, -1);
	state.x = x;
	state.y = y;
}

void Specials::lightOrArmor(MazeState &state, int param) {
	if (param == 99)
		state.lightDuration += 50;
	else if (param == -99)
		state.lightDuration = 0;
	else
		state.acBonus = param;
}

} // End of namespace Wizardry
