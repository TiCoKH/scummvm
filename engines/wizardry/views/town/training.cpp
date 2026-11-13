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

#include "wizardry/views/town/training.h"
#include "wizardry/game/inn.h"
#include "wizardry/game/training.h"
#include "wizardry/views/town/sheet.h"
#include "wizardry/wizardry.h"

namespace Wizardry {
namespace Town {

enum {
	kMsgSureYouWant = 304,
	kMsgEditYes = 306,
	kMsgEditNo = 307,
	kMsgDelete = 308,
	kMsgChangesLeave = 311,
	kMsgNewPassword = 312,
	kMsgNewPasswordAgain = 313,
	kMsgPasswordChanged = 314,
	kMsgNotTheSame1 = 315,
	kMsgNotTheSame2 = 316,
	kMsgIncorrectPassword = 411,
	kMsgSelectClass = 319,
	kMsgKeepCharacter = 320,
	kMsgYes = 321,
	kMsgNo = 322,
	kMsgEnterPassword = 329,
	kMsgEnterAgain = 330,
	kMsgChooseRace = 331,
	kMsgChooseAlignment = 332,
	kMsgRosterFull = 335,
	kMsgNameInUse = 336,
	kMsgEnterName = 337,
	kMsgFirstRace = 10000,
	kMsgFirstAlignment = 10010,

	kMaxName = 15
};

// Results of "C)REATE A CHARACTER/I)NSPECT A CHARACTER/R)OSTER OF CHARACTERS/l)EAVE"
enum {
	kCreate, kInspect, kShowRoster, kLeaveTraining
};

namespace {

class EngineDice : public Dice {
public:
	uint roll(uint count) override {
		return g_engine->getRandomNumber(count - 1);
	}
};

} // End of anonymous namespace

bool TrainingView::msgFocus(const FocusMessage &msg) {
	// Coming back from the character sheet goes on with the same character
	_state = (msg._priorView && msg._priorView == g_events->findView("Town.Sheet")) ? kEditMenu : kMenu;
	return TownView::msgFocus(msg);
}

void TrainingView::noticeDone() {
	// After a wrong password
	if (_state == kEditPassword)
		_state = kMenu;
}

void TrainingView::startEdit() {
	_listed.clear();
	for (uint i = 0; i < _G(party).rosterSize(); ++i) {
		if (!_G(party).rosterEntry(i).isFree())
			_listed.push_back(i);
	}

	_state = kEditChoose;
}

void TrainingView::openClasses(bool *open) const {
	// Any class the attributes and alignment allow, but the present one
	const Character &c = _G(party).rosterEntry(_edited);
	for (int i = 0; i < kClassCount; ++i)
		open[i] = i != c.classType && Training::isEligible(i, c.attributes, c.alignment);
}

void TrainingView::drawBackground(TownScreens &screens) {
	screens.drawTrainingBackground();
}

bool TrainingView::anyClassOpen() const {
	for (int i = 0; i < kClassCount; ++i) {
		if (Training::isEligible(i, _attributes, _draft.alignment))
			return true;
	}

	return false;
}

void TrainingView::drawTown(TownScreens &screens) {
	const MessageFile &messages = _G(messages);

	switch (_state) {
	case kMenu:
		screens.drawTrainingMenu(_menu);
		return;
	case kRoster:
		screens.drawRoster();
		return;
	case kRosterFull:
		screens.drawTrainingMenu(_menu);
		screens.drawPopup(messages.get(kMsgRosterFull), 12);
		return;
	default:
		break;
	}

	if (_state >= kEditChoose) {
		const Character &c = _G(party).rosterEntry(_edited);
		Common::String marks;

		switch (_state) {
		case kEditChoose:
			screens.drawTrainingMenu(_menu);
			_menu = Menu();
			screens.drawTavernAdd(_listed);
			break;

		case kEditPassword:
			screens.drawTavernPassword(_listed, _letter, _marks);
			break;

		case kEditMenu:
			screens.drawCharacterHeader(c, 4);
			screens.drawEditMenu(_menu);
			break;

		case kEditDelete:
			screens.drawCharacterHeader(c, 4);
			screens.drawMainLine(10, substCaret(messages.get(kMsgSureYouWant), messages.get(kMsgDelete)), true);
			break;

		case kEditClass: {
			bool open[kClassCount];
			openClasses(open);
			screens.drawCharacterHeader(c, 4);
			screens.drawMainLine(7, messages.get(kMsgChangesLeave), true);
			screens.drawClassChoices(open);
			break;
		}

		case kEditNoClass:
			screens.drawCharacterHeader(c, 4);
			screens.drawNoClassOpen();
			break;

		default:
			// Changing the password, all in the main window
			screens.drawCharacterHeader(c, 4);
			screens.drawMainLine(8, messages.get(kMsgNewPassword), true);
			for (uint i = 0; i < _marks; ++i)
				marks += 'X';
			screens.drawMainLine(10, marks, false, 10);

			if (_state != kEditNewPassword) {
				screens.drawMainLine(12, messages.get(kMsgNewPasswordAgain), true);
				marks.clear();
				for (uint i = 0; i < _marks2; ++i)
					marks += 'X';
				screens.drawMainLine(14, marks, false, 10);
			}

			if (_state == kEditPasswordResult) {
				if (_passwordChanged) {
					screens.drawMainLine(16, messages.get(kMsgPasswordChanged), true);
				} else {
					screens.drawMainLine(16, messages.get(kMsgNotTheSame1), false);
					screens.drawMainLine(17, messages.get(kMsgNotTheSame2), false);
				}
			}
			break;
		}
		return;
	}

	// The rest is the making of a character
	screens.drawCharacterHeader(_draft);

	switch (_state) {
	case kName:
	case kNameInUse:
		screens.drawTrainingPrompt(messages.get(kMsgEnterName));
		screens.drawTrainingInput(_input);
		if (_state == kNameInUse)
			screens.drawPopup(messages.get(kMsgNameInUse), 14);
		break;

	case kPassword:
	case kPasswordAgain: {
		screens.drawTrainingPrompt(messages.get(_state == kPassword ? kMsgEnterPassword : kMsgEnterAgain));
		// What is typed is not shown, only a mark or two for each character
		Common::String marks;
		for (uint i = 0; i < _marks; ++i)
			marks += 'X';
		screens.drawTrainingInput(marks);
		break;
	}

	case kRace:
		screens.drawTrainingPrompt(messages.get(kMsgChooseRace));
		screens.drawTrainingChoices(kMsgFirstRace, 5);
		break;

	case kAlignment:
		screens.drawTrainingPrompt(messages.get(kMsgChooseAlignment));
		screens.drawTrainingChoices(kMsgFirstAlignment, 3);
		break;

	case kPoints:
	case kClass:
	case kKeep: {
		bool eligible[kClassCount];
		for (int i = 0; i < kClassCount; ++i)
			eligible[i] = Training::isEligible(i, _attributes, _draft.alignment);
		screens.drawTrainingPoints(_attributes, _bonus, _cursor, eligible, _state == kPoints);

		// The prompt takes the place of the two lines of help
		if (_state == kClass)
			screens.drawTrainingPrompt(messages.get(kMsgSelectClass));
		else if (_state == kKeep)
			screens.drawTrainingPrompt(messages.get(kMsgKeepCharacter));
		break;
	}

	default:
		break;
	}
}

void TrainingView::startCreate() {
	_slot = _G(party).freeSlot();
	if (_slot < 0) {
		_state = kRosterFull;
		return;
	}

	EngineDice dice;
	Training::newCharacter(_draft, dice);
	_input.clear();
	_state = kName;
}

void TrainingView::typeInto(uint16 ascii, bool hidden) {
	if (ascii == kKeyBackspace) {
		if (!_input.empty() && !hidden)
			_input.deleteLastChar();
	} else if (ascii == kKeyEscape) {
		if (!hidden)
			_input.clear();
	} else if (ascii >= ' ' && ascii < 127 && _input.size() < kMaxName) {
		_input += (char)ascii;
		// One or two marks for each character of a password
		if (hidden)
			_marks += 1 + g_engine->getRandomNumber(1);
	}
}

void TrainingView::adjust(int by) {
	const int value = _attributes[_cursor] + by;

	// Not below what the race starts with, not above 18, and only
	// with points that are there
	if (value < Training::raceBase(_draft.race)[_cursor] || value > 18 || _bonus - by < 0)
		return;

	_attributes[_cursor] = value;
	_bonus -= by;
}

void TrainingView::choose(int result, uint16 ascii) {
	Party &party = _G(party);
	const MessageFile &messages = _G(messages);

	switch (_state) {
	case kMenu:
		switch (result) {
		case kCreate:
			startCreate();
			break;
		case kInspect:
			startEdit();
			break;
		case kShowRoster:
			_state = kRoster;
			break;
		case kLeaveTraining:
			// Back to the castle, which lies below this view
			close();
			return;
		default:
			return;
		}
		break;

	case kRoster:
	case kRosterFull:
		if (ascii != kKeyReturn)
			return;
		_state = kMenu;
		break;

	case kName:
		if (ascii != kKeyReturn) {
			typeInto(ascii, false);
			break;
		}
		// No name: no character
		if (_input.empty()) {
			_state = kMenu;
		} else if (party.nameInUse(_input)) {
			_state = kNameInUse;
		} else {
			_draft.name = _input;
			_input.clear();
			_marks = 0;
			_state = kPassword;
		}
		break;

	case kNameInUse:
		if (ascii != kKeyReturn)
			return;
		_state = kName;
		break;

	case kPassword:
		if (ascii != kKeyReturn) {
			typeInto(ascii, true);
			break;
		}
		_password = _input;
		_input.clear();
		_marks = 0;
		// No password needs no second typing
		_state = _password.empty() ? kRace : kPasswordAgain;
		break;

	case kPasswordAgain:
		if (ascii != kKeyReturn) {
			typeInto(ascii, true);
			break;
		}
		// Not the same: start again with the password
		_state = (_input == _password) ? kRace : kPassword;
		_draft.password = _password;
		_input.clear();
		_marks = 0;
		break;

	case kRace:
		if (ascii < 'A' || ascii > 'E')
			return;
		_draft.race = kRaceHuman + (ascii - 'A');
		_state = kAlignment;
		break;

	case kAlignment: {
		if (ascii < 'A' || ascii > 'C')
			return;
		_draft.alignment = kAlignGood + (ascii - 'A');

		EngineDice dice;
		const int16 *base = Training::raceBase(_draft.race);
		for (int a = 0; a < kAttributeCount; ++a)
			_attributes[a] = base[a];
		_bonus = Training::rollBonus(dice);
		_cursor = 0;
		_state = kPoints;
		break;
	}

	case kPoints:
		if (ascii == kKeyReturn) {
			_cursor = (_cursor + 1) % kAttributeCount;
		} else if (ascii == kKeyBackspace) {
			_cursor = (_cursor + kAttributeCount - 1) % kAttributeCount;
		} else if (ascii == kKeyUp || ascii == '+' || ascii == '=') {
			adjust(1);
		} else if (ascii == kKeyDown || ascii == '-') {
			adjust(-1);
		} else if (ascii == 'C' || ascii == kKeyEscape) {
			// All points must be spent and some class must be open
			if (_bonus != 0 || !anyClassOpen())
				return;
			_state = kClass;
		} else {
			return;
		}
		break;

	case kClass:
		// The original takes the letters A to F here, so a lord or a
		// ninja cannot be made even when the list offers one
		if (ascii < 'A' || ascii > 'F' || !Training::isEligible(ascii - 'A', _attributes, _draft.alignment))
			return;
		_class = ascii - 'A';
		_draft.classType = _class;
		_state = kKeep;
		break;

	case kEditChoose: {
		if (ascii == kKeyReturn) {
			_state = kMenu;
			break;
		}
		if (ascii < 'A' || ascii >= 'A' + (int)_listed.size())
			return;

		_letter = ascii - 'A';
		_edited = _listed[_letter];
		if (party.rosterEntry(_edited).password.empty()) {
			_state = kEditMenu;
		} else {
			_input.clear();
			_marks = 0;
			_state = kEditPassword;
		}
		break;
	}

	case kEditPassword:
		if (ascii != kKeyReturn) {
			typeInto(ascii, true);
			break;
		}
		if (_input == party.rosterEntry(_edited).password) {
			_state = kEditMenu;
		} else {
			notice(messages.get(kMsgIncorrectPassword));
			return;
		}
		break;

	case kEditMenu:
		// Results of "I)NSPECT/D)ELETE/C)HANGE CLASS/A)LTER PASSWORD/l)EAVE"
		switch (result) {
		case 0:
			SheetView::open(_edited, SheetView::kFromTraining);
			return;
		case 1:
			_state = kEditDelete;
			break;
		case 2: {
			bool open[kClassCount], any = false;
			openClasses(open);
			for (int i = 0; i < kClassCount; ++i)
				any |= open[i];
			_state = any ? kEditClass : kEditNoClass;
			break;
		}
		case 3:
			_input.clear();
			_marks = _marks2 = 0;
			_state = kEditNewPassword;
			break;
		case 4:
			_state = kMenu;
			break;
		default:
			return;
		}
		break;

	case kEditDelete:
		if (ascii == (byte)messages.get(kMsgEditYes)[0]) {
			party.deleteCharacter(_edited);
			_state = kMenu;
		} else if (ascii == (byte)messages.get(kMsgEditNo)[0]) {
			_state = kEditMenu;
		} else {
			return;
		}
		break;

	case kEditClass: {
		if (ascii == kKeyReturn) {
			_state = kEditMenu;
			break;
		}

		bool open[kClassCount];
		openClasses(open);
		if (ascii < 'A' || ascii > 'H' || !open[ascii - 'A'])
			return;

		// A new class is a new start: level 1 and the attributes of the race
		EngineDice dice;
		Training::setClass(party.rosterEntry(_edited), ascii - 'A', dice);
		_state = kEditMenu;
		break;
	}

	case kEditNoClass:
	case kEditPasswordResult:
		if (ascii != kKeyReturn)
			return;
		_state = kEditMenu;
		break;

	case kEditNewPassword:
		if (ascii != kKeyReturn) {
			typeInto(ascii, true);
			break;
		}
		_password = _input;
		_input.clear();
		_marks2 = 0;
		_state = kEditNewPasswordAgain;
		break;

	case kEditNewPasswordAgain:
		if (ascii != kKeyReturn) {
			// typeInto() counts into _marks; keep the first row as it is
			const uint first = _marks;
			_marks = _marks2;
			typeInto(ascii, true);
			_marks2 = _marks;
			_marks = first;
			break;
		}
		_passwordChanged = (_input == _password);
		if (_passwordChanged)
			party.rosterEntry(_edited).password = _password;
		_input.clear();
		_state = kEditPasswordResult;
		break;

	case kKeep:
		if (ascii == (byte)messages.get(kMsgYes)[0]) {
			EngineDice dice;
			Training::finish(_draft, _class, _attributes, dice);
			party.rosterEntry(_slot) = _draft;
		} else if (ascii != (byte)messages.get(kMsgNo)[0]) {
			return;
		}
		_state = kMenu;
		break;

	default:
		return;
	}

	redraw();
}

} // End of namespace Town
} // End of namespace Wizardry
