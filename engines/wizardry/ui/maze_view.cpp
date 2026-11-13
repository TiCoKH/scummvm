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

#include "wizardry/ui/maze_view.h"

namespace Wizardry {

// Glyphs of the view
enum {
	kTop = 9,           // edge along the top of a wall
	kBottom = 10,
	kRightEdge = 11,    // upright edge of a wall on the right
	kLeftEdge = 12,
	kSlantA = 13,       // the two slanting edges
	kSlantB = 14,
	kDarkness = 0x67
};

MazeViewDrawer::MazeViewDrawer(TextWindow &window, const MazeLevel &level, const MazeState &state, bool lit) :
		_window(window), _level(level), _state(state), _lit(lit),
		_x(state.x), _y(state.y), _squareLit(true), _front(0), _left(0), _right(0),
		_leftWall(false), _rightWall(false), _aheadVisible(true),
		_leftVisible(true), _rightVisible(true), _leftAheadVisible(true), _rightAheadVisible(true),
		_clipLeft(0), _clipRight(kWidth - 1), _done(false) {
}

void MazeViewDrawer::run(int x, int y, int dx, int dy, int count, uint16 glyph) {
	// As the display driver does it: a cell is drawn if its column lies
	// within the limits, and the run ends at the edge of the view
	for (; count > 0; --count) {
		if (x >= _clipLeft && x <= _clipRight) {
			_window.MVCURSOR(x, y);
			_window.PRINTCHR(glyph);
		}

		x += dx;
		y += dy;
		if (x < 0 || y < 0 || x == kWidth || y == kHeight)
			break;
	}
}

void MazeViewDrawer::dark(int x, int y, int columns, int rows) {
	for (int i = 0; i < columns; ++i)
		run(x + i, y, 0, 1, rows, kDarkness);
}

void MazeViewDrawer::slantDown(int x, int y, int columns, int rows, int clipLeft) {
	for (int i = 0; i < columns; ++i, ++y, rows -= 2) {
		cell(x + i, y, 0x68);
		run(x + i, y + 1, 0, 1, rows - 2, kDarkness);
		cell(x + i, y + rows - 1, 0x6a);
	}
	_clipLeft = clipLeft;
}

void MazeViewDrawer::slantUp(int x, int y, int columns, int rows, int clipRight) {
	for (int i = 0; i < columns; ++i, --y, rows += 2) {
		cell(x + i, y, 0x66);
		run(x + i, y + 1, 0, 1, rows - 2, kDarkness);
		cell(x + i, y + rows - 1, 0x69);
	}
	_clipRight = clipRight;
}

bool MazeViewDrawer::squareVisible(int right, int forward) const {
	int x = _x, y = _y;
	Maze::offset(x, y, _state.facing, right, forward);

	// A dark square shows only if its third number says so
	const uint index = _level.squareIndex(x, y);
	if (_level.squareType(index) == kSquareDark)
		return _level.aux(2, index) != 0;
	return true;
}

uint MazeViewDrawer::wallAt(int right, int side) const {
	int x = _x, y = _y;
	Maze::offset(x, y, _state.facing, right, 0);
	return _level.wall(x, y, (_state.facing + side) & 3);
}

uint MazeViewDrawer::showHidden(uint wall) const {
	// A hidden door shows as a door only with light, and not in darkness
	if (wall == kHiddenDoor)
		return (_squareLit && _lit) ? kDoor : kWall;
	return wall;
}

void MazeViewDrawer::scanWalls() {
	_front = showHidden(wallAt(0, 0));
	_left = showHidden(wallAt(0, 3));
	_right = showHidden(wallAt(0, 1));
	_aheadVisible = squareVisible(0, 1);
	_leftVisible = _rightVisible = _leftAheadVisible = _rightAheadVisible = true;

	// Through an open side one sees the far wall of the square beside
	if (_left == kOpen) {
		_left = showHidden(wallAt(-1, 0));
		_leftVisible = squareVisible(-1, 0);
		if (_leftVisible && _left == kOpen)
			_leftAheadVisible = squareVisible(-1, 1);
		_leftWall = false;
	} else {
		_leftWall = true;
	}

	if (_right == kOpen) {
		_right = showHidden(wallAt(1, 0));
		_rightVisible = squareVisible(1, 0);
		if (_rightVisible && _right == kOpen)
			_rightAheadVisible = squareVisible(1, 1);
		_rightWall = false;
	} else {
		_rightWall = true;
	}
}

void MazeViewDrawer::advance() {
	Maze::offset(_x, _y, _state.facing, 0, 1);
	_squareLit = true;

	const uint index = _level.squareIndex(_x, _y);
	const uint type = _level.squareType(index);
	if (type == kSquareDark) {
		_squareLit = (_level.aux(0, index) == 1);
	} else if (type == kSquareTransfer && _level.aux(0, index) == _state.level) {
		// A transfer within the level is looked through to where it leads
		_x = _level.aux(2, index);
		_y = _level.aux(1, index);
	}

	scanWalls();
}

void MazeViewDrawer::depth0() {
	// The left side
	if (_leftWall) {
		run(1, 0, 0, 1, 20, kLeftEdge);
		run(8, 0, 1, 1, 2, kSlantB);
		run(9, 2, 0, 1, 16, kLeftEdge);
		run(9, 18, -1, 1, 2, kSlantA);
		if (_left == kDoor) {
			run(3, 0, 0, 1, 20, kLeftEdge);
			run(6, 0, 1, 1, 2, kSlantB);
			run(7, 2, 0, 1, 18, kLeftEdge);
		}
		_clipLeft = 10;
	} else if (!_leftVisible) {
		dark(0, 0, 8, 20);
		slantDown(8, 0, 2, 20, 10);
	} else if (_left == kOpen) {
		if (!_leftAheadVisible) {
			dark(0, 2, 10, 16);
			_clipLeft = 10;
		}
	} else {
		run(0, 1, 1, 0, 10, kTop);
		run(0, 18, 1, 0, 10, kBottom);
		run(9, 2, 0, 1, 16, kLeftEdge);
		if (_left == kDoor) {
			run(0, 3, 1, 0, 8, kTop);
			run(7, 4, 0, 1, 14, kLeftEdge);
		}
		_clipLeft = 10;
	}

	// The right side
	if (_rightWall) {
		run(34, 0, 0, 1, 20, kRightEdge);
		run(27, 0, -1, 1, 2, kSlantA);
		run(26, 2, 0, 1, 16, kRightEdge);
		run(27, 19, -1, -1, 2, kSlantB);
		if (_right == kDoor) {
			run(32, 0, 0, 1, 20, kRightEdge);
			run(29, 0, -1, 1, 2, kSlantA);
			run(28, 2, 0, 1, 18, kRightEdge);
		}
		_clipRight = 25;
	} else if (!_rightVisible) {
		dark(28, 0, 8, 20);
		slantUp(26, 1, 2, 18, 25);
	} else if (_right == kOpen) {
		if (!_rightAheadVisible) {
			dark(26, 2, 10, 16);
			_clipRight = 25;
		}
	} else {
		run(26, 1, 1, 0, 10, kTop);
		run(26, 18, 1, 0, 10, kBottom);
		run(26, 2, 0, 1, 16, kRightEdge);
		if (_right == kDoor) {
			run(28, 3, 1, 0, 8, kTop);
			run(28, 4, 0, 1, 14, kRightEdge);
		}
		_clipRight = 25;
	}

	// The front
	if (_front == kOpen) {
		if (!_aheadVisible)
			dark(10, 2, 16, 16);
	} else {
		run(10, 1, 1, 0, 16, kTop);
		run(9, 2, 0, 1, 16, kLeftEdge);
		run(10, 18, 1, 0, 16, kBottom);
		run(26, 2, 0, 1, 16, kRightEdge);
		if (_front == kDoor) {
			run(12, 3, 1, 0, 12, kTop);
			run(11, 4, 0, 1, 14, kLeftEdge);
			run(24, 4, 0, 1, 14, kRightEdge);
		}
	}

	if (_front != kOpen || !_aheadVisible)
		_done = true;
}

void MazeViewDrawer::depth1() {
	if (_leftWall) {
		run(9, 2, 0, 1, 16, kLeftEdge);
		run(10, 2, 1, 1, 4, kSlantB);
		run(13, 6, 0, 1, 8, kLeftEdge);
		run(10, 17, 1, -1, 4, kSlantA);
		if (_left == kDoor) {
			run(10, 4, 0, 1, 13, kLeftEdge);
			run(11, 4, 1, 1, 2, kSlantB);
			run(12, 6, 0, 1, 9, kLeftEdge);
		}
		_clipLeft = 14;
	} else if (!_leftVisible) {
		slantDown(10, 2, 4, 16, 14);
	} else if (_left == kOpen) {
		if (!_leftAheadVisible) {
			dark(6, 6, 8, 8);
			_clipLeft = 14;
		}
	} else {
		run(6, 5, 1, 0, 8, kTop);
		run(6, 14, 1, 0, 8, kBottom);
		run(5, 6, 0, 1, 8, kLeftEdge);
		run(13, 6, 0, 1, 8, kLeftEdge);
		if (_left == kDoor) {
			run(7, 6, 1, 0, 6, kTop);
			run(6, 7, 0, 1, 7, kLeftEdge);
			run(12, 7, 0, 1, 7, kLeftEdge);
		}
		_clipLeft = 14;
	}

	if (_rightWall) {
		run(26, 2, 0, 1, 16, kRightEdge);
		run(25, 2, -1, 1, 4, kSlantA);
		run(22, 6, 0, 1, 8, kRightEdge);
		run(25, 17, -1, -1, 4, kSlantB);
		if (_right == kDoor) {
			run(25, 4, 0, 1, 13, kRightEdge);
			run(24, 4, -1, 1, 2, kSlantA);
			run(23, 6, 0, 1, 9, kRightEdge);
		}
		_clipRight = 21;
	} else if (!_rightVisible) {
		slantUp(22, 5, 4, 10, 21);
	} else if (_right == kOpen) {
		if (!_rightAheadVisible) {
			dark(22, 6, 8, 8);
			_clipRight = 21;
		}
	} else {
		run(22, 5, 1, 0, 8, kTop);
		run(22, 14, 1, 0, 8, kBottom);
		run(22, 6, 0, 1, 8, kRightEdge);
		run(30, 6, 0, 1, 8, kRightEdge);
		if (_right == kDoor) {
			run(23, 6, 1, 0, 6, kTop);
			run(23, 7, 0, 1, 7, kRightEdge);
			run(29, 7, 0, 1, 7, kRightEdge);
		}
		_clipRight = 21;
	}

	if (_front == kOpen) {
		if (!_aheadVisible)
			dark(14, 6, 8, 8);
	} else {
		run(14, 5, 1, 0, 8, kTop);
		run(13, 6, 0, 1, 8, kLeftEdge);
		run(14, 14, 1, 0, 8, kBottom);
		run(22, 6, 0, 1, 8, kRightEdge);
		if (_front == kDoor) {
			run(15, 6, 1, 0, 6, kTop);
			run(14, 7, 0, 1, 7, kLeftEdge);
			run(21, 7, 0, 1, 7, kRightEdge);
		}
	}

	if (_front != kOpen || !_aheadVisible)
		_done = true;
}

void MazeViewDrawer::depth2() {
	if (_leftWall) {
		run(13, 6, 0, 1, 8, kLeftEdge);
		run(14, 6, 1, 1, 2, kSlantB);
		run(15, 8, 0, 1, 4, kLeftEdge);
		run(15, 12, -1, 1, 2, kSlantA);
		if (_left == kDoor) {
			cell(14, 7, 0x1a);
			cell(15, 7, 0x1b);
			run(14, 8, 0, 1, 5, 0x0f);
			run(15, 8, 0, 1, 4, 0x1c);
			cell(14, 13, 0x19);
			cell(15, 12, 0x19);
		}
		_clipLeft = 16;
	} else if (!_leftVisible) {
		slantDown(14, 6, 2, 8, 16);
	} else if (_left == kOpen) {
		if (!_leftAheadVisible) {
			dark(12, 8, 4, 4);
			_clipLeft = 16;
		}
	} else {
		run(12, 7, 1, 0, 4, kTop);
		run(12, 12, 1, 0, 4, kBottom);
		run(11, 8, 0, 1, 4, kLeftEdge);
		run(15, 8, 0, 1, 4, kLeftEdge);
		if (_left == kDoor) {
			run(13, 8, 1, 0, 2, kTop);
			run(12, 9, 0, 1, 3, kLeftEdge);
			run(14, 9, 0, 1, 3, kLeftEdge);
		}
		_clipLeft = 16;
	}

	if (_rightWall) {
		run(22, 6, 0, 1, 8, kRightEdge);
		run(21, 6, -1, 1, 2, kSlantA);
		run(20, 8, 0, 1, 4, kRightEdge);
		run(21, 13, -1, -1, 2, kSlantB);
		if (_right == kDoor) {
			cell(21, 7, 0x1e);
			cell(20, 7, 0x1f);
			run(20, 8, 0, 1, 4, 0x80);
			run(21, 8, 0, 1, 5, 0x13);
			cell(20, 12, 0x1d);
			cell(21, 13, 0x1d);
		}
		_clipRight = 19;
	} else if (!_rightVisible) {
		slantUp(20, 7, 2, 6, 19);
	} else if (_right == kOpen) {
		if (!_rightAheadVisible) {
			dark(20, 8, 4, 4);
			_clipRight = 19;
		}
	} else {
		run(20, 7, 1, 0, 4, kTop);
		run(20, 12, 1, 0, 4, kBottom);
		run(20, 8, 0, 1, 4, kRightEdge);
		run(24, 8, 0, 1, 4, kRightEdge);
		if (_right == kDoor) {
			run(21, 8, 1, 0, 2, kTop);
			run(21, 9, 0, 1, 3, kRightEdge);
			run(23, 9, 0, 1, 3, kRightEdge);
		}
		_clipRight = 19;
	}

	if (_front == kOpen) {
		if (!_aheadVisible)
			dark(16, 8, 4, 4);
	} else {
		run(16, 7, 1, 0, 4, kTop);
		run(15, 8, 0, 1, 4, kLeftEdge);
		run(16, 12, 1, 0, 4, kBottom);
		run(20, 8, 0, 1, 4, kRightEdge);
		if (_front == kDoor) {
			run(17, 8, 1, 0, 2, kTop);
			run(16, 9, 0, 1, 3, kLeftEdge);
			run(19, 9, 0, 1, 3, kRightEdge);
		}
	}

	if (_front != kOpen || !_aheadVisible)
		_done = true;
}

void MazeViewDrawer::depth3() {
	if (_leftWall) {
		cell(16, 8, kSlantB);
		cell(16, 11, kSlantA);
		run(15, 8, 0, 1, 4, kLeftEdge);
		run(16, 9, 0, 1, 2, kLeftEdge);
		if (_left == kDoor) {
			cell(16, 8, 0x18);
			cell(16, 9, 0x11);
			cell(16, 10, 0x11);
			cell(16, 11, 0x17);
		}
		_clipLeft = 17;
	} else if (!_leftVisible) {
		slantDown(16, 8, 1, 4, 17);
	} else if (_left == kOpen) {
		if (!_leftAheadVisible) {
			dark(15, 9, 2, 2);
			_clipLeft = 17;
		}
	} else {
		run(15, 8, 1, 0, 2, kTop);
		run(15, 11, 1, 0, 2, kBottom);
		run(14, 9, 0, 1, 2, kLeftEdge);
		run(16, 9, 0, 1, 2, kLeftEdge);
		if (_left == kDoor) {
			cell(15, 9, 0x10);
			cell(16, 9, 0x5e);
			cell(15, 10, 0x0f);
			cell(16, 10, 0x1c);
		}
		_clipLeft = 17;
	}

	if (_rightWall) {
		cell(19, 8, kSlantA);
		cell(19, 11, kSlantB);
		run(19, 9, 0, 1, 2, kRightEdge);
		run(20, 8, 0, 1, 4, kRightEdge);
		if (_right == kDoor) {
			cell(19, 8, 0x16);
			cell(19, 9, 0x15);
			cell(19, 10, 0x15);
			cell(19, 11, 0x14);
		}
		_clipRight = 18;
	} else if (!_rightVisible) {
		slantUp(19, 8, 1, 4, 18);
	} else if (_right == kOpen) {
		if (!_rightAheadVisible) {
			dark(19, 9, 2, 2);
			_clipRight = 18;
		}
	} else {
		run(19, 8, 1, 0, 2, kTop);
		run(19, 11, 1, 0, 2, kBottom);
		run(19, 9, 0, 1, 2, kRightEdge);
		run(21, 9, 0, 1, 2, kRightEdge);
		if (_right == kDoor) {
			cell(19, 9, 0x5c);
			cell(19, 10, 0x5d);
			cell(20, 9, 0x12);
			cell(20, 10, 0x13);
		}
		_clipRight = 18;
	}

	if (_front == kOpen) {
		if (!_aheadVisible)
			dark(17, 9, 2, 2);
	} else {
		run(17, 8, 1, 0, 2, kTop);
		run(17, 11, 1, 0, 2, kBottom);
		run(16, 9, 0, 1, 2, kLeftEdge);
		run(19, 9, 0, 1, 2, kRightEdge);
		if (_front == kDoor) {
			cell(17, 9, 0x10);
			cell(18, 9, 0x12);
			cell(17, 10, 0x0f);
			cell(18, 10, 0x13);
		}
	}

	_done = true;
}

void MazeViewDrawer::draw() {
	_window.CLRRECT(0, 0, kWidth, kHeight);

	// Standing in darkness there is nothing to see
	if (!squareVisible(0, 0)) {
		dark(0, 0, kWidth, kHeight);
		return;
	}

	// The first advance is onto the party's own square
	Maze::offset(_x, _y, _state.facing, 0, -1);

	advance();
	depth0();
	if (_done)
		return;

	advance();
	depth1();
	if (_done || !_lit)
		return;

	advance();
	depth2();
	if (_done)
		return;

	advance();
	depth3();
}

} // End of namespace Wizardry
