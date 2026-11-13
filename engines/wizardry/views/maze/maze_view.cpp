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

#include "wizardry/views/maze/maze_view.h"
#include "wizardry/game/equip.h"
#include "wizardry/game/inn.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace InMaze {

enum {
	kMsgStairsDown = 703,
	kMsgStairsUp = 704,
	kMsgTakeThem = 705,
	kMsgYes = 706,
	kMsgNo = 707,
	kMsgRock = 709,
	kMsgOuch = 713,
	kMsgChute = 714,
	kMsgWall = 715,
	kMsgKeys = 722,
	kMsgPit = 723,
	kMsgQuit = 724,
	kMsgQuitYes = 725,
	kMsgQuitNo = 726
};

// Places in the string of command keys: four keys for each way to move
enum {
	kCmdForward = 0,     // 0 to 3
	kCmdLeft = 4,
	kCmdRight = 8,
	kCmdAbout = 12,
	kCmdWindows = 16,
	kCmdParty = 17,
	kCmdDelay = 18,
	kCmdCamp = 19,
	kCmdInspect = 20,
	kCmdQuit = 21
};

namespace {

class EngineDice : public Dice {
public:
	uint roll(uint count) override {
		return g_engine->getRandomNumber(count - 1);
	}
};

} // End of anonymous namespace

void MazeView::enter() {
	Party &party = _G(party);
	MazeState &state = _G(mazeState);

	state = MazeState();

	// Everyone goes in with all spells and with figures worked out afresh
	for (uint i = 0; i < party.size(); ++i) {
		Inn::setSpellSlots(party.member(i));
		Equip::recalc(party.member(i), _G(items));
	}

	g_events->replaceView("Maze", true);
}

bool MazeView::msgFocus(const FocusMessage &msg) {
	_state = kWalking;
	loadLevel();
	return TownView::msgFocus(msg);
}

void MazeView::loadLevel() {
	if (!_G(mazeLevel).load(_G(scenario), _G(mazeState).level))
		error("Could not load level %d of the maze", _G(mazeState).level);
}

void MazeView::drawBackground(TownScreens &screens) {
	// Everything is drawn in drawTown()
}

void MazeView::drawTown(TownScreens &screens) {
	MazeState &state = _G(mazeState);

	screens.drawMaze(_G(mazeLevel), state, _windows, _windows && _partyShown);

	switch (_state) {
	case kSaid:
		screens.drawMazePopup(_said, _said2);
		break;
	case kStairs:
		screens.drawMazePopup(_said, _G(messages).get(kMsgTakeThem));
		break;
	case kQuit:
		screens.drawMazePopup(_G(messages).get(kMsgQuit));
		break;
	default:
		break;
	}
}

void MazeView::say(const Common::String &text) {
	_said = text;
	_said2.clear();
	_state = kSaid;
	delayFrames(kPauseFrames);
	redraw();
}

void MazeView::timeout() {
	if (_state != kSaid) {
		TownView::timeout();
		return;
	}

	if (_leaveAfterSaid) {
		_leaveAfterSaid = false;
		backToTown();
		return;
	}

	_state = kWalking;
	redraw();
}

void MazeView::backToTown() {
	// TODO: what the original does on coming back: the amulet, and
	// leaving those behind who cannot walk
	g_events->replaceView("Town.Castle", true);
}

void MazeView::transfer(uint index) {
	MazeState &state = _G(mazeState);
	const MazeLevel &level = _G(mazeLevel);
	const int target = level.aux(0, index);

	state.x = level.aux(2, index);
	state.y = level.aux(1, index);

	if (target != state.level) {
		state.level = target;
		// Level 0 is the castle
		if (target <= 0) {
			backToTown();
			return;
		}
		loadLevel();
	}
}

void MazeView::enterSquare() {
	MazeState &state = _G(mazeState);
	const MazeLevel &level = _G(mazeLevel);
	Party &party = _G(party);
	const MessageFile &messages = _G(messages);
	EngineDice dice;

	const uint index = level.squareIndex(state.x, state.y);

	switch (level.squareType(index)) {
	case kSquareStairs:
		// Up if they lead to a level above this one
		_said = messages.get(level.aux(0, index) < state.level ? kMsgStairsUp : kMsgStairsDown);
		_state = kStairs;
		break;

	case kSquarePit:
		Maze::squareDamage(party, level, index, state.level, dice);
		say(messages.get(kMsgPit));
		break;

	case kSquareChute:
		transfer(index);
		if (g_events->focusedView() == this)
			say(messages.get(kMsgChute));
		break;

	case kSquareSpinner:
		state.facing = dice.roll(4);
		break;

	case kSquareDark:
		if (level.aux(1, index) == 0)
			state.lightDuration = 0;
		break;

	case kSquareTransfer:
		transfer(index);
		break;

	case kSquareDamage:
		Maze::squareDamage(party, level, index, state.level, dice);
		say(messages.get(kMsgOuch));
		break;

	case kSquareRock:
		// Solid rock: everyone is lost
		for (uint i = 0; i < party.size(); ++i)
			party.member(i).status = kStatusLost;
		while (party.size())
			party.remove(party.size() - 1);
		_leaveAfterSaid = true;
		say(messages.get(kMsgRock));
		break;

	case kSquareFizzle:
		state.spellsFizzle = true;
		break;

	default:
		// TODO: elevators, special squares and fixed encounters
		break;
	}
}

void MazeView::choose(int result, uint16 ascii) {
	MazeState &state = _G(mazeState);
	const MessageFile &messages = _G(messages);
	Party &party = _G(party);

	switch (_state) {
	case kStairs:
		if (ascii == (byte)messages.get(kMsgYes)[0]) {
			_state = kWalking;
			transfer(_G(mazeLevel).squareIndex(state.x, state.y));
			if (g_events->focusedView() != this)
				return;
		} else if (ascii == (byte)messages.get(kMsgNo)[0]) {
			_state = kWalking;
		} else {
			return;
		}
		redraw();
		return;

	case kQuit:
		if (ascii == (byte)messages.get(kMsgQuitYes)[0]) {
			Maze::quitHere(party, state);
			backToTown();
			return;
		}
		if (ascii != (byte)messages.get(kMsgQuitNo)[0])
			return;
		_state = kWalking;
		redraw();
		return;

	case kSaid:
		return;

	default:
		break;
	}

	// The command is the place of the key in a string of the scenario
	const Common::String &keys = messages.get(kMsgKeys);
	int command = -1;
	for (uint i = 0; i < keys.size(); ++i) {
		if ((byte)keys[i] == ascii)
			command = i;
	}
	// The cursor keys need nothing more: they arrive as the codes the
	// original's keyboard driver gives them, and those are in the string
	if (command < 0)
		return;

	// The lights burn down with every drawing of the view, turning included
	if (command < kCmdWindows) {
		if (state.lightDuration > 0)
			--state.lightDuration;
		if (state.monstersIdentified > 0)
			--state.monstersIdentified;
	}

	if (command < kCmdLeft) {
		if (!Maze::forward(state, _G(mazeLevel))) {
			say(messages.get(kMsgWall));
			return;
		}

		// A step taken: the square acts, poison and healing items work
		enterSquare();
		if (g_events->focusedView() != this)
			return;

		EngineDice dice;
		Maze::poisonAndRegen(party, dice);
	} else if (command < kCmdRight) {
		Maze::turn(state, 3);
	} else if (command < kCmdAbout) {
		Maze::turn(state, 1);
	} else if (command < kCmdWindows) {
		Maze::turn(state, 2);
	} else if (command == kCmdWindows) {
		_windows = !_windows;
	} else if (command == kCmdParty) {
		_partyShown = !_partyShown;
	} else if (command == kCmdQuit) {
		_state = kQuit;
	} else {
		// TODO: the message delay, the camp and inspecting
		notice("NOT IN THIS VERSION YET");
		return;
	}

	redraw();
}

} // End of namespace InMaze
} // End of namespace Wizardry
