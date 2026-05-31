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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_MENU_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_MENU_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/core/global.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

class InGameView;

namespace Dialogs {

class HorizontalMenu;

/**
 * Persistent in-game horizontal menu (DIALOG_InGame equivalent).
 *
 * Wraps a HorizontalMenu configured with:
 * - Dungeon:    "Area Cast View Encamp Search Look"
 * - Wilderness: "Cast View Encamp Search Look"
 *
 * Routes selection results back to InGameView::handleInGameMenuKey().
 * After a selection, the menu re-activates to stay persistent.
 */
class InGameMenuDialog : public Dialog {
public:
    enum MapMode {
        kModeDungeon = 0,
        kModeWilderness
    };

private:
    MapMode _mode;
    MenuItemList _menuModel;
    HorizontalMenu *_horizontalMenu;

    void buildMenuModel();

public:
    InGameMenuDialog(const Common::String &name = "InGameMenu");
    ~InGameMenuDialog() override;

    void setMode(MapMode mode);
    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_MENU_DIALOG_H
