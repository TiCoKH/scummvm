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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_TARGET_COMMAND_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_TARGET_COMMAND_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/combat_aim_events.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * The "Aim:" menu of COMBAT_TargetCommandMenu:
 * Next Prev Manual [Target] Center Exit. The chosen command, including one
 * confirmed with Enter on the highlighted item, goes to the listener;
 * Escape reports Exit.
 */
class CombatTargetCommandDialog : public Dialog {
public:
    CombatTargetCommandDialog();
    ~CombatTargetCommandDialog() override;

    void beginTargetCommand(CombatAimListener *listener, bool targetOffered);

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;

private:
    CombatAimListener *_listener;
    bool _targetOffered;
    MenuItemList _menuModel;
    HorizontalMenu *_horizontalMenu;

    void buildMenu();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_TARGET_COMMAND_DIALOG_H
