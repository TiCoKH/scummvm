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

#ifndef WIZARDRY_VIEWS_MAZE_BATTLE_VIEW_H
#define WIZARDRY_VIEWS_MAZE_BATTLE_VIEW_H

#include "wizardry/game/battle.h"
#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace InMaze {

/**
 * A fight: orders are given member by member, then the round is told line
 * by line.
 */
class BattleView : public Town::TownView {
private:
	enum State {
		kAnnounce,     ///< who surprised whom, for a moment
		kOptions,      ///< a member's choices
		kWhichGroup,   ///< which group to fight or dispel
		kSpellName,    ///< typing the name of a spell
		kSpellTarget,  ///< whom, or which group, the spell is for
		kConfirm,      ///< fight, or take the orders back
		kTelling,      ///< the round, a line at a time
		kOver          ///< how it ended, until RETURN
	};

	class EngineDice : public Dice {
	public:
		uint roll(uint count) override;
	};

	EngineDice _dice;
	Battle *_battle = nullptr;
	CellImage _picture;
	State _state = kAnnounce;
	uint _member = 0;
	bool _dispelling = false;
	Common::String _typed;
	int _spell = 0;
	Common::String _refusal;       ///< why the spell cannot be cast
	Common::String _options;
	Common::Array<Common::String> _lines;
	uint _line = 0;
	Common::Array<Common::String> _ending;
	int _monsterId = 0;
	int _kind = 0;                 ///< an EncounterKind

	void nextRound();
	void askMember(uint member);
	void nextMember();
	void playRound();
	void finish();

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	BattleView() : TownView("Battle") {}
	~BattleView() override;

	/**
	 * Starts a fight over the current view
	 * @param canRun  False for fights the party cannot run from
	 */
	static void start(int monsterId, bool canRun, int kind = 0);
};

} // End of namespace InMaze
} // End of namespace Wizardry

#endif
