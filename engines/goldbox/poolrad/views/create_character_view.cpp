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

#include "common/system.h"
#include "graphics/palette.h"
//#include "goldbox/keymapping.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/create_character_view.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"



namespace Goldbox {
namespace Poolrad {
namespace Views {

const char PICK_RACE[] = "Pick Race";
const char PICK_GENDER[] = "Pick Gender";
const char PICK_CLASS[] = "Pick Class";
const char PICK_ALIGNMENT[] = "Pick Alignment";
const char KEEP_THIS[] = "Keep this character? ";
const char CHAR_NAME[] = "Character name: ";


CreateCharacterView::CreateCharacterView() : View("CreatCharacter"), _stage(CC_STATE_RACE) {
	// Optionally, start the first stage immediately
	// chooseRace();
}
CreateCharacterView::~CreateCharacterView() {
	// Clean up any created subviews
	delete _raceMenu; _raceMenu = nullptr;
	delete _genderMenu; _genderMenu = nullptr;
	delete _classMenu; _classMenu = nullptr;
	delete _alignmentMenu; _alignmentMenu = nullptr;
	delete _profileDialog; _profileDialog = nullptr;
	delete _raceMenuItems; _raceMenuItems = nullptr;
	delete _genderMenuItems; _genderMenuItems = nullptr;
	delete _classMenuItems; _classMenuItems = nullptr;
	delete _alignmentMenuItems; _alignmentMenuItems = nullptr;
	_activeSubView = nullptr;
}
void CreateCharacterView::nextStage() {
	if (_stage < CC_STATE_DONE)
		setStage(static_cast<CharacterCreateState>(_stage + 1));
}

void CreateCharacterView::prevStage() {
	if (_stage > CC_STATE_RACE)
		setStage(static_cast<CharacterCreateState>(_stage - 1));
}

void CreateCharacterView::setStage(CharacterCreateState stage) {
	_stage = stage;
	// Show the appropriate dialog/view for the new stage
	switch (_stage) {
	case CC_STATE_RACE:
		chooseRace();
		break;
	case CC_STATE_GENDER:
		chooseGender();
		break;
	case CC_STATE_CLASS:
		chooseClass();
		break;
	case CC_STATE_ALIGNMENT:
		chooseAlignment();
		break;
	case CC_STATE_PROFILE:
		showProfileDialog();
		break;
	case CC_STATE_ICON:
		// showIconSelector();
		break;
	case CC_STATE_DONE:
		// Finalize character creation
		break;
	}
}

// Intentionally no msgKeypress override: subviews handle inputs

void CreateCharacterView::draw() {
	Surface s = getSurface();

    int rnd = getRandomNumber(12);

	drawWindow( 1, 1, 38, 22);
	// Delegate drawing to the active subview if present
	if (_activeSubView) {
		_activeSubView->draw();
	}
}

bool CreateCharacterView::msgFocus(const FocusMessage &msg) {
	return true;
}

bool CreateCharacterView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void CreateCharacterView::timeout() {
}

void CreateCharacterView::chooseRace() {
	// Define race options (example: Human, Dwarf, Elf, Gnome, Halfling, Half-Elf, Half-Orc, Monster)
	static const char *raceNames[] = {
		"Human", "Dwarf", "Elf", "Gnome", "Halfling", "Half-Elf", "Half-Orc", "Monster"
	};
	if (_raceMenuItems) { delete _raceMenuItems; }
	_raceMenuItems = new Goldbox::MenuItemList();
	for (int i = 0; i < 8; ++i) {
		_raceMenuItems->push_back(raceNames[i]);
	}

	Goldbox::Poolrad::Views::Dialogs::VerticalMenuConfig config = {
		"Pick Race", // promptTxt
		Common::Array<Common::String>(), // promptOptions
	_raceMenuItems, // menuItemList
		15, // headColor
		7,  // textColor
		14, // selectColor
		5,  // xStart
		5,  // yStart
		30, // xEnd
		15, // yEnd
		false // isAddExit
	};

	if (_raceMenu) {
		delete _raceMenu;
	}
	_raceMenu = new Goldbox::Poolrad::Views::Dialogs::VerticalMenu("RaceMenu", config);
	this->subView(_raceMenu);
	setActiveSubView(_raceMenu);
}

void CreateCharacterView::chooseGender() {
	static const char *genderNames[] = {"Male", "Female"};
	if (_genderMenuItems) { delete _genderMenuItems; }
	_genderMenuItems = new Goldbox::MenuItemList();
	for (int i = 0; i < 2; ++i) _genderMenuItems->push_back(genderNames[i]);

	Dialogs::VerticalMenuConfig config = {
		"Pick Gender",
		Common::Array<Common::String>(),
	_genderMenuItems,
		15,
		7,
		14,
		5,
		5,
		30,
		15,
		false
	};

	if (_genderMenu) {
		delete _genderMenu;
	}
	_genderMenu = new Dialogs::VerticalMenu("GenderMenu", config);
	subView(_genderMenu);
	setActiveSubView(_genderMenu);
}

void CreateCharacterView::chooseClass() {
	// Placeholder classes list; real list may depend on race/gender
	static const char *classNames[] = {"Fighter", "Mage", "Cleric", "Thief"};
	if (_classMenuItems) { delete _classMenuItems; }
	_classMenuItems = new Goldbox::MenuItemList();
	for (int i = 0; i < 4; ++i) _classMenuItems->push_back(classNames[i]);

	Dialogs::VerticalMenuConfig config = {
		"Pick Class",
		Common::Array<Common::String>(),
	_classMenuItems,
		15, 7, 14,
		5, 5, 30, 15,
		false
	};

	if (_classMenu) delete _classMenu;
	_classMenu = new Dialogs::VerticalMenu("ClassMenu", config);
	subView(_classMenu);
	setActiveSubView(_classMenu);
}

void CreateCharacterView::chooseAlignment() {
	static const char *alignmentNames[] = {
		"Lawful Good", "Neutral Good", "Chaotic Good",
		"Lawful Neutral", "True Neutral", "Chaotic Neutral",
		"Lawful Evil", "Neutral Evil", "Chaotic Evil"
	};
	if (_alignmentMenuItems) { delete _alignmentMenuItems; }
	_alignmentMenuItems = new Goldbox::MenuItemList();
	for (int i = 0; i < 9; ++i) _alignmentMenuItems->push_back(alignmentNames[i]);

	Dialogs::VerticalMenuConfig config = {
		"Pick Alignment",
		Common::Array<Common::String>(),
	_alignmentMenuItems,
		15, 7, 14,
		3, 3, 35, 20,
		false
	};

	if (_alignmentMenu) delete _alignmentMenu;
	_alignmentMenu = new Dialogs::VerticalMenu("AlignmentMenu", config);
	subView(_alignmentMenu);
	setActiveSubView(_alignmentMenu);
}

void CreateCharacterView::showProfileDialog() {
	// Note: Without a working temp character instance here, just show an empty profile safely
	if (_profileDialog) { delete _profileDialog; }
	_profileDialog = new Dialogs::CharacterProfile(nullptr, "CreateProfile");
	subView(_profileDialog);
	setActiveSubView(_profileDialog);
}

void CreateCharacterView::setActiveSubView(Dialogs::Dialog *dlg) {
	if (_activeSubView && _activeSubView != dlg) {
		_activeSubView->deactivate();
	}
	_activeSubView = dlg;
	if (_activeSubView)
		_activeSubView->activate();
}

void CreateCharacterView::handleMenuResult(bool success, Common::KeyCode key, short value) {
	// Called by subviews (VerticalMenu) to bubble selection to parent
	if (!success) return;

	switch (_stage) {
	case CC_STATE_RACE:
		if (key == Common::KEYCODE_ESCAPE) {
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			_selectedRace = value;
			nextStage();
		}
		break;
	case CC_STATE_GENDER:
		if (key == Common::KEYCODE_ESCAPE) {
			prevStage();
		} else if (key == Common::KEYCODE_RETURN) {
			_selectedGender = value;
			nextStage();
		}
		break;
	case CC_STATE_CLASS:
		if (key == Common::KEYCODE_ESCAPE) {
			prevStage();
		} else if (key == Common::KEYCODE_RETURN) {
			_selectedClass = value;
			nextStage();
		}
		break;
	case CC_STATE_ALIGNMENT:
		if (key == Common::KEYCODE_ESCAPE) {
			prevStage();
		} else if (key == Common::KEYCODE_RETURN) {
			_selectedAlignment = value;
			nextStage();
		}
		break;
	case CC_STATE_PROFILE:
		// Profile dialog likely non-interactive at this moment; proceed to done
		nextStage();
		break;
	case CC_STATE_ICON:
		// Not implemented yet
		nextStage();
		break;
	case CC_STATE_DONE:
		replaceView("Mainmenu");
		break;
	}
}
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
