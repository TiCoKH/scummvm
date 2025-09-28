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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_VERTICALMENU_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_VERTICALMENU_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

struct VerticalMenuConfig {
    Common::String promptTxt;
    Common::Array<Common::String> promptOptions;
    Goldbox::MenuItemList *menuItemList;
    int headColor;
    int textColor;
    int selectColor;
    int xStart;
    int yStart;
    int xEnd;
    int yEnd;
    Common::String title;
    bool asHeader;
};


class VerticalMenu : public Dialog {
private:
    Goldbox::MenuItemList *_menuItems;
    int _textColor;
    int _selectColor;
    int _headColor;
    Common::String _promptTxt;
    Common::Array<Common::String> _promptOptions;
    int _xStart, _yStart, _xEnd, _yEnd;
    bool _nextNeed;
    bool _prevNeed;
    int _linesAbove = 0;
    int _linesBelow = 0;
    int _menuHeight;
    int _itemNums;
    int _linesToRender;
    int _currentVisibleIndex = 0;
    Common::String _title;
    bool _titleAsHeader = false;
    bool _redraw = true;
    HorizontalMenu *_horizontalMenu;

    void selectionDown();
    void selectionUp();
    void nextPage();
    void prevPage();
    void drawText();
    void updateHorizontalMenu();

public:
    Goldbox::MenuItemList _hMenuList;

    VerticalMenu(const Common::String &name, const VerticalMenuConfig &config);
    ~VerticalMenu();

    void draw() override;
    void redrawLine(int lineNum);
	void handleMenuResult(bool success, Common::KeyCode key, short value) override;
    void activateHorizontalMenu();
    void deactivateHorizontalMenu();

    static void fillMenuItemsFromYml(Goldbox::MenuItemList *list,
        const Common::String &baseKey, const int *indices, int count);

    // Reinitialize with new items and title without destroying the widget
    void rebuild(Goldbox::MenuItemList *newItems, const Common::String &newTitle);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_VERTICALMENU_H

