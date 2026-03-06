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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_PARTY_SELECTOR_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_PARTY_SELECTOR_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/party_list.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

struct PartySelectorConfig {
    Common::String promptText;
    bool allowExit;

    PartySelectorConfig() : promptText("Select"), allowExit(true) {
    }
};

class PartySelector : public Dialog {
public:
    PartySelector(const Common::String &name,
                  const PartySelectorConfig &config = PartySelectorConfig());
    ~PartySelector() override;

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    PartySelectorConfig _config;
    Goldbox::MenuItemList _menuItems;
    Common::String _menuPrompt;

    PartyList *_partyList;
    HorizontalMenu *_horizontalMenu;

    void buildMenuItems();
    void reactivateHorizontalMenu();
    void postSelectionResult(bool success, Common::KeyCode keyCode);
    bool isNextSelectionKey(Common::KeyCode keyCode) const;
    bool isPrevSelectionKey(Common::KeyCode keyCode) const;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_PARTY_SELECTOR_H
