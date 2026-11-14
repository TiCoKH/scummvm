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

#include "wizardry/views/maze/rewards_view.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace InMaze {

enum {
	kMsgOops = 814,
	kMsgNotTrapped = 815,
	kMsgInspect = 816,
	kMsgAlreadyLooked = 817,
	kMsgCastCalfo = 818,
	kMsgDisarmed = 819,
	kMsgCouldNotDisarm = 820,
	kMsgTriggered = 821,
	kMsgDisarm = 822,
	kMsgTrapPrompt = 823,
	kMsgOpen = 824,
	kMsgEachSurvivorGets = 827,     // gold; the next message ends the sentence
	kMsgEachSurvivorGetsXp = 926    // experience; the same
};

uint RewardsView::EngineDice::roll(uint count) {
	return g_engine->getRandomNumber(count - 1);
}

void RewardsView::start(int64 experience, int monsterId, int kind) {
	RewardsView *view = static_cast<RewardsView *>(g_events->findView("Rewards"));
	const MessageFile &messages = _G(messages);
	const Monster &monster = _G(monsters)[monsterId];
	MazeState &maze = _G(mazeState);

	view->_lines.clear();
	view->_line = 0;
	view->_noTreasure = false;
	view->_chest = Chest();
	view->_goldFactor = (kind == kEncounterRoom) ? 2 : 1;

	const int table = (kind == kEncounterGuardians) ? monster.rewardLair : monster.rewardWandering;
	view->_reward = (table >= 0 && table < (int)_G(rewards).size()) ? &_G(rewards)[table] : nullptr;

	view->_lines.push_back(messages.get(kMsgEachSurvivorGetsXp) +
		Common::String::format("%lld", (long long)experience) + messages.get(kMsgEachSurvivorGetsXp + 1));

	// The fight an alarm brought has no chest
	view->_chestDone = !view->_reward || !view->_reward->hasChest || maze.alarm;
	maze.alarm = false;
	if (!view->_chestDone)
		view->_chest.pickTrap(*view->_reward, maze.level, view->_dice);

	view->_state = kTelling;
	g_events->replaceView(view);
}

void RewardsView::drawBackground(TownScreens &screens) {
	screens.drawMaze(_G(mazeLevel), _G(mazeState), false, true);
}

void RewardsView::drawTown(TownScreens &screens) {
	const MessageFile &messages = _G(messages);
	static const uint16 DEEDS[] = { kMsgOpen, kMsgInspect, kMsgCastCalfo, kMsgDisarm };

	switch (_state) {
	case kChest:
		screens.drawChest(_menu);
		break;

	case kWho:
		screens.drawChestWho(DEEDS[_deed]);
		break;

	case kTrapName: {
		TextWindow window = screens.chestWindow();
		window.clear();
		window.PRINTSTR(0, 1, messages.get(kMsgDisarm));
		window.PRINTCHR(' ');
		window.PRINTSTR(messages.get(kMsgTrapPrompt));
		window.PRINTSTR(_typed);
		break;
	}

	case kTelling:
		if (_line < _lines.size())
			screens.drawMazePopup(_lines[_line]);
		break;
	}
}

void RewardsView::tell(const Common::String &line) {
	if (_state != kTelling || _line >= _lines.size()) {
		_lines.clear();
		_line = 0;
	}
	_lines.push_back(line);
	_state = kTelling;
}

void RewardsView::handOut() {
	const MessageFile &messages = _G(messages);
	_chestDone = true;

	if (!_reward || _noTreasure)
		return;

	Common::Array<Common::String> found;
	const int64 share = Treasure::collect(_G(party), *_reward, _G(items), _goldFactor, messages, _dice, found);
	for (uint i = 0; i < found.size(); ++i)
		tell(found[i]);
	tell(messages.get(kMsgEachSurvivorGets) + Common::String::format("%lld", (long long)share) +
		messages.get(kMsgEachSurvivorGets + 1));
	_noTreasure = true;   // only once
}

void RewardsView::afterTelling() {
	if (!_chestDone) {
		_state = kChest;
		redraw();
		return;
	}

	if (!_noTreasure && _reward) {
		_lines.clear();
		_line = 0;
		handOut();
		redraw();
		return;
	}

	// Done: back to the maze below, which may have a fight waiting
	close();
}

void RewardsView::doDeed() {
	const MessageFile &messages = _G(messages);
	Party &party = _G(party);
	MazeState &maze = _G(mazeState);
	Common::Array<Common::String> harm;
	bool sprung = false;

	_lines.clear();
	_line = 0;

	switch (_deed) {
	case kOpen:
		if (_chest.trap() == Chest::kNoTrap) {
			_chestDone = true;
		} else {
			sprung = _chest.open(party, _member, maze, messages, _dice, harm);
			_chestDone = true;
		}
		break;

	case kInspect: {
		const uint name = _chest.inspect(party, _member, maze, messages, _dice, sprung, harm);
		if (!name)
			tell(messages.get(kMsgAlreadyLooked));
		else if (!sprung)
			tell(messages.get(name));
		if (sprung)
			_chestDone = true;
		break;
	}

	case kCalfo: {
		const uint name = _chest.calfo(party.member(_member), _dice);
		// Nothing is said to one who cannot cast it
		if (name)
			tell(messages.get(name));
		break;
	}

	default:
		break;
	}

	if (sprung) {
		tell(messages.get(kMsgOops));
		tell(messages.get(_chest.trapNameMessage()));
		for (uint i = 0; i < harm.size(); ++i)
			tell(harm[i]);

		// The alarm brings a fight and leaves no treasure
		if (_chest.alarmRang()) {
			maze.alarm = true;
			_noTreasure = true;
		}
	}

	if (_lines.empty())
		afterTelling();
	else
		redraw();
}

void RewardsView::choose(int result, uint16 ascii) {
	const MessageFile &messages = _G(messages);
	Party &party = _G(party);
	MazeState &maze = _G(mazeState);

	switch (_state) {
	case kTelling:
		if (ascii != kKeyReturn)
			return;
		if (++_line < _lines.size())
			redraw();
		else
			afterTelling();
		return;

	case kChest:
		if (result < kOpen || result > kLeaveAlone)
			return;
		if (result == kLeaveAlone) {
			// No treasure at all
			_chestDone = true;
			_noTreasure = true;
			afterTelling();
			return;
		}
		_deed = result;
		_state = kWho;
		redraw();
		return;

	case kWho:
		if (ascii == kKeyReturn || ascii == '0') {
			_state = kChest;
			redraw();
			return;
		}
		if (ascii < '1' || ascii > '0' + (int)party.size())
			return;
		// It takes someone in good health
		if (party.member(ascii - '1').status != kStatusOk)
			return;

		_member = ascii - '1';
		if (_deed == kDisarm) {
			_typed.clear();
			_state = kTrapName;
			redraw();
		} else {
			doDeed();
		}
		return;

	case kTrapName: {
		if (ascii == kKeyBackspace) {
			if (!_typed.empty())
				_typed.deleteLastChar();
			redraw();
			return;
		}
		if (ascii != kKeyReturn) {
			if (ascii >= ' ' && ascii < 127 && _typed.size() < 20) {
				_typed += (char)ascii;
				redraw();
			}
			return;
		}
		if (_typed.empty()) {
			_state = kChest;
			redraw();
			return;
		}

		Common::Array<Common::String> harm;
		_lines.clear();
		_line = 0;

		switch (_chest.disarm(party, _member, _typed, maze, messages, _dice, harm)) {
		case Chest::kDisarmed:
			_chestDone = true;
			tell(messages.get(kMsgDisarmed));
			break;
		case Chest::kTryAgain:
			tell(messages.get(kMsgCouldNotDisarm));
			break;
		default:
			_chestDone = true;
			tell(messages.get(kMsgTriggered));
			tell(messages.get(_chest.trapNameMessage()));
			for (uint i = 0; i < harm.size(); ++i)
				tell(harm[i]);
			if (_chest.alarmRang()) {
				maze.alarm = true;
				_noTreasure = true;
			}
			break;
		}
		redraw();
		return;
	}

	default:
		return;
	}
}

} // End of namespace InMaze
} // End of namespace Wizardry
