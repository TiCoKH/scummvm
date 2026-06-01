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

#include "goldbox/poolrad/views/dialogs/text_box_dialog.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

TextBoxDialog::TextBoxDialog(const Common::String &name)
    : Dialog(name),
      _startX(kDefaultStartX), _startY(kDefaultStartY),
      _endX(kDefaultEndX), _endY(kDefaultEndY),
      _cursorX(kDefaultStartX), _cursorY(kDefaultStartY),
      _textColor(kDefaultTextColor),
            _srcIdx(0), _pageStartIdx(0), _rendering(false), _waitingForKey(false),
      _frameCounter(0), _framesPerWord(0) {
    setBounds(Window(0, 0, 39, 24));
}

bool TextBoxDialog::isWordBreak(char c) {
    return c == '\0' || c == ' ' || c == '-' || c == '.'
        || c == ',' || c == '?' || c == '!' || c == ':'
        || c == ';';
}

void TextBoxDialog::clearArea() {
    Surface s = getSurface();
    s.clearBox(_startX, _startY, _endX, _endY, 0);
}

void TextBoxDialog::activate() {
    Dialog::activate();
    _rendering = false;
    _waitingForKey = false;
    _pendingDrawStep = false;
}

void TextBoxDialog::clearText() {
    _text.clear();
    _srcIdx = 0;
    _pageStartIdx = 0;
    _rendering = false;
    _waitingForKey = false;
    _frameCounter = 0;
    _framesPerWord = 0;
    _pendingDrawStep = false;
    _cursorX = _startX;
    _cursorY = _startY;

    clearArea();
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 0);
    redraw();
}

void TextBoxDialog::redrawCurrentPage() {
    clearArea();

    uint cursorX = _startX;
    uint cursorY = _startY;
    uint pos = _pageStartIdx;
    Surface s = getSurface();

    while (pos < _srcIdx) {
        uint wordStart = pos;
        uint wordEnd = pos;

        while (wordEnd < _srcIdx && !isWordBreak(_text[wordEnd])) {
            ++wordEnd;
        }

        if (wordEnd < _srcIdx && _text[wordEnd] != '\0') {
            ++wordEnd;
        }

        const uint wordLen = wordEnd - wordStart;
        if (wordLen > 0 && cursorX + wordLen > static_cast<uint>(_endX) + 1) {
            if (cursorX != _startX) {
                cursorY++;
                cursorX = _startX;
            }
        }

        if (cursorY > _endY)
            break;

        for (uint i = wordStart; i < wordEnd; ++i) {
            const char c = _text[i];
            if (c == '\0')
                break;
            if (c == ' ' && cursorX > _endX)
                continue;
            s.writeCharC(cursorX, cursorY, _textColor, c);
            cursorX++;
        }

        pos = wordEnd;
    }
}

void TextBoxDialog::setText(const Common::String &text, bool clearBox) {
    _text = text;
    _srcIdx = 0;
    _pageStartIdx = 0;
    _rendering = true;
    _waitingForKey = false;
    _frameCounter = 0;
    _pendingDrawStep = false;

    // Pacing: frames per word based on game speed (1-5 scale).
    // Original x86: Wait_cycle(CFG_GAME_SPEED * 10)
    // Original m68k: Delay(CFG_GAME_SPEED * 2)
    // At 20fps, speed=3 -> 3 frames per word gives similar feel.
    uint speed = VmInterface::getTextDelay();
    _framesPerWord = (speed == 0) ? 0 : speed;

    if (clearBox) {
        clearArea();
        // PRINTCLEAR semantics: also clear the prompt/status row and fully
        // reset cursor to the text-box origin before rendering new text.
        Surface s = getSurface();
        s.clearBox(0, 24, 39, 24, 0);
        _cursorX = _startX;
        _cursorY = _startY;
    } else {
        // If cursor is outside the box, reset it (matches x86 behavior).
        if (_cursorX < _startX || _cursorX > _endX
                || _cursorY < _startY || _cursorY > _endY) {
            _cursorX = _startX;
            _cursorY = _startY;
        }
    }

    redraw();
}

void TextBoxDialog::renderNextWord() {
    if (_srcIdx >= _text.size()) {
        _rendering = false;
        return;
    }

    // Scan forward to find the end of the current word (stop at break char).
    uint wordStart = _srcIdx;
    uint wordEnd = _srcIdx;

    while (wordEnd < _text.size() && !isWordBreak(_text[wordEnd])) {
        ++wordEnd;
    }

    // Include the break character itself in the word (space/punctuation).
    if (wordEnd < _text.size() && _text[wordEnd] != '\0') {
        ++wordEnd;
    }

    uint wordLen = wordEnd - wordStart;

    // Check if word fits on current line.
    // If it would overflow past endX, wrap to next line first.
    if (wordLen > 0 && (uint)_cursorX + wordLen > (uint)_endX + 1) {
        // Don't wrap if we're already at the start of a line (word is wider
        // than the entire line — just print it anyway to avoid infinite loop).
        if (_cursorX != _startX) {
            _cursorY++;
            _cursorX = _startX;
        }
    }

    // Check vertical overflow.
    if (_cursorY > _endY) {
        _cursorX = _startX;
        _cursorY = _startY;
        _pageStartIdx = wordStart;
        _waitingForKey = true;
        _rendering = false;
        // Draw "PRESS ANY KEY" on row 24.
        Surface s = getSurface();
        s.clearBox(0, 24, 39, 24, 0);
        s.writeStringC(0, 24, _textColor, "PRESS ANY KEY");
        return;
    }

    // Draw the word character by character.
    Surface s = getSurface();
    for (uint i = wordStart; i < wordEnd; ++i) {
        char c = _text[i];
        if (c == '\0')
            break;
        // Skip trailing space at end of line to avoid visual overflow.
        if (c == ' ' && _cursorX > _endX)
            continue;
        s.writeCharC(_cursorX, _cursorY, _textColor, c);
        _cursorX++;
    }

    _srcIdx = wordEnd;

    // If we've consumed all text, stop rendering.
    if (_srcIdx >= _text.size()) {
        _rendering = false;
    }
}

void TextBoxDialog::draw() {
    if (!_isVisible)
        return;
    // Render during draw, not tick, so later background/window redraws in the
    // same frame do not erase already-emitted words from the text area.
    if (_rendering && _framesPerWord == 0) {
        while (_rendering) {
            renderNextWord();
        }
    } else if (_rendering && _pendingDrawStep) {
        _pendingDrawStep = false;
        renderNextWord();
    }

    redrawCurrentPage();

    if (_waitingForKey) {
        Surface s = getSurface();
        s.clearBox(0, 24, 39, 24, 0);
        s.writeStringC(0, 24, _textColor, "PRESS ANY KEY");
    }
}

bool TextBoxDialog::tick() {
    if (_waitingForKey)
        return false;

    if (!_rendering)
        return false;

    if (_framesPerWord == 0) {
        redraw();
        return true;
    }

    // Frame-paced rendering: queue one word for the next draw pass.
    ++_frameCounter;
    if (_frameCounter >= _framesPerWord) {
        _frameCounter = 0;
        _pendingDrawStep = true;
        redraw();
    }
    return true;
}

bool TextBoxDialog::msgKeypress(const KeypressMessage &msg) {
    if (_waitingForKey) {
        (void)msg;
        // Any key resumes: clear area and continue rendering.
        _waitingForKey = false;
        _rendering = true;
        _pendingDrawStep = false;
        _pageStartIdx = _srcIdx;
        clearArea();
        _cursorX = _startX;
        _cursorY = _startY;
        // Clear the prompt row.
        Surface s = getSurface();
        s.clearBox(0, 24, 39, 24, 0);
        redraw();
        return true;
    }
    return false;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
