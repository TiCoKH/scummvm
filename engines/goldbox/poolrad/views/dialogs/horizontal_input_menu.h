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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALINPUTMENU_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALINPUTMENU_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/horizontal_input.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalInputMenu : public HorizontalInput {
private:
    Goldbox::MenuItemList _menuItems;
    int _textColor;
    int _selectColor;

    void drawText() override;
    void selectNextItem();
    void selectPreviousItem();

public:
    HorizontalInputMenu(const Common::String &name, const Common::String &promptTxt,
                        const Common::Array<Common::String> &menuStrings, int textColor, int selectColor, int promptColor);

    bool msgKeypress(const KeypressMessage &msg) override;
    MenuItem getSelectedItem() const { return _menuItems.getCurrentSelection(); }
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTALINPUTMENU_H
