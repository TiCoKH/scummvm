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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALINPUT_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALINPUT_H

#include "goldbox/gfx/surface.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalInput : public Dialog {
protected:
    byte _promptColor;
    Common::String _promptTxt;
    byte _text_offset;
    byte _maxInputLength;
    Common::String _inputText;

    void drawText();

public:
    HorizontalInput(const Common::String &name, byte maxInputLength, byte promptColor, const Common::String &promptTxt);

    bool msgKeypress(const KeypressMessage &msg) override;  // Handles user input
    void draw() override;                                   // Draws the horizontal input field
    void clear();                                           // Clears the input display

    Common::String getInput() const { return _inputText; }  // Returns the entered text
    void clearInput() { _inputText = ""; }                  // Clears the entered text
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
