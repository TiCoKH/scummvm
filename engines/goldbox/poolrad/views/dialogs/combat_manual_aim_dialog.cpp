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

#include "goldbox/poolrad/views/dialogs/combat_manual_aim_dialog.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/combat/combatant_table.h"
#include "goldbox/combat/tile_property_provider.h"
#include "goldbox/core/direction.h"
#include "goldbox/events.h"
#include "goldbox/gfx/surface.h"
#include "common/util.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

// Combat field bounds (mirrors original 50x25 grid).
static const uint8 kFieldMaxX = 49;
static const uint8 kFieldMaxY = 24;

// Direction index 8 = center/idle (no movement).
static const uint8 kDirCenter = 8;

// Delta tables for 8-way movement (indices 0-7 = N,NE,E,SE,S,SW,W,NW).
static const int8 kDirDX[9] = {  0, 1, 1, 1, 0,-1,-1,-1, 0 };
static const int8 kDirDY[9] = { -1,-1, 0, 1, 1, 1, 0,-1, 0 };

CombatManualAimDialog::CombatManualAimDialog()
    : Dialog("CombatManualAim"), _attacker(nullptr), _maxRange(0xFF),
      _withWeapon(false), _emptyAllowed(false), _session(nullptr),
      _cursorX(0), _cursorY(0), _direction(kDirCenter),
      _valid(false), _selectedCharacter(nullptr), _targetRange(0xFF),
      _horizontalMenu(nullptr) {
    setBounds(Window(0, 24, 39, 24));
}

CombatManualAimDialog::~CombatManualAimDialog() {
    delete _horizontalMenu;
}

void CombatManualAimDialog::beginManualAim(
        Goldbox::Data::PlayerCharacter *attacker,
        Goldbox::Data::PlayerCharacter *initialTarget,
        uint8 maxRange,
        bool withWeapon,
        bool emptyAllowed,
        Combat::CombatSession *session) {
    _attacker     = attacker;
    _maxRange     = maxRange;
    _withWeapon   = withWeapon;
    _emptyAllowed = emptyAllowed;
    _session      = session;
    _direction    = kDirCenter;
    _result       = ManualAimResult();

    // Cursor starts at the initial target's tile, or the attacker's tile.
    Goldbox::Data::PlayerCharacter *startChar = initialTarget ? initialTarget : attacker;
    _cursorX = 0;
    _cursorY = 0;
    if (startChar && session) {
        const int idx = session->getTable().findIndex(startChar);
        if (idx >= 0) {
            _cursorX = session->getTable().getTileCol(idx);
            _cursorY = session->getTable().getTileRow(idx);
        }
    }
}

void CombatManualAimDialog::activate() {
    Dialog::activate();
    evaluateCursor();
    buildCommandMenu();
}

void CombatManualAimDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void CombatManualAimDialog::draw() {
    if (!_isVisible || !_horizontalMenu)
        return;
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 8);
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool CombatManualAimDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    // Directional keys move the cursor directly (no menu involvement).
    const uint8 dir = decodeDirection(msg);
    if (dir < kDirCenter) {
        _direction = dir;
        // Apply movement and clamp to field bounds.
        int nx = (int)_cursorX + kDirDX[dir];
        int ny = (int)_cursorY + kDirDY[dir];
        _cursorX = (uint8)CLIP(nx, 0, (int)kFieldMaxX);
        _cursorY = (uint8)CLIP(ny, 0, (int)kFieldMaxY);
        evaluateCursor();
        buildCommandMenu();
        return true;
    }

    if (!_horizontalMenu)
        return false;

    const bool wasActive = _horizontalMenu->isActive();
    const bool handled   = _horizontalMenu->msgKeypress(msg);

    if (!handled)
        return false;

    if (wasActive && !_horizontalMenu->isActive()) {
        char ascii = msg.ascii;
        if (ascii >= 'a' && ascii <= 'z')
            ascii = (char)(ascii - 32);

        if (ascii == 'T') {
            confirmTarget();
        } else if (ascii == 'C') {
            // Center: re-center viewport on cursor (presentation only).
            if (_session)
                _session->scrollViewport(TilePos(_cursorX, _cursorY));
            buildCommandMenu();
        } else {
            // Exit / unrecognised.
            cancelAim();
        }
    }

    return true;
}

void CombatManualAimDialog::handleMenuResult(const MenuResultMessage &) {
    // Not used — results are posted directly in msgKeypress.
}

// ---------------------------------------------------------------------------
// Private helpers

void CombatManualAimDialog::evaluateCursor() {
    _valid             = false;
    _selectedCharacter = nullptr;
    _targetRange       = 0xFF;

    if (!_session || !_attacker)
        return;

    Combat::CombatContext *ctx = _session->getContext();
    if (!ctx)
        return;

    // LOS check from attacker to cursor.
    const int attackerIdx = _session->getTable().findIndex(_attacker);
    if (attackerIdx < 0)
        return;

    const TilePos attackerPos(_session->getTable().getTileCol(attackerIdx),
                               _session->getTable().getTileRow(attackerIdx));
    const TilePos cursorPos(_cursorX, _cursorY);

    uint16 rawRange = 0xFFFF;
    const bool hasLOS = ctx->lineOfSightCheck(attackerPos, cursorPos, rawRange);

    if (!hasLOS)
        return;

    _targetRange = (uint8)(rawRange >> 1);

    // Resolve occupant.
    const Combat::CombatContext::CombatCell cell = ctx->getTileAndOccupantAt(cursorPos);

    if (cell.occupantId != 0)
        _selectedCharacter = _session->getTable().getCharacter(cell.occupantId - 1);

    // Basic validity checks.
    if (_targetRange > _maxRange)
        return;

    // Tile passability (0xFF = impassable wall).
    const Combat::TilePropertyProvider *props =
        _session->getBattlefieldMap().getTilePropertyProvider();
    if (props && props->isImpassable(cell.tileId))
        return;

    if (_selectedCharacter == nullptr) {
        if (!_emptyAllowed)
            return;
    } else {
        if (!ctx->canEngageTarget(_attacker, _selectedCharacter))
            return;

        if (_withWeapon) {
            if (_selectedCharacter == _attacker)
                return;

            const Goldbox::Data::ADnDCharacter *adnd =
                dynamic_cast<const Goldbox::Data::ADnDCharacter *>(_attacker);
            if (adnd && adnd->hasRangedWeapon()) {
                Data::Items::CharacterItem *rangedItem = nullptr;
                Data::ADnDCharacter *adndMut =
                    const_cast<Data::ADnDCharacter *>(adnd);
                if (!adndMut->getRangedAttackItem(&rangedItem) || !rangedItem)
                    return;

                Combat::TargetList tmp;
                _session->buildFacingTargetList(_attacker, tmp);
                const bool hasTargets = !tmp.targetOrder.empty();
                if (hasTargets && !adndMut->isEquippedRangedWeapon())
                    return;
            }
        }
    }

    _valid = true;
}

void CombatManualAimDialog::buildCommandMenu() {
    delete _horizontalMenu;
    _horizontalMenu = nullptr;

    Common::Array<Common::String> labels;
    if (_valid)
        labels.push_back("Target");
    labels.push_back("Center");
    labels.push_back("Exit");

    _menuModel.items.clear();
    _menuModel.currentSelection = 0;
    _menuModel.generateMenuItems(labels, true);

    HorizontalMenuConfig cfg;
    cfg.promptTxt             = "Aim:";
    cfg.menuItemList          = &_menuModel;
    cfg.textColor             = 10;
    cfg.selectColor           = 15;
    cfg.promptColor           = 15;
    cfg.allowNumPad           = false;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor       = 8;
    cfg.singleItemMode        = false;

    _horizontalMenu = new HorizontalMenu("ManualAimMenu", cfg);
    _horizontalMenu->activate();
}

void CombatManualAimDialog::confirmTarget() {
    if (!_valid) {
        buildCommandMenu();
        return;
    }

    _result.target = _selectedCharacter;
    _result.x      = _cursorX;
    _result.y      = _cursorY;

    bool attackResult = false;
    if (_withWeapon && _session && _attacker && _selectedCharacter)
        attackResult = _session->executeAttackOnTarget(_attacker, _selectedCharacter, true);
    else
        attackResult = _valid;

    if (!attackResult) {
        // Attack failed — reset and let the player try again.
        _result = ManualAimResult();
        buildCommandMenu();
        return;
    }

    if (g_events)
        g_events->postMenuResult("CombatAttack", true,
                                 Common::KEYCODE_RETURN,
                                 1, Common::String(), true, false);
    deactivate();
}

void CombatManualAimDialog::cancelAim() {
    _result = ManualAimResult();
    if (g_events)
        g_events->postMenuResult("CombatAttack", false,
                                 Common::KEYCODE_ESCAPE,
                                 0, Common::String(), true, false);
    deactivate();
}

// static
uint8 CombatManualAimDialog::decodeDirection(const KeypressMessage &msg) {
    switch (msg.keycode) {
    case Common::KEYCODE_KP8: case Common::KEYCODE_UP:    return 0; // N
    case Common::KEYCODE_KP9:                             return 1; // NE
    case Common::KEYCODE_KP6: case Common::KEYCODE_RIGHT: return 2; // E
    case Common::KEYCODE_KP3:                             return 3; // SE
    case Common::KEYCODE_KP2: case Common::KEYCODE_DOWN:  return 4; // S
    case Common::KEYCODE_KP1:                             return 5; // SW
    case Common::KEYCODE_KP4: case Common::KEYCODE_LEFT:  return 6; // W
    case Common::KEYCODE_KP7:                             return 7; // NW
    default:                                              return kDirCenter;
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
