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

#ifndef WIZARDRY_GAME_INN_H
#define WIZARDRY_GAME_INN_H

#include "common/array.h"
#include "wizardry/game/character.h"
#include "wizardry/game/random.h"
#include "wizardry/resources/messages.h"
#include "wizardry/resources/scenario_file.h"

namespace Wizardry {

/**
 * Rules of the Adventurer's Inn of Wizardry I: resting, and what happens
 * to a character who has rested (a new level, new spells, attributes that
 * rise or fall with age).
 */
class Inn {
public:
	enum {
		kRoomCount = 5
	};

	struct Room {
		int16 heal;   ///< hit points a week
		int16 cost;   ///< gold a week; the stables are free and heal nothing
	};

	static const Room ROOMS[kRoomCount];

private:
	const ScenarioFile &_scenario;
	const MessageFile &_messages;
	Dice &_dice;

	int rollHitDie(const Character &c);
	void learnSpells(Character &c, Common::Array<Common::String> &lines);
	bool learnSpellLevel(Character &c, uint first, uint last, int attribute);
	bool changeAttributes(Character &c, Common::Array<Common::String> &lines);
	void gainLevel(Character &c, Common::Array<Common::String> &lines);

public:
	Inn(const ScenarioFile &scenario, const MessageFile &messages, Dice &dice) :
		_scenario(scenario), _messages(messages), _dice(dice) {}

	/**
	 * True if another week in the room would be paid for and would help
	 */
	static bool canRest(const Character &c, const Room &room);

	/**
	 * One week: older, healthier, poorer.
	 * @returns true if the character had a birthday
	 */
	static bool restOneWeek(Character &c, const Room &room);

	/**
	 * Experience needed for the level after the character's own
	 */
	int64 experienceForNextLevel(const Character &c) const;

	/**
	 * What follows a stay: a new level if there is experience enough (one
	 * at most), or word of how much is missing.
	 * @param lines  Receives what the innkeeper says
	 */
	void checkLevel(Character &c, bool birthday, Common::Array<Common::String> &lines);

	/**
	 * Works out how many spells of each level the character can cast:
	 * as many as are known, or more by class and level; nine at most
	 */
	static void setSpellSlots(Character &c);
};

/**
 * Puts a text in place of the first '^' of a message
 */
extern Common::String substCaret(const Common::String &message, const Common::String &text);

} // End of namespace Wizardry

#endif
