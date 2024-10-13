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

#include "goldbox/gfx/prompt_line_menu.h"

namespace Goldbox {
namespace Shared {
namespace Gfx {


PromptLineMenu::PromptLineMenu(const Common::String &name, UIElement *uiParent, const Common::String &prompt, const Common::Array<Common::String> &menuItems, int x, int y, int width)
    : PromptLine(name, uiParent, prompt, x, y, width), _menuItems(menuItems), _selectedIndex(0), _selectionMade(false) {

    // Request focus to receive keypress events
    focus();

    // Mark the element for redrawing
    redraw();
}

void PromptLineMenu::drawContent(Surface &s) {
    // Calculate the position to start drawing the menu items
    int promptLength = _prompt.size();
    s.setTextPos(promptLength, 0); // Move cursor after prompt

    // Draw the menu items
    for (size_t i = 0; i < _menuItems.size(); ++i) {
        // Highlight the selected item
        if (i == _selectedIndex) {
            s.setTextColor(Text::kColorHighlight);
        } else {
            s.setTextColor(Text::kColorNormal);
        }

        s.writeString(_menuItems[i]);

        // Add a space between items
        if (i < _menuItems.size() - 1) {
            s.writeChar(' ');
        }
    }
}

bool PromptLineMenu::msgKeypress(const KeypressMessage &msg) {
    if (_selectionMade)
        return false;

    if (msg.keycode == Common::KEYCODE_LEFT || msg.keycode == Common::KEYCODE_UP) {
        // Move selection to the previous item
        if (_selectedIndex > 0) {
            _selectedIndex--;
            redraw();
        }
        return true;
    } else if (msg.keycode == Common::KEYCODE_RIGHT || msg.keycode == Common::KEYCODE_DOWN) {
        // Move selection to the next item
        if (_selectedIndex < (int)_menuItems.size() - 1) {
            _selectedIndex++;
            redraw();
        }
        return true;
    } else if (msg.keycode == Common::KEYCODE_RETURN || msg.keycode == Common::KEYCODE_KP_ENTER) {
        // Enter key pressed - make selection
        _selectionMade = true;
        // Optionally, send a message or handle selection here
        close();
        return true;
    } else if (msg.keycode == Common::KEYCODE_ESCAPE) {
        // Escape key pressed - cancel selection
        _selectionMade = true;
        _selectedIndex = -1; // Indicate no selection
        close();
        return true;
    }

    return false;
}

Common::String PromptLineMenu::getSelectedItem() const {
    if (_selectedIndex >= 0 && _selectedIndex < (int)_menuItems.size()) {
        return _menuItems[_selectedIndex];
    }
    return Common::String();
}

bool PromptLineMenu::isSelectionMade() const {
    return _selectionMade;
}

} // namespace Gfx
} // namespace Shared
} // namespace Goldbox
