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

#include "goldbox/poolrad/views/dialogs/set_icon.h"
#include "goldbox/gfx/icon.h"
#include "goldbox/core/icon_manager.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using Common::String;
using Common::KeyCode;
using Common::Array;
using Goldbox::Poolrad::Data::PoolradCharacter;
using Goldbox::Gfx::Icon;

SetIcon::SetIcon(const String &name, PoolradCharacter *pc)
    : Dialog(name), _pc(pc) {
    if (_pc) {
        _pc->iconData.iconBody = 0;
        _pc->iconData.iconHead = 0;
        _backupIconData = _pc->iconData;

        // Load selection frame from COMSPR (block 25 = ready, block 153 = action)
        IconManager *mgr = VmInterface::getIconManager();
        if (mgr) {
            mgr->loadIcon(SLOT_SELECTFRAME, ICON_KIND_SPRITE, 25);
            // Publish initial previews into the global IconManager
            syncIconManagerSlots();
        }
    }

    // Initialize stage-based menu flow
    setStage(ICON_STATE_MAIN_MENU);
}

SetIcon::~SetIcon() {
    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }
}

uint8 SetIcon::getLowNibble(uint8 byte) const {
    return byte & 0x0F;
}

uint8 SetIcon::getHighNibble(uint8 byte) const {
    return (byte >> 4) & 0x0F;
}

uint8 SetIcon::setLowNibble(uint8 byte, uint8 nibble) const {
    return (byte & 0xF0) | (nibble & 0x0F);
}

uint8 SetIcon::setHighNibble(uint8 byte, uint8 nibble) const {
    return (byte & 0x0F) | ((nibble & 0x0F) << 4);
}

uint8 SetIcon::incrementNibble(uint8 value) {
    return (value + 1) % 16;
}

uint8 SetIcon::decrementNibble(uint8 value) {
    return (value == 0) ? 15 : (value - 1);
}

void SetIcon::cycleColorIndex(uint8 *colorPtr, uint8 maxValue, bool increment) {
    if (!colorPtr)
        return;

    if (increment) {
        *colorPtr = (*colorPtr >= maxValue) ? 0 : (*colorPtr + 1);
    } else {
        *colorPtr = (*colorPtr == 0) ? maxValue : (*colorPtr - 1);
    }
}

uint8 SetIcon::packSubpartColor(SubPartIndex index) const {
    if (!_pc)
        return 0;

    switch (index) {
    case SUBPART_BODY:
        return static_cast<uint8>((_pc->iconData.iconColorBody2 << 4) | (_pc->iconData.iconColorBody1 & 0x0F));
    case SUBPART_ARMS:
        return static_cast<uint8>((_pc->iconData.iconColorArm2 << 4) | (_pc->iconData.iconColorArm1 & 0x0F));
    case SUBPART_LEGS:
        return static_cast<uint8>((_pc->iconData.iconColorLeg2 << 4) | (_pc->iconData.iconColorLeg1 & 0x0F));
    case SUBPART_HEAD_FACE:
        return static_cast<uint8>((_pc->iconData.iconColorFace << 4) | (_pc->iconData.iconColorHair & 0x0F));
    case SUBPART_SHIELD:
        return static_cast<uint8>((_pc->iconData.iconColorShield2 << 4) | (_pc->iconData.iconColorShield1 & 0x0F));
    case SUBPART_WEAPON:
        return static_cast<uint8>((_pc->iconData.iconColorWeapon2 << 4) | (_pc->iconData.iconColorWeapon1 & 0x0F));
    default:
        return 0;
    }
}

void SetIcon::applySubpartColor(SubPartIndex index, uint8 value) {
    if (!_pc)
        return;

    switch (index) {
    case SUBPART_BODY:
        _pc->iconData.setBodyColor(value);
        break;
    case SUBPART_ARMS:
        _pc->iconData.setArmColor(value);
        break;
    case SUBPART_LEGS:
        _pc->iconData.setLegColor(value);
        break;
    case SUBPART_HEAD_FACE:
        _pc->iconData.setHairFaceColor(value);
        break;
    case SUBPART_SHIELD:
        _pc->iconData.setShieldColor(value);
        break;
    case SUBPART_WEAPON:
        _pc->iconData.setWeaponColor(value);
        break;
    default:
        break;
    }
}

void SetIcon::nextStage() {
    setStage(static_cast<IconMenuState>(_stage + 1));
}

void SetIcon::setStage(IconMenuState stage) {
    _stage = stage;

    switch (_stage) {
    case ICON_STATE_MAIN_MENU:
        _menuItems.items.clear();
        {
            Common::Array<String> labels;
            labels.push_back("Parts");
            labels.push_back("color-1");
            labels.push_back("color-2");
            labels.push_back("Size");
            labels.push_back("Exit");
            _menuItems.generateMenuItems(labels, true);
            // Ensure distinct shortcuts for color items: use trailing digit
            _menuItems.setShortcutToLast(1); // color-1 -> '1'
            _menuItems.setShortcutToLast(2); // color-2 -> '2'
        }
        buildAndShowMenu("");
        break;
    case ICON_STATE_MAJOR_PART:
        _menuItems.items.clear();
        {
            Common::Array<String> labels;
            labels.push_back("Head");
            labels.push_back("Weapon");
            labels.push_back("Exit");
            _menuItems.generateMenuItems(labels, true);
        }
        buildAndShowMenu("");
        break;
    case ICON_STATE_SUB_PART:
        setMenuStage(ICON_STATE_SUB_PART);
        break;
    case ICON_STATE_SIZE_SELECT:
        _menuItems.items.clear();
        {
            Common::Array<String> labels;
            labels.push_back("Large");
            labels.push_back("Small");
            labels.push_back("Exit");
            _menuItems.generateMenuItems(labels, true);
        }
        buildAndShowMenu("");
        break;
    case ICON_PARTS_ADJUSTMENT:
        _menuItems.items.clear();
        {
            Common::Array<String> labels;
            labels.push_back("Next");
            labels.push_back("Prev");
            labels.push_back("Keep");
            labels.push_back("Exit");
            _menuItems.generateMenuItems(labels, true);
        }
        buildAndShowMenu("");
        break;
    case ICON_COLORS_ADJUSTMENT:
        _menuItems.items.clear();
        {
            Common::Array<String> labels;
            labels.push_back("Next");
            labels.push_back("Prev");
            labels.push_back("Keep");
            labels.push_back("Exit");
            _menuItems.generateMenuItems(labels, true);
        }
        buildAndShowMenu("");
        break;
    case ICON_STATE_CONFIRM:
        _menuItems.items.clear();
        {
            Common::Array<String> labels;
            labels.push_back("Keep");
            labels.push_back("Exit");
            _menuItems.generateMenuItems(labels, true);
        }
        buildAndShowMenu("");
        break;
    }
}

Common::String SetIcon::getSubPartLabel(int index) const {
    // Dynamically choose 'Hair' or 'Face' based on _colorAdjustMode
    switch (index) {
    case SUBPART_BODY:
        return "Body";
    case SUBPART_ARMS:
        return "Arm";
    case SUBPART_LEGS:
        return "Leg";
    case SUBPART_HEAD_FACE:
        // Dynamic: show Hair for color-1, Face for color-2
        return (_colorAdjustMode == 0) ? "Hair" : "Face";
    case SUBPART_SHIELD:
        return "Shield";
    case SUBPART_WEAPON:
        return "Weapon";
    default:
        return "";
    }
}

void SetIcon::buildAndShowMenu(const Common::String &prompt) {
    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }

    HorizontalMenuConfig cfg = {
        prompt,
        &_menuItems,
        kTextColor,
        kSelectColor,
        kPromptColor,
        false,
        kBackgroundColor
    };

    _menu = new HorizontalMenu(getName() + "_Menu", cfg);
    _menu->setParent(this);
    _menu->activate();
}

void SetIcon::setMenuStage(IconMenuState stage) {
    if (stage == ICON_STATE_SUB_PART) {
        if (_menu) {
            _menu->setParent(nullptr);
            delete _menu;
            _menu = nullptr;
        }

        _menuItems.items.clear();
        _indexMap.clear();

        // Build sub-part menu with dynamic Hair/Face label in specified order
        Common::Array<String> labels;
        labels.push_back(getSubPartLabel(SUBPART_WEAPON));
        labels.push_back(getSubPartLabel(SUBPART_BODY));
        labels.push_back(getSubPartLabel(SUBPART_HEAD_FACE));
        labels.push_back(getSubPartLabel(SUBPART_SHIELD));
        labels.push_back(getSubPartLabel(SUBPART_ARMS));
        labels.push_back(getSubPartLabel(SUBPART_LEGS));
        labels.push_back("Exit");

        _menuItems.generateMenuItems(labels, true);

        // Map menu indices to SubPartIndex
        _indexMap.push_back(SUBPART_WEAPON);
        _indexMap.push_back(SUBPART_BODY);
        _indexMap.push_back(SUBPART_HEAD_FACE);
        _indexMap.push_back(SUBPART_SHIELD);
        _indexMap.push_back(SUBPART_ARMS);
        _indexMap.push_back(SUBPART_LEGS);

        Common::String prompt = "";

        HorizontalMenuConfig cfg = {
            prompt,
            &_menuItems,
            kTextColor,
            kSelectColor,
            kPromptColor,
            false,
            kBackgroundColor
        };

        _menu = new HorizontalMenu(getName() + "_SubPart", cfg);
        _menu->setParent(this);
        _menu->activate();
    }
}

void SetIcon::commitChanges() {
    // Update backup to current state
    _backupIconData = _pc->iconData;
    // Sync committed previews into global IconManager
    syncIconManagerSlots();
}

void SetIcon::revertChanges() {
    // Restore icon data from backup
    _pc->iconData = _backupIconData;
    // Reflect reverted state in IconManager
    syncIconManagerSlots();
}

void SetIcon::rebuildNewIcon() {
    // Sync updated previews into global IconManager
    syncIconManagerSlots();
}

// Icon manipulation methods
void SetIcon::cycleHead(bool forward) {
    if (!_pc)
        return;
    if (forward) {
        _pc->iconData.iconHead = Icon::nextHead(_pc->iconData.iconHead);
    } else {
        _pc->iconData.iconHead = Icon::prevHead(_pc->iconData.iconHead);
    }
}

void SetIcon::cycleBody(bool forward) {
    if (!_pc)
        return;
    if (forward) {
        _pc->iconData.iconBody = Icon::nextBody(_pc->iconData.iconBody);
    } else {
        _pc->iconData.iconBody = Icon::prevBody(_pc->iconData.iconBody);
    }
}

void SetIcon::applySpeckle() {
    if (!_pc)
        return;
    // Placeholder: would apply speckle pattern variations
}

void SetIcon::rebuildAllIcons() {
    if (!_pc)
        return;
    syncIconManagerSlots();
}

void SetIcon::syncIconManagerSlots() {
    IconManager *mgr = VmInterface::getIconManager();
    if (!mgr || !_pc)
        return;
    debug("SetIcon::syncIconManagerSlots: backup size=%u head=%u body=%u | working size=%u head=%u body=%u",
          (unsigned)_backupIconData.iconSize,
          (unsigned)_backupIconData.iconHead,
          (unsigned)_backupIconData.iconBody,
          (unsigned)_pc->iconData.iconSize,
          (unsigned)_pc->iconData.iconHead,
          (unsigned)_pc->iconData.iconBody);
    // Publish icons via IconManager
    mgr->loadIcon(SLOT_OVERLAY_BUFFER, _backupIconData, false);
    mgr->loadIcon(SLOT_RESERVED_START, _backupIconData, true);
    mgr->loadIcon(SLOT_EDITOR_WORKING, _pc->iconData, false);
    mgr->loadIcon(static_cast<uint8>(SLOT_RESERVED_START + 1), _pc->iconData, true);
}

void SetIcon::drawIconPair(int x, int y, uint8 slotId) {
    Surface s = getSurface();
    IconManager *mgr = VmInterface::getIconManager();
    if (!mgr)
        return;
    mgr->drawAtPos(&s, x, y, 0, 0, SLOT_SELECTFRAME);
    mgr->drawAtPos(&s, x + 3, y, 0, 1, SLOT_SELECTFRAME);
    mgr->drawAtPos(&s, x, y, 0, 0, slotId);
    mgr->drawAtPos(&s, x + 3, y, 0, 1, slotId);
}

bool SetIcon::msgKeypress(const KeypressMessage &msg) {
    KeyCode keyCode = msg.keycode;

    if (_menu) {
        return _menu->msgKeypress(msg);
    }

    return true;
}

void SetIcon::draw() {
    drawEditorText();
    drawEditorIcons();
}

void SetIcon::drawEditorText() {
    Surface s = getSurface();
    s.clearBox(0, 0, 40, 25, kBackgroundColor);
    drawFrame(Common::Rect(kWindowLeft - 1, kWindowTop - 1, kWindowRight + 1, kWindowBottom + 1), kBackgroundColor);
    s.clearBox(kWindowLeft, kWindowTop, kWindowRight, kWindowBottom, kBackgroundColor);

    // Static labels and layout
	s.writeStringC(8, 6, 15, "old");
	s.writeStringC(3, 10, 15, "ready   action");
	s.writeStringC(8, 12, 15, "new");
	s.writeStringC(3, 16, 15, "ready   action");
}

void SetIcon::drawEditorIcons() {
    // Draw icons using drawIconPair helper
    // OLD ICON: Committed state
    drawIconPair(1, 2, SLOT_OVERLAY_BUFFER);

    // NEW ICON: Working edits
    drawIconPair(1, 4, SLOT_EDITOR_WORKING);

    // Dynamic content: menus
    if (_menu)
        _menu->draw();
}

void SetIcon::redrawWorkingIcons() {
    Surface s = getSurface();
    // Clear only the "new" icon region.
    // Icon coords (1,4) and (4,4) map to char coords:
    // charX = iconX * 3 + 1, charY = iconY * 3 + 1. Each icon spans 3 chars.
    // First icon: x 4..6, second icon: x 13..15, y 13..15.
    s.clearBox(4, 13, 15, 15, kBackgroundColor);
    drawIconPair(1, 4, SLOT_EDITOR_WORKING);
}

void SetIcon::handleMenuResult(bool success, Common::KeyCode key, short value) {
    if (!success) {
        // Cancel pressed - revert changes
        revertChanges();
        if (_parent) {
            _parent->handleMenuResult(false, key, value);
        }
        return;
    }

    switch (_stage) {
    case ICON_STATE_MAIN_MENU: {
        // "Parts color-1 color-2 Size Exit"
        switch (key) {
        case Common::KEYCODE_p:
            // Enter major part selection
            setStage(ICON_STATE_MAJOR_PART);
            break;
        case Common::KEYCODE_c: {
            // Disambiguate color-1 vs color-2 by current selection index
            int sel = _menuItems.currentSelection;
            if (sel == 1) {
                _colorAdjustMode = 0;
                setStage(ICON_STATE_SUB_PART);
            } else if (sel == 2) {
                _colorAdjustMode = 1;
                setStage(ICON_STATE_SUB_PART);
            }
            break;
        }
        case Common::KEYCODE_1:
            // Color-1 (Hair/Shield1/etc)
            _colorAdjustMode = 0;
            setStage(ICON_STATE_SUB_PART);
            break;
        case Common::KEYCODE_2:
            // Color-2 (Face/Shield2/etc)
            _colorAdjustMode = 1;
            setStage(ICON_STATE_SUB_PART);
            break;
        case Common::KEYCODE_s:
            // Size menu
            setStage(ICON_STATE_SIZE_SELECT);
            break;
        case Common::KEYCODE_e:
            // Exit - commit and finish
            commitChanges();
            if (_parent)
                _parent->handleMenuResult(true, key, 0);
            break;
        default: {
            // Fallback: use selection index to resolve color items
            int sel = _menuItems.currentSelection;
            if (sel == 1) {
                _colorAdjustMode = 0;
                setStage(ICON_STATE_SUB_PART);
            } else if (sel == 2) {
                _colorAdjustMode = 1;
                setStage(ICON_STATE_SUB_PART);
            }
            break;
        }
        }
        break;
    }

    case ICON_STATE_MAJOR_PART: {
        // "Head Weapon Exit"
        switch (key) {
        case Common::KEYCODE_h:
            _selectedMajorPart = 0;
            setStage(ICON_PARTS_ADJUSTMENT);
            break;
        case Common::KEYCODE_w:
            _selectedMajorPart = 1;
            setStage(ICON_PARTS_ADJUSTMENT);
            break;
        case Common::KEYCODE_e:
            setStage(ICON_STATE_MAIN_MENU);
            break;
        }
        break;
    }

    case ICON_STATE_SUB_PART: {
        // Dynamic sub-part menu
        switch (key) {
        case Common::KEYCODE_b:
            _selectedSubPart = SUBPART_BODY;
            break;
        case Common::KEYCODE_a:
            _selectedSubPart = SUBPART_ARMS;
            break;
        case Common::KEYCODE_l:
            _selectedSubPart = SUBPART_LEGS;
            break;
        case Common::KEYCODE_h:
        case Common::KEYCODE_f:
            _selectedSubPart = SUBPART_HEAD_FACE;
            break;
        case Common::KEYCODE_s:
            _selectedSubPart = SUBPART_SHIELD;
            break;
        case Common::KEYCODE_w:
            _selectedSubPart = SUBPART_WEAPON;
            break;
        case Common::KEYCODE_e:
            setStage(ICON_STATE_MAIN_MENU);
            return;
        }

        // Enter color adjustment stage for this sub-part
        setStage(ICON_COLORS_ADJUSTMENT);
        break;
    }

    case ICON_STATE_SIZE_SELECT: {
        // "Large Small Exit"
        switch (key) {
        case Common::KEYCODE_l:
            _pc->iconData.iconSize = 2; // Large
            rebuildNewIcon();
            setStage(ICON_STATE_MAIN_MENU);
            break;
        case Common::KEYCODE_s:
            _pc->iconData.iconSize = 1; // Small
            rebuildNewIcon();
            setStage(ICON_STATE_MAIN_MENU);
            break;
        case Common::KEYCODE_e:
            setStage(ICON_STATE_MAIN_MENU);
            break;
        }
        break;
    }

    case ICON_PARTS_ADJUSTMENT: {
        // "Next Prev Keep Exit" - for head/weapon parts
        switch (key) {
        case Common::KEYCODE_n:
            // Cycle part forward
            if (_selectedMajorPart == 0) {
                cycleHead(true);
            } else {
                cycleBody(true);
            }
            rebuildNewIcon();
            redrawWorkingIcons();
            break;
        case Common::KEYCODE_p:
            // Cycle part backward
            if (_selectedMajorPart == 0) {
                cycleHead(false);
            } else {
                cycleBody(false);
            }
            rebuildNewIcon();
            redrawWorkingIcons();
            break;
        case Common::KEYCODE_k:
            // Keep selection and return to major part menu
            setStage(ICON_STATE_MAJOR_PART);
            break;
        case Common::KEYCODE_e:
            // Exit adjustment and return to main menu
            setStage(ICON_STATE_MAIN_MENU);
            break;
        }
        break;
    }

    case ICON_COLORS_ADJUSTMENT: {
        // "Next Prev Keep Exit" - for color values
        switch (key) {
        case Common::KEYCODE_n: {
            // Adjust color nibbles forward
            uint8 colorByte = packSubpartColor(static_cast<SubPartIndex>(_selectedSubPart));
            uint8 nibbleVal;
            if (_colorAdjustMode == 0) {
                // color-1 -> low nibble
                nibbleVal = getLowNibble(colorByte);
                nibbleVal = incrementNibble(nibbleVal);
                colorByte = setLowNibble(colorByte, nibbleVal);
            } else {
                // color-2 -> high nibble
                nibbleVal = getHighNibble(colorByte);
                nibbleVal = incrementNibble(nibbleVal);
                colorByte = setHighNibble(colorByte, nibbleVal);
            }
            applySubpartColor(static_cast<SubPartIndex>(_selectedSubPart), colorByte);
            rebuildNewIcon();
            redrawWorkingIcons();
            break;
        }
        case Common::KEYCODE_p: {
            // Adjust color nibbles backward
            uint8 colorByte = packSubpartColor(static_cast<SubPartIndex>(_selectedSubPart));
            uint8 nibbleVal;
            if (_colorAdjustMode == 0) {
                // color-1 -> low nibble
                nibbleVal = getLowNibble(colorByte);
                nibbleVal = decrementNibble(nibbleVal);
                colorByte = setLowNibble(colorByte, nibbleVal);
            } else {
                // color-2 -> high nibble
                nibbleVal = getHighNibble(colorByte);
                nibbleVal = decrementNibble(nibbleVal);
                colorByte = setHighNibble(colorByte, nibbleVal);
            }
            applySubpartColor(static_cast<SubPartIndex>(_selectedSubPart), colorByte);
            rebuildNewIcon();
            redrawWorkingIcons();
            break;
        }
        case Common::KEYCODE_k:
            // Keep selection and return to sub-part menu
            setStage(ICON_STATE_SUB_PART);
            break;
        case Common::KEYCODE_e:
            // Exit adjustment and return to main menu
            setStage(ICON_STATE_MAIN_MENU);
            break;
        }
        break;
    }

    case ICON_STATE_CONFIRM: {
        // " Keep Exit"
        switch (key) {
        case Common::KEYCODE_k:
            commitChanges();
            if (_parent)
                _parent->handleMenuResult(true, key, 0);
            break;
        case Common::KEYCODE_e:
            revertChanges();
            if (_parent)
                _parent->handleMenuResult(false, key, 0);
            break;
        }
        break;
    }

    default:
        break;
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

