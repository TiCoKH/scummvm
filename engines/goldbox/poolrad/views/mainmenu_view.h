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

#ifndef GOLDBOX_POOLRAD_VIEWS_MAINMENU_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_MAINMENU_VIEW_H

#include "common/rect.h"
//#include "common/array.h"
#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

struct Menuitem {
    char shortcut;
    const char* text;
    bool active;
};

class MainmenuView : public View {
private:

//    static const Common::Rect _area_party;
//    static const Common::Rect _area_menu;

    Menuitem mainmenu[11] = {
        {'C',"reate New Character",        true},
        {'D',"rop Character",              false},
        {'M',"odify Character",            false},
        {'T',"rain Character",             false},
        {'V',"iew Character",              false},
        {'A',"dd Character to Party",      true},
        {'R',"emove Character from Party", false},
        {'L',"oad Saved Game",             true},
        {'S',"ave Current Game",           false},
        {'B',"EGIN Adventuring",           false},
        {'E',"xit to DOS",                 true}
    };

public:
    MainmenuView();
    virtual ~MainmenuView() {}

    bool msgKeypress(const KeypressMessage &msg) override;
    bool msgFocus(const FocusMessage &msg) override;
    bool msgUnfocus(const UnfocusMessage &msg) override;
    void draw() override;
    void timeout() override;
    void showMenu();
    void showParty();
    void menuSetActive(char shortcut);
    void menuSetInactive(char shortcut);
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
