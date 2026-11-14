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

#ifndef WIZARDRY_VIEWS_MAZE_REWARDS_VIEW_H
#define WIZARDRY_VIEWS_MAZE_REWARDS_VIEW_H

#include "wizardry/game/treasure.h"
#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace InMaze {

/**
 * After a fight is won: the experience, the chest if there is one, and
 * the treasure.
 */
class RewardsView : public Town::TownView {
private:
	enum State {
		kChest,      ///< the chest's menu
		kWho,        ///< which member does it
		kTrapName,   ///< typing the name of the trap to disarm
		kTelling     ///< lines, one at each RETURN; the last one ends it
	};

	enum Deed {
		kOpen, kInspect, kCalfo, kDisarm, kLeaveAlone
	};

	class EngineDice : public Dice {
	public:
		uint roll(uint count) override;
	};

	EngineDice _dice;
	State _state = kTelling;
	Chest _chest;
	const Reward *_reward = nullptr;
	int _goldFactor = 1;
	int _deed = kOpen;
	uint _member = 0;
	Common::String _typed;
	Common::Array<Common::String> _lines;
	uint _line = 0;
	bool _chestDone = true;      ///< no chest, or it is open, or left alone
	bool _noTreasure = false;

	void tell(const Common::String &line);
	void afterTelling();
	void doDeed();
	void handOut();

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;

public:
	RewardsView() : TownView("Rewards") {}

	/**
	 * Shows what a victory brought, in place of the fight's view
	 * @param experience  What each survivor got
	 * @param kind        The kind of fight it was, an EncounterKind
	 */
	static void start(int64 experience, int monsterId, int kind);
};

} // End of namespace InMaze
} // End of namespace Wizardry

#endif
