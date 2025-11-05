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
#include "goldbox/data/spells/spell.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/create_character_view.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/views/dialogs/horizontal_input.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

// Local UI constants and helpers
namespace {
const int kWinLeft = 1;
const int kWinTop = 1;
const int kWinRight = 38;
const int kWinBottom = 22;

const int kMenuLeft = 1;
const int kMenuTop = 2;
const int kMenuRight = 38;
const int kMenuBottom = 22;

const byte kMenuHeadColor = 13;
const byte kMenuTextColor = 10;
const byte kMenuSelectColor = 15;

template <typename T>
inline void detachAndDelete(T *&ptr) {
	if (ptr) {
		ptr->setParent(nullptr);
		delete ptr;
		ptr = nullptr;
	}
}
} // anonymous namespace

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
	detachAndDelete(_profileDialog);
	detachAndDelete(_nameInput);
	// ensure any lingering yes/no prompt is removed
	detachAndDelete(_yesNoPrompt);
	detachAndDelete(_menu);
	// _menuItems is not a UIElement
	delete _menuItems; _menuItems = nullptr;
	delete _newCharacter; _newCharacter = nullptr;
}

// Initialize base-class levels on the current character from its classType
void CreateCharacterView::initLevelsForClassType() {
	if (!_newCharacter)
		return;
	using namespace Goldbox::Data;
	// Log before
	{
		const Common::Array<uint8> &lv = _newCharacter->levels.levels;
		debug("initLevels(run): BEFORE [%u,%u,%u,%u,%u,%u,%u,%u]",
			  (unsigned)(lv.size() > 0 ? lv[0] : 0),
			  (unsigned)(lv.size() > 1 ? lv[1] : 0),
			  (unsigned)(lv.size() > 2 ? lv[2] : 0),
			  (unsigned)(lv.size() > 3 ? lv[3] : 0),
			  (unsigned)(lv.size() > 4 ? lv[4] : 0),
			  (unsigned)(lv.size() > 5 ? lv[5] : 0),
			  (unsigned)(lv.size() > 6 ? lv[6] : 0),
			  (unsigned)(lv.size() > 7 ? lv[7] : 0));
	}
	for (uint i = 0; i < _newCharacter->levels.levels.size(); ++i)
		_newCharacter->levels.levels[i] = 0;

	switch (_newCharacter->classType) {
	case C_CLERIC_FIGHTER:
		_newCharacter->levels.levels[C_CLERIC] = 1;
		_newCharacter->levels.levels[C_FIGHTER] = 1;
		break;
	case C_CLERIC_FIGHTER_MAGICUSER:
		_newCharacter->levels.levels[C_CLERIC] = 1;
		_newCharacter->levels.levels[C_FIGHTER] = 1;
		_newCharacter->levels.levels[C_MAGICUSER] = 1;
		break;
	case C_CLERIC_RANGER:
		_newCharacter->levels.levels[C_CLERIC] = 1;
		_newCharacter->levels.levels[C_RANGER] = 1;
		break;
	case C_CLERIC_MAGICUSER:
		_newCharacter->levels.levels[C_CLERIC] = 1;
		_newCharacter->levels.levels[C_MAGICUSER] = 1;
		break;
	case C_CLERIC_THIEF:
		_newCharacter->levels.levels[C_CLERIC] = 1;
		_newCharacter->levels.levels[C_THIEF] = 1;
		break;
	case C_FIGHTER_MAGICUSER:
		_newCharacter->levels.levels[C_FIGHTER] = 1;
		_newCharacter->levels.levels[C_MAGICUSER] = 1;
		break;
	case C_FIGHTER_THIEF:
		_newCharacter->levels.levels[C_FIGHTER] = 1;
		_newCharacter->levels.levels[C_THIEF] = 1;
		break;
	case C_FIGHTER_MAGICUSER_THIEF:
		_newCharacter->levels.levels[C_FIGHTER] = 1;
		_newCharacter->levels.levels[C_MAGICUSER] = 1;
		_newCharacter->levels.levels[C_THIEF] = 1;
		break;
	case C_MAGICUSER_THIEF:
		_newCharacter->levels.levels[C_MAGICUSER] = 1;
		_newCharacter->levels.levels[C_THIEF] = 1;
		break;
	default:
		if (_newCharacter->classType < BASE_CLASS_NUM) {
			_newCharacter->levels.levels[_newCharacter->classType] = 1;
		} else {
			warning("initLevels(run): classType=%u is not a base class; no base levels set",
					(unsigned)_newCharacter->classType);
		}
		break;
	}

	// Log after
	{
		const Common::Array<uint8> &lv = _newCharacter->levels.levels;
		debug("initLevels(run): AFTER  [%u,%u,%u,%u,%u,%u,%u,%u]",
			  (unsigned)(lv.size() > 0 ? lv[0] : 0),
			  (unsigned)(lv.size() > 1 ? lv[1] : 0),
			  (unsigned)(lv.size() > 2 ? lv[2] : 0),
			  (unsigned)(lv.size() > 3 ? lv[3] : 0),
			  (unsigned)(lv.size() > 4 ? lv[4] : 0),
			  (unsigned)(lv.size() > 5 ? lv[5] : 0),
			  (unsigned)(lv.size() > 6 ? lv[6] : 0),
			  (unsigned)(lv.size() > 7 ? lv[7] : 0));
	}
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
			// Apply ageing then race/gender caps after initial recompute
			ageingEffects();
			applyStatMinMax();
			// Initialize starting spell slots/known spells per rules
			applySpells();
			// Initialize starting gold based on base classes
			setInitGold();
			// Initialize and average HP according to class/Con rules
			setInitHP();
			_newCharacter->hitPoints.current = _newCharacter->hitPoints.max;
			_newCharacter->primaryAttacks = 2;
			_newCharacter->priDmgDiceNum = 1;
			_newCharacter->priDmgDiceSides = 2;
			_newCharacter->strengthBonusAllowed = 1;
			_newCharacter->baseMovement = 12;
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

	drawWindow(kWinLeft, kWinTop, kWinRight, kWinBottom);
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
		// If the Yes/No prompt is active, route the key directly to it and consume
		if (_yesNoPrompt && _activeSubView == static_cast<Dialogs::Dialog *>(_yesNoPrompt)) {
			_yesNoPrompt->msgKeypress(msg);
			return true;
		}
		if (msg.keycode == Common::KEYCODE_r || msg.ascii == 'R') {
			rollAndRecompute();
			ageingEffects();
			applyStatMinMax();
			applySpells();
			setInitGold();
			setInitHP();
			if (_profileDialog)
				_profileDialog->redrawStats(), _profileDialog->redrawValuables(), _profileDialog->redrawCombat();
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
	detachAndDelete(_profileDialog);
	_profileDialog = new Dialogs::CharacterProfile(_newCharacter, "CreateProfile");
	subView(_profileDialog);
	// Attach a Yes/No prompt under the profile asking whether to keep current stats
	detachAndDelete(_yesNoPrompt);
	Dialogs::HorizontalYesNoConfig ynCfg { "Keep this character? ", 15 };
	_yesNoPrompt = new Dialogs::HorizontalYesNo("ProfileYN", ynCfg);
	// Parent the Yes/No prompt to the profile dialog (child of profile)
	if (_profileDialog)
		_yesNoPrompt->setParent(_profileDialog);
	// Make the prompt active to capture Y/N while profile remains visible
	setActiveSubView(static_cast<Dialogs::Dialog *>(_yesNoPrompt));
}

void CreateCharacterView::chooseName() {
	using namespace Goldbox::Poolrad::Views::Dialogs;
	detachAndDelete(_nameInput);
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
		kMenuHeadColor,      // headColor
		kMenuTextColor,      // textColor
		kMenuSelectColor,    // selectColor
		kMenuLeft, kMenuTop, kMenuRight, kMenuBottom, // bounds
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
				// Initialize base-class levels from selected class
				initLevelsForClassType();
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
		} else if (key == Common::KEYCODE_y || key == Common::KEYCODE_RETURN) {
			// Yes: proceed to name input, remove the prompt
			detachAndDelete(_yesNoPrompt);
			setStage(CC_STATE_NAME);
		} else if (key == Common::KEYCODE_n) {
			// No: reroll and recompute derived values while keeping Profile view active
			if (_yesNoPrompt)
				_yesNoPrompt->deactivate();
			if (_profileDialog)
				setActiveSubView(_profileDialog);
			recomputeAfterAlignment();
			if (_profileDialog) {
				_profileDialog->redrawStats();
				_profileDialog->redrawValuables();
				_profileDialog->redrawCombat();
			}
			// After redraw, hand control back to Yes/No prompt for next decision
			if (_yesNoPrompt)
				setActiveSubView(static_cast<Dialogs::Dialog *>(_yesNoPrompt));
		} else if (key == Common::KEYCODE_r) {
				rollAndRecompute();
				if (_profileDialog) {
					_profileDialog->redrawStats();
					_profileDialog->redrawValuables();
					_profileDialog->redrawCombat();
				}
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
	detachAndDelete(_profileDialog);
	detachAndDelete(_nameInput);
	detachAndDelete(_yesNoPrompt);
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
	// Start as level 1 in chosen class (supports multi-class)
	initLevelsForClassType();
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
	// Re-initialize levels from classType to preserve multiclass composition
	initLevelsForClassType();
	_newCharacter->calculateHitPoints();
}

void CreateCharacterView::recomputeAfterAlignment() {
	if (!_newCharacter)
		return;
	// Reroll ability scores and recompute all post-alignment data
	rollAndRecompute();
	// Derived attributes and caps/effects
	ageingEffects();
	applyStatMinMax();
	if (_newCharacter->levels.levels[Goldbox::Data::C_THIEF] > 0)
		setThiefSkillsForNewCharacter();
	setThac0();
	setSavingThrows();
	applySpells();
	setInitGold();
	setInitHP();
	_newCharacter->hitPoints.current = _newCharacter->hitPoints.max;
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
	if (!_newCharacter)
		return;
	_newCharacter->computeThac0();
}

// Helper to clamp an int to uint8 range
static inline uint8 clampU8(int v) { return v < 0 ? 0 : (v > 255 ? 255 : (uint8)v); }

void CreateCharacterView::setSavingThrows() {
	if (!_newCharacter)
		return;
	_newCharacter->computeSavingThrows();
}

void CreateCharacterView::setAge() {
	if (!_newCharacter)
		return;
	_newCharacter->rollInitialAge();
}

void CreateCharacterView::ageingEffects() {
	if (!_newCharacter)
		return;
	_newCharacter->applyAgeingEffects();
}

void CreateCharacterView::applyStatMinMax() {
    if (!_newCharacter)
        return;

    using namespace Goldbox::Data;

    // Do not apply for monster race per requirement
	if (_newCharacter->race == R_MONSTER) {
		debug("applyStatMinMax: skipped for monster race");
		return;
	}

    const RaceStatMinMax &mm = Goldbox::Data::Rules::getRaceStatMinMaxForRace(_newCharacter->race);

    // Strength and exceptional strength depend on gender for max/min
    const bool isFemale = (_newCharacter->gender == G_FEMALE);
    const uint8 strMin = isFemale ? mm.strengthMinFemale : mm.strengthMinMale;
    const uint8 strMax = isFemale ? mm.strengthMaxFemale : mm.strengthMaxMale;
    const uint8 extStrMax = isFemale ? mm.extStrengthMaxFemale : mm.extStrengthMaxMale;

	debug("applyStatMinMax: start race=%u gender=%u class=%u", (unsigned)_newCharacter->race, (unsigned)_newCharacter->gender, (unsigned)_newCharacter->classType);
	debug("applyStatMinMax: race bounds STR[%u..%u] STREx<=%u INT[%u..%u] WIS[%u..%u] DEX[%u..%u] CON[%u..%u] CHA[%u..%u]",
		  (unsigned)strMin, (unsigned)strMax, (unsigned)extStrMax,
		  (unsigned)mm.intelligenceMin, (unsigned)mm.intelligenceMax,
		  (unsigned)mm.wisdomMin, (unsigned)mm.wisdomMax,
		  (unsigned)mm.dexterityMin, (unsigned)mm.dexterityMax,
		  (unsigned)mm.constitutionMin, (unsigned)mm.constitutionMax,
		  (unsigned)mm.charismaMin, (unsigned)mm.charismaMax);

    // Helper to clamp a Stat between min and max inclusive (current value only)
    auto clampStatCur = [](Stat &s, uint8 minV, uint8 maxV) {
        if (s.current < minV) s.current = minV;
        if (s.current > maxV) s.current = maxV;
    };

	// Apply Strength min/max
	debug("applyStatMinMax: before STR=%u, STREx=%u, INT=%u, WIS=%u, DEX=%u, CON=%u, CHA=%u",
		  (unsigned)_newCharacter->abilities.strength.current,
		  (unsigned)_newCharacter->abilities.strException.current,
		  (unsigned)_newCharacter->abilities.intelligence.current,
		  (unsigned)_newCharacter->abilities.wisdom.current,
		  (unsigned)_newCharacter->abilities.dexterity.current,
		  (unsigned)_newCharacter->abilities.constitution.current,
		  (unsigned)_newCharacter->abilities.charisma.current);
    clampStatCur(_newCharacter->abilities.strength, strMin, strMax);
    // Intelligence
    clampStatCur(_newCharacter->abilities.intelligence, mm.intelligenceMin, mm.intelligenceMax);
    // Wisdom
    clampStatCur(_newCharacter->abilities.wisdom, mm.wisdomMin, mm.wisdomMax);
    // Dexterity
    clampStatCur(_newCharacter->abilities.dexterity, mm.dexterityMin, mm.dexterityMax);
    // Constitution
    clampStatCur(_newCharacter->abilities.constitution, mm.constitutionMin, mm.constitutionMax);
	// Charisma
	clampStatCur(_newCharacter->abilities.charisma, mm.charismaMin, mm.charismaMax);

	// After race bounds, enforce class minimum stats if below thresholds
	const ClassMinStats &cms = Goldbox::Data::Rules::getClassMinStats(_newCharacter->classType);
	if (_newCharacter->abilities.strength.current < cms.strength)
		_newCharacter->abilities.strength.current = cms.strength;
	debug("applyStatMinMax: class minima STR>=%u INT>=%u WIS>=%u DEX>=%u CON>=%u CHA>=%u",
	      (unsigned)cms.strength, (unsigned)cms.intelligence, (unsigned)cms.wisdom,
	      (unsigned)cms.dexterity, (unsigned)cms.constitution, (unsigned)cms.charisma);

	// Exceptional Strength rule: applicable if fighter level > 0 (any combination)
	bool hasFighterLevel = false;

	hasFighterLevel = (_newCharacter->levels.levels[Goldbox::Data::C_FIGHTER] > 0);

	if (hasFighterLevel && _newCharacter->abilities.strength.current >= 18) {
		// Roll 1d100 for exceptional strength
		uint8 roll = (uint8)VmInterface::rollDice(1, 100);
		_newCharacter->abilities.strException.current = roll;
		debug("applyStatMinMax: rolled STREx=%u (fighter level detected)", (unsigned)roll);
	} else {
		// Other classes do not have exceptional strength (or STR != 18)
		_newCharacter->abilities.strException.current = 0;
		debug("applyStatMinMax: STREx set to 0 (no fighter level or STR!=18)");
	}
	// Clamp Exceptional Strength to gender/race maximum
	if (_newCharacter->abilities.strException.current > extStrMax)
		_newCharacter->abilities.strException.current = extStrMax;
	debug("applyStatMinMax: after clamps STR=%u, STREx=%u, INT=%u, WIS=%u, DEX=%u, CON=%u, CHA=%u",
	      (unsigned)_newCharacter->abilities.strength.current,
	      (unsigned)_newCharacter->abilities.strException.current,
	      (unsigned)_newCharacter->abilities.intelligence.current,
	      (unsigned)_newCharacter->abilities.wisdom.current,
	      (unsigned)_newCharacter->abilities.dexterity.current,
	      (unsigned)_newCharacter->abilities.constitution.current,
	      (unsigned)_newCharacter->abilities.charisma.current);

	if (_newCharacter->abilities.intelligence.current < cms.intelligence)
		_newCharacter->abilities.intelligence.current = cms.intelligence;
	if (_newCharacter->abilities.wisdom.current < cms.wisdom)
		_newCharacter->abilities.wisdom.current = cms.wisdom;
	if (_newCharacter->abilities.dexterity.current < cms.dexterity)
		_newCharacter->abilities.dexterity.current = cms.dexterity;
	if (_newCharacter->abilities.constitution.current < cms.constitution)
		_newCharacter->abilities.constitution.current = cms.constitution;
	if (_newCharacter->abilities.charisma.current < cms.charisma)
		_newCharacter->abilities.charisma.current = cms.charisma;
}

void CreateCharacterView::applySpells() {
	if (!_newCharacter)
		return;
	_newCharacter->computeSpellSlots();
}

void CreateCharacterView::setInitGold() {
	if (!_newCharacter)
		return;

	using namespace Goldbox::Data;
	using Goldbox::Data::Rules::getInitGoldRoll;

	int totalGold = 0;
	int classCount = 0;
	debug("setInitGold: start");
	debug("setInitGold: classType=%u", (unsigned)_newCharacter->classType);

	// Iterate all base classes (0..7) and roll per-class starting gold
	for (uint8 base = 0; base < BASE_CLASS_NUM; ++base) {
		uint8 lvl = 0;
		// LevelData operator[] expects ClassADnD; cast base index accordingly
		lvl = _newCharacter->levels[static_cast<Goldbox::Data::ClassADnD>(base)];
		debug("setInitGold: base=%u lvl=%u", (unsigned)base, (unsigned)lvl);
		if (lvl > 0) {
			const DiceRoll &dr = getInitGoldRoll(base);
			int classGold = VmInterface::rollDice(dr.numDice, dr.diceSides);
			classGold += 1;
			totalGold += classGold;
			++classCount;
			debug("setInitGold: base=%u lvl=%u roll=%uD%u+1 -> %d",
				  (unsigned)base, (unsigned)lvl, (unsigned)dr.numDice, (unsigned)dr.diceSides, classGold);
		} else {
			debug("setInitGold: skipping base=%u (lvl==0)", (unsigned)base);
		}
	}

	// Average across classes (truncate)
	uint16 finalGold = (classCount > 0) ? static_cast<uint16>(totalGold / classCount) : 0;
	_newCharacter->valuableItems[VAL_GOLD] = finalGold;
	debug("setInitGold: classes=%d total=%d avg=%u (final gold)", classCount, totalGold, (unsigned)finalGold);
	if (classCount == 0) {
		warning("setInitGold: no base classes with level > 0; gold set to 0. classType=%u", (unsigned)_newCharacter->classType);
	}
}

void CreateCharacterView::setInitHP() {
	if (!_newCharacter)
		return;

	// Step 1: roll raw HP for levels beyond 1 across all active base classes
	debug("rollHP: calling with CF_ALL; pre-rolled=%u", (unsigned)_newCharacter->hitPointsRolled);
	_newCharacter->hitPointsRolled = _newCharacter->getRolledHP(Goldbox::Data::CF_ALL);
	// Align max HP with raw rolled HP first (pre-Con averaging)
	_newCharacter->hitPoints.max = _newCharacter->hitPointsRolled;
	debug("rollHP: post-rolled=%u max=%u (pre-Con)", (unsigned)_newCharacter->hitPointsRolled, (unsigned)_newCharacter->hitPoints.max);

	// Step 2: determine class count divisor from active base classes
	int classCount = (int)_newCharacter->countActiveBaseClasses();
	if (classCount <= 0) {
		warning("setInitHP: no active base classes; leaving HP unchanged");
		_newCharacter->hitPoints.current = _newCharacter->hitPoints.max;
		return;
	}

	int8 conSum = _newCharacter->getConHPModifier();
	int hpMax = (int)_newCharacter->hitPoints.max;
	int newHpMax = hpMax;
	if (conSum >= 0) {
		newHpMax = (hpMax + conSum) / classCount;
	} else {
		const int m = -conSum; // absolute value of negative Con sum
		// Safety: ensure average cannot drop HP to 0 or less
		if (hpMax <= (classCount + m)) {
			newHpMax = 1;
		} else {
			newHpMax = (hpMax + conSum) / classCount;
		}
	}

	// Store results, clamping to uint8 range
	_newCharacter->hitPoints.max = clampU8(newHpMax);
	_newCharacter->hitPoints.current = _newCharacter->hitPoints.max;
	// Average the raw roll (without Con) separately
	_newCharacter->hitPointsRolled = (uint8)(_newCharacter->hitPointsRolled / classCount);
	debug("setInitHP: classes=%d conSum=%d -> max=%u cur=%u rolled(avgRaw)=%u",
		classCount, conSum,
		(unsigned)_newCharacter->hitPoints.max,
		(unsigned)_newCharacter->hitPoints.current,
		(unsigned)_newCharacter->hitPointsRolled);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
