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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_PORTRAIT_DISPLAY_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_PORTRAIT_DISPLAY_H

#include "goldbox/gfx/pic.h"
#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * PortraitDisplay - handles loading and displaying portrait graphics.
 * Used to display character head and body portraits from DAX blocks.
 * Can be reused in any dialog or view by creating an instance and calling
 * loadHead()/loadBody() to populate portraits, then draw() to render them.
 *
 * Portrait graphics are owned by this object and automatically cleaned up.
 */
class PortraitDisplay {
public:
    PortraitDisplay();
    ~PortraitDisplay();

    /**
     * Load a head portrait from DAX block ID.
     * Clears any previously loaded head portrait.
     * @param daxBlockId The DAX block ID to load from body container
     * @return true if load was successful, false if DAX block not found
     */
    bool loadHead(uint8 daxBlockId);

    /**
     * Load a body portrait from DAX block ID.
     * Clears any previously loaded body portrait.
     * @param daxBlockId The DAX block ID to load from body container
     * @return true if load was successful, false if DAX block not found
     */
    bool loadBody(uint8 daxBlockId);

private:
    Goldbox::Gfx::Pic *_headPic;          // Head portrait (owned)
    Goldbox::Gfx::Pic *_bodyPic;          // Body portrait (owned)
    uint8 _windowLeft;                    // Window bounds
    uint8 _windowTop;
    uint8 _windowRight;
    uint8 _windowBottom;
    bool _hasWindow;                      // Whether to draw window frame

    /**
     * Load a portrait from DAX block ID.
     * @param daxBlockId The DAX block ID to load
     * @return Pointer to loaded Pic on success, nullptr on failure
     */
    Goldbox::Gfx::Pic *_loadPicFromDaxBlock(uint8 daxBlockId);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_PORTRAIT_DISPLAY_H
