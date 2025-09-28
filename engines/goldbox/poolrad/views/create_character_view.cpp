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
#include "common/file.h"
#include "common/savefile.h"
#include "graphics/palette.h"
//#include "goldbox/keymapping.h"
#include "goldbox/vm_interface.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/create_character_view.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/views/dialogs/horizontal_input.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

const char PICK_RACE[] = "Pick Race";
const char PICK_GENDER[] = "Pick Gender";
const char PICK_CLASS[] = "Pick Class";
const char PICK_ALIGNMENT[] = "Pick Alignment";
const char CHAR_NAME[] = "Character name: ";


CreateCharacterView::CreateCharacterView() : View("CreatCharacter"), _stage(CC_STATE_RACE) {
	// Initialize character under construction
	_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
	// Build initial race selection menu immediately
	chooseRace();
}

CreateCharacterView::~CreateCharacterView() {
	// Clean up any created subviews
	delete _menu; _menu = nullptr;
	delete _nameInput; _nameInput = nullptr;
	delete _profileDialog; _profileDialog = nullptr;
	delete _menuItems; _menuItems = nullptr;
	_activeSubView = nullptr;
	delete _newCharacter; _newCharacter = nullptr;
}

void CreateCharacterView::nextStage() {
	if (_stage < CC_STATE_DONE)
		setStage(static_cast<CharacterCreateState>(_stage + 1));
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
		// First time entering profile: roll initial stats
		if (!_hasRolled && _newCharacter) {
			rollAndRecompute();
			_hasRolled = true;
			if (_profileDialog)
				_profileDialog->draw();
		}
		break;
	case CC_STATE_NAME:
		chooseName();
		break;
	case CC_STATE_ICON:
		// showIconSelector();
		break;
	case CC_STATE_DONE:
		// Finalize character creation
		break;
	}
}

void CreateCharacterView::draw() {
	Surface s = getSurface();

	drawWindow( 1, 1, 38, 22);
	// Delegate drawing to the active subview if present
	if (_activeSubView) {
		_activeSubView->draw();
	}
}

bool CreateCharacterView::msgFocus(const FocusMessage &msg) {
	// When (re)entering this view, ensure a clean initial state if we're beyond first stage
	if (_stage != CC_STATE_RACE) {
		resetState();
	}
	return true;
}

bool CreateCharacterView::msgUnfocus(const UnfocusMessage &msg) {
	return true;
}

void CreateCharacterView::timeout() {
}

bool CreateCharacterView::msgKeypress(const KeypressMessage &msg) {
	// Global immediate exit on Escape from any stage (except DONE which already returns)
	if (msg.keycode == Common::KEYCODE_ESCAPE) {
		replaceView("Mainmenu");
		return true;
	}
	// Only intercept profile specific actions; otherwise let active dialog handle
	if (_stage == CC_STATE_PROFILE) {
		if (msg.keycode == Common::KEYCODE_r || msg.ascii == 'R') {
			rollAndRecompute();
			if (_profileDialog)
				_profileDialog->draw();
			return true;
		}
		if (msg.keycode == Common::KEYCODE_RETURN) {
			if (_confirmSave) {
				finalizeCharacterAndSave();
				setStage(CC_STATE_DONE);
			} else {
				setStage(CC_STATE_NAME);
			}
			return true;
		}
	}
	return View::msgKeypress(msg);
}

void CreateCharacterView::chooseRace() {
	if (_menuItems) { delete _menuItems; }
	_menuItems = new Goldbox::MenuItemList();
	// Race id 6 (half-orc) currently not playable -> exclude it
	static const int raceIndices[] = {1, 2, 3, 4, 5, 7};
	_indexMap.clear();
	for (int i = 0; i < 6; ++i)
		_indexMap.push_back(raceIndices[i]);
	Dialogs::VerticalMenu::fillMenuItemsFromYml(_menuItems, "stats.races", raceIndices, 6);
	buildAndShowMenu(PICK_RACE);
}

void CreateCharacterView::chooseGender() {
	if (_menuItems) { delete _menuItems; }
	_menuItems = new Goldbox::MenuItemList();
	static const int genderIndices[] = {0, 1};
	_indexMap.clear();
	for (int i = 0; i < 2; ++i) _indexMap.push_back(genderIndices[i]);
	Dialogs::VerticalMenu::fillMenuItemsFromYml(_menuItems, "stats.gender", genderIndices, 2);
	buildAndShowMenu(PICK_GENDER);
}

void CreateCharacterView::chooseClass() {
	if (_menuItems) { delete _menuItems; }
	_menuItems = new Goldbox::MenuItemList();
	// Build class list filtered by race using Rules
	_indexMap.clear();
	Common::Array<int> allowed;
	uint8 raceId = 0;
	if (_newCharacter) raceId = _newCharacter->race; // already mapped
	const uint8 clsCount = Goldbox::Data::Rules::classEnumCount();
	for (int cid = 0; cid < clsCount; ++cid) {
		if (Goldbox::Data::Rules::isClassAllowed(raceId, (uint8)cid))
			allowed.push_back(cid);
	}
	if (allowed.empty()) {
		// Fallback: show all classes to avoid dead-end if tables are placeholders
		for (int cid = 0; cid < clsCount; ++cid) allowed.push_back(cid);
	}
	for (uint i = 0; i < allowed.size(); ++i) _indexMap.push_back(allowed[i]);
	// Prepare C-array for helper
	int *indices = nullptr;
	if (!allowed.empty()) {
		indices = new int[allowed.size()];
		for (uint i = 0; i < allowed.size(); ++i) indices[i] = allowed[i];
		Dialogs::VerticalMenu::fillMenuItemsFromYml(_menuItems, "stats.classes", indices, (int)allowed.size());
		delete[] indices;
	}
	buildAndShowMenu(PICK_CLASS);
}

void CreateCharacterView::chooseAlignment() {
	if (_menuItems) { delete _menuItems; }
	_menuItems = new Goldbox::MenuItemList();
	// Filter alignments based on selected class using Rules
	_indexMap.clear();
	Common::Array<int> allowed;
	uint8 classId = 0;
	if (_newCharacter) classId = _newCharacter->classType;
	const uint8 alignCount = Goldbox::Data::Rules::alignmentEnumCount();
	for (int aid = 0; aid < alignCount; ++aid) {
		if (Goldbox::Data::Rules::isAlignmentAllowed(classId, (uint8)aid))
			allowed.push_back(aid);
	}
	if (allowed.empty()) {
		for (int aid = 0; aid < alignCount; ++aid) allowed.push_back(aid);
	}
	for (uint i = 0; i < allowed.size(); ++i) _indexMap.push_back(allowed[i]);
	int *indices = nullptr;
	if (!allowed.empty()) {
		indices = new int[allowed.size()];
		for (uint i = 0; i < allowed.size(); ++i) indices[i] = allowed[i];
		Dialogs::VerticalMenu::fillMenuItemsFromYml(_menuItems, "stats.alignments", indices, (int)allowed.size());
		delete[] indices;
	}
	buildAndShowMenu(PICK_ALIGNMENT);
}

void CreateCharacterView::showProfileDialog() {
	// Note: Without a working temp character instance here, just show an empty profile safely
	if (_profileDialog) { delete _profileDialog; }
	_profileDialog = new Dialogs::CharacterProfile(_newCharacter, "CreateProfile");
	subView(_profileDialog);
	setActiveSubView(_profileDialog);
}

void CreateCharacterView::chooseName() {
	using namespace Goldbox::Poolrad::Views::Dialogs;
	if (_nameInput) { delete _nameInput; }
	HorizontalInputConfig cfg { CHAR_NAME, 15, 15 };
	_nameInput = new HorizontalInput("NameInput", cfg);
	subView(static_cast<Dialogs::Dialog *>(_nameInput));
	setActiveSubView(static_cast<Dialogs::Dialog *>(_nameInput));
}

void CreateCharacterView::buildAndShowMenu(const Common::String &topline) {
	Common::Array<Common::String> promptOptions = {"Exit"};
    Dialogs::VerticalMenuConfig menuConfig = {
        "",                  // promptTxt
        promptOptions,       // promptOptions
        _menuItems,          // menuItemList (initialized later)
        13,                  // headColor
        10,                  // textColor
        15,                  // selectColor
        1, 2, 38, 22,        // bounds
		topline,	         // title
        false                // asHeader
    };
	if (_menu) {
		_menu->rebuild(_menuItems, topline);
		setActiveSubView(_menu);
	} else {
		_menu = new Dialogs::VerticalMenu("CreateCharMenu", menuConfig);
		subView(_menu);
		setActiveSubView(_menu);
	}
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
	if (!success) {
		replaceView("Mainmenu");
		return;
	}

	switch (_stage) {
	case CC_STATE_RACE:
		if (key == Common::KEYCODE_ESCAPE) {
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			debug("Selected race menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			// map visible index to race enum value (1..7 per strings mapping)
			if (value >= 0 && value < (int)_indexMap.size())
				_newCharacter->race = (uint8)_indexMap[value];
				debug("Selected race ID: %d", _newCharacter->race);
			nextStage();
		}
		break;
	case CC_STATE_GENDER:
		if (key == Common::KEYCODE_ESCAPE) {
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			_selectedGender = value;
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (_selectedGender >= 0 && _selectedGender < (int)_indexMap.size())
				_newCharacter->gender = (Goldbox::Data::Gender)_indexMap[_selectedGender];
			nextStage();
		}
		break;
	case CC_STATE_CLASS:
		if (key == Common::KEYCODE_ESCAPE) {
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			_selectedClass = value;
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (_selectedClass >= 0 && _selectedClass < (int)_indexMap.size())
				_newCharacter->classType = (uint8)_indexMap[_selectedClass];
			nextStage();
		}
		break;
	case CC_STATE_ALIGNMENT:
		if (key == Common::KEYCODE_ESCAPE) {
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			_selectedAlignment = value;
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (_selectedAlignment >= 0 && _selectedAlignment < (int)_indexMap.size())
				_newCharacter->alignment = (uint8)_indexMap[_selectedAlignment];
			// proceed to profile where stats are rolled and can be accepted/rerolled
			_hasRolled = false;
			_confirmSave = false;
			setStage(CC_STATE_PROFILE);
		}
		break;
	case CC_STATE_PROFILE:
		// In profile view:
		// - ENTER accepts current stats -> go to NAME
		// - 'r' or 'R' rerolls stats
		// - ESC goes back to alignment selection
		if (key == Common::KEYCODE_ESCAPE) {
			setStage(CC_STATE_ALIGNMENT);
		} else if (key == Common::KEYCODE_RETURN) {
			setStage(CC_STATE_NAME);
		} else if (key == Common::KEYCODE_r) {
			rollAndRecompute();
			if (_profileDialog)
				_profileDialog->draw();
		}
		break;
	case CC_STATE_NAME:
		if (key == Common::KEYCODE_ESCAPE) {
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			// finalize name from input dialog and proceed to icon
			Dialogs::HorizontalInput *hi = dynamic_cast<Dialogs::HorizontalInput *>(_nameInput);
			if (hi) {
				_enteredName = hi->getInput();
				if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
				_newCharacter->name = _enteredName;
				_newCharacter->finalizeName();
			}
			setStage(CC_STATE_ICON);
		}
		break;
	case CC_STATE_ICON:
		// Not implemented yet: when leaving icon editor, confirm save and return to PROFILE to confirm
		// For now, simulate icon customization complete and ask to save
		_confirmSave = true;
		setStage(CC_STATE_PROFILE);
		break;
	case CC_STATE_DONE:
		replaceView("Mainmenu");
		break;
	}
}

void CreateCharacterView::resetState() {
	// Delete subviews except persistent _menu (will rebuild)
	if (_profileDialog) { delete _profileDialog; _profileDialog = nullptr; }
	if (_nameInput) { delete _nameInput; _nameInput = nullptr; }
	// Reset character
	if (_newCharacter) { delete _newCharacter; }
	_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
	// Clear selections and flags
	_selectedRace = _selectedGender = _selectedClass = _selectedAlignment = -1;
	_enteredName.clear();
	_indexMap.clear();
	_confirmSave = false;
	_hasRolled = false;
	_stage = CC_STATE_RACE;
	// Rebuild initial race menu
	if (_menuItems) { delete _menuItems; _menuItems = nullptr; }
	chooseRace();
}

Common::String CreateCharacterView::formatBaseFilename(const Common::String &name) {
	Common::String formatted;
	for (uint i = 0; i < name.size() && formatted.size() < 8; ++i) {
		if (name[i] != ' ')
			formatted += name[i];
	}
	return formatted;
}

void CreateCharacterView::appendLineToTextFile(const Common::String &fileName, const Common::String &line) {
	// Append by reading existing content and writing back with new line at end
	Common::DumpFile df;
	if (!df.open(fileName.c_str(), true)) { // createPath=true
		warning("Failed to open %s for append", fileName.c_str());
		return;
	}
	// Ensure trailing newline
	Common::String out = line;
	out += "\n";
	df.write(out.c_str(), out.size());
	df.flush();
	df.close();
}

void CreateCharacterView::finalizeCharacterAndSave() {
	if (!_newCharacter)
		return;

	// Roll base stats and compute initial values
	_newCharacter->rollAbilityScores();
	_newCharacter->applyRacialAdjustments();
	// Start as level 1 in chosen class
	if (_newCharacter->classType < _newCharacter->levels.levels.size())
		_newCharacter->levels.levels[_newCharacter->classType] = 1;
	_newCharacter->calculateHitPoints();

	// Save .CHA
	Common::String base = formatBaseFilename(_newCharacter->name);
	Common::String chrFile = base + ".CHA";
	Common::DumpFile out;
	if (out.open(chrFile.c_str(), true)) {
		_newCharacter->save(out);
		out.flush();
		out.close();
	} else {
		warning("Failed to create %s", chrFile.c_str());
	}

	// Create empty .ITM and .SPC via inventory/effects save
	_newCharacter->inventory.save(base + ".ITM");
	_newCharacter->effects.save(base + ".SPC");
	appendLineToTextFile("CHARLIST.TXT", _newCharacter->name);
}

void CreateCharacterView::rollAndRecompute() {
	if (!_newCharacter)
		return;
	_newCharacter->rollAbilityScores();
	_newCharacter->applyRacialAdjustments();
	for (uint i = 0; i < _newCharacter->levels.levels.size(); ++i)
		_newCharacter->levels.levels[i] = 0;
	if (_newCharacter->classType < _newCharacter->levels.levels.size())
		_newCharacter->levels.levels[_newCharacter->classType] = 1;
	_newCharacter->calculateHitPoints();
}
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
