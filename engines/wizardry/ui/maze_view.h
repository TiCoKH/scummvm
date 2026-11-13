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

#ifndef WIZARDRY_UI_MAZE_VIEW_H
#define WIZARDRY_UI_MAZE_VIEW_H

#include "wizardry/game/maze.h"
#include "wizardry/gfx/text_window.h"

namespace Wizardry {

/**
 * Draws the view into the maze as the original does: with glyphs of the
 * character set, into the 36 x 20 cells inside the main window. Up to four
 * squares ahead are shown with light, two without.
 *
 * The lists of runs of glyphs are those of the original program, one list
 * for each depth and for a wall, an opening or a door on each side.
 *
 * Not drawn yet: the marks on floor and ceiling for stairs, elevators
 * and special squares.
 */
class MazeViewDrawer {
public:
	enum {
		kWidth = 36,
		kHeight = 20
	};

private:
	TextWindow &_window;
	const MazeLevel &_level;
	const MazeState &_state;
	bool _lit;

	// The view point and what was found around it
	int _x, _y;
	bool _squareLit;           ///< false in a dark square that light does not reach
	uint _front, _left, _right;   ///< wall values; for an open side, the wall beyond it
	bool _leftWall, _rightWall;   ///< a wall or door on that side of the square
	bool _aheadVisible;
	bool _leftVisible, _rightVisible;
	bool _leftAheadVisible, _rightAheadVisible;

	// Columns still to be drawn into: what lies outside is hidden
	// behind nearer walls
	int _clipLeft, _clipRight;
	bool _done;

	void run(int x, int y, int dx, int dy, int count, uint16 glyph);
	void cell(int x, int y, uint16 glyph) { run(x, y, 0, 0, 1, glyph); }
	void dark(int x, int y, int columns, int rows);
	void slantDown(int x, int y, int columns, int rows, int clipLeft);
	void slantUp(int x, int y, int columns, int rows, int clipRight);

	bool squareVisible(int right, int forward) const;
	uint wallAt(int right, int side) const;
	uint showHidden(uint wall) const;
	void advance();
	void scanWalls();

	void depth0();
	void depth1();
	void depth2();
	void depth3();

public:
	MazeViewDrawer(TextWindow &window, const MazeLevel &level, const MazeState &state, bool lit);

	void draw();
};

} // End of namespace Wizardry

#endif
