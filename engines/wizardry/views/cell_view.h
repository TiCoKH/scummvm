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

#ifndef WIZARDRY_VIEWS_CELL_VIEW_H
#define WIZARDRY_VIEWS_CELL_VIEW_H

#include "wizardry/gfx/text_window.h"
#include "wizardry/views/view.h"

namespace Wizardry {

/**
 * Base class for the views of the releases that draw through the cell
 * screen. A view owns a rectangle of cells, with or without a frame, and
 * draws by printing into it; it never deals with pixels, so the same view
 * serves every display.
 */
class CellView : public View {
private:
	Common::Rect _cells;
	bool _framed;

protected:
	/**
	 * Draws the view. The window covers the cells of the view.
	 */
	virtual void drawCells(TextWindow &window) = 0;

public:
	/**
	 * Creates a view that covers the whole screen
	 */
	CellView(const Common::String &name, bool framed);
	CellView(const Common::String &name, const Common::Rect &cells, bool framed);

	/**
	 * Sets the place of the view in cells. The pixel bounds used for the
	 * mouse follow from it.
	 */
	void setCellBounds(const Common::Rect &cells);
	const Common::Rect &getCellBounds() const { return _cells; }

	void draw() override;
};

} // End of namespace Wizardry

#endif
