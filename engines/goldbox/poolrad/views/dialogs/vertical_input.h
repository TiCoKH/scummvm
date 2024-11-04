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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_VERTICALINPUT_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_VERTICALINPUT_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class VerticalInput : public Dialog {
protected:
    byte _promptColor;             // Color of the prompt text
    Common::String _promptTxt;     // Prompt message displayed before input

    virtual void drawText() = 0;   // Pure virtual function for child-specific drawing

public:
    VerticalInput(const Common::String &name, byte promptColor, const Common::String &promptTxt)
        : Dialog(name), _promptColor(promptColor), _promptTxt(promptTxt) {
        activate();
    }

    virtual bool msgKeypress(const KeypressMessage &msg) = 0;  // Pure virtual for specific key handling
    virtual void draw() override;

    void clear();  // Clear a vertical section of the screen
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
