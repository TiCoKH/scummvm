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

#include "goldbox/gfx/prompt_line.h"

namespace Goldbox {
namespace Shared {
namespace Gfx {

PromptLine::PromptLine(const Common::String &name, UIElement *uiParent, const Common::String &prompt, int x, int y, int width)
    : UIElement(name, uiParent), _prompt(prompt), _x(x), _y(y), _width(width) {

    // Set the bounds of the UIElement
    setBounds(Common::Rect(_x * FONT_W, _y * FONT_H, (_x + _width) * FONT_W, (_y + 1) * FONT_H));

    // Mark the element for redrawing
    redraw();
}

void PromptLine::draw() {
    Surface s = getSurface();

    // Draw the prompt
    s.writeString(_prompt);

    // Draw subclass-specific content
    drawContent(s);

    _needsRedraw = false;
}

} // namespace Gfx
} // namespace Shared
} // namespace Goldbox
