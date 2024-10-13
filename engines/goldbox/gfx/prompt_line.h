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

#ifndef GOLDBOX_GFX_PROMPT_LINE_H
#define GOLDBOX_GFX_PROMPT_LINE_H

#include "goldbox/events.h"

namespace Goldbox {
namespace Shared {
namespace Gfx {

class PromptLine : public UIElement {
protected:
    Common::String _prompt;
    int _x;
    int _y;
    int _width;

public:
    PromptLine(const Common::String &name, UIElement *uiParent, const Common::String &prompt, int x, int y, int width);

    void draw() override;
    virtual void drawContent(Surface &s) = 0; // Pure virtual method to draw subclass-specific content

};

} // namespace Gfx
} // namespace Shared
} // namespace Goldbox

#endif // GOLDBOX_GFX_PROMPT_LINE_H
