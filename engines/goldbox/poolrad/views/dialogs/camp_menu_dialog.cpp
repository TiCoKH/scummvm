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

#include "goldbox/poolrad/views/dialogs/camp_menu_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/in_game_view.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/gfx/encounter_sprite_cache.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"
#include "common/debug.h"
#include "common/system.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CampMenuDialog::CampMenuDialog(const Common::String &name, InGameView *parent)
    : Dialog(name), _parentView(parent), _horizontalMenu(nullptr),
      _currentFrame(0), _lastFrameTime(0), _frameIntervalMs(500),
      _isInterrupted(false) {
    setBounds(Window(0, 0, 39, 24));
}

CampMenuDialog::~CampMenuDialog() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
}

void CampMenuDialog::buildMenuModel() {
    _menuModel.items.clear();
    _menuModel.currentSelection = 0;

    Common::Array<Common::String> menuStrings;
    menuStrings.push_back("Save");
    menuStrings.push_back("View");
    menuStrings.push_back("Magic");
    menuStrings.push_back("Rest");
    menuStrings.push_back("Alter");
    menuStrings.push_back("Exit");

    _menuModel.generateMenuItems(menuStrings, true);
}

void CampMenuDialog::loadCampfireFrames() {
    _campfireFrames.clear();
    _currentFrame = 0;

    // Load campfire PIC into EncounterSpriteCache so the normal
    // InGameMainScreenDialog viewport draw path renders it.
    if (Poolrad::g_engine) {
        Poolrad::g_engine->getEncounterSpriteCache().loadHead(0xFF, kCampfirePicId);
    }

    _lastFrameTime = g_system->getMillis();
}

void CampMenuDialog::activate() {
    Dialog::activate();
    _isInterrupted = false;

    // Legacy party-camp flow sanitizes memorized spell/item flags on entry.
    if (Common::Array<Goldbox::Data::PlayerCharacter *> *party =
            Goldbox::VmInterface::getParty()) {
        Goldbox::Poolrad::Data::PoolradCharacter::
            clearPartyMemorizedSpellState(*party);
    }

    loadCampfireFrames();

    // Read game speed for animation interval.
    _frameIntervalMs = 500;
    if (Poolrad::g_engine) {
        ECL::AddressSpace *mem = Poolrad::g_engine->getEclMemory();
        if (mem) {
            const uint16 speedAddr = 0x49FC; // kVmFieldGameSpeed
            uint8 speed = mem->read8(speedAddr);
            if (speed == 0) speed = 1;
            _frameIntervalMs = static_cast<uint16>(speed) * 500;
        }
    }

    // Start animation timer - store interval, animation ticks in draw().
    if (Poolrad::g_engine) {
        ::Goldbox::Gfx::EncounterSpriteCache &cache =
            Poolrad::g_engine->getEncounterSpriteCache();
        cache.setAnimInterval(_frameIntervalMs);
        debug(2, "CampMenuDialog::activate: isAnimated=%d headFrameCount=%d interval=%u",
            (int)cache.isAnimated(), cache.headFrameCount(),
            (unsigned)_frameIntervalMs);
    }

    // Build and create menu.
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    buildMenuModel();

    HorizontalMenuConfig cfg;
    cfg.promptTxt = "Camp: ";
    cfg.menuItemList = &_menuModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 15;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;

    _horizontalMenu = new HorizontalMenu("CampHMenu", cfg);
    _horizontalMenu->activate();
}

void CampMenuDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    _campfireFrames.clear();

    // Clear the encounter cache when leaving camp.
    if (Poolrad::g_engine)
        Poolrad::g_engine->getEncounterSpriteCache().clear();

    Dialog::deactivate();
}

void CampMenuDialog::draw() {
    if (!_isVisible)
        return;

    Surface s = getSurface();

    s.clearBox(1, 17, 38, 22, 0);
    s.writeStringC(1, 18, 10, "The party makes camp...");

    // Draw menu at row 24.
    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }
}

void CampMenuDialog::timeout() {
    // Not used - animation is driven from draw().
}

bool CampMenuDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    bool wasActive = _horizontalMenu->isActive();
    bool handled = _horizontalMenu->msgKeypress(msg);

    if (!handled) {
        // Not consumed by menu - let party list navigation keys pass through.
        switch (msg.keycode) {
        case Common::KEYCODE_KP_PLUS:
        case Common::KEYCODE_KP_MINUS:
        case Common::KEYCODE_PAGEUP:
        case Common::KEYCODE_PAGEDOWN:
            return false;
        default:
            break;
        }
        return true; // Consume everything else.
    }

    // Menu made a selection (deactivated itself).
    if (wasActive && !_horizontalMenu->isActive()) {
        // Escape is an explicit camp-exit intent.
        if (msg.keycode == Common::KEYCODE_ESCAPE) {
            exitCamp();
            redraw();
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

        // Reactivate menu unless we're exiting.
        if (_isActive && _horizontalMenu) {
            _horizontalMenu->activate();
            _horizontalMenu->setRedraw();
        }
    }

    redraw();
    return true;
}

void CampMenuDialog::handleMenuKey(char key) {
    switch (key) {
    case 'S':
        // TODO: DIALOG_SaveGame + quit prompt
        break;
    case 'V':
        if (_parentView)
            _parentView->addView("ViewCharacter");
        break;
    case 'M':
        // TODO: VIEW_Magic(isInterrupted)
        break;
    case 'R':
        // TODO: ACTION_Rest(isInterrupted)
        break;
    case 'A':
        // TODO: DIALOG_Preferences
        break;
    case 'E':
        exitCamp();
        break;
    default:
        break;
    }
}

void CampMenuDialog::exitCamp() {
    // Legacy party-camp flow sanitizes memorized spell/item flags on exit.
    if (Common::Array<Goldbox::Data::PlayerCharacter *> *party =
            Goldbox::VmInterface::getParty()) {
        Goldbox::Poolrad::Data::PoolradCharacter::
            clearPartyMemorizedSpellState(*party);
    }

    if (_parentView && g_events) {
        g_events->postMenuResult(_parentView->getName(), true,
            Common::KEYCODE_e, _isInterrupted ? 1 : 0,
            Common::String(), true, false);
    }
    deactivate();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
