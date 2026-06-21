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
#include "goldbox/events.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Common::String;
using Common::KeyCode;

VerticalMenu::VerticalMenu(const String &name, const VerticalMenuConfig &config)
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
      _title(config.title),
      _titleAsHeader(config.asHeader),
      _horizontalMenu(nullptr) {
	// Restrict this dialog's drawing to the vertical list area.
	setBounds(Window(_xStart, _yStart, _xEnd, _yEnd));

    // Total vertical span including possible title/header line
    int totalHeight = _yEnd - _yStart + 1;
    if (!_title.empty()) {
        // Reserve first line for title (non-selectable)
        _menuHeight = MAX(0, totalHeight - 1);
    } else {
        _menuHeight = totalHeight;
    }
    _itemNums = static_cast<int>(_menuItems->items.size());
    _linesToRender = MIN(_menuHeight, _itemNums);
    if (_itemNums > _menuHeight) {
        _nextNeed = true;
        _linesBelow = _itemNums - _menuHeight;
    }

    // Count leading consecutive separator (inactive) rows so Prev-page
    // navigation never scrolls above the first level-header.
    _selectMin = 0;
    for (int i = 0; i < _itemNums; ++i) {
        if (!_menuItems->items[i].active)
            _selectMin++;
        else
            break;
    }

    if (_promptOptions) {
        _hMenuList.generateMenuItems(*_promptOptions, true);
    }

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
}

VerticalMenu::~VerticalMenu() {
    if (_horizontalMenu) {
        // Detach from parent's child list before deleting to avoid issues
        // when parent destructor tries to clean up children
        _horizontalMenu->setParent(nullptr);
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
}

void VerticalMenu::activate() {
    debug(7, "VerticalMenu::activate() - itemNums=%d, menuHeight=%d", _itemNums, _menuHeight);
    Dialog::activate();

    // Sync _currentVisibleIndex with externally pre-set currentSelection and
    // skip past any inactive (separator) items at the start of the visible page.
    if (_menuItems && !_menuItems->items.empty()) {
        _currentVisibleIndex = _menuItems->currentSelection - _linesAbove;
        _currentVisibleIndex = CLIP(_currentVisibleIndex, 0, MAX(0, _linesToRender - 1));
        int tries = _linesToRender;
        while (tries > 0 && _currentVisibleIndex < _linesToRender &&
                !_menuItems->items[_linesAbove + _currentVisibleIndex].active) {
            _currentVisibleIndex++;
            tries--;
        }
        // If all visible items are inactive, keep at 0
        if (tries == 0 || _currentVisibleIndex >= _linesToRender) {
            _currentVisibleIndex = 0;
        }
        _menuItems->currentSelection = _linesAbove + _currentVisibleIndex;
    }

    activateHorizontalMenu();
}

void VerticalMenu::deactivate() {
    deactivateHorizontalMenu();
    Dialog::deactivate();
}

void VerticalMenu::draw() {
    if (!isActive()) {
        debug(7, "VerticalMenu::draw() - NOT ACTIVE, skipping");
        return;
    }
    debug(7, "VerticalMenu::draw() - drawing %d items", _linesToRender);
    drawText();

    if (_horizontalMenu) {
        _horizontalMenu->draw();
    }
}

void VerticalMenu::drawText() {
    Surface s = getSurface();
    const int localWidth = _xEnd - _xStart + 1;
    const int localHeight = _yEnd - _yStart + 1;
    s.clearBox(0, 0, localWidth - 1, localHeight - 1, 0);

    debug(7, "VerticalMenu::drawText() - clearing box (%d,%d) to (%d,%d), rendering %d items",
          _xStart, _yStart, _xEnd, _yEnd, _linesToRender);

    // Optional fixed title line (not part of selectable list)
    int titleOffset = 0;
    if (!_title.empty()) {
        s.writeStringC(0, 0, _headColor, _title);
        titleOffset = 1;
    }
    // Indent items by 2 spaces if a title is present
    int itemX = (_title.empty()) ? 0 : 2;

    for (int i = 0; i < _linesToRender; i++) {
        int menuIndex = i + _linesAbove;
        if (menuIndex >= _itemNums) {
            break;
        }
        const auto &item = _menuItems->items[menuIndex];
        int color;
        if (!item.active) {
            // Separator / level-header rows use the heading colour.
            color = _headColor;
        } else {
            color = (menuIndex == _menuItems->currentSelection) ? _selectColor : _textColor;
        }
        s.writeStringC(itemX, titleOffset + i, color, item.text);
    }
}

void VerticalMenu::redrawLine(int index) {
    if (!_menuItems || index < 0 || index >= _itemNums)
        return;

    Surface s = getSurface();

    int relativeIndex = index - _linesAbove;
    if (relativeIndex < 0 || relativeIndex >= _menuHeight)
        return;

    int titleOffset = _title.empty() ? 0 : 1;
    int itemX = (_title.empty()) ? 0 : 2;
    const int localWidth = _xEnd - _xStart + 1;
    // Clear that line
    s.clearBox(0, titleOffset + relativeIndex,
        localWidth - 1, titleOffset + relativeIndex, 0);

    // Draw the text
    const auto &item = _menuItems->items[index];
    int color = (index == _menuItems->currentSelection) ? _selectColor : _textColor;
    s.writeStringC(itemX, titleOffset + relativeIndex, color, item.text);
}

void VerticalMenu::updateHorizontalMenu() {
    // Remove any previously inserted Next/Prev items.
    for (int i = (int)_hMenuList.items.size() - 1; i >= 0; --i) {
        char sc = _hMenuList.items[i].shortcut;
        if (sc == 'N' || sc == 'P')
            _hMenuList.items.remove_at(i);
    }

    // Insert Next/Prev before the last item (Exit) so Exit stays last.
    int insertPos = MAX(0, (int)_hMenuList.items.size() - 1);

    if (_linesBelow > 0) {
        _hMenuList.items.insert_at(insertPos, MenuItem());
        _hMenuList.items[insertPos].text = "Next";
        _hMenuList.items[insertPos].active = true;
        _hMenuList.items[insertPos].shortcutFirst = true;
        _hMenuList.generateShortcut(insertPos);
        ++insertPos;
    }
    // Show Prev only when there are selectable items above the current page.
    // _selectMin is the number of leading separator rows that must always stay
    // visible; scrolling back stops when _linesAbove == _selectMin.
    if (_selectMin < _linesAbove) {
        _hMenuList.items.insert_at(insertPos, MenuItem());
        _hMenuList.items[insertPos].text = "Prev";
        _hMenuList.items[insertPos].active = true;
        _hMenuList.items[insertPos].shortcutFirst = true;
        _hMenuList.generateShortcut(insertPos);
    }

    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }
}

void VerticalMenu::handleMenuResult(const MenuResultMessage &result) {
	bool success = result._success;
	KeyCode key = result._keyCode;
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
				g_events->postMenuResult(_parent->getName(), success,
					key, _menuItems->currentSelection,
					Common::String(), true, false);
            }
			return;
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
        // Advance cursor past any separator row at the top of the new page.
        _currentVisibleIndex = 0;
        const int visOnPage = MIN(_menuHeight, _itemNums - _linesAbove);
        while (_currentVisibleIndex < visOnPage &&
                !_menuItems->items[_linesAbove + _currentVisibleIndex].active) {
            _currentVisibleIndex++;
        }
        if (_currentVisibleIndex >= visOnPage)
            _currentVisibleIndex = 0;
        _menuItems->currentSelection = _linesAbove + _currentVisibleIndex;
        _redraw = true;
    }
    updateHorizontalMenu();
}

void VerticalMenu::prevPage() {
    // Can only scroll back above _selectMin (the leading separator floor).
    const int canGoBack = _linesAbove - _selectMin;
    if (canGoBack > 0) {
        int moveLines = MIN(canGoBack, _menuHeight);
        _linesAbove -= moveLines;
        _linesBelow += moveLines;
        // Advance cursor past any separator row at the top of the revealed page.
        _currentVisibleIndex = 0;
        const int visOnPage = MIN(_menuHeight, _itemNums - _linesAbove);
        while (_currentVisibleIndex < visOnPage &&
                !_menuItems->items[_linesAbove + _currentVisibleIndex].active) {
            _currentVisibleIndex++;
        }
        if (_currentVisibleIndex >= visOnPage)
            _currentVisibleIndex = 0;
        _menuItems->currentSelection = _linesAbove + _currentVisibleIndex;
        _redraw = true;
    }
    updateHorizontalMenu();
}

void VerticalMenu::selectionDown() {
    int visibleCount = MIN(_menuHeight, _itemNums - _linesAbove);
    if (visibleCount <= 0) {
        return;
    }
    int maxVisibleIndex = visibleCount - 1;
    int oldScreenIndex = _currentVisibleIndex;

    // Advance within the visible page, skipping over inactive (separator) items.
    int nextIndex = _currentVisibleIndex;
    int tries = visibleCount;
    do {
        nextIndex = (nextIndex < maxVisibleIndex) ? nextIndex + 1 : 0;
        tries--;
    } while (tries > 0 && !_menuItems->items[_linesAbove + nextIndex].active);

    _menuItems->currentSelection = _linesAbove + nextIndex;
    _currentVisibleIndex = nextIndex;

    redrawLine(_linesAbove + oldScreenIndex);
    redrawLine(_linesAbove + _currentVisibleIndex);
}

void VerticalMenu::selectionUp() {
    int visibleCount = MIN(_menuHeight, _itemNums - _linesAbove);
    if (visibleCount <= 0) {
        return;
    }
    int maxVisibleIndex = visibleCount - 1;
    int oldScreenIndex = _currentVisibleIndex;

    // Retreat within the visible page, skipping over inactive (separator) items.
    int nextIndex = _currentVisibleIndex;
    int tries = visibleCount;
    do {
        nextIndex = (nextIndex > 0) ? nextIndex - 1 : maxVisibleIndex;
        tries--;
    } while (tries > 0 && !_menuItems->items[_linesAbove + nextIndex].active);

    _menuItems->currentSelection = _linesAbove + nextIndex;
    _currentVisibleIndex = nextIndex;

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

void VerticalMenu::fillMenuItemsFromYml(Goldbox::MenuItemList *list,
        const Common::String &baseKey, const int *indices, int count) {
    if (!list || !indices || count <= 0)
        return;

    for (int i = 0; i < count; ++i) {
        Common::String key = Common::String::format("%s.%d", baseKey.c_str(), indices[i]);
        Common::String text = Goldbox::VmInterface::getString(key);
        list->push_back(text);
    }
}

void VerticalMenu::rebuild(Goldbox::MenuItemList *newItems, const String &newTitle) {
    _menuItems = newItems;
    _title = newTitle;
    _linesAbove = 0;
    _linesBelow = 0;
    _currentVisibleIndex = 0;
    _itemNums = _menuItems ? (int)_menuItems->items.size() : 0;
    int totalHeight = _yEnd - _yStart + 1;
    if (!_title.empty()) {
        _menuHeight = MAX(0, totalHeight - 1);
    } else {
        _menuHeight = totalHeight;
    }
    _linesToRender = MIN(_menuHeight, _itemNums);
    if (_itemNums > _menuHeight) {
        _linesBelow = _itemNums - _menuHeight;
    }
    // Recompute leading separator floor after new item list is wired in.
    _selectMin = 0;
    for (int i = 0; i < _itemNums; ++i) {
        if (_menuItems && !_menuItems->items[i].active)
            _selectMin++;
        else
            break;
    }
    if (_menuItems)
        _menuItems->currentSelection = 0;
    _redraw = true;
    updateHorizontalMenu();
    drawText();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

