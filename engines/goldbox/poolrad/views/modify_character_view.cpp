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

#include "goldbox/poolrad/views/modify_character_view.h"

#include "common/keyboard.h"
#include "goldbox/data/rules/rules.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

namespace {

static uint8 clampToByte(int value) {
	if (value < 0)
		return 0;
	if (value > 255)
		return 255;
	return static_cast<uint8>(value);
}

static Goldbox::Data::Stat *getAbility(
		Goldbox::Poolrad::Data::PoolradCharacter &character, int index) {
	switch (index) {
	case 0: return &character.abilities.strength;
	case 1: return &character.abilities.intelligence;
	case 2: return &character.abilities.wisdom;
	case 3: return &character.abilities.dexterity;
	case 4: return &character.abilities.constitution;
	case 5: return &character.abilities.charisma;
	default: return nullptr;
	}
}

static uint8 getRaceMinimum(const Goldbox::Data::RaceStatMinMax &raceStats,
		Goldbox::Poolrad::Data::PoolradCharacter &character, int index) {
	switch (index) {
	case 0:
		return character.gender == Goldbox::Data::G_FEMALE ?
			raceStats.strengthMinFemale : raceStats.strengthMinMale;
	case 1: return raceStats.intelligenceMin;
	case 2: return raceStats.wisdomMin;
	case 3: return raceStats.dexterityMin;
	case 4: return raceStats.constitutionMin;
	case 5: return raceStats.charismaMin;
	default: return 0;
	}
}

static uint8 getClassMinimum(const Goldbox::Data::ClassMinStats &classStats,
		int index) {
	switch (index) {
	case 0: return classStats.strength;
	case 1: return classStats.intelligence;
	case 2: return classStats.wisdom;
	case 3: return classStats.dexterity;
	case 4: return classStats.constitution;
	case 5: return classStats.charisma;
	default: return 0;
	}
}

static uint8 getRaceMaximum(const Goldbox::Data::RaceStatMinMax &raceStats,
		Goldbox::Poolrad::Data::PoolradCharacter &character, int index) {
	switch (index) {
	case 0:
		return character.gender == Goldbox::Data::G_FEMALE ?
			raceStats.strengthMaxFemale : raceStats.strengthMaxMale;
	case 1: return raceStats.intelligenceMax;
	case 2: return raceStats.wisdomMax;
	case 3: return raceStats.dexterityMax;
	case 4: return raceStats.constitutionMax;
	case 5: return raceStats.charismaMax;
	default: return 0;
	}
}

static void clampMaxHitPoints(Goldbox::Poolrad::Data::PoolradCharacter &character,
		uint8 limit) {
	if (character.hitPoints.max > limit)
		character.hitPoints.max = limit;
}

} // namespace

ModifyCharacterView::ModifyCharacterView()
		: View("ModifyCharacter"), _character(nullptr),
		  _profileDialog(new Dialogs::CharacterProfile("ModifyCharacterProfile")),
		  _horizontalMenu(nullptr), _selectedStat(0), _canModify(false),
		  _menuActive(false) {
	Common::Array<Common::String> menuOptions;
	menuOptions.push_back("Keep");
	menuOptions.push_back("Exit");
	_menuItems.generateMenuItems(menuOptions, true);

	Dialogs::HorizontalMenuConfig menuConfig = {
		"Modify character: ",
		&_menuItems,
		10,
		15,
		13,
		true,
		true,
		0,
		false
	};
	_horizontalMenu = new Dialogs::HorizontalMenu(
		"ModifyCharacterMenu", menuConfig);

	attachDialog(_profileDialog);
	attachDialog(_horizontalMenu);
}

ModifyCharacterView::~ModifyCharacterView() {
	detachDialog(_horizontalMenu);
	delete _horizontalMenu;
	detachDialog(_profileDialog);
	delete _profileDialog;
}

bool ModifyCharacterView::msgFocus(const FocusMessage &msg) {
	View::msgFocus(msg);

	_character = static_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
		VmInterface::getSelectedCharacter());
	_canModify = _character && _character->experiencePoints == 0;

	if (_character) {
		_backup.abilities[0] = _character->abilities.strength.current;
		_backup.abilities[1] = _character->abilities.intelligence.current;
		_backup.abilities[2] = _character->abilities.wisdom.current;
		_backup.abilities[3] = _character->abilities.dexterity.current;
		_backup.abilities[4] = _character->abilities.constitution.current;
		_backup.abilities[5] = _character->abilities.charisma.current;
		_backup.baseAbilities[0] = _character->abilities.strength.base;
		_backup.baseAbilities[1] = _character->abilities.intelligence.base;
		_backup.baseAbilities[2] = _character->abilities.wisdom.base;
		_backup.baseAbilities[3] = _character->abilities.dexterity.base;
		_backup.baseAbilities[4] = _character->abilities.constitution.base;
		_backup.baseAbilities[5] = _character->abilities.charisma.base;
		_backup.strengthException =
			_character->abilities.strException.current;
		_backup.baseStrengthException =
			_character->abilities.strException.base;
		_backup.maxHitPoints = _character->hitPoints.max;
		VmInterface::setSelectedCharacter(_character);
		_profileDialog->activate();
		if (_canModify)
			_profileDialog->setHighlightedStat(_selectedStat);
		if (_canModify)
			activateMenu();
	}

	_selectedStat = 0;
	return true;
}

void ModifyCharacterView::draw() {
	if (!_character) {
		drawWindow(1, 1, 38, 22);
		Surface surface = getSurface();
		surface.writeStringC(1, 2, 12, "No selected character to modify.");
		surface.writeStringC(1, 24, 13, "Press Esc to return.");
		return;
	}

	_profileDialog->draw();
	Surface surface = getSurface();

	if (_canModify) {
		if (_menuActive)
			_horizontalMenu->draw();
	} else {
		surface.clearBox(0, 23, 39, 24, 0);
		surface.writeStringC(0, 24, 12,
			"Character cannot be modified (0 experience required).");
	}
}

bool ModifyCharacterView::msgKeypress(const KeypressMessage &msg) {
	if (!_character) {
		if (msg.keycode == Common::KEYCODE_ESCAPE ||
				msg.keycode == Common::KEYCODE_RETURN) {
			close();
		}
		return true;
	}

	if (!_canModify) {
		if (msg.keycode == Common::KEYCODE_ESCAPE) {
			close();
		}
		return true;
	}

	return _menuActive ? _horizontalMenu->send(msg) :
		View::msgKeypress(msg);
}

void ModifyCharacterView::handleMenuResult(
		const MenuResultMessage &result) {
	if (!_character || !_canModify)
		return;

	if (!result._success || result._keyCode == Common::KEYCODE_ESCAPE) {
		finishModification(false);
		return;
	}

	if (result._keyCode == Common::KEYCODE_k ||
			(result._keyCode == Common::KEYCODE_SPACE &&
				result._hasIntValue && result._intValue == 0)) {
		finishModification(true);
		return;
	}

	if (result._keyCode == Common::KEYCODE_e ||
			(result._keyCode == Common::KEYCODE_SPACE &&
				result._hasIntValue && result._intValue == 1)) {
		finishModification(false);
		return;
	}

	switch (result._keyCode) {
	case Common::KEYCODE_DOWN:
	case Common::KEYCODE_RETURN:
		_selectedStat = (_selectedStat + 1) % 7;
		break;
	case Common::KEYCODE_UP:
		_selectedStat = _selectedStat == 0 ? 6 : _selectedStat - 1;
		break;
	case Common::KEYCODE_LEFT:
		adjustSelectedStat(-1);
		break;
	case Common::KEYCODE_RIGHT:
		adjustSelectedStat(1);
		break;
	default:
		activateMenu();
		return;
	}

	_profileDialog->setHighlightedStat(_selectedStat);
	redraw();
	activateMenu();
}

void ModifyCharacterView::activateMenu() {
	_menuActive = true;
	_horizontalMenu->setRedraw();
	_horizontalMenu->activate();
}

void ModifyCharacterView::finishModification(bool commit) {
	_menuActive = false;
	_horizontalMenu->deactivate();
	if (commit) {
		_character->modified = 1;
		refreshCharacter();
	} else {
		restoreCharacter();
	}
	close();
}

void ModifyCharacterView::adjustSelectedStat(int adjustment) {
	if (_selectedStat == 6) {
		int hp = (int)_character->hitPoints.max + adjustment;
		if (adjustment < 0) {
			const uint8 minimum = getAverageLevelWithCon();
			if (hp < minimum)
				hp = minimum;
		} else {
			const uint8 maximum = getAverageHitDieWithCon();
			if (hp > maximum)
				hp = maximum;
		}
		_character->hitPoints.max = clampToByte(hp);
		_character->hitPoints.current = _character->hitPoints.max;
	} else {
		adjustAbility(_selectedStat, adjustment);
	}

	refreshCharacter();
}

void ModifyCharacterView::adjustAbility(int index, int adjustment) {
	Goldbox::Data::Stat *ability = getAbility(*_character, index);
	if (!ability)
		return;

	const Goldbox::Data::RaceStatMinMax &raceStats =
		Goldbox::Data::Rules::getRaceStatMinMaxForRace(_character->race);
	const Goldbox::Data::ClassMinStats &classStats =
		Goldbox::Data::Rules::getClassMinStats(_character->classType);

	ability->current = clampToByte((int)ability->current + adjustment);
	ability->base = ability->current;

	if (adjustment < 0) {
		if (index == 0) {
			if (_character->abilities.strException.current == 0) {
				const uint8 raceMin = getRaceMinimum(raceStats,
					*_character, 0);
				if (_character->abilities.strength.current < raceMin ||
						_character->abilities.strength.current <
							classStats.strength) {
					_character->abilities.strength.current = raceMin;
					_character->abilities.strength.base = raceMin;
				}
			} else {
				--_character->abilities.strException.current;
				_character->abilities.strException.base =
					_character->abilities.strException.current;
				++_character->abilities.strength.current;
				++_character->abilities.strength.base;
			}
		} else {
			const uint8 raceMinimum =
				getRaceMinimum(raceStats, *_character, index);
			const uint8 classMinimum = getClassMinimum(classStats, index);
			if (ability->current < raceMinimum ||
					ability->current < classMinimum) {
				ability->current = raceMinimum;
				ability->base = raceMinimum;
			}
		}

		if (index == 4) {
			clampMaxHitPoints(*_character, getAverageHitDieWithCon());
			_character->hitPoints.current = _character->hitPoints.max;
		}
	} else {
		const uint8 raceMaximum = getRaceMaximum(raceStats, *_character, index);
		if (ability->current > raceMaximum) {
			if (index == 0 && ability->current > 18 &&
					_character->levels.levels[Goldbox::Data::C_FIGHTER] > 0) {
				const uint8 exceptionalMaximum =
					_character->gender == Goldbox::Data::G_FEMALE ?
					raceStats.extStrengthMaxFemale :
					raceStats.extStrengthMaxMale;
				if (_character->abilities.strException.current <
						exceptionalMaximum) {
					++_character->abilities.strException.current;
					_character->abilities.strException.base =
						_character->abilities.strException.current;
				}
			}
			ability->current = raceMaximum;
			ability->base = raceMaximum;
		}

		if (index == 4) {
			clampMaxHitPoints(*_character, getAverageLevelWithCon());
			_character->hitPoints.current = _character->hitPoints.max;
		}
	}
}

void ModifyCharacterView::restoreCharacter() {
	_character->abilities.strength.current = _backup.abilities[0];
	_character->abilities.strength.base = _backup.baseAbilities[0];
	_character->abilities.intelligence.current = _backup.abilities[1];
	_character->abilities.intelligence.base = _backup.baseAbilities[1];
	_character->abilities.wisdom.current = _backup.abilities[2];
	_character->abilities.wisdom.base = _backup.baseAbilities[2];
	_character->abilities.dexterity.current = _backup.abilities[3];
	_character->abilities.dexterity.base = _backup.baseAbilities[3];
	_character->abilities.constitution.current = _backup.abilities[4];
	_character->abilities.constitution.base = _backup.baseAbilities[4];
	_character->abilities.charisma.current = _backup.abilities[5];
	_character->abilities.charisma.base = _backup.baseAbilities[5];
	_character->abilities.strException.current = _backup.strengthException;
	_character->abilities.strException.base = _backup.baseStrengthException;
	_character->hitPoints.max = _backup.maxHitPoints;
	_character->hitPoints.current = _backup.maxHitPoints;
	refreshCharacter();
}

void ModifyCharacterView::refreshCharacter() {
	_character->recalcCombatStats();
	_character->addClericBonusSpells();
	_profileDialog->redrawStats();
	_profileDialog->redrawCombat();
	redraw();
}

uint8 ModifyCharacterView::getAverageHitDieWithCon() const {
	const int classCount = _character->countActiveBaseClasses();
	if (classCount == 0)
		return 1;

	int hitPoints = 0;
	for (uint i = 0; i < BASE_CLASS_NUM &&
			i < _character->levels.levels.size(); ++i) {
		if (_character->levels.levels[i] == 0)
			continue;
		const Goldbox::Data::DiceRoll &hitDie =
			Goldbox::Data::Rules::getHPRoll(static_cast<uint8>(i));
		hitPoints += (hitDie.diceSides + 1) / 2;
	}
	hitPoints += _character->getConHPModifier();
	hitPoints /= classCount;

	return clampToByte(hitPoints < 1 ? 1 : hitPoints);
}

uint8 ModifyCharacterView::getAverageLevelWithCon() const {
	const int classCount = _character->countActiveBaseClasses();
	if (classCount == 0)
		return 1;

	int hitPoints = 0;
	for (uint i = 0; i < BASE_CLASS_NUM &&
			i < _character->levels.levels.size(); ++i) {
		const uint8 level = _character->levels.levels[i];
		if (level == 0)
			continue;
		const Goldbox::Data::DiceRoll &hitDie =
			Goldbox::Data::Rules::getHPRoll(static_cast<uint8>(i));
		hitPoints += ((hitDie.diceSides + 1) / 2) * level;
	}
	hitPoints += _character->getConHPModifier();
	hitPoints /= classCount;

	return clampToByte(hitPoints < 1 ? 1 : hitPoints);
}

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
