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

#include "alis_screen.h"

namespace Alis {

AlisScreen::AlisScreen() {
    // Constructor implementation
}

AlisScreen::~AlisScreen() {
    // Destructor implementation
}

void AlisScreen::init(uint16 width, uint16 height) {
    // Initialize screen resources
    _surface.create(width, height, Graphics::PixelFormat::createFormatCLUT8());
}

void AlisScreen::draw() {
    // Draw on the screen surface
    // Example: Fill with a color
    _surface.fillRect(Common::Rect(0, 0, 320, 200), 0);
    // Add more drawing code here
}

} // End of namespace Alis
