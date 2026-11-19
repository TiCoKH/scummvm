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

#ifndef WIZARDRY_VIEWS_TILE_VIEW_H
#define WIZARDRY_VIEWS_TILE_VIEW_H

#include "wizardry/gfx/text_window.h"
#include "wizardry/views/view.h"

namespace Wizardry {

/**
 * Base class for the views of the releases that draw through the tile
 * screen. A view owns a rectangle of tiles, with or without a frame, and
 * draws by printing into it; it never deals with pixels, so the same view
 * serves every display.
 */
class TileView : public View {
private:
	Common::Rect _tiles;
	bool _framed;

protected:
	/**
	 * Draws the view. The window covers the tiles of the view.
	 */
	virtual void drawTiles(TextWindow &window) = 0;

public:
	/**
	 * Creates a view that covers the whole screen
	 */
	TileView(const Common::String &name, bool framed);
	TileView(const Common::String &name, const Common::Rect &tiles, bool framed);

	/**
	 * Sets the place of the view in tiles. The pixel bounds used for the
	 * mouse follow from it.
	 */
	void setTileBounds(const Common::Rect &tiles);
	const Common::Rect &getTileBounds() const { return _tiles; }

	void draw() override;
};

} // End of namespace Wizardry

#endif
