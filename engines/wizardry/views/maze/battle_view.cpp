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

#include "wizardry/views/maze/battle_view.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace InMaze {

enum {
	kMsgSlaughtered = 510,
	kMsgLeaveCemetery = 511,
	kMsgEachSurvivorGets = 926,   // the next message ends the sentence
	kMsgPartySurprises = 1020,
	kMsgMonstersSurprise = 1021,
	kMsgOptFight = 1022,       // "f)IGHT/"
	kMsgOptDispel = 1023,      // "D)ISPELL/"
	kMsgOptSpell = 1024,       // "S)PELL/"
	kMsgSpellPointsExhausted = 1015,
	kMsgDontKnowSpell = 1016,
	kMsgCastOnPerson = 1017,
	kMsgCastOnGroup = 1018,
	kMsgSpellName = 1019,
	kMsgOptRest = 1025,        // "U)SE ITEM/p)ARRY/R)UN/T)AKE BACK"
	kMsgDispelWhich = 1028,
	kMsgFightWhich = 1030,
	kMsgPressToFight = 1035,
	kMsgTakeBackOrders = 1036,
	kMsgTakeBackKey = 1037
};

uint BattleView::EngineDice::roll(uint count) {
	return g_engine->getRandomNumber(count - 1);
}

BattleView::~BattleView() {
	delete _battle;
}

void BattleView::start(int monsterId, bool canRun) {
	BattleView *view = static_cast<BattleView *>(g_events->findView("Battle"));

	delete view->_battle;
	view->_battle = new Battle(_G(party), _G(monsters), _G(messages), _G(mazeState), view->_dice);
	view->_battle->begin(monsterId, canRun);

	// The picture of the first group. The number in the monster record
	// is the picture's number as the game counts them, from 1.
	view->_picture = CellImage();
	if (monsterId >= 0 && monsterId < (int)_G(monsters).size())
		_G(pictures).load(_G(monsters)[monsterId].pictureId & 0xff, view->_picture);

	view->addView();
	view->nextRound();
}

void BattleView::drawBackground(TownScreens &screens) {
	// Everything is drawn in drawTown()
}

void BattleView::drawTown(TownScreens &screens) {
	const MessageFile &messages = _G(messages);

	screens.drawBattle(*_battle, _picture.empty() ? nullptr : &_picture);
	TextWindow window = screens.battleWindow();

	switch (_state) {
	case kAnnounce:
		window.printCenteredLine(messages.get(
			_battle->surprise() == Battle::kPartySurprises ? kMsgPartySurprises : kMsgMonstersSurprise));
		break;

	case kOptions:
		screens.drawBattleOptions(_G(party).member(_member), _options, _menu);
		break;

	case kWhichGroup:
		window.PRINTSTR(messages.get(_dispelling ? kMsgDispelWhich : kMsgFightWhich));
		break;

	case kSpellName:
		window.PRINTSTR(messages.get(kMsgSpellName));
		window.PRINTSTR(_typed);
		if (!_refusal.empty())
			window.PRINTSTR(0, 2, _refusal);
		break;

	case kSpellTarget:
		window.PRINTSTR(messages.get(Spells::target(_spell) == Spells::kTargetMember ?
			kMsgCastOnPerson : kMsgCastOnGroup));
		break;

	case kConfirm:
		window.printCenteredLine(messages.get(kMsgPressToFight));
		window.printCenteredLine(messages.get(kMsgTakeBackOrders));
		break;

	case kTelling:
		if (_line < _lines.size()) {
			// A line may be longer than the window: it goes on below
			const Common::String &line = _lines[_line];
			for (uint i = 0, row = 0; i < line.size(); i += window.width(), ++row)
				window.PRINTSTR(0, row, line.substr(i, window.width()));
		}
		break;

	case kOver:
		for (uint i = 0; i < _ending.size(); ++i)
			window.printCenteredLine(_ending[i]);
		break;
	}
}

void BattleView::nextRound() {
	_battle->prepareRound();
	if (_battle->outcome() != Battle::kUndecided) {
		finish();
		return;
	}

	if (_battle->surprise() != Battle::kNoSurprise) {
		_state = kAnnounce;
		delayFrames(kPauseFrames);
		redraw();
		return;
	}

	_battle->startOrders();
	askMember(0);
}

void BattleView::askMember(uint member) {
	const Party &party = _G(party);
	const MessageFile &messages = _G(messages);

	// On to the first one who can give an order
	while (member < party.size() && !_battle->canOrder(member))
		++member;

	if (member >= party.size()) {
		// With nobody to order, as when surprised, the round just runs
		bool anyone = false;
		for (uint i = 0; i < party.size(); ++i)
			anyone |= _battle->canOrder(i);
		if (!anyone) {
			playRound();
			return;
		}

		_state = kConfirm;
		redraw();
		return;
	}

	_member = member;
	_options.clear();
	if (_battle->mayFight(member))
		_options += messages.get(kMsgOptFight);
	if (_battle->mayDispel(member))
		_options += messages.get(kMsgOptDispel);
	_options += messages.get(kMsgOptSpell);
	_options += messages.get(kMsgOptRest);

	// Without "fight", parrying is what RETURN gives; the list marks it
	_state = kOptions;
	redraw();
}

void BattleView::nextMember() {
	askMember(_member + 1);
}

void BattleView::playRound() {
	_lines.clear();
	_battle->runRound(_lines);
	_line = 0;

	if (_lines.empty()) {
		nextRound();
		return;
	}
	// What was told is shown before a spell carries the party away

	_state = kTelling;
	delayFrames(kPauseFrames);
	redraw();
}

void BattleView::finish() {
	const MessageFile &messages = _G(messages);
	_ending.clear();

	switch (_battle->outcome()) {
	case Battle::kVictory: {
		const int64 share = _battle->awardExperience();
		_ending.push_back(messages.get(kMsgEachSurvivorGets) + Common::String::format("%lld", (long long)share) +
			messages.get(kMsgEachSurvivorGets + 1));
		// TODO: the treasure, with its chest and traps
		break;
	}

	case Battle::kDefeat:
		_ending.push_back(messages.get(kMsgSlaughtered));
		_ending.push_back(messages.get(kMsgLeaveCemetery));
		break;

	case Battle::kRecalled:
		// LOKTOFEIT: home
		g_events->replaceView("Town.Castle", true);
		return;

	default:
		// Ran away, or carried off by MALOR: back to the maze, which
		// loads the level again when it gets the focus
		close();
		return;
	}

	_state = kOver;
	redraw();
}

void BattleView::timeout() {
	switch (_state) {
	case kAnnounce:
		_battle->startOrders();
		askMember(0);
		break;

	case kTelling:
		if (++_line < _lines.size()) {
			delayFrames(kPauseFrames);
			redraw();
		} else {
			nextRound();
		}
		break;

	default:
		TownView::timeout();
		break;
	}
}

void BattleView::choose(int result, uint16 ascii) {
	const MessageFile &messages = _G(messages);
	Party &party = _G(party);

	switch (_state) {
	case kOptions: {
		// The lists differ from member to member, so go by the letter.
		// RETURN is the option marked for it.
		uint16 key = ascii;
		if (ascii == kKeyReturn)
			key = _battle->mayFight(_member) ? 'F' : 'P';

		switch (key) {
		case 'F':
		case 'D':
			if (key == 'F' ? !_battle->mayFight(_member) : !_battle->mayDispel(_member))
				return;
			_dispelling = (key == 'D');
			// With one group there is nothing to ask
			if (_battle->enemyGroups() == 1) {
				if (_dispelling)
					_battle->orderDispel(_member, 1);
				else
					_battle->orderFight(_member, 1);
				nextMember();
			} else {
				_state = kWhichGroup;
				redraw();
			}
			return;

		case 'P':
			_battle->orderParry(_member);
			nextMember();
			return;

		case 'S':
			_typed.clear();
			_refusal.clear();
			_state = kSpellName;
			redraw();
			return;

		case 'R':
			if (_battle->tryRun())
				finish();
			else
				playRound();
			return;

		case 'T':
			_battle->startOrders();
			askMember(0);
			return;

		case 'U':
			notice("NOT IN THIS VERSION YET");   // TODO: using items
			return;

		default:
			return;
		}
	}

	case kWhichGroup: {
		const int group = ascii - '0';
		if (group < 1 || group > _battle->enemyGroups())
			return;
		if (_dispelling)
			_battle->orderDispel(_member, group);
		else
			_battle->orderFight(_member, group);
		nextMember();
		return;
	}

	case kSpellName: {
		if (ascii == kKeyBackspace) {
			if (!_typed.empty())
				_typed.deleteLastChar();
		} else if (ascii == kKeyEscape) {
			_typed.clear();
		} else if (ascii != kKeyReturn) {
			if (ascii >= 'A' && ascii <= 'Z' && _typed.size() < 12)
				_typed += (char)ascii;
		} else if (_typed.empty()) {
			// No name: back to the choices
			askMember(_member);
			return;
		} else {
			// The name, or as much of it as tells it from the others known.
			// TODO: the original lists the spells when several begin alike.
			const Character &c = party.member(_member);
			const Common::Array<int> found = Spells::find(messages, c, _typed);
			_typed.clear();

			if (found.size() != 1 || !c.knowsSpell(found[0])) {
				_refusal = messages.get(kMsgDontKnowSpell);
			} else if (!Spells::canCast(c, found[0])) {
				_refusal = messages.get(kMsgSpellPointsExhausted);
			} else {
				_spell = found[0];
				const Spells::Target target = Spells::target(_spell);
				if (target == Spells::kTargetNone) {
					_battle->orderCast(_member, _spell, 0);
					nextMember();
					return;
				}
				if (target == Spells::kTargetGroup && _battle->enemyGroups() == 1) {
					_battle->orderCast(_member, _spell, 1);
					nextMember();
					return;
				}
				_state = kSpellTarget;
			}
		}
		if (ascii != kKeyReturn)
			_refusal.clear();
		redraw();
		return;
	}

	case kSpellTarget: {
		const int number = ascii - '0';
		if (Spells::target(_spell) == Spells::kTargetMember) {
			if (number < 1 || number > (int)party.size())
				return;
			_battle->orderCast(_member, _spell, number - 1);
		} else {
			if (number < 1 || number > _battle->enemyGroups())
				return;
			_battle->orderCast(_member, _spell, number);
		}
		nextMember();
		return;
	}

	case kConfirm:
		if (ascii == kKeyReturn) {
			playRound();
		} else if (ascii == (byte)messages.get(kMsgTakeBackKey)[0]) {
			_battle->startOrders();
			askMember(0);
		}
		return;

	case kTelling:
		// A key hurries the telling along
		cancelDelay();
		timeout();
		return;

	case kOver:
		if (ascii != kKeyReturn)
			return;
		if (_battle->outcome() == Battle::kDefeat) {
			// TODO: what the original does to a party that was wiped out
			while (party.size())
				party.remove(party.size() - 1);
			g_events->replaceView("Town.Castle", true);
		} else {
			close();
		}
		return;

	default:
		return;
	}
}

} // End of namespace InMaze
} // End of namespace Wizardry
