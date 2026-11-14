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

#ifndef WIZARDRY_VIEWS_TOWN_SHEET_H
#define WIZARDRY_VIEWS_TOWN_SHEET_H

#include "wizardry/views/town/town_view.h"

namespace Wizardry {
namespace Town {

/**
 * The character sheet, as opened from the tavern and from the training
 * grounds: statistics, spells and the pack, and what can be done with them.
 */
class SheetView : public TownView {
public:
	enum Mode {
		kFromTraining,   ///< read the spell books, or leave
		kFromTavern,     ///< also equip, trade, drop, pool gold and identify
		kFromCamp        ///< all of that, and for the healthy casting spells and using items
	};

private:
	enum State {
		kMenu,
		kDropWhich,
		kTradeWho,
		kTradeGold,
		kTradeItem,
		kSpellName,     ///< typing the name of a spell to cast
		kSpellTarget,   ///< whom the spell is for
		kSaid           ///< a notice in the small window, then on
	};

	Mode _mode = kFromTavern;
	uint _rosterIndex = 0;
	State _state = kMenu;
	State _afterSaid = kMenu;
	Common::String _said;
	Common::String _input;
	uint _receiver = 0;   ///< party member who gets what is traded
	int _spell = 0;

	bool fullMenu() const;
	void castSpell(uint target);
	void sayText(const Common::String &text, State next);

	Character &character();
	void say(uint message, State next);

protected:
	void drawBackground(TownScreens &screens) override;
	void drawTown(TownScreens &screens) override;
	void choose(int result, uint16 ascii) override;
	void timeout() override;

public:
	SheetView() : TownView("Town.Sheet") {}

	/**
	 * Opens the sheet of a character of the roster over the current view
	 */
	static void open(uint rosterIndex, Mode mode);
};

} // End of namespace Town
} // End of namespace Wizardry

#endif
