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

#include "common/endian.h"
#include "wizardry/game/maze.h"

namespace Wizardry {

// Offsets in the record. The walls are four arrays of 60 words with two
// bits for each square, indexed [x][y]; side 0 is north.
enum {
	kOfsWallsWest = 0,
	kOfsWallsSouth = 120,
	kOfsWallsEast = 240,
	kOfsWallsNorth = 360,
	kOfsFightSquares = 480,   // 40 words, one bit for each square
	kOfsSquareIndex = 560,    // 100 words, four bits for each square
	kOfsSquareType = 760,     // 4 words, four bits for each of the 16 kinds
	kOfsAux = 768,            // three arrays of 16 words
	kOfsEncounters = 864      // three tables of five words
};

static const uint16 WALL_OFFSETS[4] = { kOfsWallsNorth, kOfsWallsEast, kOfsWallsSouth, kOfsWallsWest };

MazeLevel::MazeLevel() {
	memset(_data, 0, sizeof(_data));
}

uint MazeLevel::word(uint offset) const {
	return READ_LE_UINT16(_data + offset);
}

bool MazeLevel::load(const ScenarioFile &scenario, int level) {
	// The first record is level 1
	const ScenarioFile::Type &type = scenario.type(kRecordMaze);
	const byte *src = (level >= 1) ? scenario.record(kRecordMaze, level - 1) : nullptr;
	if (!src || type.recordSize < kRecordSize)
		return false;

	memcpy(_data, src, kRecordSize);
	return true;
}

uint MazeLevel::wall(int x, int y, int side) const {
	return (word(WALL_OFFSETS[side & 3] + (x * 3 + y / 8) * 2) >> ((y % 8) * 2)) & 3;
}

bool MazeLevel::isFightSquare(int x, int y) const {
	return (word(kOfsFightSquares + (x * 2 + y / 16) * 2) >> (y % 16)) & 1;
}

uint MazeLevel::squareIndex(int x, int y) const {
	return (word(kOfsSquareIndex + (x * 5 + y / 4) * 2) >> ((y % 4) * 4)) & 15;
}

uint MazeLevel::squareType(uint index) const {
	return (word(kOfsSquareType + (index / 4) * 2) >> ((index % 4) * 4)) & 15;
}

int MazeLevel::aux(uint which, uint index) const {
	return (int16)word(kOfsAux + which * 32 + index * 2);
}

void MazeLevel::setAux(uint which, uint index, int value) {
	WRITE_LE_UINT16(_data + kOfsAux + which * 32 + index * 2, (uint16)value);
}

void MazeLevel::setSquareType(uint index, uint type) {
	const uint offset = kOfsSquareType + (index / 4) * 2;
	const uint shift = (index % 4) * 4;
	WRITE_LE_UINT16(_data + offset, (word(offset) & ~(15 << shift)) | ((type & 15) << shift));
}

int MazeLevel::encounterTable(uint table, uint field) const {
	return (int16)word(kOfsEncounters + (table * 5 + field) * 2);
}

int Maze::pickMonster(const MazeLevel &level, Dice &dice) {
	uint table = 0;
	while (dice.roll(4) == 2 && table < 2)
		++table;

	const int first = level.encounterTable(table, 0), step = level.encounterTable(table, 1);
	const int maxSteps = level.encounterTable(table, 2), range = level.encounterTable(table, 3);
	const int stepChance = level.encounterTable(table, 4);

	int steps = 0;
	while ((int)dice.roll(100) < stepChance && steps < maxSteps)
		++steps;

	return first + step * steps + dice.roll(MAX(range, 1));
}

EncounterKind Maze::encounter(const MazeLevel &level, const MazeState &state,
		const RoomFights &rooms, Dice &dice) {
	const bool room = level.isFightSquare(state.x, state.y);
	const bool wandering = dice.roll(99) == 35;

	if (rooms.isPending(state.x, state.y))
		return kEncounterGuardians;
	if (wandering)
		return room ? kEncounterRoom : kEncounterCorridor;
	if (room && dice.roll(8) == 3)
		return kEncounterRoom;
	return kEncounterNone;
}

RoomFights::RoomFights() {
	memset(_pending, 0, sizeof(_pending));
	memset(_visited, 0, sizeof(_visited));
}

// Marks the squares of a room: those of a room joined through open sides
static void floodRoom(const MazeLevel &level, bool marked[MazeLevel::kSize][MazeLevel::kSize], int x, int y) {
	x = (x + MazeLevel::kSize) % MazeLevel::kSize;
	y = (y + MazeLevel::kSize) % MazeLevel::kSize;
	if (marked[x][y] || !level.isFightSquare(x, y))
		return;

	marked[x][y] = true;
	if (level.wall(x, y, kNorth) == kOpen)
		floodRoom(level, marked, x, y + 1);
	if (level.wall(x, y, kEast) == kOpen)
		floodRoom(level, marked, x + 1, y);
	if (level.wall(x, y, kSouth) == kOpen)
		floodRoom(level, marked, x, y - 1);
	if (level.wall(x, y, kWest) == kOpen)
		floodRoom(level, marked, x - 1, y);
}

void RoomFights::reset(const MazeLevel &level, Dice &dice) {
	const int total = MazeLevel::kSize * MazeLevel::kSize;
	memset(_pending, 0, sizeof(_pending));

	for (int room = 0; room < 16; ++room) {
		// A room square not yet marked, searching on from a place at random
		const int start = dice.roll(total);
		int found = -1;
		for (int n = 0; n < total && found < 0; ++n) {
			const int at = (start + n) % total;
			if (level.isFightSquare(at / MazeLevel::kSize, at % MazeLevel::kSize) &&
					!_pending[at / MazeLevel::kSize][at % MazeLevel::kSize])
				found = at;
		}
		if (found < 0)
			break;

		floodRoom(level, _pending, found / MazeLevel::kSize, found % MazeLevel::kSize);
	}

	// The rooms of the fixed encounters are always guarded. A fixed
	// encounter outside any room guards its own square.
	for (int x = 0; x < MazeLevel::kSize; ++x) {
		for (int y = 0; y < MazeLevel::kSize; ++y) {
			if (level.squareTypeAt(x, y) != kSquareEncounter)
				continue;
			if (level.isFightSquare(x, y))
				floodRoom(level, _pending, x, y);
			else
				_pending[x][y] = true;
		}
	}
}

void RoomFights::arm(int x, int y, int distance) {
	for (int dx = -distance; dx <= distance; ++dx) {
		for (int dy = -distance; dy <= distance; ++dy)
			_pending[(x + dx + MazeLevel::kSize * 2) % MazeLevel::kSize][(y + dy + MazeLevel::kSize * 2) % MazeLevel::kSize] = true;
	}
	_pending[x][y] = false;
}

void RoomFights::visit(const MazeLevel &level, int x, int y, uint wall) {
	if (wall != kOpen)
		return;

	x = (x + MazeLevel::kSize) % MazeLevel::kSize;
	y = (y + MazeLevel::kSize) % MazeLevel::kSize;
	if (_visited[x][y] || level.squareTypeAt(x, y) == kSquareEncounter)
		return;

	_visited[x][y] = true;
	_pending[x][y] = false;

	visit(level, x, y + 1, level.wall(x, y, kNorth));
	visit(level, x + 1, y, level.wall(x, y, kEast));
	visit(level, x, y - 1, level.wall(x, y, kSouth));
	visit(level, x - 1, y, level.wall(x, y, kWest));
}

void RoomFights::clearRoom(const MazeLevel &level, int x, int y) {
	memset(_visited, 0, sizeof(_visited));
	visit(level, x, y, kOpen);
}

void Maze::offset(int &x, int &y, int facing, int right, int forward) {
	switch (facing & 3) {
	case kNorth:
		x += right;
		y += forward;
		break;
	case kEast:
		x += forward;
		y -= right;
		break;
	case kSouth:
		x -= right;
		y -= forward;
		break;
	default:
		x -= forward;
		y += right;
		break;
	}

	x = (x + MazeLevel::kSize) % MazeLevel::kSize;
	y = (y + MazeLevel::kSize) % MazeLevel::kSize;
}

bool Maze::forward(MazeState &state, const MazeLevel &level) {
	if (level.wall(state.x, state.y, state.facing) == kWall)
		return false;

	int x = state.x, y = state.y;
	offset(x, y, state.facing, 0, 1);
	state.x = x;
	state.y = y;
	return true;
}

void Maze::turn(MazeState &state, int by) {
	state.facing = (state.facing + by) & 3;
}

void Maze::poisonAndRegen(Party &party, Dice &dice) {
	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);
		if (dice.roll(8) != 0)
			continue;

		if (c.poison > 0)
			c.hp -= (c.hp < 51) ? c.poison : c.hp / 10;
		c.hp += c.healPoints;

		if (c.hp < 1) {
			c.poison = 0;
			if (c.status < kStatusDead) {
				c.hp = 0;
				c.status = kStatusDead;
			}
		} else {
			c.hp = MIN(c.hp, c.hpMax);
		}
	}
}

void Maze::squareDamage(Party &party, const MazeLevel &level, uint index, int mazeLevel, Dice &dice) {
	const int bonus = level.aux(0, index), sides = level.aux(1, index), count = level.aux(2, index);

	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);
		if (c.status >= kStatusDead)
			continue;
		if (c.attributes[kAgility] >= (int)dice.roll(25) + mazeLevel)
			continue;

		int damage = bonus;
		for (int d = 0; d < count; ++d)
			damage += dice.roll(MAX(sides, 1)) + 1;

		c.hp -= damage;
		// Below zero, not at zero: the original leaves a member alive
		// with no hit points
		if (c.hp < 0) {
			c.hp = 0;
			c.status = kStatusDead;
		}
	}
}

void Maze::quitHere(Party &party, const MazeState &state) {
	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);
		c.lostPosition = state.x * 100 + state.y;
		c.lostLevel = state.level;
	}

	// Emptying the party clears "with a party" for everyone
	while (party.size())
		party.remove(party.size() - 1);
}

int Maze::fixedEncounter(MazeLevel &level, const MazeState &state, RoomFights &rooms, Dice &dice) {
	const uint index = level.squareIndex(state.x, state.y);
	const int times = level.aux(0, index);

	if (times == 0 || !rooms.isPending(state.x, state.y))
		return -1;

	int monster = level.aux(2, index);
	if (level.aux(1, index) > 1)
		monster += dice.roll(level.aux(1, index));

	// A number above zero is how often it is still there; below, always
	if (times > 0) {
		level.setAux(0, index, times - 1);
		if (times == 1)
			level.setSquareType(index, kSquareNormal);
	}

	rooms.clearRoom(level, state.x, state.y);
	return monster;
}

void Maze::wipedOut(Party &party, const MazeState &state, Dice &dice) {
	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);

		if (c.status < kStatusDead)
			c.status = kStatusDead;
		c.hp = 0;
		c.gold /= 2;

		// Each item not cursed is lost unless luck holds
		for (int item = c.itemCount - 1; item >= 0; --item) {
			if (!c.items[item].cursed && (int)dice.roll(21) > c.attributes[kLuck])
				c.removeItem(item);
		}

		if ((int)dice.roll(50) < state.level) {
			c.status = kStatusLost;
		} else {
			// The body stays, for another party to find
			c.lostPosition = state.x * 100 + state.y;
			c.lostLevel = state.level;
		}
	}

	while (party.size())
		party.remove(party.size() - 1);
}

bool Maze::returnToTown(Party &party) {
	enum {
		kAmulet = 94
	};
	bool won = false;

	for (uint i = 0; i < party.size(); ++i) {
		Character &c = party.member(i);
		for (int item = c.itemCount - 1; item >= 0; --item) {
			if (c.items[item].id == kAmulet) {
				c.removeItem(item);
				won = true;
			}
		}
		c.poison = 0;
		c.lostPosition = 0;
		c.lostLevel = 0;
	}

	if (won) {
		for (uint i = 0; i < party.size(); ++i) {
			Character &c = party.member(i);
			c.experience = MIN<int64>(c.experience + 50000, 999999999999LL);
			c.gold = MIN<int64>(c.gold + 50000, 999999999999LL);
			c.honors |= 1;
		}
	}

	// Those who cannot walk are left to the care of the temple
	for (int i = (int)party.size() - 1; i >= 0; --i) {
		if (party.member(i).status != kStatusOk)
			party.remove(i);
	}

	return won;
}

bool Maze::anyoneAlive(const Party &party) {
	for (uint i = 0; i < party.size(); ++i) {
		if (party.member(i).status == kStatusOk)
			return true;
	}

	return false;
}

} // End of namespace Wizardry
