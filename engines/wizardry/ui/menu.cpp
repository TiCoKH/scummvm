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

#include "wizardry/ui/menu.h"

namespace Wizardry {

int Menu::textWidth(const Common::String &options) {
	int widest = 0, width = 0;

	for (uint i = 0; i < options.size(); ++i) {
		if (options[i] == '/') {
			widest = MAX(widest, width);
			width = 0;
		} else {
			++width;
		}
	}

	return MAX(widest, width);
}

void Menu::print(TextWindow &window, const Common::String &options) {
	_items.clear();
	_defaultResult = -1;

	const int firstRow = window.cursorY();
	int column = window.cursorX();
	int widest = -1;
	int result = 0;
	int markCountdown = -1;
	bool startOfOption = true;

	for (uint i = 0; i < options.size(); ++i) {
		byte c = options[i];

		if (c == '/') {
			widest = MAX(widest, window.cursorX());
			if (!_items.empty())
				_items.back().width = window.cursorX() - column;

			window.newLine();
			if (window.cursorY() == 0) {
				// Window full: carry on to the right of what is there
				window.MVCURSOR(0, firstRow);
				column = widest + 1;
			}
			startOfOption = true;
			continue;
		}

		if (startOfOption) {
			window.MVCURSOR(column, window.cursorY());

			if (c > 0x60) {
				if (_defaultResult < 0) {
					_defaultResult = result;
					markCountdown = 1;
				}
				c -= 0x20;
			}

			Item item;
			item.key = c;
			item.col = window.innerLeft() + window.cursorX();
			item.row = window.innerTop() + window.cursorY();
			item.width = 0;
			item.result = result;
			item.party = (c == '#');
			_items.push_back(item);

			result += item.party ? kPartySlots : 1;
			startOfOption = false;
		}

		window.PRINTCHR(markCountdown == 0 ? (uint16)kReturnGlyph : c);
		if (markCountdown >= 0)
			--markCountdown;
	}

	if (!_items.empty())
		_items.back().width = window.cursorX() - column;
}

int Menu::choose(uint16 ascii, int partyCount) const {
	if (ascii == 13)
		return _defaultResult;

	for (uint i = 0; i < _items.size(); ++i) {
		const Item &item = _items[i];

		if (item.party) {
			if (ascii >= '1' && ascii <= '0' + partyCount)
				return item.result + ascii - '1';
		} else if (item.key == ascii) {
			return item.result;
		}
	}

	return -1;
}

int Menu::chooseAt(int col, int row) const {
	for (uint i = 0; i < _items.size(); ++i) {
		const Item &item = _items[i];
		if (!item.party && row == item.row && col >= item.col && col < item.col + item.width)
			return item.result;
	}

	return -1;
}

} // End of namespace Wizardry
