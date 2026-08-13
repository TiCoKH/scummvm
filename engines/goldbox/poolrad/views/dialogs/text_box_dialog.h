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
class GameText;
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * UI adapter for the shared GameText message state.
 *
 * GameText owns TEXT_BlockPrint-compatible wrapping and paging state. This
 * dialog owns activation, redraws, pacing, and keyboard acknowledgement.
 */
class TextBoxDialog : public Dialog {
public:
    static const int kDefaultTextColor = 10;

private:
    GameText *_gameText;
    uint _frameCounter;
    uint _framesPerWord;
    bool _pendingDrawStep = false;

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
    bool isBusy() const;

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
