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

#ifndef GOLDBOX_GFX_PROMPT_LINE_MENU_H
#define GOLDBOX_GFX_PROMPT_LINE_MENU_H

#include "goldbox/gfx/prompt_line.h"

namespace Goldbox {
namespace Shared {
namespace Gfx {

class PromptLineMenu : public PromptLine {
private:
    Common::Array<Common::String> _menuItems;
    int _selectedIndex;
    bool _selectionMade;

public:
    PromptLineMenu(const Common::String &name, UIElement *uiParent, const Common::String &prompt, const Common::Array<Common::String> &menuItems, int x, int y, int width);

    void drawContent(Surface &s) override;
    bool msgKeypress(const KeypressMessage &msg) override;

    Common::String getSelectedItem() const;
    bool isSelectionMade() const;
};

} // namespace Gfx
} // namespace Shared
} // namespace Goldbox

#endif // GOLDBOX_GFX_PROMPT_LINE_MENU_H
