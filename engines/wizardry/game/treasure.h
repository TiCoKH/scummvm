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

#ifndef WIZARDRY_GAME_TREASURE_H
#define WIZARDRY_GAME_TREASURE_H

#include "wizardry/game/items.h"
#include "wizardry/game/maze.h"
#include "wizardry/resources/messages.h"

namespace Wizardry {

/**
 * What a fight may leave behind: record type 3 of the scenario file of
 * IBM-PC Wizardry I (168 bytes). Up to nine entries, each with a chance:
 * either gold (dice x a factor x dice) or an item from a range.
 */
struct Reward {
	enum {
		kRecordSize = 168,
		kMaxEntries = 9
	};

	struct Entry {
		int16 chance;     ///< percent
		int16 isItem;
		int16 values[7];  ///< gold: dice, factor, dice. Item: first, step, most steps, range, chance of a step.
	};

	bool hasChest = false;
	uint16 traps = 0;     ///< a bit for each kind of trap the chest may have
	int16 count = 0;
	Entry entries[kMaxEntries] = {};
};

class RewardList {
private:
	Common::Array<Reward> _rewards;

public:
	bool load(const ScenarioFile &scenario);

	uint size() const { return _rewards.size(); }
	const Reward &operator[](uint index) const { return _rewards[index]; }
};

/**
 * A chest and its trap
 */
class Chest {
public:
	enum Trap {
		kNoTrap, kPoisonNeedle, kGasBomb, kMechanical, kTeleporter,
		kMageBlaster, kPriestBlaster, kAlarm
	};

	/** The three traps that count as one kind */
	enum Variant {
		kCrossbowBolt, kExplodingBox, kStunner
	};

	enum DisarmResult {
		kDisarmed,
		kTryAgain,   ///< "you could not disarm it"
		kSetOff
	};

private:
	int _trap = kNoTrap;
	int _variant = 0;
	bool _inspected[Party::kMaxMembers];
	bool _alarm = false;

	void damage(Character &c, int amount, const MessageFile &messages, Common::Array<Common::String> &lines);
	void classCurse(Party &party, int classType, Dice &dice);

public:
	Chest();

	/**
	 * Chooses the trap: none where the reward allows none, and
	 * sometimes none on the upper levels
	 */
	void pickTrap(const Reward &reward, int mazeLevel, Dice &dice);
	void setTrap(int trap, int variant) { _trap = trap; _variant = variant; }

	int trap() const { return _trap; }

	/** Message with the name of a trap */
	static uint trapNameMessage(int trap, int variant);
	uint trapNameMessage() const { return trapNameMessage(_trap, _variant); }

	/** After it was set off: true if the alarm rang, which brings a fight and no treasure */
	bool alarmRang() const { return _alarm; }

	/**
	 * Opening it just so. With luck even a trapped chest does nothing.
	 * @returns true if the trap went off
	 */
	bool open(Party &party, uint member, MazeState &maze, const MessageFile &messages, Dice &dice,
		Common::Array<Common::String> &lines);

	/**
	 * Looking for the trap. Thieves and ninjas are good at it; a clumsy
	 * look may set the trap off, and a wrong answer looks like a right one.
	 * @param sprung  Set if the trap went off
	 * @returns the message with the trap's name as it seems, or 0 if this
	 *          member has looked already
	 */
	uint inspect(Party &party, uint member, MazeState &maze, const MessageFile &messages, Dice &dice,
		bool &sprung, Common::Array<Common::String> &lines);

	/**
	 * CALFO tells the trap, and is right 19 times in 20
	 * @returns the message with the trap's name, or 0 if the member cannot cast it
	 */
	uint calfo(Character &c, Dice &dice);

	/**
	 * Disarming needs the trap's name. The wrong name sets it off.
	 */
	DisarmResult disarm(Party &party, uint member, const Common::String &name, MazeState &maze,
		const MessageFile &messages, Dice &dice, Common::Array<Common::String> &lines);

	/**
	 * The trap goes off on the member who touched the chest
	 */
	void spring(Party &party, uint member, MazeState &maze, const MessageFile &messages, Dice &dice,
		Common::Array<Common::String> &lines);
};

class Treasure {
public:
	/**
	 * Hands out what a reward holds: items to whoever has room, starting
	 * with a member at random, and the gold in equal shares to those
	 * still standing.
	 * @param goldFactor  2 for a fight in a room already cleared
	 * @param lines       Receives what is found
	 * @returns the gold each survivor got
	 */
	static int64 collect(Party &party, const Reward &reward, const ItemList &items, int goldFactor,
		const MessageFile &messages, Dice &dice, Common::Array<Common::String> &lines);
};

} // End of namespace Wizardry

#endif
