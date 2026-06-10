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

#include "goldbox/poolrad/views/dialogs/door_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/data/poolrad_vm_layout.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/data/spells/spell.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/runtime/runtime_geo.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"
#include "goldbox/core/direction.h"
#include "common/debug.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

static bool readRuntimeDoorPos(int &x, int &y, uint8 &wireDir) {
    if (!Poolrad::g_engine)
        return false;

    const ECL::AddressSpace *mem = Poolrad::g_engine->getEclMemory();
    if (!mem)
        return false;

    const Goldbox::VmGlobalLayout &globalLayout =
        Data::getPoolradGlobalVmLayout();
    const uint16 xAddr = globalLayout.field(kVmGlobalFieldDungeonX).vmAddr;
    const uint16 yAddr = globalLayout.field(kVmGlobalFieldDungeonY).vmAddr;
    const uint16 dirAddr = globalLayout.field(kVmGlobalFieldDungeonDir).vmAddr;

    x = (int)mem->read8(xAddr);
    y = (int)mem->read8(yAddr);
    wireDir = static_cast<uint8>((mem->read8(dirAddr) & 0x03) * 2);
    return true;
}

DoorDialog::DoorDialog(const Common::String &name)
    : Dialog(name), _horizontalMenu(nullptr),
      _doorFlag(0), _bashAllowed(true), _pickAllowed(true),
      _knockAllowed(true) {
    setBounds(Window(0, 24, 39, 24));
}

DoorDialog::~DoorDialog() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Party query helpers
// ---------------------------------------------------------------------------

bool DoorDialog::partyHasThiefClass() const {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return false;
    for (uint i = 0; i < party->size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[i]);
        if (!pc)
            continue;
        if (pc->levels[Goldbox::Data::C_THIEF] > 0)
            return true;
    }
    return false;
}

bool DoorDialog::partyHasMageClass() const {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return false;
    for (uint i = 0; i < party->size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[i]);
        if (!pc)
            continue;
        if (pc->levels[Goldbox::Data::C_MAGICUSER] > 0)
            return true;
    }
    return false;
}

bool DoorDialog::partyHasKnockSpell() const {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return false;
    for (uint i = 0; i < party->size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[i]);
        if (!pc)
            continue;
        for (int s = 0; s < 21; ++s) {
            if (pc->spells.memorizedSpells[s] ==
                    Goldbox::Data::Spells::SP_MUL2_KNOCK)
                return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Door actions
// ---------------------------------------------------------------------------

bool DoorDialog::tryBash() {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return false;

    for (uint i = 0; i < party->size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[i]);
        if (!pc || pc->healthStatus != Goldbox::Data::S_OKAY)
            continue;

        uint8 str = pc->abilities.strength.current;
        uint8 strExt = pc->abilities.strException.current;

        if (_doorFlag == 3) {
            if (str == 18) {
                if (strExt >= 91 && strExt <= 99) {
                    if (Goldbox::VmInterface::rollDice(1, 6) == 1)
                        return true;
                } else if (strExt == 100) {
                    if (Goldbox::VmInterface::rollDice(1, 6) < 3)
                        return true;
                } else {
                    _bashAllowed = false;
                }
            } else if (str == 19 || str == 20) {
                if (Goldbox::VmInterface::rollDice(1, 6) < 4)
                    return true;
            } else if (str == 21 || str == 22) {
                if (Goldbox::VmInterface::rollDice(1, 6) < 5)
                    return true;
            } else if (str == 23) {
                if (Goldbox::VmInterface::rollDice(1, 6) < 6)
                    return true;
            } else if (str == 24) {
                if (Goldbox::VmInterface::rollDice(1, 8) < 8)
                    return true;
            } else if (str >= 25) {
                return true;
            } else {
                _bashAllowed = false;
            }
        }

        if (_doorFlag == 2) {
            if (str >= 3 && str <= 7) {
                if (Goldbox::VmInterface::rollDice(1, 6) == 1)
                    return true;
            } else if (str >= 8 && str <= 15) {
                if (Goldbox::VmInterface::rollDice(1, 6) < 3)
                    return true;
            } else if (str == 16 || str == 17) {
                if (Goldbox::VmInterface::rollDice(1, 6) < 4)
                    return true;
            } else if (str == 18) {
                if (strExt < 51) {
                    if (Goldbox::VmInterface::rollDice(1, 6) < 4)
                        return true;
                } else if (strExt <= 99) {
                    if (Goldbox::VmInterface::rollDice(1, 6) < 5)
                        return true;
                } else if (strExt == 100) {
                    if (Goldbox::VmInterface::rollDice(1, 6) < 6)
                        return true;
                }
            }
        }
    }
    return false;
}

bool DoorDialog::tryLockpick() {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return false;

    for (uint i = 0; i < party->size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[i]);
        if (!pc || pc->healthStatus != Goldbox::Data::S_OKAY)
            continue;

        int roll = Goldbox::VmInterface::rollDice(1, 100);
        if (roll <= (int)pc->thiefSkills.openLocks)
            return true;
    }
    _pickAllowed = false;
    return false;
}

bool DoorDialog::useKnock() {
    Common::Array<Goldbox::Data::PlayerCharacter *> *party =
        Goldbox::VmInterface::getParty();
    if (!party)
        return false;

    for (uint i = 0; i < party->size(); ++i) {
        Goldbox::Poolrad::Data::PoolradCharacter *pc =
            dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>((*party)[i]);
        if (!pc)
            continue;
        for (int s = 0; s < 21; ++s) {
            if (pc->spells.memorizedSpells[s] ==
                    Goldbox::Data::Spells::SP_MUL2_KNOCK) {
                pc->spells.memorizedSpells[s] = 0;
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Menu building
// ---------------------------------------------------------------------------

void DoorDialog::buildMenuModel() {
    _menuModel.items.clear();
    _menuModel.currentSelection = 0;

    Common::Array<Common::String> menuStrings;
    if (_bashAllowed)
        menuStrings.push_back("Bash");

    if (_pickAllowed) {
        if (_doorFlag == 2 && partyHasThiefClass())
            menuStrings.push_back("Pick");
        else if (_doorFlag == 3 && partyHasMageClass())
            menuStrings.push_back("Pick");
    }

    if (_knockAllowed) {
        if (_doorFlag == 2 && partyHasKnockSpell())
            menuStrings.push_back("Knock");
        else if (_doorFlag == 3 && partyHasMageClass())
            menuStrings.push_back("Knock");
    }

    menuStrings.push_back("Exit");
    _menuModel.generateMenuItems(menuStrings, true);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void DoorDialog::openDoor(uint8 doorFlag) {
    _doorFlag = doorFlag;
    _bashAllowed = true;
    _pickAllowed = true;
    _knockAllowed = true;

    // Only flag 2/3 should reach here; guard against misuse.
    if (_doorFlag < 2)
        return;

    // Build menu and check if only "Exit" is available.
    // If so, clear the geo flag silently (original behavior).
    buildMenuModel();
    if (_menuModel.items.size() == 1 &&
            _menuModel.items[0].shortcut == 'E') {
        RuntimeGeoBlock &rtGeo = Goldbox::VmInterface::getRuntimeGeo();
        int x = 0;
        int y = 0;
        uint8 wireDir = 0;
        if (rtGeo.isLoaded() && readRuntimeDoorPos(x, y, wireDir)) {
            rtGeo.clearFlag(x, y, wireDir);
        }
        postResult(false);
        return;
    }

    activate();
}

void DoorDialog::activate() {
    Dialog::activate();

    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    buildMenuModel();

    HorizontalMenuConfig cfg;
    cfg.promptTxt = "Locked!";
    cfg.menuItemList = &_menuModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 15;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;

    _horizontalMenu = new HorizontalMenu("DoorHMenu", cfg);
    _horizontalMenu->activate();
}

void DoorDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void DoorDialog::draw() {
    if (!_isVisible)
        return;
    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }
}

bool DoorDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    bool wasActive = _horizontalMenu->isActive();
    bool handled = _horizontalMenu->msgKeypress(msg);

    if (!handled)
        return true; // Consume all keys while door dialog is active.

    if (wasActive && !_horizontalMenu->isActive()) {
        if (msg.keycode == Common::KEYCODE_ESCAPE) {
            postResult(false);
            return true;
        }

        char ascii = msg.ascii;
        if (ascii >= 'a' && ascii <= 'z')
            ascii = ascii - 32;

        bool validKey = false;
        for (uint i = 0; i < _menuModel.items.size(); ++i) {
            if (_menuModel.items[i].shortcut == ascii) {
                validKey = true;
                break;
            }
        }

        if (!validKey && msg.keycode == Common::KEYCODE_RETURN) {
            int sel = _menuModel.currentSelection;
            if (sel >= 0 && sel < (int)_menuModel.items.size()) {
                ascii = _menuModel.items[sel].shortcut;
                validKey = true;
            }
        }

        if (validKey)
            handleMenuKey(ascii);
        else {
            _horizontalMenu->activate();
            _horizontalMenu->setRedraw();
        }
    }

    redraw();
    return true;
}

// ---------------------------------------------------------------------------
// Menu dispatch
// ---------------------------------------------------------------------------

void DoorDialog::handleMenuKey(char key) {
    bool opened = false;

    switch (key) {
    case 'B':
        opened = tryBash();
        break;
    case 'P':
        if (_doorFlag == 3) {
            _pickAllowed = false;
        } else {
            opened = tryLockpick();
        }
        break;
    case 'K':
        opened = useKnock();
        break;
    case 'E':
        postResult(false);
        return;
    default:
        break;
    }

    if (opened) {
        RuntimeGeoBlock &rtGeo = Goldbox::VmInterface::getRuntimeGeo();
        int x = 0;
        int y = 0;
        uint8 wireDir = 0;
        if (rtGeo.isLoaded() && readRuntimeDoorPos(x, y, wireDir)) {

            rtGeo.setTileDirectionState(x, y, wireDir);

            int oppX = x + kDirDeltaX[wireDir];
            int oppY = y + kDirDeltaY[wireDir];
            uint8 oppDir = dirReverse(wireDir);
            rtGeo.setTileDirectionState(oppX, oppY, oppDir);
        }
        postResult(true);
    } else {
        buildMenuModel();
        if (_menuModel.items.size() == 1 &&
                _menuModel.items[0].shortcut == 'E') {
            RuntimeGeoBlock &rtGeo = Goldbox::VmInterface::getRuntimeGeo();
            int x = 0;
            int y = 0;
            uint8 wireDir = 0;
            if (rtGeo.isLoaded() && readRuntimeDoorPos(x, y, wireDir)) {
                rtGeo.clearFlag(x, y, wireDir);
            }
            postResult(false);
        } else if (_horizontalMenu) {
            delete _horizontalMenu;
            _horizontalMenu = nullptr;

            HorizontalMenuConfig cfg;
            cfg.promptTxt = "Locked!";
            cfg.menuItemList = &_menuModel;
            cfg.textColor = 10;
            cfg.selectColor = 15;
            cfg.promptColor = 15;
            cfg.allowNumPad = false;
            cfg.suppressUnhandledKeys = true;
            cfg.backgroundColor = 0;
            cfg.singleItemMode = false;

            _horizontalMenu = new HorizontalMenu("DoorHMenu", cfg);
            _horizontalMenu->activate();
        }
    }
}

// ---------------------------------------------------------------------------
// Result posting
// ---------------------------------------------------------------------------

void DoorDialog::postResult(bool opened) {
    deactivate();
    if (g_events) {
        g_events->postMenuResult("InGame", true,
            Common::KEYCODE_RETURN,
            opened ? kDoorOpened : kDoorBlocked,
            Common::String("DoorResult"), true, true);
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
