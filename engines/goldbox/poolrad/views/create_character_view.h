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

#ifndef GOLDBOX_POOLRAD_VIEWS_CREATE_CHARACTER_VIEW_H
#define GOLDBOX_POOLRAD_VIEWS_CREATE_CHARACTER_VIEW_H

#include "common/rect.h"
//#include "common/array.h"
#include "goldbox/poolrad/views/view.h"

namespace Goldbox {
struct MenuItemList; // forward declaration
namespace Poolrad {
namespace Views {

namespace Dialogs {
class Dialog;
class VerticalMenu;
class CharacterProfile;
}

// Character creation stages
enum CharacterCreateState {
    CC_STATE_RACE = 0,
    CC_STATE_GENDER,
    CC_STATE_CLASS,
    CC_STATE_ALIGNMENT,
    CC_STATE_PROFILE,
    CC_STATE_ICON,
    CC_STATE_DONE
};

class CreateCharacterView : public View {

public:
    CreateCharacterView();
    virtual ~CreateCharacterView();
    // Stage management
    void nextStage();
    void prevStage();
    void setStage(CharacterCreateState stage);
    CharacterCreateState getStage() const { return _stage; }
    void handleMenuResult(bool success, Common::KeyCode key, short value) override;

private:
    CharacterCreateState _stage = CC_STATE_RACE;
    // selection indices for each step
    int _selectedRace = -1;
    int _selectedGender = -1;
    int _selectedClass = -1;
    int _selectedAlignment = -1;

    // subviews for each state
    Dialogs::VerticalMenu *_raceMenu = nullptr;
    Dialogs::VerticalMenu *_genderMenu = nullptr;
    Dialogs::VerticalMenu *_classMenu = nullptr;
    Dialogs::VerticalMenu *_alignmentMenu = nullptr;
    Goldbox::MenuItemList *_raceMenuItems = nullptr;
    Goldbox::MenuItemList *_genderMenuItems = nullptr;
    Goldbox::MenuItemList *_classMenuItems = nullptr;
    Goldbox::MenuItemList *_alignmentMenuItems = nullptr;
    Dialogs::CharacterProfile *_profileDialog = nullptr;
    Dialogs::Dialog *_activeSubView = nullptr;

    void setActiveSubView(Dialogs::Dialog *dlg);

    // Do not override msgKeypress so active subview receives input first
    bool msgFocus(const FocusMessage &msg) override;
    bool msgUnfocus(const UnfocusMessage &msg) override;
    void draw() override;
    void timeout() override;
    void showMenu();
    void showParty();
    void menuSetActive(char shortcut);
    void menuSetInactive(char shortcut);

    // Show race selection dialog
    void chooseRace();
    void chooseGender();
    void chooseClass();
    void chooseAlignment();
    void showProfileDialog();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
