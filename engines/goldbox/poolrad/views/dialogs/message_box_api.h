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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_MESSAGE_BOX_API_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_MESSAGE_BOX_API_H

#include "common/scummsys.h"
#include "common/str.h"

namespace Goldbox {
namespace Data {
class PlayerCharacter;
} // namespace Data

namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * Shared message-box contract, mirroring the original
 * TEXT_drawIntoMsgBox(isTimed, row, msg, actor) routine that is used both
 * by the in-game text box and by the combat right-hand panel.
 *
 * Implementations only differ in the window they render into; the text
 * layout itself is owned by the shared GameText state.
 */
class MessageBoxApi {
public:
    virtual ~MessageBoxApi() {}

    /**
     * Print a message, optionally prefixed with the speaker's name.
     * @param speaker Character whose name heads the message, or nullptr
     * @param text    Message body
     * @param row     Original line_pos: name row, text starts below it
     * @param timed   Original is_timed flag (pause, then clear)
     */
    virtual void showMessage(::Goldbox::Data::PlayerCharacter *speaker,
                             const Common::String &text, uint8 row,
                             bool timed) = 0;

    /** Clear the message area (and its prompt line). */
    virtual void clearMessage() = 0;

    /** True while text is still being rendered or waiting for a key. */
    virtual bool isBusy() const = 0;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_MESSAGE_BOX_API_H
