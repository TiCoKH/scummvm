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
#include "goldbox/poolrad/views/dialogs/spell_book_dialog.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/gfx/picture_display_cache.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"
#include "common/debug.h"
#include "common/system.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CampMenuDialog::CampMenuDialog(const Common::String &name)
    : Dialog(name), _horizontalMenu(nullptr),
            _spellBookDialog(nullptr),
      _currentFrame(0), _lastFrameTime(0), _frameIntervalMs(500),
      _isInterrupted(false) {
    setBounds(Window(0, 0, 39, 24));

        _spellBookDialog = new SpellBookDialog();
        _spellBookDialog->deactivate();
        subView(_spellBookDialog);
}

CampMenuDialog::~CampMenuDialog() {
    if (_horizontalMenu) {
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }

    if (_spellBookDialog) {
        delete _spellBookDialog;
        _spellBookDialog = nullptr;
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

    // Camp picture loading is handled by the host on GS_CAMPING entry.

    _lastFrameTime = g_system->getMillis();
}

void CampMenuDialog::activate() {
    Dialog::activate();
    _isInterrupted = false;

    if (_spellBookDialog)
        _spellBookDialog->deactivate();

    // Legacy party-camp flow sanitizes memorized spell/item flags on entry.
    if (Common::Array<Goldbox::Data::PlayerCharacter *> *party =
            Goldbox::VmInterface::getParty()) {
        Goldbox::Poolrad::Data::PoolradCharacter::
            clearPartyMemorizedSpellState(*party);
    }

    loadCampfireFrames();

    // Read game speed via VM/host interface.
    _frameIntervalMs = static_cast<uint16>(VmInterface::getGameSpeed()) * 500;

    // Start animation timer - store interval, animation ticks in draw().
    if (Poolrad::g_engine) {
        ::Goldbox::Gfx::PictureDisplayCache &picCache =
            Poolrad::g_engine->getPictureDisplayCache();
        picCache.setAnimInterval(_frameIntervalMs);
        debug(2, "CampMenuDialog::activate: isAnimated=%d headFrameCount=%d interval=%u",
            (int)picCache.isAnimated(), picCache.headFrameCount(),
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
    cfg.promptColor = 13;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;

    _horizontalMenu = new HorizontalMenu("CampHMenu", cfg);
    _horizontalMenu->activate();
}

void CampMenuDialog::deactivate() {
    if (_spellBookDialog)
        _spellBookDialog->deactivate();

    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    _campfireFrames.clear();

    // Clear the picture cache when leaving camp and signal viewport redraw.
    if (Poolrad::g_engine) {
        Poolrad::g_engine->getPictureDisplayCache().clear();
        if (g_events) {
            g_events->postEclStateMessage(EclVmMessage::ST_SKYBOX_DIRTY, 1,
                EclVmMessage::VT_UINT8);
        }
    }

    Dialog::deactivate();
}

void CampMenuDialog::draw() {
    if (!_isVisible)
        return;

    Surface s = getSurface();

    s.clearBox(1, 17, 38, 22, 0);
    s.writeStringC(1, 18, 10, "The party makes camp...");

    if (_spellBookDialog && _spellBookDialog->isActive()) {
        _spellBookDialog->draw();
        return;
    }

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
    if (!_isActive)
        return false;

    if (_spellBookDialog && _spellBookDialog->isActive()) {
        if (_spellBookDialog->msgKeypress(msg)) {
            if (!_spellBookDialog->isActive() && _horizontalMenu) {
                _horizontalMenu->activate();
                _horizontalMenu->setRedraw();
            }
            redraw();
            return true;
        }
        return true;
    }

    if (!_horizontalMenu)
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
        if (_isActive && _horizontalMenu
            && !(_spellBookDialog && _spellBookDialog->isActive())) {
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
        addView("ViewCharacter");
        break;
    case 'M':
        if (_horizontalMenu)
            _horizontalMenu->deactivate();
        if (_spellBookDialog)
            _spellBookDialog->activate();
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

    if (g_events) {
        g_events->postMenuResult("InGame", true,
            Common::KEYCODE_e, _isInterrupted ? 1 : 0,
            Common::String(), true, false);
    }
    deactivate();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
