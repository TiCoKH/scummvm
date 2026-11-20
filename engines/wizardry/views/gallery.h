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

#ifndef WIZARDRY_VIEWS_GALLERY_H
#define WIZARDRY_VIEWS_GALLERY_H

#include "wizardry/game/maze.h"
#include "wizardry/views/tile_view.h"

namespace Wizardry {

/**
 * A debug view, reached with M on the title: pages through the pictures
 * of the release, its character set and messages, and the data of the
 * scenario (items, monsters, spells and maze levels), all drawn through
 * the tile screen.
 */
class Gallery : public TileView {
private:
	enum Page {
		kPagePictures,
		kPageCharset,
		kPageMessages,
		kPageItems,
		kPageMonsters,
		kPageSpells,
		kPageMaze,
		kPageCount
	};

	int _page = kPagePictures;
	uint _picture = 1;
	uint _firstMessage = 0;
	uint _item = 0;
	uint _monster = 0;
	int _firstSpell = 1;
	int _mazeLevel = 1;
	int _mazeX = 0, _mazeY = 0;
	Common::Array<MazeLevel> _levels;   ///< the levels as the scenario has them, when no game has loaded them

	const Common::Array<MazeLevel> &levels();
	void drawMaze(TextWindow &window);

	/** Moves an index by a step, wrapping round within a count */
	static uint step(uint value, int by, uint count);

	void drawPictures(TextWindow &window);
	void drawCharset(TextWindow &window);
	void drawMessages(TextWindow &window);

protected:
	void drawTiles(TextWindow &window) override;

public:
	Gallery() : TileView("Gallery", true) {}

	bool msgKeypress(const KeypressMessage &msg) override;
};

} // End of namespace Wizardry

#endif
