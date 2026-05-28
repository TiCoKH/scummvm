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

#include "goldbox/gfx/viewport_background.h"

#include "common/rect.h"

namespace Goldbox {
namespace Gfx {

ViewportBackground::ViewportBackground()
		: _colorSky(0), _colorSkyline(0), _colorHorizon(0), _colorFloor(0) {
	_surface.create(320, 200, Graphics::PixelFormat::createFormatCLUT8());
	_surface.fillRect(Common::Rect(0, 0, 320, 200), 0);
}

void ViewportBackground::set3DViewportColors(uint8 sky, uint8 skyline,
		uint8 horizon, uint8 floor) {
	if (_colorSky == sky && _colorSkyline == skyline &&
			_colorHorizon == horizon && _colorFloor == floor)
		return;

	_colorSky = sky;
	_colorSkyline = skyline;
	_colorHorizon = horizon;
	_colorFloor = floor;
	rebuild();
}

void ViewportBackground::rebuild() {
	const int x0 = kViewportX;
	const int x1 = kViewportX + kViewportSize;
	const int y0 = kViewportY;

	// Sky: 43 rows
	_surface.fillRect(Common::Rect(x0, y0, x1, y0 + 43), _colorSky);
	// Skyline: 1 row
	_surface.fillRect(Common::Rect(x0, y0 + 43, x1, y0 + 44), _colorSkyline);
	// Horizon: 2 rows
	_surface.fillRect(Common::Rect(x0, y0 + 44, x1, y0 + 46), _colorHorizon);
	// Floor: 42 rows
	_surface.fillRect(Common::Rect(x0, y0 + 46, x1, y0 + 88), _colorFloor);
}

} // namespace Gfx
} // namespace Goldbox
