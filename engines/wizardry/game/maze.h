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

#ifndef WIZARDRY_GAME_MAZE_H
#define WIZARDRY_GAME_MAZE_H

#include "wizardry/game/party.h"
#include "wizardry/game/random.h"
#include "wizardry/resources/scenario_file.h"

namespace Wizardry {

enum Facing {
	kNorth, kEast, kSouth, kWest
};

enum Wall {
	kOpen, kWall, kDoor, kHiddenDoor
};

enum SquareType {
	kSquareNormal, kSquareStairs, kSquarePit, kSquareChute, kSquareSpinner,
	kSquareDark, kSquareTransfer, kSquareDamage, kSquareElevator, kSquareRock,
	kSquareFizzle, kSquareSpecial, kSquareEncounter
};

/**
 * One level of the maze: record type 1 of the scenario file of IBM-PC
 * Wizardry I (894 bytes). A level is 20 x 20 squares and wraps round at
 * its edges. Each square has a wall value for each side and a number
 * from 0 to 15, which says what kind of square it is and gives three
 * numbers whose meaning depends on the kind.
 */
class MazeLevel {
public:
	enum {
		kRecordSize = 894,
		kSize = 20
	};

private:
	byte _data[kRecordSize];

	uint word(uint offset) const;

public:
	MazeLevel();

	bool load(const ScenarioFile &scenario, int level);

	/**
	 * The wall of a square on one side, as a Wall value
	 */
	uint wall(int x, int y, int side) const;

	/** True for a square of a room with guardians */
	bool isFightSquare(int x, int y) const;

	/** The number, 0 to 15, of the kind of a square */
	uint squareIndex(int x, int y) const;
	uint squareType(uint index) const;
	uint squareTypeAt(int x, int y) const { return squareType(squareIndex(x, y)); }

	/** The three numbers of a kind of square */
	int aux(uint which, uint index) const;

	/**
	 * A number of one of the level's three tables of monsters: first
	 * monster, step, most steps, range, chance in percent of each step
	 */
	int encounterTable(uint table, uint field) const;
};

/**
 * Where the party is, and what lasts while it is in the maze
 */
struct MazeState {
	int16 x = 0, y = 0;
	int16 level = 1;         ///< 1 is the first level below the castle
	int16 facing = kNorth;
	int16 lightDuration = 0;       ///< counts down with every drawing of the view
	int16 monstersIdentified = 0;  ///< the same
	int16 acBonus = 0;             ///< taken off everyone's armour class
	bool spellsFizzle = false;
};

/**
 * Rules for walking in the maze
 */
class Maze {
public:
	/**
	 * Moves a position sideways and forward as seen from a facing; the
	 * level wraps round
	 */
	static void offset(int &x, int &y, int facing, int right, int forward);

	/**
	 * One step forward. Only a plain wall stops the party: doors, hidden
	 * ones too, are walked through.
	 * @returns false if a wall is in the way
	 */
	static bool forward(MazeState &state, const MazeLevel &level);

	/**
	 * Turns by quarter turns to the right: 1 right, 2 about, 3 left
	 */
	static void turn(MazeState &state, int by);

	/**
	 * What comes after every step: for each member, one time in eight,
	 * poison hurts and items that heal do so
	 */
	static void poisonAndRegen(Party &party, Dice &dice);

	/**
	 * A pit or another square that hurts. Each member not dead is hit
	 * unless nimble enough; deeper levels are harder to escape.
	 * @param index  The number of the kind of the square
	 */
	static void squareDamage(Party &party, const MazeLevel &level, uint index, int mazeLevel, Dice &dice);

	/**
	 * Leaving the party where it stands: every member is marked as lying
	 * at that place, and the party is empty afterwards
	 */
	static void quitHere(Party &party, const MazeState &state);

	/** True if anyone in the party can still walk */
	static bool anyoneAlive(const Party &party);

	/**
	 * Picks the monster of an encounter from the level's tables: the
	 * first table three times in four, tougher monsters now and then
	 */
	static int pickMonster(const MazeLevel &level, Dice &dice);

	/**
	 * True if walking on brings a fight: about one step in a hundred
	 * anywhere, and one in eight on entering a square of a room
	 */
	static bool encounterHappens(const MazeLevel &level, const MazeState &state, Dice &dice);
};

} // End of namespace Wizardry

#endif
