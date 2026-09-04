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
#include "common/fs.h"
#include "graphics/palette.h"
//#include "goldbox/keymapping.h"
#include "goldbox/engine.h"
#include "goldbox/vm_interface.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/poolrad/data/legacy_save_utils.h"
#include "goldbox/poolrad/views/create_character_view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

using Goldbox::Poolrad::Data::PoolradCharacter;
using Goldbox::Data::CombatRoll;

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

static void logCombatDice(const Goldbox::Poolrad::Data::PoolradCharacter *pc, const char *tag) {
	if (!pc)
		return;

	const Goldbox::Data::CombatRoll &bp = pc->getBasePrimaryRoll();
	const Goldbox::Data::CombatRoll &bs = pc->getBaseSecondaryRoll();
	const Goldbox::Data::CombatRoll &cp = pc->getCurrentPrimaryRoll();
	const Goldbox::Data::CombatRoll &cs = pc->getCurrentSecondaryRoll();

	debug(4, "CreateChar[%s]: base pri att=%u %ud%u%+d sec att=%u %ud%u%+d | cur pri att=%u %ud%u%+d sec att=%u %ud%u%+d",
	      tag,
	      (unsigned)bp.attacks,
	      (unsigned)bp.action.roll.diceNum,
	      (unsigned)bp.action.roll.diceSides,
	      (int)bp.action.modifier,
	      (unsigned)bs.attacks,
	      (unsigned)bs.action.roll.diceNum,
	      (unsigned)bs.action.roll.diceSides,
	      (int)bs.action.modifier,
	      (unsigned)cp.attacks,
	      (unsigned)cp.action.roll.diceNum,
	      (unsigned)cp.action.roll.diceSides,
	      (int)cp.action.modifier,
	      (unsigned)cs.attacks,
	      (unsigned)cs.action.roll.diceNum,
	      (unsigned)cs.action.roll.diceSides,
	      (int)cs.action.modifier);
}

static Common::Path getLegacySavePath() {
	if (Goldbox::g_engine)
		return Goldbox::g_engine->resolveSavePath();

	warning("Goldbox engine unavailable while resolving legacy save path");
	return Common::Path();
}
} // anonymous namespace

const char PICK_RACE[]      = "Pick Race";
const char PICK_GENDER[]    = "Pick Gender";
const char PICK_CLASS[]     = "Pick Class";
const char PICK_ALIGNMENT[] = "Pick Alignment";
const char KEEP_CHARACTER[] = "Keep this character? ";
const char CHAR_NAME[]      = "Character name: ";


CreateCharacterView::CreateCharacterView() : View("CreatCharacter"), _stage(CC_STATE_RACE) {
	_newCharacter = new PoolradCharacter();
	_newCharacter->initializeNewCharacter();
	chooseRace();
}

CreateCharacterView::~CreateCharacterView() {
	setActiveSubView(nullptr);
	detachAndDelete(_profileDialog);
	detachAndDelete(_nameInput);
	detachAndDelete(_yesNoPrompt);
	detachAndDelete(_portraitSelector);
	detachAndDelete(_iconSelector);
	detachAndDelete(_listmenu);
	delete _menuItems; _menuItems = nullptr;
	delete _newCharacter; _newCharacter = nullptr;
}

// Initialize base-class levels on the current character from its classType
void CreateCharacterView::initLevelsForClassType() {
	if (!_newCharacter)
		return;
	using namespace Goldbox::Data;

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
}

void CreateCharacterView::nextStage() {
	if (_stage < CC_STATE_DONE)
		setStage(static_cast<CharacterCreateState>(_stage + 1));
}

void CreateCharacterView::setStage(CharacterCreateState stage) {
	_stage = stage;
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
	case CC_STATE_ROLLSTATS:
		// Deactivate the menu but don't delete it yet - let pending HorizontalMenu events complete
		if (_activeSubView == static_cast<Dialogs::Dialog *>(_listmenu)) {
			setActiveSubView(nullptr);
		}
		// First time entering profile: roll initial stats BEFORE showing profile
		if (!_hasRolled && _newCharacter) {
			initializeRollStatsOnce();
		}
		// Now show the profile dialog with freshly computed values
		showProfileDialog();
		// Always attach the Yes/No prompt for this stage
		attachKeepCharacterPrompt();
		break;
	case CC_STATE_NAME:
		// Deactivate and delete yes/no prompt
		if (_yesNoPrompt) {
			if (_activeSubView == static_cast<Dialogs::Dialog *>(_yesNoPrompt)) {
				setActiveSubView(nullptr);
			}
			detachAndDelete(_yesNoPrompt);
		}
		// Menu is already inactive; just ensure it's not active
		if (_activeSubView == static_cast<Dialogs::Dialog *>(_listmenu))
			setActiveSubView(nullptr);
		if (!_profileDialog)
			showProfileDialog();
		chooseName();
		break;
	case CC_STATE_PORTRAIT:
		// Remove text input
		if (_nameInput) {
			if (_activeSubView == static_cast<Dialogs::Dialog *>(_nameInput)) {
				setActiveSubView(nullptr);
			}
			detachAndDelete(_nameInput);
		}
		// Menu is already inactive; just ensure it's not active
		if (_activeSubView == static_cast<Dialogs::Dialog *>(_listmenu))
			setActiveSubView(nullptr);
		if (!_profileDialog)
			showProfileDialog();
		choosePortrait();
		break;
	case CC_STATE_ICON:
		if (_portraitSelector) {
			if (_activeSubView == static_cast<Dialogs::Dialog *>(_portraitSelector)) {
				setActiveSubView(nullptr);
			}
			detachAndDelete(_portraitSelector);
		}
		// Menu is already inactive; just ensure it's not active
		if (_activeSubView == static_cast<Dialogs::Dialog *>(_listmenu))
			setActiveSubView(nullptr);
		// Profile is hidden for icon selection
		if (_profileDialog) {
			_profileDialog->deactivate();
		}
		chooseIcon();
		break;
	case CC_STATE_DONE:
		saveCharacter();
		replaceView("Mainmenu");
		break;
	default:
		debug(4, "CreateCharacterView: setStage(%d) unknow stage", (int)stage);
	}
}

void CreateCharacterView::draw() {
	// In CC_STATE_ICON, let SetIcon handle all drawing (including custom background)
	if (_stage != CC_STATE_ICON) {
		drawWindow(kWinLeft, kWinTop, kWinRight, kWinBottom);
	}
	if (_stage != CC_STATE_ICON && _profileDialog && _profileDialog->isActive()) {
		_profileDialog->draw();
	}
	if (_activeSubView && _activeSubView != static_cast<Dialogs::Dialog *>(_profileDialog)) {
		_activeSubView->draw();
	} else {
		debug(4, "CreateCharacterView::draw() - No active subview to draw");
	}
}

bool CreateCharacterView::msgFocus(const FocusMessage &msg) {
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
	// Let active dialogs handle the keypress first (including ESC if appropriate)
	if (_stage == CC_STATE_ROLLSTATS) {
		// If the Yes/No prompt is active, route the key directly to it and consume
		if (_yesNoPrompt && _activeSubView == static_cast<Dialogs::Dialog *>(_yesNoPrompt)) {
			_yesNoPrompt->msgKeypress(msg);
			return true;
		}
	}
	// In all other cases, if we have an active subview (e.g., name input or menu),
	// forward the keypress to the concrete dialog instance.
	if (_activeSubView) {
		if (_activeSubView == static_cast<Dialogs::Dialog *>(_nameInput) && _nameInput) {
			static_cast<Dialogs::HorizontalInput *>(_nameInput)->msgKeypress(msg);
			return true;
		}
	}
	// Global immediate exit on Escape from any stage (except DONE which already returns)
	// This happens AFTER active dialogs have had their chance to handle the key
	if (msg.keycode == Common::KEYCODE_ESCAPE) {
		// Proactively reset internal state so that re-opening the view does not
		// reference stale dialogs or deleted objects.
		resetState();
		replaceView("Mainmenu");
		return true;
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
	// Set the selected character so CharacterProfile can fetch it
	VmInterface::setSelectedCharacter(_newCharacter);
	if (_activeSubView == static_cast<Dialogs::Dialog *>(_profileDialog))
		setActiveSubView(nullptr);
	detachAndDelete(_profileDialog);
	_profileDialog = new Dialogs::CharacterProfile();
	attachDialog(_profileDialog);
	_profileDialog->activate();
	// Do not attach input handlers here; profile is reused across stages.
}

// Attach a Yes/No prompt under the profile and make it active
void CreateCharacterView::attachKeepCharacterPrompt() {
	// Clean any previous prompt
	if (_activeSubView == _yesNoPrompt)
		setActiveSubView(nullptr);
	detachAndDelete(_yesNoPrompt);
	// All 5 fields must be given explicitly: default member initializer on
	// backgroundColor makes this a non-aggregate under C++11 aggregate rules.
	Dialogs::HorizontalYesNoConfig ynCfg { KEEP_CHARACTER, kMenuHeadColor, kMenuTextColor, kMenuSelectColor, 0 };
	_yesNoPrompt = new Dialogs::HorizontalYesNo("ProfileYN", ynCfg);
	if (_profileDialog)
		setDialogParent(_yesNoPrompt, _profileDialog);
	setActiveSubView(static_cast<Dialogs::Dialog *>(_yesNoPrompt));
}

void CreateCharacterView::chooseName() {
	using namespace Goldbox::Poolrad::Views::Dialogs;
	detachAndDelete(_nameInput);
	HorizontalInputConfig cfg { CHAR_NAME, 13, 15 };
	_nameInput = new HorizontalInput("NameInput", cfg);
	// Attach the input dialog under the profile dialog so the profile stays visible
	if (_profileDialog)
		setDialogParent(_nameInput, _profileDialog);
	setActiveSubView(static_cast<Dialogs::Dialog *>(_nameInput));
}

void CreateCharacterView::choosePortrait() {
	using namespace Goldbox::Poolrad::Views::Dialogs;
	// Portrait selection does not use the vertical menu; remove it from the hierarchy
	if (_activeSubView == static_cast<Dialogs::Dialog *>(_listmenu))
		setActiveSubView(nullptr);
	detachAndDelete(_listmenu);
	detachAndDelete(_portraitSelector);
	if (!_newCharacter)
		_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
	_portraitSelector = new SetPortrait("SetPortrait", _newCharacter, _profileDialog);
	attachDialog(_portraitSelector);
	setActiveSubView(_portraitSelector);
}

void CreateCharacterView::chooseIcon() {
	using namespace Goldbox::Poolrad::Views::Dialogs;
	// Icon selection does not use the vertical menu or profile; remove them from the hierarchy
	if (_activeSubView == static_cast<Dialogs::Dialog *>(_listmenu))
		setActiveSubView(nullptr);
	detachAndDelete(_listmenu);
	detachAndDelete(_iconSelector);
	if (!_newCharacter)
		_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
	_iconSelector = new SetIcon("SetIcon", _newCharacter);
	attachDialog(_iconSelector);
	setActiveSubView(_iconSelector);
}

void CreateCharacterView::buildAndShowMenu(const Common::String &topline) {
	_menuPromptOptions.clear();
	_menuPromptOptions.push_back("Exit");
    Dialogs::VerticalMenuConfig menuConfig = {
        "",                  // promptTxt
        &_menuPromptOptions, // promptOptions (pointer)
        _menuItems,          // menuItemList (initialized later)
		kMenuHeadColor,      // headColor
		kMenuTextColor,      // textColor
		kMenuSelectColor,    // selectColor
		kMenuLeft, kMenuTop, kMenuRight, kMenuBottom, // bounds
		topline,	         // title
        false                // addExit
    };
	if (_listmenu) {
		_listmenu->rebuild(_menuItems, topline);
		setActiveSubView(_listmenu);
	} else {
		_listmenu = new Dialogs::VerticalMenu("CreateCharMenu", menuConfig);
		attachDialog(_listmenu);
		setActiveSubView(_listmenu);
	}
}

// One-time initialization when entering the ROLLSTATS stage the first time
void CreateCharacterView::initializeRollStatsOnce() {
	// Seed base unarmed rolls before any recompute so current rolls mirror them
	if (_newCharacter) {
		Goldbox::Data::CombatRoll pri;
		Goldbox::Data::CombatRoll sec;
		pri.attacks = 2;
		pri.action.roll.diceNum = 1;
		pri.action.roll.diceSides = 2;
		pri.action.modifier = 0;
		sec.attacks = 0;
		sec.action.roll.diceNum = 0;
		sec.action.roll.diceSides = 0;
		sec.action.modifier = 0;
		_newCharacter->setBasePrimaryRoll(pri);
		_newCharacter->setBaseSecondaryRoll(sec);
		_newCharacter->setCurrentPrimaryRoll(pri);
		_newCharacter->setCurrentSecondaryRoll(sec);
		_newCharacter->strengthBonusAllowed = 1;
		_newCharacter->movement.base = 12;
	}

	// Run a first reroll-and-recompute to populate all dynamic values
	performRerollAndRecompute();

	//logCombatDice(_newCharacter, "init");

	_hasRolled = true;
}

// Consolidated dynamic reroll path used for initial roll and subsequent rerolls
void CreateCharacterView::performRerollAndRecompute() {
	if (!_newCharacter)
		return;

	// Roll abilities and recompute base derived values
	rollAndRecompute();

	// Post-roll adjustments and derived tables
	ageingEffects();
	applyStatMinMax();

	// Recompute thief skills if applicable (depends on DEX and race)
	if (_newCharacter->levels.levels.size() > Goldbox::Data::C_THIEF &&
		_newCharacter->levels.levels[Goldbox::Data::C_THIEF] > 0) {
		setThiefSkillsForNewCharacter();
	}

	// Derived combat tables
	setThac0();
	setSavingThrows();

	// Starting resources and HP per rules
	applySpells();
	setInitGold();
	setInitHP();
	_newCharacter->hitPoints.current = _newCharacter->hitPoints.max;
	_newCharacter->recalcCombatStats();

	//logCombatDice(_newCharacter, "reroll");
}

void CreateCharacterView::setActiveSubView(Dialogs::Dialog *dlg) {
	switchActiveDialog(_activeSubView, dlg);
}

void CreateCharacterView::handleMenuResult(const MenuResultMessage &result) {
	short value = result._hasIntValue ? (short)result._intValue : 0;
	bool success = result._success;
	Common::KeyCode key = result._keyCode;

	if (_stage == CC_STATE_NAME &&
		result._success &&
		result._keyCode == Common::KEYCODE_RETURN &&
		result._hasStringValue) {
		_enteredName = result._stringValue;
		if (!_newCharacter)
			_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
		_newCharacter->name = _enteredName;
		if (_profileDialog)
			_profileDialog->drawName();
		if (_activeSubView == static_cast<Dialogs::Dialog *>(_nameInput))
			setActiveSubView(nullptr);
		detachAndDelete(_nameInput);
		setStage(CC_STATE_PORTRAIT);
		return;
	}

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
			//debug("Selected race menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->race = (uint8)_indexMap[value];
				//debug("Selected race ID: %d", _newCharacter->race);
				if (_newCharacter) {
					switch (_newCharacter->race) {
					case Goldbox::Data::R_DWARF:
						_newCharacter->iconData.bodyType = 1;
						_newCharacter->addEffect(90, 0, 0xFF, false);
						_newCharacter->addEffect(97, 0, 0xFF, false);
						_newCharacter->addEffect(26, 0, 0xFF, false);
						_newCharacter->addEffect(47, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_ELF:
						_newCharacter->iconData.bodyType = 2;
						_newCharacter->addEffect(107, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_GNOME:
						_newCharacter->iconData.bodyType = 1;
						_newCharacter->addEffect(97, 0, 0xFF, false);
						_newCharacter->addEffect(18, 0, 0xFF, false);
						_newCharacter->addEffect(47, 0, 0xFF, false);
						_newCharacter->addEffect(48, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_HALF_ELF:
						_newCharacter->iconData.bodyType = 2;
						_newCharacter->addEffect(124, 0, 0xFF, false);
						break;
					case Goldbox::Data::R_HALFLING:
						_newCharacter->iconData.bodyType = 1;
						_newCharacter->addEffect(90, 0, 0xFF, false);
						_newCharacter->addEffect(97, 0, 0xFF, false);
						break;
					default: // Monster / Human / Half-Orc or any other
						_newCharacter->iconData.bodyType = 2;
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
			//debug("Selected gender menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->gender = (Goldbox::Data::Gender)_indexMap[value];
				//debug("Selected gender ID: %d", _newCharacter->gender);
				nextStage();
			}
		}
		break;
	case CC_STATE_CLASS:
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			resetState();
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			//debug("Selected class menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->classType = (uint8)_indexMap[value];
				//debug("Selected class ID: %d", _newCharacter->classType);
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
			//debug("Selected alignment menuID: %d", value);
			if (!_newCharacter) _newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
			if (value >= 0 && value < (int)_indexMap.size()) {
				_newCharacter->alignment = (uint8)_indexMap[value];
				//debug("Selected alignment ID: %d", _newCharacter->alignment);
				_hasRolled = false;
			}
			setAge();
			nextStage();
		}
		break;
	case CC_STATE_ROLLSTATS:
		if (key == Common::KEYCODE_ESCAPE || key == Common::KEYCODE_e) {
			resetState();
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_y) {
				// Yes: proceed to name input. Make sure we don't leave _activeSubView
				// pointing at the prompt which we are about to delete (avoid UAF).
				if (_activeSubView == _yesNoPrompt)
					setActiveSubView(nullptr);
				detachAndDelete(_yesNoPrompt);
				setStage(CC_STATE_NAME);
		} else if (key == Common::KEYCODE_n) {
			// No: reroll and recompute derived values while keeping Profile view active
			if (_profileDialog)
				setActiveSubView(_profileDialog);
			performRerollAndRecompute();
			if (_profileDialog) {
				_profileDialog->redrawStats();
				_profileDialog->redrawValuables();
				_profileDialog->redrawCombat();
			}
			// After redraw, hand control back to Yes/No prompt for next decision
			if (_yesNoPrompt)
				setActiveSubView(static_cast<Dialogs::Dialog *>(_yesNoPrompt));
		}
		break;
	case CC_STATE_NAME:
		if (key == Common::KEYCODE_ESCAPE) {
			resetState();
			replaceView("Mainmenu");
		} else if (key == Common::KEYCODE_RETURN) {
			Dialogs::HorizontalInput *hi = dynamic_cast<Dialogs::HorizontalInput *>(_nameInput);
			if (hi) {
				_enteredName = hi->getInput();
				if (!_newCharacter) {
					_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
				}
				_newCharacter->name = _enteredName;
				if (_profileDialog) {
					_profileDialog->drawName();
				}
			}
			if (_activeSubView == static_cast<Dialogs::Dialog *>(_nameInput))
				setActiveSubView(nullptr);
			detachAndDelete(_nameInput);
			setStage(CC_STATE_PORTRAIT);
		}
		break;
	case CC_STATE_PORTRAIT:
		if (key == Common::KEYCODE_k) {
			if (_activeSubView == _portraitSelector)
				setActiveSubView(nullptr);
			detachAndDelete(_portraitSelector);
			setStage(CC_STATE_ICON);
		}
		break;
	case CC_STATE_ICON:
		if (key == Common::KEYCODE_ESCAPE) {
			resetState();
			replaceView("Mainmenu");
		} else if (success && key == Common::KEYCODE_y) {
			// Icon confirmed (Yes in SetIcon confirm dialog) - save and finish
			if (_activeSubView == _iconSelector)
				setActiveSubView(nullptr);
			detachAndDelete(_iconSelector);
			setStage(CC_STATE_DONE);
		}
		// No (KEYCODE_n) is handled inside SetIcon itself - returns to main menu
		break;
	case CC_STATE_DONE:
		replaceView("Mainmenu");
		break;
	default:
		debug(4, "CreateCharacterView::handleMenuResult - unhandled stage %d", (int)_stage);
		break;
	}
}

void CreateCharacterView::resetState() {
	setActiveSubView(nullptr);
	// Delete subviews safely: detach from parent so redraw traversal doesn't
	// see freed memory.
	detachAndDelete(_profileDialog);
	detachAndDelete(_nameInput);
	detachAndDelete(_yesNoPrompt);
	detachAndDelete(_portraitSelector);
	detachAndDelete(_iconSelector);
	detachAndDelete(_listmenu);
	if (_menuItems) {
		delete _menuItems;
		_menuItems = nullptr;
	}
	// Reset character
	if (_newCharacter) { delete _newCharacter; }
	_newCharacter = new Goldbox::Poolrad::Data::PoolradCharacter();
	_newCharacter->initializeNewCharacter();
	// Clear selections and flags
	_selectedRace = _selectedGender = _selectedClass = _selectedAlignment = -1;
	_enteredName.clear();
	_indexMap.clear();
	_hasRolled = false;
	_stage = CC_STATE_RACE;
	// Rebuild initial race menu
	if (_menuItems) { delete _menuItems; _menuItems = nullptr; }
	chooseRace();
}

void CreateCharacterView::saveCharacter() {
	if (!_newCharacter)
		return;

	Common::Path savePath = getLegacySavePath();
	if (savePath.empty()) {
		warning("Failed to resolve legacy save directory for character save");
		return;
	}

	Goldbox::Poolrad::Data::saveNewCharacter(_newCharacter, savePath);
}

void CreateCharacterView::rollAndRecompute() {
	if (!_newCharacter)
		return;
	_newCharacter->rollAbilityScores();
	// Re-initialize levels from classType to preserve multiclass composition
	initLevelsForClassType();
}

// recomputeAfterAlignment was redundant with performRerollAndRecompute; removed as unused

void CreateCharacterView::setThiefSkillsForNewCharacter() {
    if (!_newCharacter)
        return;

    uint8 thiefLevel = 0;
    thiefLevel = _newCharacter->levels[Goldbox::Data::C_THIEF];
    const uint8 race = _newCharacter->race;
    const uint8 dexterity = _newCharacter->abilities.dexterity.current;
	//"Thief skills for race %d, dex: %d, level: %d", race, dexterity, thiefLevel);
    // Compute final thief skills via rules
    _newCharacter->thiefSkills =
        Goldbox::Data::Rules::computeThiefSkills(race, dexterity, thiefLevel);
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

	Goldbox::Data::Rules::applyStatMinMax(_newCharacter->race, _newCharacter->gender,
		_newCharacter->classType, _newCharacter->levels, _newCharacter->abilities);
}

void CreateCharacterView::applySpells() {
	if (!_newCharacter)
		return;
	// Compute spell slots and initialize known spells based on character class:
	// - Clerics automatically know ALL cleric spells of levels they can cast (L1-3 initially)
	// - Magic-Users start with only 4 level 1 spells and must find/scribe others
	// - Multiclass characters (e.g., Cleric/Magic-User) get both spell types
	// The SpellBook now tracks all spells by ID with separate known/memorized counts
	// and provides class-specific query methods to distinguish cleric vs magic-user spells
	_newCharacter->computeSpellSlots();

	debug(4, "CreateCharacterView::applySpells - Character has %u cleric spells, %u magic-user spells known",
	      _newCharacter->spellBook.countKnownByClass(Goldbox::Data::Spells::SC_CLERIC),
	      _newCharacter->spellBook.countKnownByClass(Goldbox::Data::Spells::SC_MAGICUSER));
}

void CreateCharacterView::setInitGold() {
	if (!_newCharacter)
		return;

	uint16 finalGold = Goldbox::Data::Rules::rollInitialGold(_newCharacter->levels);
	_newCharacter->valuableItems[Goldbox::Data::VAL_GOLD] = finalGold;
	if (finalGold == 0) {
		warning("setInitGold: no base classes with level > 0; gold set to 0. classType=%u", (unsigned)_newCharacter->classType);
	}
}

void CreateCharacterView::setInitHP() {
	if (!_newCharacter)
		return;

	// Step 1: roll raw HP for levels beyond 1 across all active base classes
	debug(4, "rollHP: calling with CF_ALL; pre-rolled=%u", (unsigned)_newCharacter->hitPointsRolled);
	_newCharacter->hitPointsRolled = _newCharacter->getRolledHP(Goldbox::Data::CF_ALL);
	// Align max HP with raw rolled HP first (pre-Con averaging)
	_newCharacter->hitPoints.max = _newCharacter->hitPointsRolled;
	debug(4, "rollHP: post-rolled=%u max=%u (pre-Con)", (unsigned)_newCharacter->hitPointsRolled, (unsigned)_newCharacter->hitPoints.max);

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
	debug(4, "setInitHP: classes=%d conSum=%d -> max=%u cur=%u rolled(avgRaw)=%u",
		classCount, conSum,
		(unsigned)_newCharacter->hitPoints.max,
		(unsigned)_newCharacter->hitPoints.current,
		(unsigned)_newCharacter->hitPointsRolled);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
