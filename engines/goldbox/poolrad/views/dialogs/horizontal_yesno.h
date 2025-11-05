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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALYESNO_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALYESNO_H

#include "goldbox/gfx/surface.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

struct HorizontalYesNoConfig {
    Common::String promptTxt;   // Text/question to display
    byte promptColor;           // Color for the prompt text
};

// A simple horizontal yes/no prompt displayed on the prompt line.
// Shows a question and accepts Y/N (case-insensitive). ESC maps to No, RETURN maps to Yes.
class HorizontalYesNo : public Dialog {
protected:
    byte _promptColor;
    Common::String _promptTxt;

    void drawText();

public:
    HorizontalYesNo(const Common::String &name, const HorizontalYesNoConfig &config);

    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void clear();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
