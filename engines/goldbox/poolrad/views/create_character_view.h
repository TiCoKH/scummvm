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
#include "common/array.h"
#include "goldbox/poolrad/views/view.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/views/dialogs/character_profile.h"
#include "goldbox/poolrad/views/dialogs/horizontal_input.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {

// Character creation stages
enum CharacterCreateState {
    CC_STATE_RACE = 0,
    CC_STATE_GENDER,
    CC_STATE_CLASS,
    CC_STATE_ALIGNMENT,
    CC_STATE_PROFILE,
    CC_STATE_NAME,
    CC_STATE_ICON,
    CC_STATE_DONE
};

class CreateCharacterView : public View {

public:
    CreateCharacterView();
    virtual ~CreateCharacterView();
    void nextStage();
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
    Common::String _enteredName;
    uint8 _baseClassNums = 0;

    // subviews for each state
    // Reusable single vertical menu (we rebuild its items per stage)

    Dialogs::Dialog *_nameInput = nullptr;
    // Reusable menu items container
    Goldbox::MenuItemList *_menuItems = nullptr;
    Dialogs::VerticalMenu *_menu = nullptr;

    Dialogs::CharacterProfile *_profileDialog = nullptr;
    Dialogs::Dialog *_activeSubView = nullptr;
    Common::String formatFilename(const Common::String &name); // helper to format save filename
    bool _confirmSave = false; // true when returning from icon editor to confirm save
    bool _hasRolled = false;    // ensures we roll once when entering profile

    // mapping arrays: visible index -> enum/index value
    Common::Array<int> _indexMap;

    // character under construction
    Goldbox::Poolrad::Data::PoolradCharacter *_newCharacter = nullptr;

    void setActiveSubView(Dialogs::Dialog *dlg);

    // Do not override msgKeypress so active subview receives input first
    bool msgFocus(const FocusMessage &msg) override;
    bool msgUnfocus(const UnfocusMessage &msg) override;
    void draw() override;
    void timeout() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    // (Removed legacy menu helpers present in earlier version)

    // Show race selection dialog
    void chooseRace();
    void chooseGender();
    void chooseClass();
    void chooseAlignment();
    void showProfileDialog();
    void chooseName();
    void buildAndShowMenu(const Common::String &prompt);
    void resetState();
    void setThiefSkillsForNewCharacter();
    void setThac0();
    void setSavingThrows();
    void applySpells();
    void setInitGold();
    void setAge();
    void ageingEffects();
    void applyStatMinMax();

    // persistence helpers
    void finalizeCharacterAndSave();
    Common::String formatBaseFilename(const Common::String &name);
    void appendLineToTextFile(const Common::String &fileName,
                              const Common::String &line);

    // reroll helper
    void rollAndRecompute();
};

} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif
