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
    : Dialog(name), _pc(pc), _inHeadBodyMode(false) {
    if (_pc) {
        // Backup current icon data for commit/cancel operations
        _backupIconData = _pc->iconData;

        // Load selection frame from COMSPR (block 25 = ready, block 153 = action)
        IconManager *mgr = VmInterface::getIconManager();
        if (mgr) {
            mgr->loadIcon(SLOT_SELECTFRAME, ICON_KIND_SPRITE, 25);
            // Publish initial previews into the global IconManager
            syncIconManagerSlots();
        }
    }

    showMainMenu();
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
        return static_cast<uint8>((_pc->iconData.iconColorBody1 << 4) | _pc->iconData.iconColorBody2);
    case SUBPART_ARMS:
        return static_cast<uint8>((_pc->iconData.iconColorArm1 << 4) | _pc->iconData.iconColorArm2);
    case SUBPART_LEGS:
        return static_cast<uint8>((_pc->iconData.iconColorLeg1 << 4) | _pc->iconData.iconColorLeg2);
    case SUBPART_HEAD_FACE:
        return static_cast<uint8>((_pc->iconData.iconColorHair << 4) | _pc->iconData.iconColorFace);
    case SUBPART_SHIELD:
        return static_cast<uint8>((_pc->iconData.iconColorShield1 << 4) | _pc->iconData.iconColorShield2);
    case SUBPART_WEAPON:
        return static_cast<uint8>((_pc->iconData.iconColorWeapon1 << 4) | _pc->iconData.iconColorWeapon2);
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

void SetIcon::rebuildNewIcon() {
    // Sync updated previews into global IconManager for external access
    syncIconManagerSlots();
}

void SetIcon::commitChanges() {
    // Update backup to current state (current state is now "committed")
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

void SetIcon::showMainMenu() {
    _state = STATE_MAIN_MENU;

    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }

    _menuItems.items.clear();
    _menuItems.generateMenuItems(Array<String>(), false);

    HorizontalMenuConfig cfg = {
        "P 1 2 S Keep",
        &_menuItems,
        kTextColor,
        kSelectColor,
        kPromptColor,
        false,
        kBackgroundColor
    };

    _menu = new HorizontalMenu(getName() + "_MainMenu", cfg);
    _menu->setParent(this);
    _menu->activate();
}

void SetIcon::showMajorPartMenu() {
    _state = STATE_MAJOR_PART;

    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }

    Array<String> labels;
    labels.push_back("Head");
    labels.push_back("Weapon");
    _menuItems.generateMenuItems(labels, true);

    HorizontalMenuConfig cfg = {
        "Head Weapon",
        &_menuItems,
        kTextColor,
        kSelectColor,
        kPromptColor,
        false,
        kBackgroundColor
    };

    _menu = new HorizontalMenu(getName() + "_MajorPart", cfg);
    _menu->setParent(this);
    _menu->activate();
}

void SetIcon::showSubPartMenu() {
    _state = STATE_NIBBLE_EDIT;

    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }

    Array<String> labels;
    labels.push_back("Body");
    labels.push_back("Arms");
    labels.push_back("Legs");
    labels.push_back("Head");
    labels.push_back("Shield");
    labels.push_back("Weapon");
    _menuItems.generateMenuItems(labels, true);

    HorizontalMenuConfig cfg = {
        "Body Arms Legs Head Shield Weapon",
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

void SetIcon::showBinaryAttrMenu() {
    _state = STATE_BINARY_ATTR;

    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }

    Array<String> labels;
    labels.push_back("Option1");
    labels.push_back("Option2");
    _menuItems.generateMenuItems(labels, true);

    HorizontalMenuConfig cfg = {
        "S L Keep",
        &_menuItems,
        kTextColor,
        kSelectColor,
        kPromptColor,
        false,
        kBackgroundColor
    };

    _menu = new HorizontalMenu(getName() + "_BinaryAttr", cfg);
    _menu->setParent(this);
    _menu->activate();
}

void SetIcon::showAdjustmentMenu() {
    _state = STATE_ADJUSTMENT;

    if (_menu) {
        _menu->setParent(nullptr);
        delete _menu;
        _menu = nullptr;
    }

    _menuItems.items.clear();
    _menuItems.generateMenuItems(Array<String>(), false);

    HorizontalMenuConfig cfg = {
        "P N K E",
        &_menuItems,
        kTextColor,
        kSelectColor,
        kPromptColor,
        false,
        kBackgroundColor
    };

    _menu = new HorizontalMenu(getName() + "_Adjustment", cfg);
    _menu->setParent(this);
    _menu->activate();
}

void SetIcon::handleMajorPartEdit(Common::KeyCode key) {
    uint8 *colorPtr = nullptr;
    uint8 maxValue = 0;

    if (_majorPart == 'H') {
        colorPtr = &_pc->iconData.iconHead;
        maxValue = MAX_HEAD_ICON;
    } else if (_majorPart == 'W') {
        colorPtr = &_pc->iconData.iconBody;
        maxValue = MAX_BODY_ICON;
    }

    if (!colorPtr)
        return;

    switch (key) {
    case Common::KEYCODE_n:
        cycleColorIndex(colorPtr, maxValue, true);
        rebuildNewIcon();
        break;
    case Common::KEYCODE_p:
        cycleColorIndex(colorPtr, maxValue, false);
        rebuildNewIcon();
        break;
    case Common::KEYCODE_k:
        commitChanges();
        showMainMenu();
        break;
    case Common::KEYCODE_e:
    case Common::KEYCODE_ESCAPE:
        revertChanges();
        showMainMenu();
        break;
    default:
        break;
    }
}

void SetIcon::handleSubPartSelection(Common::KeyCode key) {
    switch (key) {
    case Common::KEYCODE_b:
        _subPartIndex = SUBPART_BODY;
        break;
    case Common::KEYCODE_a:
        _subPartIndex = SUBPART_ARMS;
        break;
    case Common::KEYCODE_l:
        _subPartIndex = SUBPART_LEGS;
        break;
    case Common::KEYCODE_h:
        _subPartIndex = SUBPART_HEAD_FACE;
        break;
    case Common::KEYCODE_s:
        _subPartIndex = SUBPART_SHIELD;
        break;
    case Common::KEYCODE_w:
        _subPartIndex = SUBPART_WEAPON;
        break;
    default:
        _subPartIndex = SUBPART_BODY;
        break;
    }

    _state = STATE_ADJUSTMENT;
    _adjustMode = ADJUST_NIBBLE;
    showAdjustmentMenu();
}

void SetIcon::handleNibbleEdit(Common::KeyCode key) {
    uint8 targetByte = packSubpartColor(_subPartIndex);

    switch (key) {
    case Common::KEYCODE_n:
        if (_nibbleSelection == 0) {
            uint8 lowNib = getLowNibble(targetByte);
            lowNib = incrementNibble(lowNib);
            targetByte = setLowNibble(targetByte, lowNib);
        } else {
            uint8 highNib = getHighNibble(targetByte);
            highNib = incrementNibble(highNib);
            targetByte = setHighNibble(targetByte, highNib);
        }
        applySubpartColor(_subPartIndex, targetByte);
        rebuildNewIcon();
        break;

    case Common::KEYCODE_p:
        if (_nibbleSelection == 0) {
            uint8 lowNib = getLowNibble(targetByte);
            lowNib = decrementNibble(lowNib);
            targetByte = setLowNibble(targetByte, lowNib);
        } else {
            uint8 highNib = getHighNibble(targetByte);
            highNib = decrementNibble(highNib);
            targetByte = setHighNibble(targetByte, highNib);
        }
        applySubpartColor(_subPartIndex, targetByte);
        rebuildNewIcon();
        break;

    case Common::KEYCODE_k:
        commitChanges();
        showMainMenu();
        break;

    case Common::KEYCODE_e:
    case Common::KEYCODE_ESCAPE:
        revertChanges();
        showMainMenu();
        break;

    default:
        handleSubPartSelection(key);
        break;
    }
}

void SetIcon::handleBinaryAttrEdit(Common::KeyCode key) {
    switch (key) {
    case Common::KEYCODE_s:
        _pc->iconData.iconSize = 1;
        rebuildNewIcon();
        break;

    case Common::KEYCODE_l:
        _pc->iconData.iconSize = 2;
        rebuildNewIcon();
        break;

    case Common::KEYCODE_k:
        commitChanges();
        showMainMenu();
        break;

    case Common::KEYCODE_e:
    case Common::KEYCODE_ESCAPE:
        revertChanges();
        showMainMenu();
        break;

    default:
        break;
    }
}

void SetIcon::handleHeadBodyCycle(Common::KeyCode key) {
    switch (key) {
    case Common::KEYCODE_n:
        if (_majorPart == 'H') {
            cycleHead(true);
        } else if (_majorPart == 'W') {
            cycleBody(true);
        }
        rebuildAllIcons();
        break;

    case Common::KEYCODE_p:
        if (_majorPart == 'H') {
            cycleHead(false);
        } else if (_majorPart == 'W') {
            cycleBody(false);
        }
        rebuildAllIcons();
        break;

    case Common::KEYCODE_s:
        applySpeckle();
        rebuildAllIcons();
        break;

    case Common::KEYCODE_k:
        commitChanges();
        _inHeadBodyMode = false;
        showMainMenu();
        break;

    case Common::KEYCODE_e:
    case Common::KEYCODE_ESCAPE:
        revertChanges();
        _inHeadBodyMode = false;
        showMainMenu();
        break;

    default:
        break;
    }
}

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

    // Randomize pattern while keeping colors
    // This randomizes the internal pixel pattern of the icon
    // Note: This would require access to the underlying sprite data
    // For now, we'll apply a simple variation to the icon size as a placeholder
    // A full implementation would randomize the sprite pattern bits
    _pc->iconData.iconSize = (_pc->iconData.iconSize == 1) ? 2 : 1;
}

void SetIcon::rebuildAllIcons() {
    if (!_pc)
        return;

    // Sync all previews into IconManager after full rebuild
    syncIconManagerSlots();
}

void SetIcon::syncIconManagerSlots() {
    IconManager *mgr = VmInterface::getIconManager();
    if (!mgr || !_pc)
        return;

    // Publish manager-owned icons via IconManager::loadIcon overload
    mgr->loadIcon(SLOT_OVERLAY_BUFFER, _backupIconData, false);                     // old ready
    mgr->loadIcon(SLOT_RESERVED_START, _backupIconData, true);                    // old action
    mgr->loadIcon(SLOT_EDITOR_WORKING, _pc->iconData, false);                     // new ready
    mgr->loadIcon(static_cast<uint8>(SLOT_RESERVED_START + 1), _pc->iconData, true); // new action
}

void SetIcon::drawIcons(int x, int y, uint8 slotId) {
    Surface s = getSurface();
    IconManager *mgr = VmInterface::getIconManager();
    if (!mgr)
        return;

    // Draw selection frame underneath if requested (slot 25 from COMSPR)
    mgr->drawAtPos(&s, SLOT_SELECTFRAME, 0, 0, y, x);
    mgr->drawAtPos(&s, SLOT_SELECTFRAME, 1, 0, y, x + 3);

    // Draw action state at (y, x) and ready state at (y, x+3)
    // Matches original: GFX_DrawTile(slotId, 0, 0, y, x) and GFX_DrawTile(slotId, 1, 0, y, x+3)
    mgr->drawAtPos(&s, slotId, 0, 0, y, x);       // ready
    mgr->drawAtPos(&s, slotId, 1, 0, y, x + 3);   // action
}

bool SetIcon::msgKeypress(const KeypressMessage &msg) {
    Common::KeyCode key = msg.keycode;

    if (key == Common::KEYCODE_ESCAPE) {
        if (_parent)
            _parent->handleMenuResult(false, key, -1);
        return true;
    }

    if (key == Common::KEYCODE_RETURN) {
        if (_parent)
            _parent->handleMenuResult(true, key, -1);
        return true;
    }

    if (_menu) {
        return _menu->msgKeypress(msg);
    }

    return true;
}

void SetIcon::draw() {
    if (!_staticDrawn) {
        drawEditorText();
        _staticDrawn = true;
    }
    drawEditorIcons();
}

void SetIcon::drawEditorText() {
    Surface s = getSurface();
    s.clearBox(0, 0, 40, 25, kBackgroundColor);
    drawFrame(Common::Rect(kWindowLeft - 1, kWindowTop - 1, kWindowRight + 1, kWindowBottom + 1), kBackgroundColor);
    s.clearBox(kWindowLeft, kWindowTop, kWindowRight, kWindowBottom, kBackgroundColor);

    // Static labels and layout only once
    s.writeStringC("old", 15, 8, 6);
    s.writeStringC("ready   action", 15, 3, 10);
    s.writeStringC("new", 15, 8, 12);
    s.writeStringC("ready   action", 15, 3, 16);
}

void SetIcon::drawEditorIcons() {
    // Draw icons using drawIcons helper (mimics original GFX_DrawIcon)
    // OLD ICON: Committed state (slot 11 = SLOT_OVERLAY_BUFFER)
    drawIcons(1, 2, SLOT_OVERLAY_BUFFER);

    // NEW ICON: Working edits (slot 12 = SLOT_EDITOR_WORKING)
    drawIcons(1, 4, SLOT_EDITOR_WORKING);

    // Dynamic content: menus (bottom row)
    if (_menu)
        _menu->draw();
}

void SetIcon::handleMenuResult(bool success, Common::KeyCode key, short value) {
    if (!success) {
        if (_parent) {
            _parent->handleMenuResult(false, key, value);
        }
        return;
    }

    switch (_state) {
    case STATE_MAIN_MENU:
        switch (key) {
        case Common::KEYCODE_p:
            // Enter head/body cycling mode
            _inHeadBodyMode = true;
            _majorPart = 'H';
            _state = STATE_ADJUSTMENT;
            _adjustMode = ADJUST_MAJOR_PART;
            showAdjustmentMenu();
            break;

        case Common::KEYCODE_1:
            _nibbleSelection = 0;
            showSubPartMenu();
            break;

        case Common::KEYCODE_2:
            _nibbleSelection = 1;
            showSubPartMenu();
            break;

        case Common::KEYCODE_s:
            showBinaryAttrMenu();
            break;

        case Common::KEYCODE_k:
            commitChanges();
            if (_parent)
                _parent->handleMenuResult(true, key, 0);
            break;

        default:
            break;
        }
        break;

    case STATE_MAJOR_PART:
        switch (key) {
        case Common::KEYCODE_h:
            _majorPart = 'H';
            _state = STATE_ADJUSTMENT;
            _adjustMode = ADJUST_MAJOR_PART;
            showAdjustmentMenu();
            break;

        case Common::KEYCODE_w:
            _majorPart = 'W';
            _state = STATE_ADJUSTMENT;
            _adjustMode = ADJUST_MAJOR_PART;
            showAdjustmentMenu();
            break;

        default:
            break;
        }
        break;

    case STATE_NIBBLE_EDIT:
        handleSubPartSelection(key);
        break;

    case STATE_BINARY_ATTR:
        handleBinaryAttrEdit(key);
        break;

    case STATE_ADJUSTMENT:
        if (_inHeadBodyMode) {
            handleHeadBodyCycle(key);
        } else if (_adjustMode == ADJUST_MAJOR_PART) {
            handleMajorPartEdit(key);
        } else if (_adjustMode == ADJUST_NIBBLE) {
            handleNibbleEdit(key);
        } else if (_adjustMode == ADJUST_BINARY_ATTR) {
            handleBinaryAttrEdit(key);
        }
        break;

    default:
        break;
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox


