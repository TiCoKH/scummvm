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

#ifndef GOLDBOX_POOLRAD_VIEWS_VIEWCHARACTERVIEW_H
#define GOLDBOX_POOLRAD_VIEWS_VIEWCHARACTERVIEW_H

#include "common/array.h"
#include "goldbox/poolrad/views/view.h"
#include "goldbox/data/player_character.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

namespace Dialogs {
class Dialog;
class ItemsMenu;
}

class ViewCharacterView : public View {
public:
	ViewCharacterView();
    ViewCharacterView(Goldbox::Poolrad::Data::PoolradCharacter *character);
    ~ViewCharacterView() override;

    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(bool success, Common::KeyCode key, short value) override;

private:
    enum ViewCharacterState {
        VC_STATE_PROFILE = 0,
        VC_STATE_ITEMS
    };

    ViewCharacterState _stage = VC_STATE_PROFILE;

    Goldbox::Poolrad::Data::PoolradCharacter *_character;
    Goldbox::MenuItemList _menuList;
    Dialogs::HorizontalMenu *_horizontalMenu;
    Dialogs::CharacterProfile *_profileDialog;
    //Dialogs::ItemsMenu *_itemsMenu;
    Dialogs::Dialog *_activeSubView = nullptr;

    void buildMenu();
    void drawMenu();
    void setStage(ViewCharacterState stage);
    void setActiveSubView(Dialogs::Dialog *dlg);

    // Action handlers
    void handleViewItems();
    void handleViewSpells();
    void handleTradeValuables();
    void handleDropValuables();
    void handleRenameCharacter();
    void handleExit();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
