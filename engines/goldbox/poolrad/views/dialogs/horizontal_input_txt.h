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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALINPUTTXT_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALINPUTTXT_H

#include "goldbox/poolrad/views/dialogs/horizontal_input.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalInputTxt : public HorizontalInput {
private:
    byte _maxInputLength;          // Maximum number of characters
    Common::String _inputText;     // Text being entered by the user

    void drawText() override;      // Draws the prompt and input text

public:
    HorizontalInputTxt(const Common::String &name, byte maxInputLength, byte promptColor,
                       const Common::String &promptTxt)
        : HorizontalInput(name, promptColor, promptTxt), 
          _maxInputLength(maxInputLength), _inputText("") {}

    bool msgKeypress(const KeypressMessage &msg) override;  // Handle user text input
    Common::String getInput() const { return _inputText; }  // Retrieve the entered text
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
