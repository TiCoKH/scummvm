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

#ifndef WIZARDRY_VIEWS_TOWN_TRAINING_H
#define WIZARDRY_VIEWS_TOWN_TRAINING_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * The training grounds: making characters and listing the roster
 */
class TrainingView : public TownView {
private:
	enum State {
		kMenu,
		kRoster,          ///< the list, until RETURN
		kRosterFull,      ///< a notice, until RETURN
		kName,
		kNameInUse,       ///< a notice, until RETURN
		kPassword,
		kPasswordAgain,
		kRace,
		kAlignment,
		kPoints,          ///< sharing out the bonus points
		kClass,
		kKeep,            ///< keep this character, yes or no

		// Looking after a character that exists
		kEditChoose,      ///< which character, by letter
		kEditPassword,    ///< its password, if it has one
		kEditMenu,
		kEditDelete,      ///< sure, yes or no
		kEditClass,       ///< which class to change to
		kEditNoClass,     ///< a notice, until RETURN
		kEditNewPassword,
		kEditNewPasswordAgain,
		kEditPasswordResult   ///< until RETURN
	};

	State _state = kMenu;
	int _slot = 0;                    ///< place in the roster for the new character
	Character _draft;
	Common::String _input;            ///< what is being typed
	Common::String _password;
	uint _marks = 0;                  ///< marks shown for a password
	int16 _attributes[6];
	int _bonus = 0;
	int _cursor = 0;
	int _class = 0;

	Common::Array<uint> _listed;      ///< roster numbers behind the letters
	uint _letter = 0;
	uint _edited = 0;                 ///< roster number of the character looked after
	uint _marks2 = 0;                 ///< marks of the second password typed
	bool _passwordChanged = false;

	void startCreate();
	void startEdit();
	void openClasses(bool *open) const;
	void typeInto(uint16 ascii, bool hidden);
	void adjust(int by);
	bool anyClassOpen() const;

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;

public:
	TrainingView() : TownView("Town.Training") {}

	bool msgFocus(const FocusMessage &msg) override;

protected:
	void noticeDone() override;
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
