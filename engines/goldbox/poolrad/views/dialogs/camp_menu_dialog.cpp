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
#include "goldbox/poolrad/views/dialogs/game_speed_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/load_save_dialog.h"
#include "goldbox/poolrad/views/dialogs/rest_dialog.h"
#include "goldbox/poolrad/views/dialogs/set_icon.h"
#include "goldbox/poolrad/views/dialogs/spell_book_dialog.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/poolrad/training_rules.h"
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

namespace {

// The menu key a HorizontalMenu ended on: its shortcut, the highlighted
// entry for Return, 27 for Escape, or 0 for anything else.
char resolveMenuKey(const KeypressMessage &msg, const MenuItemList &model) {
    if (msg.keycode == Common::KEYCODE_ESCAPE)
        return 27;

    char ascii = msg.ascii;
    if (ascii >= 'a' && ascii <= 'z')
        ascii = ascii - 32;
    for (uint i = 0; i < model.items.size(); ++i) {
        if (model.items[i].shortcut == ascii)
            return ascii;
    }

    if (msg.keycode == Common::KEYCODE_RETURN) {
        const int sel = model.currentSelection;
        if (sel >= 0 && sel < (int)model.items.size())
            return model.items[sel].shortcut;
    }
    return 0;
}

} // namespace

CampMenuDialog::CampMenuDialog(const Common::String &name)
    : Dialog(name), _horizontalMenu(nullptr),
      _spellBookDialog(nullptr),
      _currentFrame(0), _lastFrameTime(0), _frameIntervalMs(500),
      _isInterrupted(false) {
    setBounds(Window(0, 0, 39, 24));

    _spellBookDialog = new SpellBookDialog();
    _spellBookDialog->deactivate();
    subView(_spellBookDialog);

    _loadSaveDialog = new LoadSaveDialog("CampSave");
    _loadSaveDialog->deactivate();
    _speedDialog = new GameSpeedDialog();
    _speedDialog->deactivate();
    _restDialog = new RestDialog("CampRest");
    _restDialog->deactivate();
}

CampMenuDialog::~CampMenuDialog() {
    delete _horizontalMenu;
    _horizontalMenu = nullptr;
    delete _spellBookDialog;
    _spellBookDialog = nullptr;
    delete _subMenu;
    delete _yesNo;
    delete _loadSaveDialog;
    delete _speedDialog;
    delete _iconDialog;
    delete _restDialog;
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
    _inSpellBook = false;
    _stage = CAMP_MENU;
    _orderPlacing = false;
    _messageText.clear();
    _showCampText = true;

    if (_spellBookDialog)
        _spellBookDialog->deactivate();

    // Legacy party-camp flow sanitizes memorized spell/item flags on entry.
    if (Common::List<Goldbox::Data::PlayerCharacter *> *party =
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

    closeSubMenu();
    closeYesNo();
    if (_loadSaveDialog && _loadSaveDialog->isActive())
        _loadSaveDialog->deactivate();
    if (_speedDialog && _speedDialog->isActive())
        _speedDialog->deactivate();
    if (_restDialog && _restDialog->isActive())
        _restDialog->deactivate();
    if (_iconDialog) {
        detachDialog(_iconDialog);
        delete _iconDialog;
        _iconDialog = nullptr;
    }
    _stage = CAMP_MENU;

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

    // Full-screen children draw on their own.
    if (_stage == CAMP_ICON && _iconDialog) {
        _iconDialog->draw();
        return;
    }
    if (_stage == CAMP_SAVE && _loadSaveDialog &&
            _loadSaveDialog->isActive()) {
        _loadSaveDialog->draw();
        return;
    }

    Surface s = getSurface();

    s.clearBox(1, 17, 38, 22, 0);
    if (!_messageText.empty()) {
        Goldbox::Data::PlayerCharacter *ch = selectedCharacter();
        if (ch) {
            s.writeStringC(1, 18, ch->enabled ? 11 : 12, ch->name);
            s.writeStringC(1, 19, 10, _messageText);
        }
    } else if (_showCampText) {
        s.writeStringC(1, 18, 10, "The party makes camp...");
    }

    if (_spellBookDialog && _spellBookDialog->isActive()) {
        _spellBookDialog->draw();
        return;
    }

    switch (_stage) {
    case CAMP_REST:
        if (_restDialog && _restDialog->isActive())
            _restDialog->draw();
        return;
    case CAMP_SPEED:
        if (_speedDialog && _speedDialog->isActive())
            _speedDialog->draw();
        return;
    case CAMP_QUIT_CONFIRM:
    case CAMP_DROP_CONFIRM:
    case CAMP_DROP_QUIT:
        if (_yesNo && _yesNo->isActive()) {
            _yesNo->setRedraw();
            _yesNo->draw();
        }
        return;
    case CAMP_ALTER:
    case CAMP_ORDER:
    case CAMP_PICS:
        if (_subMenu) {
            _subMenu->setRedraw();
            _subMenu->draw();
        }
        return;
    default:
        break;
    }

    // Draw menu at row 24.
    if (_horizontalMenu) {
        _horizontalMenu->setRedraw();
        _horizontalMenu->draw();
    }
}

bool CampMenuDialog::tick() {
    resumeAfterSpellBook();
    if (_stage == CAMP_REST && _restDialog && _restDialog->isActive())
        _restDialog->tick();
    return Dialog::tick();
}

void CampMenuDialog::timeout() {
    // Not used - animation is driven from draw().
}

bool CampMenuDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_spellBookDialog && _spellBookDialog->isActive()) {
        if (_spellBookDialog->msgKeypress(msg)) {
            resumeAfterSpellBook();
            redraw();
        }
        return true;
    }

    resumeAfterSpellBook();
    if (!_isActive)
        return true;

    switch (_stage) {
    case CAMP_REST:
        if (_restDialog && _restDialog->isActive())
            _restDialog->msgKeypress(msg);
        redraw();
        return true;
    case CAMP_SAVE:
        if (_loadSaveDialog && _loadSaveDialog->isActive())
            _loadSaveDialog->msgKeypress(msg);
        redraw();
        return true;
    case CAMP_ICON:
        if (_iconDialog)
            _iconDialog->msgKeypress(msg);
        redraw();
        return true;
    case CAMP_SPEED:
        if (_speedDialog && _speedDialog->isActive())
            _speedDialog->msgKeypress(msg);
        redraw();
        return true;
    case CAMP_QUIT_CONFIRM:
    case CAMP_DROP_CONFIRM:
    case CAMP_DROP_QUIT:
        if (_yesNo && _yesNo->isActive())
            _yesNo->msgKeypress(msg);
        redraw();
        return true;
    case CAMP_ORDER:
        // CampOrderMenu Place mode: Home moves up, End moves down.
        if (_orderPlacing) {
            if (msg.keycode == Common::KEYCODE_HOME ||
                    msg.keycode == Common::KEYCODE_KP7) {
                handleOrderMoveKey(true);
                return true;
            }
            if (msg.keycode == Common::KEYCODE_END ||
                    msg.keycode == Common::KEYCODE_KP1) {
                handleOrderMoveKey(false);
                return true;
            }
        }
        // fall through
    case CAMP_ALTER:
    case CAMP_PICS:
        if (_subMenu) {
            const bool wasActive = _subMenu->isActive();
            _subMenu->msgKeypress(msg);
            if (wasActive && !_subMenu->isActive())
                handleSubMenuKey(resolveMenuKey(msg, _subModel));
        }
        redraw();
        return true;
    default:
        break;
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

        const char key = resolveMenuKey(msg, _menuModel);
        if (key != 0)
            handleMenuKey(key);

        // Reactivate menu unless a sub-menu took over or we are exiting.
        if (_isActive && _horizontalMenu && _stage == CAMP_MENU
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
        beginSave();
        break;
    case 'V':
        addView("ViewCharacter");
        break;
    case 'M':
        if (_horizontalMenu)
            _horizontalMenu->deactivate();
        if (_spellBookDialog) {
            _spellBookDialog->activate();
            _inSpellBook = true;
        }
        break;
    case 'R':
        // Camp.CampRestMenu: the rest menu, then resting in ticks. The
        // engine owns the timers; an interruption ends the camp so exitCamp
        // routes through the ONRESTINTR ECL entry.
        closeSubMenu();
        _stage = CAMP_REST;
        if (_horizontalMenu)
            _horizontalMenu->deactivate();
        _restDialog->setParent(this);
        _restDialog->begin();
        break;
    case 'A':
        openAlterMenu();
        break;
    case 'E':
        exitCamp();
        break;
    default:
        break;
    }
}

void CampMenuDialog::handleMenuResult(const MenuResultMessage &result) {
    switch (_stage) {
    case CAMP_REST:
        if (_restDialog && _restDialog->wasInterrupted()) {
            _isInterrupted = true;
            exitCamp();
        } else {
            returnToCampMenu();
        }
        break;
    case CAMP_SAVE:
        // SaveLoad.SaveGame returns whether or not a game was saved; the
        // quit question follows either way.
        if (_loadSaveDialog && _loadSaveDialog->isActive())
            _loadSaveDialog->deactivate();
        openYesNo(CAMP_QUIT_CONFIRM, "Quit TO DOS ");
        break;
    case CAMP_ICON:
        // SetIcon commits (Y) or reverts (cancel) by itself.
        if (_iconDialog) {
            detachDialog(_iconDialog);
            delete _iconDialog;
            _iconDialog = nullptr;
        }
        openAlterMenu();
        break;
    case CAMP_SPEED:
        if (_speedDialog && _speedDialog->isActive())
            _speedDialog->deactivate();
        _messageText.clear();
        openAlterMenu();
        break;
    case CAMP_QUIT_CONFIRM:
    case CAMP_DROP_CONFIRM:
    case CAMP_DROP_QUIT:
        handleYesNo(result._success &&
            result._keyCode == Common::KEYCODE_y);
        break;
    default:
        break;
    }
    redraw();
}

void CampMenuDialog::resumeAfterSpellBook() {
    if (!_isActive || !_inSpellBook || !_spellBookDialog
            || _spellBookDialog->isActive())
        return;

    _inSpellBook = false;
    if (_spellBookDialog->wasInterrupted()) {
        _isInterrupted = true;
        exitCamp();
        return;
    }

    if (_horizontalMenu) {
        _horizontalMenu->activate();
        _horizontalMenu->setRedraw();
    }
    redraw();
}

void CampMenuDialog::exitCamp() {
    // Legacy party-camp flow sanitizes memorized spell/item flags on exit.
    if (Common::List<Goldbox::Data::PlayerCharacter *> *party =
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

// --- Sub-menus ---

void CampMenuDialog::openSubMenu(CampStage stage, const Common::String &prompt,
        const Common::Array<Common::String> &labels) {
    closeSubMenu();
    _stage = stage;
    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    _subModel.items.clear();
    _subModel.currentSelection = 0;
    _subModel.generateMenuItems(labels, true);

    HorizontalMenuConfig cfg;
    cfg.promptTxt = prompt;
    cfg.menuItemList = &_subModel;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.promptColor = 13;
    cfg.allowNumPad = false;
    cfg.suppressUnhandledKeys = true;
    cfg.backgroundColor = 0;
    cfg.singleItemMode = false;

    _subMenu = new HorizontalMenu("CampSubMenu", cfg);
    _subMenu->activate();
    redraw();
}

void CampMenuDialog::closeSubMenu() {
    if (_subMenu) {
        _subMenu->deactivate();
        delete _subMenu;
        _subMenu = nullptr;
    }
}

void CampMenuDialog::openYesNo(CampStage stage, const Common::String &prompt) {
    closeSubMenu();
    closeYesNo();
    _stage = stage;
    if (_horizontalMenu)
        _horizontalMenu->deactivate();

    // MenuCore.YesNoMenu(query, Yellow, LightGreen, White).
    HorizontalYesNoConfig cfg;
    cfg.promptTxt = prompt;
    cfg.promptColor = 14;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.backgroundColor = 0;
    _yesNo = new HorizontalYesNo("CampYesNo", cfg);
    setDialogParent(_yesNo, this);
    _yesNo->activate();
    redraw();
}

void CampMenuDialog::closeYesNo() {
    if (_yesNo) {
        _yesNo->deactivate();
        delete _yesNo;
        _yesNo = nullptr;
    }
}

void CampMenuDialog::returnToCampMenu() {
    closeSubMenu();
    closeYesNo();
    _stage = CAMP_MENU;
    _orderPlacing = false;
    if (_horizontalMenu) {
        _horizontalMenu->activate();
        _horizontalMenu->setRedraw();
    }
    redraw();
}

// Camp.CampAlterMenu.
void CampMenuDialog::openAlterMenu() {
    Common::Array<Common::String> labels;
    labels.push_back("Order");
    labels.push_back("Drop");
    labels.push_back("Speed");
    labels.push_back("Icon");
    labels.push_back("Pics");
    labels.push_back("Exit");
    openSubMenu(CAMP_ALTER, "Alter: ", labels);
}

// Camp.CampOrderMenu: Select mode picks the character, Place mode moves it.
void CampMenuDialog::openOrderMenu() {
    Common::Array<Common::String> labels;
    labels.push_back(_orderPlacing ? "Place" : "Select");
    labels.push_back("Exit");
    openSubMenu(CAMP_ORDER, "Party Order: ", labels);
}

// Camp.CampAlterMenu 'P': toggle monster pictures and portraits.
void CampMenuDialog::openPicsMenu() {
    const bool monsters = Poolrad::g_engine && Poolrad::g_engine->monsterPicsOn();
    const bool portraits = Poolrad::g_engine && Poolrad::g_engine->portraitsOn();
    Common::Array<Common::String> labels;
    labels.push_back(monsters ? "Monsters on" : "Monsters off");
    labels.push_back(portraits ? "Portraits on" : "Portraits off");
    labels.push_back("Exit");
    openSubMenu(CAMP_PICS, "", labels);
}

void CampMenuDialog::handleSubMenuKey(char key) {
    switch (_stage) {
    case CAMP_ALTER:
        switch (key) {
        case 'O':
            _orderPlacing = false;
            openOrderMenu();
            break;
        case 'D':
            beginDrop();
            break;
        case 'S':
            beginSpeed();
            break;
        case 'I':
            beginIcon();
            break;
        case 'P':
            openPicsMenu();
            break;
        case 'E':
        case 27:
            returnToCampMenu();
            break;
        default:
            openAlterMenu();
            break;
        }
        break;

    case CAMP_ORDER:
        if (key == 'S' || key == 'P') {
            _orderPlacing = !_orderPlacing;
            if (_orderPlacing)
                showNameMessage("has been selected", false);
            else
                _messageText.clear();
            openOrderMenu();
        } else if (key == 'E' || key == 27) {
            _orderPlacing = false;
            _messageText.clear();
            openAlterMenu();
        } else {
            openOrderMenu();
        }
        break;

    case CAMP_PICS:
        if (key == 'M' && Poolrad::g_engine)
            Poolrad::g_engine->setMonsterPics(!Poolrad::g_engine->monsterPicsOn());
        else if (key == 'P' && Poolrad::g_engine)
            Poolrad::g_engine->setPortraitsOn(!Poolrad::g_engine->portraitsOn());

        if (key == 'E' || key == 27)
            openAlterMenu();
        else
            openPicsMenu();
        break;

    default:
        break;
    }
}

void CampMenuDialog::handleOrderMoveKey(bool up) {
    Goldbox::Data::PlayerCharacter *ch = selectedCharacter();
    PoolradEngineHostImpl *host =
        Poolrad::g_engine ? Poolrad::g_engine->getEclHost() : nullptr;
    if (ch && host)
        host->moveCharacterInParty(ch, up);
    redraw();
}

// SaveLoad.SaveGame, then the quit question.
void CampMenuDialog::beginSave() {
    closeSubMenu();
    _stage = CAMP_SAVE;
    if (_horizontalMenu)
        _horizontalMenu->deactivate();
    _loadSaveDialog->setMode(LoadSaveDialog::kModeSave);
    _loadSaveDialog->setParent(this);
    _loadSaveDialog->activate();
    redraw();
}

// Camp.CampDropMenu.
void CampMenuDialog::beginDrop() {
    Common::List<Goldbox::Data::PlayerCharacter *> *party =
        VmInterface::getParty();
    if (!party || party->empty() || !selectedCharacter()) {
        openAlterMenu();
        return;
    }

    // The last member can only quit the game.
    if (party->size() == 1) {
        openYesNo(CAMP_DROP_QUIT, "quit TO DOS: ");
        return;
    }

    showNameMessage("will be gone", false);
    openYesNo(CAMP_DROP_CONFIRM, "Drop from party? ");
}

void CampMenuDialog::beginSpeed() {
    closeSubMenu();
    _stage = CAMP_SPEED;
    _speedDialog->setParent(this);
    _speedDialog->activate();
    redraw();
}

// CharGen.CustomizeIcon for the selected character.
void CampMenuDialog::beginIcon() {
    Goldbox::Poolrad::Data::PoolradCharacter *pc =
        dynamic_cast<Goldbox::Poolrad::Data::PoolradCharacter *>(
            selectedCharacter());
    if (!pc) {
        openAlterMenu();
        return;
    }

    closeSubMenu();
    _stage = CAMP_ICON;
    _iconDialog = new SetIcon("CampSetIcon", pc);
    attachDialog(_iconDialog);
    _iconDialog->activate();
    redraw();
}

void CampMenuDialog::handleYesNo(bool yes) {
    const CampStage stage = _stage;
    closeYesNo();

    switch (stage) {
    case CAMP_QUIT_CONFIRM:
        if (yes && Poolrad::g_engine) {
            Poolrad::g_engine->quitGame();
            return;
        }
        returnToCampMenu();
        break;

    case CAMP_DROP_QUIT:
        if (yes) {
            Goldbox::Data::PlayerCharacter *ch = selectedCharacter();
            if (ch)
                RosterActions::removeSelectedFromParty(ch,
                    RosterActions::ACTION_DROP);
            if (Poolrad::g_engine)
                Poolrad::g_engine->quitGame();
            return;
        }
        openAlterMenu();
        break;

    case CAMP_DROP_CONFIRM: {
        Goldbox::Data::PlayerCharacter *ch = selectedCharacter();
        if (yes && ch) {
            showNameMessage(ch->enabled ? "bids you farewell"
                                        : "is dumped in a ditch", true);
            RosterActions::removeSelectedFromParty(ch,
                RosterActions::ACTION_DROP);
        } else {
            showNameMessage("Breathes A sigh of relief", true);
        }
        openAlterMenu();
        break;
    }

    default:
        returnToCampMenu();
        break;
    }
}

void CampMenuDialog::showNameMessage(const Common::String &text, bool pause) {
    _messageText = text;
    _showCampText = false;
    redraw();
    if (!pause)
        return;

    // TextRend.PauseOnly, then CharHlp.ClearMessageWindow.
    if (g_events) {
        const uint32 end = g_system->getMillis() +
            VmInterface::getTextDelay() * 225;
        while (g_system->getMillis() < end)
            if (!g_events->pumpModalInputFrame())
                break;
    }
    _messageText.clear();
    redraw();
}

Goldbox::Data::PlayerCharacter *CampMenuDialog::selectedCharacter() const {
    return VmInterface::getSelectedCharacter();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
