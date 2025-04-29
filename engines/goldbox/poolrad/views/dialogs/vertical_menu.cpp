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

#include "common/util.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "vertical_menu.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

VerticalMenu::VerticalMenu(const Common::String &name, const VerticalMenuConfig &config)
    : Dialog(name),
      _promptTxt(config.promptTxt),
      _promptOptions(config.promptOptions),
      _headColor(config.headColor),
      _textColor(config.textColor),
      _selectColor(config.selectColor),
      _xStart(config.xStart),
      _yStart(config.yStart),
      _xEnd(config.xEnd),
      _yEnd(config.yEnd),
      _menuItems(config.menuItemList),
      _addExit(config.isAddExit),
      _horizontalMenu(nullptr) {

    _menuHeight = _yEnd - _yStart + 1;
    _itemNums = static_cast<int>(_menuItems->items.size());
    _linesToRender = MIN(_menuHeight, _itemNums);
    if (_itemNums > _menuHeight) {
        _nextNeed = true;
        _linesBelow = _itemNums - _menuHeight;
    }

    _hMenuList.generateMenuItems(_promptOptions, true);

    updateHorizontalMenu();

    HorizontalMenuConfig hMenuConfig = {
        _promptTxt,
        &_hMenuList,
        _textColor,
        _selectColor,
        _headColor,
        true
    };

    _horizontalMenu = new HorizontalMenu(name + "_Horizontal", hMenuConfig);
    subView(_horizontalMenu);
    activate();
}

VerticalMenu::~VerticalMenu() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
}

void VerticalMenu::draw() {
    drawText();

    if (_horizontalMenu) {
        _horizontalMenu->draw();
    }
}

void VerticalMenu::drawText() {
    Surface s = getSurface();
    s.clearBox(_xStart, _yStart, _xEnd, _yEnd, 0);

    for (int i = 0; i < _linesToRender; i++) {
        int menuIndex = i + _linesAbove;
        if (menuIndex >= _itemNums) {
            break;
        }
        const auto &item = _menuItems->items[menuIndex];
        int color = (menuIndex == _menuItems->currentSelection) ? _selectColor : _textColor;
        s.writeStringC(item.text, color, _xStart, _yStart + i);
    }
}

void VerticalMenu::redrawLine(int index) {
    if (!_menuItems || index < 0 || index >= _itemNums)
        return;

    Surface s = getSurface();

    int relativeIndex = index - _linesAbove;
    if (relativeIndex < 0 || relativeIndex >= _menuHeight)
        return;

    // Clear that line
    s.clearBox(_xStart, _yStart + relativeIndex, _xEnd, _yStart + relativeIndex, 0);

    // Draw the text
    const auto &item = _menuItems->items[index];
    int color = (index == _menuItems->currentSelection) ? _selectColor : _textColor;
    s.writeStringC(item.text, color, _xStart, _yStart + relativeIndex);
}

void VerticalMenu::updateHorizontalMenu() {
    while (!_hMenuList.items.empty() &&
        (_hMenuList.items.back().shortcut == 'N' || _hMenuList.items.back().shortcut == 'P')) {
        _hMenuList.items.pop_back();
    }

    if (_linesBelow > 0) {
        _hMenuList.push_back("Next");
        _hMenuList.generateShortcut(_hMenuList.items.size() - 1);
    }
    if (_linesAbove > 0) {
        _hMenuList.push_back("Prev");
        _hMenuList.generateShortcut(_hMenuList.items.size() - 1);
    }

    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }
}

void VerticalMenu::handleMenuResult(bool success, Common::KeyCode key, short value) {
    switch (key) {
        case Common::KEYCODE_END:
            selectionDown();
            break;

        case Common::KEYCODE_HOME:
            selectionUp();
            break;

        case Common::KEYCODE_PAGEDOWN:
        case Common::KEYCODE_n:
            nextPage();
            break;

        case Common::KEYCODE_PAGEUP:
        case Common::KEYCODE_p:
            prevPage();
            break;

        default:
            if (_parent) {
                _parent->handleMenuResult(success, key, _menuItems->currentSelection);
            }
            break;
    }

    if (_redraw) {
        drawText();
    }
}

void VerticalMenu::nextPage() {
    if (_linesBelow > 0) {
        int moveLines = MIN(_linesBelow, _menuHeight);
        _linesAbove += moveLines;
        _linesBelow -= moveLines;
        _menuItems->currentSelection = _linesAbove;
        _currentVisibleIndex = 0;
        _redraw = true;
    }
    updateHorizontalMenu();
}

void VerticalMenu::prevPage() {
    if (_linesAbove > 0) {
        int moveLines = MIN(_linesAbove, _menuHeight);
        _linesAbove -= moveLines;
        _linesBelow += moveLines;
        _menuItems->currentSelection = _linesAbove;
        _currentVisibleIndex = 0;
        _redraw = true;
    }
    updateHorizontalMenu();
}

void VerticalMenu::selectionDown() {
    int oldScreenIndex = _currentVisibleIndex;
    if (_currentVisibleIndex < _menuHeight - 1) {
        _menuItems->next();
        _currentVisibleIndex++;
    } else {
        _menuItems->currentSelection = _linesAbove;
        _currentVisibleIndex = 0;
    }
    redrawLine(_linesAbove + oldScreenIndex);
    redrawLine(_linesAbove + _currentVisibleIndex);
}

void VerticalMenu::selectionUp() {
    int oldScreenIndex = _currentVisibleIndex;
    if (_currentVisibleIndex > 0) {
        _menuItems->prev();
        _currentVisibleIndex--;
    } else {
        _currentVisibleIndex = _menuHeight - 1;
        _menuItems->currentSelection = _linesAbove + _currentVisibleIndex;
    }
    redrawLine(_linesAbove + oldScreenIndex);
    redrawLine(_linesAbove + _currentVisibleIndex);
}

void VerticalMenu::activateHorizontalMenu() {
    if (_horizontalMenu) {
        _horizontalMenu->activate();
    }
}

void VerticalMenu::deactivateHorizontalMenu() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

