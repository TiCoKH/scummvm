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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_TEXT_BOX_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_TEXT_BOX_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "common/str.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * Word-wrapping text renderer for the ECL message area.
 *
 * Implements the logic of TEXT_boxMessage (x86) / TEXT_BlockPrint (m68k):
 * - Scans source text for word boundaries (break chars: space, -, ., ,, ?, !, :, ;)
 * - Wraps words that would overflow past endX to the next line
 * - On vertical overflow past endY: resets cursor, shows "PRESS ANY KEY",
 *   clears the area, then continues
 * - Per-character draw with optional delay pacing (CFG_GAME_SPEED based)
 *
 * Default text area for PRINT/PRINTCLEAR: (1,17)-(38,22), color 10.
 */
class TextBoxDialog : public Dialog {
public:
    static const uint8 kDefaultStartX = 1;
    static const uint8 kDefaultStartY = 17;
    static const uint8 kDefaultEndX = 38;
    static const uint8 kDefaultEndY = 22;
    static const int kDefaultTextColor = 10;

private:
    // Text area bounds (character coordinates)
    uint8 _startX;
    uint8 _startY;
    uint8 _endX;
    uint8 _endY;

    // Cursor position within the text area
    uint8 _cursorX;
    uint8 _cursorY;

    // Text color
    int _textColor;

    // Source text and rendering state
    Common::String _text;
    uint _srcIdx;
    uint _pageStartIdx;
    bool _rendering;
    bool _waitingForKey;

    // Per-character pacing
    uint _frameCounter;
    uint _framesPerWord;
    bool _pendingDrawStep = false;

    static bool isWordBreak(char c);
    void redrawCurrentPage();
    void renderNextWord();
    void clearArea();

public:
    TextBoxDialog(const Common::String &name = "TextBox");
    ~TextBoxDialog() override {}

    /**
     * Start rendering text into the box.
     * @param text     Source text to render
     * @param clearBox If true, clear the text area and reset cursor before printing
     */
    void setText(const Common::String &text, bool clearBox);

    /**
     * Clear the text area immediately and reset paging/cursor state.
     */
    void clearText();

    /**
     * Returns true while text is still being rendered or waiting for key.
     */
    bool isBusy() const { return _rendering || _waitingForKey; }

    void activate() override;
    void draw() override;
    bool tick() override;
    bool msgKeypress(const KeypressMessage &msg) override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_TEXT_BOX_DIALOG_H
