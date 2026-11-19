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

#include "wizardry/views/tile_view.h"
#include "wizardry/data/globals.h"

namespace Wizardry {

TileView::TileView(const Common::String &name, bool framed) :
		View(name), _framed(framed) {
	setTileBounds(Common::Rect(TileScreen::COLS, TileScreen::ROWS));
}

TileView::TileView(const Common::String &name, const Common::Rect &tiles, bool framed) :
		View(name), _framed(framed) {
	setTileBounds(tiles);
}

void TileView::setTileBounds(const Common::Rect &tiles) {
	_tiles = tiles;

	const int w = _G(renderer).tileWidth(), h = _G(renderer).tileHeight();
	setBounds(Common::Rect(tiles.left * w, tiles.top * h, tiles.right * w, tiles.bottom * h));
}

void TileView::draw() {
	TextWindow window(_G(tiles), _tiles, _framed);
	drawTiles(window);
	g_globals->present();
}

} // End of namespace Wizardry
