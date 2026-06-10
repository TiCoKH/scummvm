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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_DOOR_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_DOOR_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

namespace Dialogs {

class HorizontalMenu;

/**
 * Door interaction dialog (DIALOG_OpenDoor equivalent).
 *
 * Activated when the party encounters a locked door during dungeon
 * movement. Presents a horizontal menu with contextual options:
 *   Bash / Pick / Knock / Exit
 *
 * Menu options are built dynamically based on door type and party
 * capabilities. On success, clears GEO door flags on both sides
 * and posts result to InGameView which advances the party forward.
 *
 * Door flag values (from GEO plane 3, 2 bits per direction):
 *   1 = unlocked (auto-open)
 *   2 = locked (Pick requires thief, Knock requires memorized spell)
 *   3 = locked (Pick requires mage class, Knock requires mage class)
 */
class DoorDialog : public Dialog {
public:
    /** Result posted back to InGameView via MenuResultMessage. */
    enum DoorResult {
        kDoorOpened = 1,
        kDoorBlocked = 0
    };

private:
    MenuItemList _menuModel;
    HorizontalMenu *_horizontalMenu;

    // Door state for current interaction.
    uint8 _doorFlag;        // 1=unlocked, 2=locked-A, 3=locked-B
    bool _bashAllowed;
    bool _pickAllowed;
    bool _knockAllowed;

    void buildMenuModel();
    void handleMenuKey(char key);
    void postResult(bool opened);

    // Door action implementations.
    bool tryBash();
    bool tryLockpick();
    bool useKnock();

    // Party query helpers.
    bool partyHasThiefClass() const;
    bool partyHasMageClass() const;
    bool partyHasKnockSpell() const;

public:
    DoorDialog(const Common::String &name);
    ~DoorDialog() override;

    /**
     * Begin a door interaction for the given door flag value.
     * @param doorFlag  2-bit door state (1/2/3) from RuntimeGeoBlock.
     */
    void openDoor(uint8 doorFlag);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_DOOR_DIALOG_H
