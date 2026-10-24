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

#ifndef GOLDBOX_POOLRAD_VIEWS_MODIFY_CHARACTER_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_MODIFY_CHARACTER_VIEW_H

#include "goldbox/poolrad/views/view.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

/**
 * Allows experience-free party characters to adjust their ability scores and HP.
 */
class ModifyCharacterView : public View {
public:
	ModifyCharacterView();
	~ModifyCharacterView() override;

	bool msgFocus(const FocusMessage &msg) override;
	void draw() override;
	bool msgKeypress(const KeypressMessage &msg) override;
	void handleMenuResult(const MenuResultMessage &result) override;

private:
	struct CharacterBackup {
		uint8 abilities[6];
		uint8 baseAbilities[6];
		uint8 strengthException;
		uint8 baseStrengthException;
		uint8 maxHitPoints;
	};

	Goldbox::Poolrad::Data::PoolradCharacter *_character;
	Dialogs::CharacterProfile *_profileDialog;
	Dialogs::HorizontalMenu *_horizontalMenu;
	Goldbox::MenuItemList _menuItems;
	CharacterBackup _backup;
	uint8 _selectedStat;
	bool _canModify;
	bool _menuActive;

	void activateMenu();
	void finishModification(bool commit);
	void adjustSelectedStat(int adjustment);
	void adjustAbility(int index, int adjustment);
	void restoreCharacter();
	void refreshCharacter();
	uint8 getAverageHitDieWithCon() const;
	uint8 getAverageLevelWithCon() const;
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
