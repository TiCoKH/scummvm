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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MANUAL_AIM_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MANUAL_AIM_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/combat_aim_events.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Combat {
class CombatSession;
} // namespace Combat

namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * The free aim cursor of COMBAT_manualAim: cursor keys move a one-cell
 * cursor over the 50x25 field, each position is re-evaluated, and the
 * "(Use Cursor keys)" menu offers [Target] Center Exit. Target picks the
 * cell; Exit/Escape returns to the Aim menu.
 */
class CombatManualAimDialog : public Dialog {
public:
    CombatManualAimDialog();
    ~CombatManualAimDialog() override;

    void beginManualAim(CombatAimListener *listener,
                        Combat::CombatSession *session,
                        const Combat::AimRequest &request, uint8 range,
                        TilePos start);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;

private:
    CombatAimListener *_listener;
    Combat::CombatSession *_session;
    Combat::AimRequest _request;
    uint8 _range;
    TilePos _cursor;
    Combat::AimCell _cell;

    MenuItemList _menuModel;
    HorizontalMenu *_horizontalMenu;

    void evaluateCursor();
    void buildCommandMenu();
    void finish(bool picked);

    /** Direction index 0-7 (N..NW) for a cursor key, or -1. */
    static int decodeDirection(const KeypressMessage &msg);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_MANUAL_AIM_DIALOG_H
