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

#ifndef WIZARDRY_UI_MENU_H
#define WIZARDRY_UI_MENU_H

#include "common/array.h"
#include "wizardry/gfx/text_window.h"

namespace Wizardry {

/**
 * A menu as the original program builds it from one string. The options
 * are separated by '/', and an option is chosen by typing its first
 * character:
 *
 *   A)DD/R)EMOVE/#)INSPECT/D)IVVY GOLD/l)EAVE
 *
 * - An option that starts with a lower case letter is the one RETURN
 *   chooses. It is shown in upper case and the character after its first
 *   letter is shown as the RETURN symbol (glyph 126).
 * - An option that starts with '#' stands for the party members: it is
 *   chosen with the digits 1 to the size of the party and takes six places
 *   in the numbering of the results.
 * - Options go one below the other from the cursor. When the window is
 *   full, they go on in a new column to the right.
 */
class Menu {
public:
	enum {
		kPartySlots = 6,
		kReturnGlyph = 126
	};

private:
	struct Item {
		byte key;
		int16 col, row;   ///< screen cell where the option starts
		int16 width;
		int16 result;     ///< what choosing the option returns
		bool party;       ///< the '#' option
	};

	Common::Array<Item> _items;
	int _defaultResult = -1;

public:
	/**
	 * Width in cells of the widest option
	 */
	static int textWidth(const Common::String &options);

	/**
	 * Prints the options at the cursor of the window and remembers where
	 * they are
	 */
	void print(TextWindow &window, const Common::String &options);

	bool empty() const { return _items.empty(); }

	/**
	 * Returns the result for a typed character, or -1 if it chooses nothing
	 * @param ascii       The character; letters in upper case, RETURN as 13
	 * @param partyCount  Number of party members the '#' option accepts
	 */
	int choose(uint16 ascii, int partyCount) const;

	/**
	 * Returns the result for the option at a screen cell, or -1. The '#'
	 * option cannot be chosen this way.
	 */
	int chooseAt(int col, int row) const;
};

} // End of namespace Wizardry

#endif
