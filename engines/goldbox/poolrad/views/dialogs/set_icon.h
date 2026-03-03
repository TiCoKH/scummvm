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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_SET_ICON_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_SET_ICON_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Gfx {
    class Icon;
}

namespace Poolrad {
namespace Views {
namespace Dialogs {

// Icon editor menu stages
enum IconMenuState {
    ICON_STATE_MAIN_MENU      = 0,   // Parts color-1 color-2 Size Exit
    ICON_STATE_MAJOR_PART     = 1,   // Head Weapon Exit
    ICON_STATE_SUB_PART       = 2,   // Weapon Body Hair/Face Shield Arm Leg Exit
    ICON_STATE_SIZE_SELECT    = 3,   // Large Small Exit
    ICON_PARTS_ADJUSTMENT     = 4,   // Next Prev Keep Exit (for head/weapon)
    ICON_COLORS_ADJUSTMENT    = 5,   // Next Prev Keep Exit (for colors)
    ICON_STATE_CONFIRM        = 6    // Keep Exit
};

class SetIcon : public Dialog {
public:
    SetIcon(const Common::String &name,
            Goldbox::Poolrad::Data::PoolradCharacter *pc);
    ~SetIcon() override;

    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void handleMenuResult(const MenuResultMessage &result) override;
    void handleMenuResult(bool success, Common::KeyCode key, short value) override;
    void nextStage();
    void setStage(IconMenuState stage);
    IconMenuState getStage() const { return _stage; }

private:
    static const uint8 MAX_HEAD_ICON = 13;
    static const uint8 MAX_BODY_ICON = 31;

    // Sub-part indices
    enum SubPartIndex {
        SUBPART_BODY      = 0,
        SUBPART_ARMS      = 1,
        SUBPART_LEGS      = 2,
        SUBPART_HEAD_FACE = 3,
        SUBPART_SHIELD    = 4,
        SUBPART_WEAPON    = 5
    };

    // UI constants
    static const uint8 kBackgroundColor = 8;
    static const uint8 kWindowLeft      = 1;
    static const uint8 kWindowTop       = 1;
    static const uint8 kWindowRight     = 38;
    static const uint8 kWindowBottom    = 22;

    static const byte kPromptColor = 13;
    static const byte kTextColor   = 10;
    static const byte kSelectColor = 15;

    // Data
    Goldbox::Poolrad::Data::PoolradCharacter *_pc;
    HorizontalMenu *_menu = nullptr;
    HorizontalYesNo *_yesNoPrompt = nullptr;
    Goldbox::MenuItemList _menuItems;

    // Backups for commit/cancel
    Goldbox::Data::CombatIconData _backupIconData;

    // Stage tracking
    IconMenuState _stage = ICON_STATE_MAIN_MENU;

    // Current selections
    int _selectedMajorPart = 0;    // 0 = Head, 1 = Weapon
    int _selectedSubPart   = 0;    // Body, Arms, Legs, Head/Face, Shield, Weapon
    int _selectedSize      = 0;    // 0 = Large, 1 = Small
    int _colorAdjustMode   = 0;    // Which color being adjusted

    uint8 _originalColorByte;

    // Index mapping for sub-part menu (accounts for dynamic Hair/Face)
    Common::Array<int> _indexMap;

    // State helpers
    void buildAndShowMenu(const Common::String &prompt);
    void setActiveMenu();
    void setMenuStage(IconMenuState stage);
    Common::String getSubPartLabel(int index) const;
    void commitChanges();
    void revertChanges();
    void rebuildNewIcon();

    // Utility methods
    uint8 getLowNibble(uint8 byte) const;
    uint8 getHighNibble(uint8 byte) const;
    uint8 setLowNibble(uint8 byte, uint8 nibble) const;
    uint8 setHighNibble(uint8 byte, uint8 nibble) const;
    uint8 incrementNibble(uint8 value);
    uint8 decrementNibble(uint8 value);
    void cycleColorIndex(uint8 *colorPtr, uint8 maxValue, bool increment);

    uint8 packSubpartColor(SubPartIndex index) const;
    void applySubpartColor(SubPartIndex index, uint8 value);

    // Icon manipulation
    void cycleHead(bool forward);
    void cycleBody(bool forward);
    void applySpeckle();
    void rebuildAllIcons();
    void syncIconManagerSlots();
    void drawIconPair(int x, int y, uint8 slotId);
    void drawEditorText();
    void drawEditorIcons();
    void redrawWorkingIcons();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SET_ICON_H

