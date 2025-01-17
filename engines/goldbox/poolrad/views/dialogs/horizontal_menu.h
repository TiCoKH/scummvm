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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTAL_MENU_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTAL_MENU_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

struct HorizontalMenuConfig {
    Common::String promptTxt;
    Goldbox::MenuItemList *menuItemList;
    int textColor;
    int selectColor;
    int promptColor;
    bool allowNumPad;
};

class HorizontalMenu : public Dialog {
private:
    Goldbox::MenuItemList *_menuItems;
    int _textColor;
    int _selectColor;
    int _promptColor;
    Common::String _promptTxt;
    bool _allowNumPad;
    bool _redraw = true;

    void drawText();

public:
    HorizontalMenu(const Common::String &name, const HorizontalMenuConfig &config);
    bool msgKeypress(const KeypressMessage &msg);
    void draw() override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_HORIZONTAL_MENU_H
