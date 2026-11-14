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
#include "wizardry/game/specials.h"
#include "wizardry/views/maze/battle_view.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace InMaze {

enum {
	kMsgYesNo = 100,
	kMsgGotAnItem = 102,
	kMsgWillYouSearch = 107,
	kMsgEventYes = 115,
	kMsgEventNo = 116,
	kMsgSlaughtered = 510,
	kMsgLeaveCemetery = 511,
	kMsgEncounter = 702,
	kMsgCongratulations = 1600,
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
	static_cast<MazeView *>(g_events->findView("Maze"))->_loadedLevel = -1;

	// Everyone goes in with all spells and with figures worked out afresh
	for (uint i = 0; i < party.size(); ++i) {
		Inn::setSpellSlots(party.member(i));
		Equip::recalc(party.member(i), _G(items));
	}

	g_events->replaceView("Maze", true);
}

bool MazeView::msgFocus(const FocusMessage &msg) {
	MazeState &state = _G(mazeState);
	EngineDice dice;

	_state = kWalking;
	_encounter = -1;
	loadLevel();

	// After a teleporter the party comes down on a plain square
	if (state.randomPosition) {
		for (int tries = 0; tries < 1000 && _G(mazeLevel).squareTypeAt(state.x, state.y) != kSquareNormal; ++tries) {
			state.x = dice.roll(MazeLevel::kSize);
			state.y = dice.roll(MazeLevel::kSize);
		}
		state.randomPosition = false;
	}

	// There is no fight in the room the party stands in
	_G(roomFights).clearRoom(_G(mazeLevel), state.x, state.y);

	// An alarm brings its fight at once
	if (state.alarm) {
		_encounterKind = kEncounterGuardians;
		_encounter = Maze::pickMonster(_G(mazeLevel), dice);
		_canRun = true;
		_said = _G(messages).get(kMsgEncounter);
		_said2.clear();
		_state = kSaid;
		delayFrames(kPauseFrames);
	}

	return TownView::msgFocus(msg);
}

void MazeView::loadLevel() {
	const int level = _G(mazeState).level;
	Common::Array<MazeLevel> &levels = _G(mazeLevels);

	// The levels are kept as they are played, since they change
	if (levels.empty()) {
		for (int n = 1; n <= 64; ++n) {
			MazeLevel one;
			if (!one.load(_G(scenario), n))
				break;
			levels.push_back(one);
		}
	}
	if (level < 1 || level > (int)levels.size())
		error("There is no level %d in the maze", level);
	_G(mazeLevel) = levels[level - 1];

	// Coming to a level, it is decided afresh which rooms are guarded,
	// and a field where spells fizzle is left behind
	if (_loadedLevel != level) {
		EngineDice dice;
		_loadedLevel = level;
		_G(roomFights).reset(_G(mazeLevel), dice);
		_G(mazeState).spellsFizzle = false;
	}
}

void MazeView::storeLevel() {
	const int level = _G(mazeState).level;
	if (level >= 1 && level <= (int)_G(mazeLevels).size())
		_G(mazeLevels)[level - 1] = _G(mazeLevel);
}

void MazeView::partyWipedOut() {
	EngineDice dice;
	Maze::wipedOut(_G(party), _G(mazeState), dice);

	_mainText.clear();
	_mainText.push_back(_G(messages).get(kMsgSlaughtered));
	_mainText.push_back("");
	_mainText.push_back(_G(messages).get(kMsgLeaveCemetery));
	_state = kMainText;
	redraw();
}

void MazeView::drawBackground(TownScreens &screens) {
	// Everything is drawn in drawTown()
}

void MazeView::drawTown(TownScreens &screens) {
	MazeState &state = _G(mazeState);

	screens.drawMaze(_G(mazeLevel), state, _windows, _windows && _partyShown);

	switch (_state) {
	case kMainText:
		screens.drawMainText(_mainText);
		break;
	case kElevator: {
		const uint index = _G(mazeLevel).squareIndex(state.x, state.y);
		screens.drawElevator(_G(mazeLevel).aux(1, index) - _G(mazeLevel).aux(2, index) + 1);
		break;
	}
	case kEventText:
		_eventNext = screens.drawEventText(_eventLines, _eventPage);
		break;
	case kEventSearch:
		screens.drawEventLine("", _G(messages).get(kMsgWillYouSearch) + _G(messages).get(kMsgYesNo));
		break;
	case kEventFound:
		screens.drawEventLine(_said);
		break;
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

	if (_encounter >= 0) {
		const int monster = _encounter;
		_encounter = -1;
		_state = kWalking;
		BattleView::start(monster, _canRun, _encounterKind);
		return;
	}

	// A pit or a trap may have finished the party
	if (!Maze::anyoneAlive(_G(party))) {
		partyWipedOut();
		return;
	}

	_state = kWalking;
	redraw();
}

void MazeView::backToTown() {
	const MessageFile &messages = _G(messages);

	if (Maze::returnToTown(_G(party))) {
		// The amulet was brought back: the text ends with a line "!END!"
		_mainText.clear();
		for (uint id = kMsgCongratulations; messages.contains(id) && messages.get(id) != "!END!"; ++id)
			_mainText.push_back(messages.get(id));
		_state = kMainText;
		redraw();
		return;
	}

	g_events->replaceView("Town.Castle", true);
}

void MazeView::showEventText(int text) {
	_eventLines = Specials::text(_G(messages), text);
	_eventPage = 0;

	if (_eventLines.empty()) {
		eventAfterText();
	} else {
		_state = kEventText;
		redraw();
	}
}

void MazeView::startEvent(uint index) {
	MazeState &state = _G(mazeState);

	_event = Specials::begin(_G(mazeLevel), index);
	storeLevel();
	_eventRefusal = false;
	if (!_event.happens)
		return;

	// Two kinds say nothing if the party may pass
	if (_event.kind == Specials::kRequireItem || _event.kind == Specials::kAlignmentGate) {
		const bool passes = (_event.kind == Specials::kRequireItem) ?
			Specials::anyoneCarries(_G(party), _event.param) : Specials::gateLetsBy(_G(party), _event.param);
		if (passes)
			return;

		Specials::pushBack(state);
		_eventRefusal = true;
		showEventText(_event.text);
		return;
	}

	showEventText(_event.text);
}

void MazeView::eventGive(int item) {
	const int member = Specials::giveItem(_G(party), item);
	if (member < 0) {
		_state = kWalking;
	} else {
		_said = substCaret(_G(messages).get(kMsgGotAnItem), _G(party).member(member).name);
		_state = kEventFound;
	}
	redraw();
}

void MazeView::eventAfterText() {
	MazeState &state = _G(mazeState);
	_state = kWalking;

	if (_eventRefusal) {
		redraw();
		return;
	}

	switch (_event.kind) {
	case Specials::kGiveItem:
		eventGive(_event.param);
		return;

	case Specials::kSearch:
		_state = kEventSearch;
		break;

	case Specials::kLightOrArmor:
		Specials::lightOrArmor(state, _event.param);
		break;

	case Specials::kToCastle:
		backToTown();
		return;

	case Specials::kArmRooms:
		_G(roomFights).arm(state.x, state.y, _event.param);
		break;

	case Specials::kPool:
	case Specials::kRiddle:
	case Specials::kPayFee:
		// TODO: none of these is used by the first scenario
		break;

	default:
		break;
	}

	redraw();
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

	case kSquareElevator:
		_state = kElevator;
		break;

	case kSquareSpecial:
		startEvent(index);
		break;

	case kSquareEncounter: {
		MazeLevel &changing = _G(mazeLevel);
		const int monster = Maze::fixedEncounter(changing, state, _G(roomFights), dice);
		storeLevel();
		if (monster >= 0) {
			// No running from these
			_encounter = monster;
			_encounterKind = kEncounterGuardians;
			_canRun = false;
			say(messages.get(kMsgEncounter));
		}
		break;
	}

	default:
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

	case kMainText:
		if (ascii != kKeyReturn)
			return;
		g_events->replaceView("Town.Castle", true);
		return;

	case kElevator: {
		const MazeLevel &level = _G(mazeLevel);
		const uint index = level.squareIndex(state.x, state.y);
		const int first = level.aux(2, index), last = level.aux(1, index);

		// RETURN stays on this floor
		if (ascii == kKeyReturn) {
			_state = kWalking;
			redraw();
			return;
		}
		if (ascii < 'A' || ascii > 'A' + (last - first))
			return;

		// Some elevators let the party out at a place at random
		if (level.aux(0, index) > 0) {
			EngineDice dice;
			state.x = dice.roll(MazeLevel::kSize);
			state.y = dice.roll(MazeLevel::kSize);
		}
		state.level = first + (ascii - 'A');
		_state = kWalking;
		if (state.level <= 0) {
			backToTown();
			return;
		}
		loadLevel();
		_G(roomFights).clearRoom(_G(mazeLevel), state.x, state.y);
		redraw();
		return;
	}

	case kEventText:
		if (ascii != kKeyReturn)
			return;
		if (_eventNext < _eventLines.size()) {
			_eventPage = _eventNext;
			redraw();
		} else {
			eventAfterText();
		}
		return;

	case kEventSearch:
		if (ascii == (byte)messages.get(kMsgEventNo)[0]) {
			_state = kWalking;
			redraw();
		} else if (ascii == (byte)messages.get(kMsgEventYes)[0]) {
			const int monster = Specials::searchMonster(_event);
			if (monster >= 0) {
				// What was waiting does not let the party run
				_encounter = monster;
				_encounterKind = kEncounterCorridor;
				_canRun = false;
				say(messages.get(kMsgEncounter));
			} else if (Specials::searchItem(_event) >= 0) {
				eventGive(Specials::searchItem(_event));
			} else {
				_state = kWalking;
				redraw();
			}
		}
		return;

	case kEventFound:
		if (ascii != kKeyReturn)
			return;
		_state = kWalking;
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

		// The step may have been the party's last
		if (!Maze::anyoneAlive(party) && _state != kSaid) {
			partyWipedOut();
			return;
		}

		// A fight: the guardians of a room for certain, others now and then
		if (_state == kWalking) {
			_encounterKind = Maze::encounter(_G(mazeLevel), state, _G(roomFights), dice);
			if (_encounterKind != kEncounterNone) {
				_encounter = Maze::pickMonster(_G(mazeLevel), dice);
				_canRun = true;
				say(messages.get(kMsgEncounter));
				return;
			}
		}
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
	} else if (command == kCmdCamp) {
		addView("Camp");
		return;
	} else {
		// TODO: the message delay and inspecting
		notice("NOT IN THIS VERSION YET");
		return;
	}

	redraw();
}

} // End of namespace InMaze
} // End of namespace Wizardry
