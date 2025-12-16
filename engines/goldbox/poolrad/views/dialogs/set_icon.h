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
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Gfx {
    class Icon;
}

namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * SetIcon dialog allows customization of character icon appearance.
 *
 * State machine for editing character icon colors:
 * - STATE_MAIN_MENU: Initial menu (P/1/2/S keys)
 * - STATE_MAJOR_PART: Select Head or Weapon for color cycling
 * - STATE_NIBBLE_EDIT: Select and edit sub-part color nibbles
 * - STATE_BINARY_ATTR: Toggle binary attribute
 * - STATE_ADJUSTMENT: Inner loop for P/N/K/E adjustments
 */
class SetIcon : public Dialog {
public:
    SetIcon(const Common::String &name,
            Goldbox::Poolrad::Data::PoolradCharacter *pc);
    ~SetIcon() override;

    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void handleMenuResult(bool success, Common::KeyCode key, short value) override;

private:
    static const uint8 MAX_HEAD_ICON = 0x0D;
    static const uint8 MAX_BODY_ICON = 0x1F;

    // Sub-part indices
    enum SubPartIndex {
        SUBPART_BODY = 1,
        SUBPART_ARMS = 2,
        SUBPART_LEGS = 3,
        SUBPART_HEAD_FACE = 4,
        SUBPART_SHIELD = 5,
        SUBPART_WEAPON = 6
    };

    // State machine
    enum IconState {
        STATE_MAIN_MENU = 1,
        STATE_MAJOR_PART = 2,
        STATE_NIBBLE_EDIT = 3,
        STATE_BINARY_ATTR = 4,
        STATE_ADJUSTMENT = 5
    };

    // Adjustment mode (used within STATE_ADJUSTMENT)
    enum AdjustmentMode {
        ADJUST_MAJOR_PART = 1,
        ADJUST_NIBBLE = 2,
        ADJUST_BINARY_ATTR = 3
    };

    // UI constants
    static const uint8 kBackgroundColor = 8;
    static const uint8 kWindowLeft = 1;
    static const uint8 kWindowTop = 1;
    static const uint8 kWindowRight = 38;
    static const uint8 kWindowBottom = 22;

    static const byte kPromptColor = 13;
    static const byte kTextColor = 10;
    static const byte kSelectColor = 15;

    // Data
    Goldbox::Poolrad::Data::PoolradCharacter *_pc;
    HorizontalMenu *_menu = nullptr;
    Goldbox::MenuItemList _menuItems;
    
    // Icon preview: old (committed) vs new (working)
    // Each state has ready and action variants
    Goldbox::Gfx::Icon *_oldIcon;       // Icon before any changes (ready state)
    Goldbox::Gfx::Icon *_oldIconAction; // Icon before any changes (action state)
    Goldbox::Gfx::Icon *_newIcon;       // Icon with current edits (ready state)
    Goldbox::Gfx::Icon *_newIconAction; // Icon with current edits (action state)

    // State tracking
    IconState _state = STATE_MAIN_MENU;
    AdjustmentMode _adjustMode = ADJUST_MAJOR_PART;

    // Editing context
    uint8 _majorPart = 'H';       // 'H' or 'W'
    SubPartIndex _subPartIndex = SUBPART_BODY;
    uint8 _nibbleSelection = 0;   // 0 = low, 1 = high

    // Backups for commit/cancel (full icon data backup)
    Goldbox::Data::CombatIconData _backupIconData;
    bool _staticDrawn = false;
    
    // Icon cycling state (head and body sprite indices)
    bool _inHeadBodyMode = false;  // Whether cycling head/body sprites

    // State methods
    void showMainMenu();
    void showMajorPartMenu();
    void showSubPartMenu();
    void showBinaryAttrMenu();
    void showAdjustmentMenu();

    void drawStatic();
    void drawDynamic();

    // Edit handlers
    void handleMajorPartEdit(Common::KeyCode key);
    void handleNibbleEdit(Common::KeyCode key);
    void handleBinaryAttrEdit(Common::KeyCode key);
    void handleSubPartSelection(Common::KeyCode key);
    void handleHeadBodyCycle(Common::KeyCode key);

    uint8 packSubpartColor(SubPartIndex index) const;
    void applySubpartColor(SubPartIndex index, uint8 value);

    // Utility methods
    void cycleColorIndex(uint8 *colorPtr, uint8 maxValue, bool increment);
    uint8 incrementNibble(uint8 value);
    uint8 decrementNibble(uint8 value);

    uint8 getLowNibble(uint8 byte) const;
    uint8 getHighNibble(uint8 byte) const;
    uint8 setLowNibble(uint8 byte, uint8 nibble) const;
    uint8 setHighNibble(uint8 byte, uint8 nibble) const;

    void commitChanges();
    void revertChanges();
    void rebuildNewIcon();
    
    // Head/Body/Speckle features
    void cycleHead(bool forward);
    void cycleBody(bool forward);
    void applySpeckle();
    void rebuildAllIcons();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SET_ICON_H

