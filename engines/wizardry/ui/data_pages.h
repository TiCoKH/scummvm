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

#ifndef WIZARDRY_UI_DATA_PAGES_H
#define WIZARDRY_UI_DATA_PAGES_H

#include "wizardry/game/items.h"
#include "wizardry/game/maze.h"
#include "wizardry/game/monsters.h"
#include "wizardry/gfx/text_window.h"
#include "wizardry/resources/messages.h"

namespace Wizardry {

/**
 * Pages of the debug gallery that show the data of the scenario: one
 * item, one monster, the list of spells, and a map of a maze level. They
 * only draw; the first row of the window is the heading, and the last
 * row is left free for the gallery's line of keys.
 */
class DataPages {
public:
	static void drawItem(TextWindow &window, const MessageFile &messages, const ItemList &items, uint id);
	static void drawMonster(TextWindow &window, const MessageFile &messages, const MonsterList &monsters, uint id);

	/**
	 * The spells by number, from the first one given, as many as fit in
	 * two columns
	 */
	static void drawSpells(TextWindow &window, const MessageFile &messages, int first);

	/** How many spells drawSpells() shows at a time */
	static int spellsPerPage(const TextWindow &window);

	/**
	 * A map of a level: a character for each kind of square, north at the
	 * top, '@' at the cursor, and beside it what there is to know of the
	 * square at the cursor
	 * @param level   Level number, from 1
	 * @param levels  Number of levels
	 */
	static void drawMaze(TextWindow &window, const MazeLevel &maze, int level, int levels, int x, int y);
};

} // End of namespace Wizardry

#endif
