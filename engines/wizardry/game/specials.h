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

#ifndef WIZARDRY_GAME_SPECIALS_H
#define WIZARDRY_GAME_SPECIALS_H

#include "wizardry/game/maze.h"
#include "wizardry/resources/messages.h"

namespace Wizardry {

/**
 * The special squares of the maze: a text, and something that happens.
 * The three numbers of the square say what, which text, and one more
 * thing that depends on what.
 */
class Specials {
public:
	enum Kind {
		kNothing,
		kTextOnly,       ///< so many times, or for ever
		kGiveItem,
		kPool,           ///< someone steps in (not in yet)
		kSearch,         ///< asked first: a fight, or something to find
		kRequireItem,    ///< nobody has it: back a square
		kAlignmentGate,
		kLightOrArmor,
		kToCastle,
		kArmRooms,       ///< the rooms around are guarded again
		kRiddle,         ///< (not in yet)
		kPayFee          ///< (not in yet)
	};

	struct Event {
		int kind;
		int text;      ///< which text; below zero for none
		int param;
		bool happens;  ///< false if it was there only so many times and they are over
	};

	/**
	 * Reads the event of a square and counts it down if it is one of
	 * those that happen only so many times
	 */
	static Event begin(MazeLevel &level, uint index);

	/**
	 * The lines of a text. A line may start with '@' for a new page,
	 * '$' to stand at the left, or '^' to stand under the line before;
	 * other lines are centred.
	 */
	static Common::Array<Common::String> text(const MessageFile &messages, int text);

	/** What a search finds: a monster to fight (above zero), or an item */
	static int searchMonster(const Event &event);
	static int searchItem(const Event &event);

	/**
	 * Gives an item to the first member who has room and does not carry
	 * one already
	 * @returns that member's place in the party, or -1
	 */
	static int giveItem(Party &party, int item);

	static bool anyoneCarries(const Party &party, int item);

	/**
	 * True if everyone in the party is of an alignment the gate lets by:
	 * 1 good, 2 neutral, 4 evil
	 */
	static bool gateLetsBy(const Party &party, int allowed);

	/** One square back, against the way the party faces */
	static void pushBack(MazeState &state);

	static void lightOrArmor(MazeState &state, int param);
};

} // End of namespace Wizardry

#endif
