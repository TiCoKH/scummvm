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

#include "wizardry/game/temple.h"

namespace Wizardry {

bool Temple::isPatient(const Character &c) {
	return c.status != kStatusLost && c.status != kStatusOk && c.lostLevel == 0;
}

Common::Array<uint> Temple::patients(const Party &party) {
	Common::Array<uint> result;

	for (uint i = 0; i < party.rosterSize() && result.size() < Party::kMaxListed; ++i) {
		if (isPatient(party.rosterEntry(i)))
			result.push_back(i);
	}

	return result;
}

int64 Temple::donation(const Character &c) {
	int64 fee;

	switch (c.status) {
	case kStatusParalysed:
		fee = 100;
		break;
	case kStatusStoned:
		fee = 200;
		break;
	case kStatusDead:
		fee = 250;
		break;
	case kStatusAshes:
		fee = 500;
		break;
	default:
		// The original sets no fee for the afraid and the asleep, who do
		// not come to town in that state
		fee = 0;
		break;
	}

	return fee * c.level;
}

bool Temple::collect(Party &party, uint payer, int64 amount) {
	if (party.member(payer).gold < amount) {
		party.poolGold(payer);
		if (party.member(payer).gold < amount)
			return false;
	}

	party.member(payer).gold -= amount;
	return true;
}

Temple::Outcome Temple::ritual(Character &c, Dice &dice) {
	const int vitality = c.attributes[kVitality];

	if (c.status == kStatusDead) {
		if ((int)dice.roll(100) > vitality * 3 + 50) {
			c.status = kStatusAshes;
			return kNowAshes;
		}
		c.hp = 1;
	} else if (c.status == kStatusAshes) {
		if ((int)dice.roll(100) > vitality * 3 + 40) {
			c.status = kStatusLost;
			return kNowLost;
		}
		c.hp = c.hpMax;
	}

	// Age is counted in weeks
	c.age += dice.roll(52) + 1;
	c.poison = 0;
	c.lostPosition = 0;
	c.lostLevel = 0;
	c.status = kStatusOk;
	return kHealed;
}

} // End of namespace Wizardry
