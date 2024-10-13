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

#include "goldbox/gfx/prompt_line_input.h"

namespace Goldbox {
namespace Shared {
namespace Gfx {

PromptLineInput::PromptLineInput(const Common::String &name, UIElement *uiParent, const Common::String &prompt, size_t maxLength, int x, int y, int width)
    : PromptLine(name, uiParent, prompt, x, y, width), _maxLength(maxLength), _inputComplete(false), _caretVisible(false), _caretCtr(0) {

    // Request focus to receive keypress events
    focus();

    // Mark the element for redrawing
    redraw();
}

void PromptLineInput::drawContent(Surface &s) {
    // Calculate the position to start drawing the input text
    int promptLength = _prompt.size();
    s.setTextPos(promptLength + 2, 0); // Move cursor after prompt

    // Draw the input text
    s.writeString(_inputText);

    // Draw the caret
    if (_caretVisible && !_inputComplete) {
        s.writeChar('_');
    } else {
        s.writeChar(' ');
    }
}

bool PromptLineInput::msgKeypress(const KeypressMessage &msg) {
    if (_inputComplete)
        return false;

    bool changed = false;

    if (msg.ascii >= ' ' && msg.ascii <= 127 && _inputText.size() < _maxLength) {
        // Add character to input text
        _inputText += msg.ascii;
        changed = true;
    } else if (msg.keycode == Common::KEYCODE_BACKSPACE && !_inputText.empty()) {
        // Remove last character
        _inputText.deleteLastChar();
        changed = true;
    } else if (msg.keycode == Common::KEYCODE_RETURN || msg.keycode == Common::KEYCODE_KP_ENTER) {
        // Enter key pressed - complete input
        _inputComplete = true;
        _inputText.toUppercase(); // Convert to uppercase
        // Optionally, send a message or handle input completion here
        close();
        return true;
    } else if (msg.keycode == Common::KEYCODE_ESCAPE) {
        // Escape key pressed - cancel input
        _inputText.clear();
        _inputComplete = true;
        close();
        return true;
    }

    if (changed) {
        redraw();
        return true;
    }

    return false;
}

bool PromptLineInput::tick() {
    // Handle caret blinking
    if (++_caretCtr >= 5) {
        _caretCtr = 0;
        _caretVisible = !_caretVisible;
        redraw();
    }

    return false;
}

Common::String PromptLineInput::getInputText() const {
    return _inputText;
}

bool PromptLineInput::isInputComplete() const {
    return _inputComplete;
}

} // namespace Gfx
} // namespace Shared
} // namespace Goldbox
