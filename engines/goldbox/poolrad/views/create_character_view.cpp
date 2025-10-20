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
	_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
	_newCharacter->initializeNewCharacter();
	chooseRace();
}

CreateCharacterView::~CreateCharacterView() {
	// Clean up any created subviews ensuring they are detached from the
	// parent's _children list to prevent stale pointers during redraw.
	if (_activeSubView == _profileDialog) _activeSubView = nullptr;
	if (_activeSubView == _nameInput) _activeSubView = nullptr;
	if (_activeSubView == _menu) _activeSubView = nullptr;
	if (_profileDialog) { _profileDialog->setParent(nullptr); delete _profileDialog; _profileDialog = nullptr; }
	if (_nameInput) { _nameInput->setParent(nullptr); delete _nameInput; _nameInput = nullptr; }
	if (_menu) { _menu->setParent(nullptr); delete _menu; _menu = nullptr; }
	// _menuItems is not a UIElement
	delete _menuItems; _menuItems = nullptr;
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
		// Proactively reset internal state so that re-opening the view does not
		// reference stale dialogs or deleted objects.
		resetState();
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
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			resetState();
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			debug("Selected race menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->race = (uint8)_indexMap[value];
				debug("Selected race ID: %d", _newCharacter->race);
				if (_newCharacter) {
					switch (_newCharacter->race) {
					case Goldbox::Data::R_DWARF:
						_newCharacter->iconDimension = 1;
						_newCharacter->setEffect(90, 0, 0xFF, false);
						_newCharacter->setEffect(97, 0, 0xFF, false);
						_newCharacter->setEffect(26, 0, 0xFF, false);
						_newCharacter->setEffect(47, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_ELF:
						_newCharacter->iconDimension = 2;
						_newCharacter->setEffect(107, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_GNOME:
						_newCharacter->iconDimension = 1;
						_newCharacter->setEffect(97, 0, 0xFF, false);
						_newCharacter->setEffect(18, 0, 0xFF, false);
						_newCharacter->setEffect(47, 0, 0xFF, false);
						_newCharacter->setEffect(48, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_HALF_ELF:
						_newCharacter->iconDimension = 2;
						_newCharacter->setEffect(124, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_HALFLING:
						_newCharacter->iconDimension = 1;
						_newCharacter->setEffect(90, 0, 0xFF, false);
						_newCharacter->setEffect(97, 0, 0xFF, false);
						break;
					default: // Monster / Human / Half-Orc or any other
						_newCharacter->iconDimension = 2;
						break;
					}
				}
				nextStage();
			}
		}
		break;
	case CC_STATE_GENDER:
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			resetState();
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			debug("Selected gender menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->gender = (Goldbox::Data::Gender)_indexMap[value];
				debug("Selected gender ID: %d", _newCharacter->gender);
				nextStage();
			}
		}
		break;
	case CC_STATE_CLASS:
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			resetState();
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			debug("Selected class menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->classType = (uint8)_indexMap[value];
				debug("Selected class ID: %d", _newCharacter->classType);
				_newCharacter->highestLevel = 1;
				switch (_newCharacter->classType) {
				case Goldbox::Data::C_CLERIC_FIGHTER:
					_newCharacter->levels.levels[Goldbox::Data::C_CLERIC] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_FIGHTER] = 1;
					break;
				case Goldbox::Data::C_CLERIC_FIGHTER_MAGICUSER:
					_newCharacter->levels.levels[Goldbox::Data::C_CLERIC] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_FIGHTER] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_MAGICUSER] = 1;
					break;
				case Goldbox::Data::C_CLERIC_RANGER:
					_newCharacter->levels.levels[Goldbox::Data::C_CLERIC] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_RANGER] = 1;
					break;
				case Goldbox::Data::C_CLERIC_MAGICUSER:
					_newCharacter->levels.levels[Goldbox::Data::C_CLERIC] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_MAGICUSER] = 1;
					break;
				case Goldbox::Data::C_CLERIC_THIEF:
					_newCharacter->levels.levels[Goldbox::Data::C_CLERIC] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_THIEF] = 1;
					break;
				case Goldbox::Data::C_FIGHTER_MAGICUSER:
					_newCharacter->levels.levels[Goldbox::Data::C_FIGHTER] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_MAGICUSER] = 1;
					break;
				case Goldbox::Data::C_FIGHTER_THIEF:
					_newCharacter->levels.levels[Goldbox::Data::C_FIGHTER] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_THIEF] = 1;
					break;
				case Goldbox::Data::C_FIGHTER_MAGICUSER_THIEF:
					_newCharacter->levels.levels[Goldbox::Data::C_FIGHTER] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_MAGICUSER] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_THIEF] = 1;
					break;
				case Goldbox::Data::C_MAGICUSER_THIEF:
					_newCharacter->levels.levels[Goldbox::Data::C_MAGICUSER] = 1;
					_newCharacter->levels.levels[Goldbox::Data::C_THIEF] = 1;
					break;
				default:
					_newCharacter->levels.levels[_newCharacter->classType] = 1;
					break;
				}
			}
			if ( _newCharacter->levels.levels[Goldbox::Data::C_THIEF] > 0) {
				setThiefSkillsForNewCharacter();
			}
			setThac0();
			setSavingThrows();
			nextStage();
		}
		break;
	case CC_STATE_ALIGNMENT:
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			resetState();
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			debug("Selected alignment menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->alignment = (uint8)_indexMap[value];
				debug("Selected alignment ID: %d", _newCharacter->alignment);
				_hasRolled = false;
				_confirmSave = false;
			}
			setAge();
			nextStage();
		}
		break;
	case CC_STATE_PROFILE:
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			resetState();
			replaceView("Mainmenu");
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
			resetState();
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
	// Delete subviews safely: detach from parent so redraw traversal doesn't
	// see freed memory. Keep _menu to reuse, but if active, deactivate.
	if (_activeSubView == _profileDialog) _activeSubView = nullptr;
	if (_activeSubView == _nameInput) _activeSubView = nullptr;
	if (_profileDialog) { _profileDialog->setParent(nullptr); delete _profileDialog; _profileDialog = nullptr; }
	if (_nameInput) { _nameInput->setParent(nullptr); delete _nameInput; _nameInput = nullptr; }
	// Reset character
	if (_newCharacter) { delete _newCharacter; }
	_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
	_newCharacter->initializeNewCharacter();
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
	for (uint i = 0; i < _newCharacter->levels.levels.size(); ++i)
		_newCharacter->levels.levels[i] = 0;
	if (_newCharacter->classType < _newCharacter->levels.levels.size())
		_newCharacter->levels.levels[_newCharacter->classType] = 1;
	_newCharacter->calculateHitPoints();
}

void CreateCharacterView::setThiefSkillsForNewCharacter() {
    if (!_newCharacter)
        return;

    uint8 thiefLevel = 0;
    thiefLevel = _newCharacter->levels[Goldbox::Data::C_THIEF];
    const uint8 race = _newCharacter->race;
    const uint8 dexterity = _newCharacter->abilities.dexterity.current;
	debug("Thief skills for race %d, dex: %d, level: %d", race, dexterity, thiefLevel);
    // Compute final thief skills via rules
    _newCharacter->thiefSkills =
        Goldbox::Data::Rules::computeThiefSkills(race, dexterity, thiefLevel);
	debug("Thief skills set:\n pick locks: %d\n find-remove traps: %d\n stealth %d\n hide in shadow: %d\n hear noise: %d\n climb walls: %d\n read languages: %d\n",
		_newCharacter->thiefSkills.openLocks,
		_newCharacter->thiefSkills.findRemoveTraps,
		_newCharacter->thiefSkills.moveSilently,
		_newCharacter->thiefSkills.hideInShadows,
		_newCharacter->thiefSkills.hearNoise,
		_newCharacter->thiefSkills.climbWalls,
		_newCharacter->thiefSkills.readLanguages
	);
}

void CreateCharacterView::setThac0() {
	// Determine THAC0 base from the best (highest) among all base-class levels
	uint8 bestThac0 = 0;
	if (_newCharacter) {
		const Common::Array<uint8> &lvls = _newCharacter->levels.levels;
		for (uint i = 0; i < lvls.size(); ++i) {
			uint8 lvl = lvls[i];
			if (lvl > 0) {
				int v = Goldbox::Data::Rules::thac0AtLevel((uint8)i, lvl);
				if (v > bestThac0)
					bestThac0 = (uint8)v;
			}
		}
		_newCharacter->thac0.base = bestThac0;

		debug("Base raw THAC0: %d, actual: %d", _newCharacter->thac0.base, 60 - _newCharacter->thac0.base);

		// Set item-limit bitfield: OR of bits for all base classes with level > 0
		_newCharacter->itemsLimit =
			Goldbox::Data::Rules::computeItemLimitMask(_newCharacter->levels.levels);
		debug("Item limit bitmask value: %d", _newCharacter->itemsLimit);
	}
}

// Helper to clamp an int to uint8 range
static inline uint8 clampU8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : (uint8)v); }

void CreateCharacterView::setSavingThrows() {
	if (!_newCharacter)
		return;

	using namespace Goldbox::Data;
	using Goldbox::Data::Rules::savingThrowsAt;

	// 1) Initialize to worst values (higher is worse)
	SavingThrows best;
	best.vsParalysis     = 20;
	best.vsPetrification = 20;
	best.vsRodStaffWand  = 20;
	best.vsBreathWeapon  = 20;
	best.vsSpell         = 20;

	// 2) For each base class 0..7 with level > 0, keep the minimum per category
	const Common::Array<uint8> &lvls = _newCharacter->levels.levels;
	const uint sz = MIN<uint>((uint)lvls.size(), BASE_CLASS_NUM);
	for (uint i = 0; i < sz; ++i) {
		uint8 lvl = lvls[i];
		if (lvl == 0)
			continue;

		const SavingThrows &st = savingThrowsAt((uint8)i, lvl);
		if (st.vsParalysis     < best.vsParalysis)     best.vsParalysis     = st.vsParalysis;
		if (st.vsPetrification < best.vsPetrification) best.vsPetrification = st.vsPetrification;
		if (st.vsRodStaffWand  < best.vsRodStaffWand)  best.vsRodStaffWand  = st.vsRodStaffWand;
		if (st.vsBreathWeapon  < best.vsBreathWeapon)  best.vsBreathWeapon  = st.vsBreathWeapon;
		if (st.vsSpell         < best.vsSpell)         best.vsSpell         = st.vsSpell;

		// 3) Special dual-class quirk for class index 7 only: consider old level as well
		if (i == 7) {
			uint8 oldLvl = _newCharacter->highestLevel;
			if (oldLvl > 0 && lvl > oldLvl) {
				const SavingThrows &oldSt = savingThrowsAt(7, oldLvl);
				if (oldSt.vsParalysis     < best.vsParalysis)     best.vsParalysis     = oldSt.vsParalysis;
				if (oldSt.vsPetrification < best.vsPetrification) best.vsPetrification = oldSt.vsPetrification;
				if (oldSt.vsRodStaffWand  < best.vsRodStaffWand)  best.vsRodStaffWand  = oldSt.vsRodStaffWand;
				if (oldSt.vsBreathWeapon  < best.vsBreathWeapon)  best.vsBreathWeapon  = oldSt.vsBreathWeapon;
				if (oldSt.vsSpell         < best.vsSpell)         best.vsSpell         = oldSt.vsSpell;
			}
		}
	}

	_newCharacter->savingThrows = best;

	debug("Saving throws set: vsParalysis=%d, vsPetrification=%d, vsRod/Wand=%d, vsBreath=%d, vsSpell=%d",
		_newCharacter->savingThrows.vsParalysis,
		_newCharacter->savingThrows.vsPetrification,
		_newCharacter->savingThrows.vsRodStaffWand,
		_newCharacter->savingThrows.vsBreathWeapon,
		_newCharacter->savingThrows.vsSpell);
}

void CreateCharacterView::setAge() {
	if (!_newCharacter)
		return;

	using namespace Goldbox::Data;

	const uint8 race = _newCharacter->race;
	debug("setAge: classType=%u race=%u", (unsigned)_newCharacter->classType, (unsigned)race);

	// Multiclass special handling: use specified base class and MAX dice roll
	// - Cleric-based combos: Cleric/Fighter, Cleric/Fighter/Magic-User,
	//   Cleric/Magic-User, Cleric/Thief -> Cleric base + max dice
	// - Magic-User-based combos: Fighter/Magic-User, Fighter/Magic-User/Thief,
	//   Magic-User/Thief -> Magic-User base + max dice
	// - Fighter/Thief -> Fighter base + max dice
	uint8 forcedBaseIdx = 0xFF;
	switch (_newCharacter->classType) {
	case C_CLERIC_FIGHTER:
	case C_CLERIC_FIGHTER_MAGICUSER:
	case C_CLERIC_MAGICUSER:
	case C_CLERIC_THIEF:
		forcedBaseIdx = C_CLERIC; // 0
		break;
	case C_FIGHTER_MAGICUSER:
	case C_FIGHTER_MAGICUSER_THIEF:
	case C_MAGICUSER_THIEF:
		forcedBaseIdx = C_MAGICUSER; // 5
		break;
	case C_FIGHTER_THIEF:
		forcedBaseIdx = C_FIGHTER; // 2
		break;
	default:
		break; // fall back should be happened
	}

	if (forcedBaseIdx != 0xFF) {
		const Goldbox::Data::Rules::AgeDefEntry &adef =
			Goldbox::Data::Rules::getAgeDef(race, forcedBaseIdx);
		// Max dice roll: dices * sides
		const uint16 maxExtra = (uint16)(adef.dices * adef.sides);
		debug("AgeDef (forced base=%u): base=%u dice=%u sides=%u maxExtra=%u",
			  (unsigned)forcedBaseIdx, (unsigned)adef.base, (unsigned)adef.dices,
			  (unsigned)adef.sides, (unsigned)maxExtra);
		_newCharacter->age = adef.base + maxExtra;
		debug("Set character age (multiclass rule): %u", (unsigned)_newCharacter->age);
		if (_newCharacter->age == 0) {
			warning("Age computed as 0 in forced multiclass branch (base=%u) for race=%u", (unsigned)forcedBaseIdx, (unsigned)race);
		}
		return;
	}


	// If single-class (base class id), just roll that class's age
	if (_newCharacter->classType < C_CLERIC_FIGHTER) {
		uint8 baseIdx = _newCharacter->classType;
		const Goldbox::Data::Rules::AgeDefEntry &adef = Goldbox::Data::Rules::getAgeDef(race, baseIdx);
		uint16 extra = (uint16)VmInterface::rollDice(adef.dices, adef.sides);
		debug("AgeDef (single base=%u): base=%u dice=%u sides=%u rolled=%u",
			  (unsigned)baseIdx, (unsigned)adef.base, (unsigned)adef.dices,
			  (unsigned)adef.sides, (unsigned)extra);
		_newCharacter->age = adef.base + extra;
		debug("Set character age (single-class): %u", (unsigned)_newCharacter->age);
		if (_newCharacter->age == 0) {
			warning("Age computed as 0 in single-class branch (base=%u) for race=%u", (unsigned)baseIdx, (unsigned)race);
		}
		return;
	}

	// Fallback for any other multiclass not explicitly handled above
	{
		uint8 baseIdx = (uint8)C_FIGHTER; // reasonable default; non-POoR combos shouldn't hit this
		const Goldbox::Data::Rules::AgeDefEntry &adef = Goldbox::Data::Rules::getAgeDef(race, baseIdx);
	  uint16 extra = (uint16)VmInterface::rollDice(adef.dices, adef.sides);
	  debug("AgeDef (fallback base=%u): base=%u dice=%u sides=%u rolled=%u",
		  (unsigned)baseIdx, (unsigned)adef.base, (unsigned)adef.dices,
		  (unsigned)adef.sides, (unsigned)extra);
		_newCharacter->age = adef.base + extra;
	  warning("Set character age FALLBACK should not be happened: %u", (unsigned)_newCharacter->age);
	}
}

void CreateCharacterView::ageingEffects() {
	if (!_newCharacter)
		return;

	using namespace Goldbox::Data;

	// Monster class doesn't have age categories/effects per requirement
	if (_newCharacter->classType == C_MONSTER)
		return;

	const uint8 race = _newCharacter->race;
	const uint16 age = _newCharacter->age;

	// Get thresholds for the character's race
	const AgeCategories &cats = Goldbox::Data::Rules::getAgeCategoriesForRace(race);
	const Common::Array<AgeingEffects> &effects = Goldbox::Data::Rules::getStatAgeingEffects();

	// Determine the highest category index reached by this age, then apply all effects
	// from the first category up to and including that index (cumulative stacking).
	// Categories: 0=young, 1=adult, 2=middle, 3=old, 4=venitiar.
	int stageMax = -1;
	if (age > 0) {
		if (age < cats.young) stageMax = 0;                 // younger than "young"
		else if (age < cats.adult) stageMax = 1;            // in young..adult
		else if (age < cats.middle) stageMax = 2;           // in adult..middle
		else if (age < cats.old) stageMax = 3;              // in middle..old
		else if (age < cats.venitiar) stageMax = 4;         // in old..venitiar
		else stageMax = 4;                                  // >= venitiar: apply all
	}

	if (stageMax < 0)
		return;

	// Helper lambda to accumulate a particular stage index into a Stat
	auto applyStage = [&](Stat &st, int statRow, int stageIdx) {
		if (statRow < 0 || statRow >= (int)effects.size()) return;
		const AgeingEffects &ae = effects[statRow];
		int delta = 0;
		switch (stageIdx) {
		case 0: delta = ae.young; break;
		case 1: delta = ae.adult; break;
		case 2: delta = ae.middle; break;
		case 3: delta = ae.old; break;
		case 4: delta = ae.venitiar; break;
		default: return;
		}
		if (delta == 0)
			return; // don't clamp if we're not actually changing the stat
		int cur = (int)st.current + delta;
		// Exceptional strength (row 1) ranges up to 100; others up to 25
		const int maxVal = (statRow == 1) ? 100 : 25;
		if (cur < 0) cur = 0;
		if (cur > maxVal) cur = maxVal;
		st.current = (uint8)cur;
	};

	// Apply cumulatively for each required stage
	for (int s = 0; s <= stageMax; ++s) {
		// Row mapping in effects: 0 Str, 1 StrEx, 2 Int, 3 Wis, 4 Dex, 5 Con, 6 Cha
		applyStage(_newCharacter->abilities.strength,      0, s);
		applyStage(_newCharacter->abilities.strException,  1, s);
		applyStage(_newCharacter->abilities.intelligence,  2, s);
		applyStage(_newCharacter->abilities.wisdom,        3, s);
		applyStage(_newCharacter->abilities.dexterity,     4, s);
		applyStage(_newCharacter->abilities.constitution,  5, s);
		applyStage(_newCharacter->abilities.charisma,      6, s);
	}
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
