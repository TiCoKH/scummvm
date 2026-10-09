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

#include "goldbox/poolrad/views/dialogs/combat_target_command_dialog.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/items/character_item.h"
#include "goldbox/combat/combat_session.h"
#include "goldbox/combat/combat_context.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/events.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatTargetCommandDialog::CombatTargetCommandDialog()
    : Dialog("CombatTargetCmd"), _attacker(nullptr), _target(nullptr),
      _maxRange(0xFF), _showRange(false), _session(nullptr),
      _horizontalMenu(nullptr), _targetCmd(0) {
    setBounds(Window(0, 24, 39, 24));
}

CombatTargetCommandDialog::~CombatTargetCommandDialog() {
    delete _horizontalMenu;
}

void CombatTargetCommandDialog::beginTargetCommand(
        Goldbox::Data::PlayerCharacter *attacker,
        Goldbox::Data::PlayerCharacter *target,
        uint8 maxRange,
        bool showRange,
        Combat::CombatSession *session) {
    _attacker  = attacker;
    _target    = target;
    _maxRange  = maxRange;
    _showRange = showRange;
    _session   = session;
}

void CombatTargetCommandDialog::activate() {
    if (_isActive && _horizontalMenu)
        return;
    Dialog::activate();
    buildMenu();
}

void CombatTargetCommandDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void CombatTargetCommandDialog::buildMenu() {
    _targetCmd = 0;

    // Determine target range and whether a Target/Aim command is available.
    // Mirrors COMBAT_TargetCommandMenu game-rule logic.
    if (_attacker && _target && _session) {
        Combat::CombatContext *ctx = _session->getContext();
        if (ctx) {
            const uint8 range = ctx->getTargetRange(_attacker, _target);

            if (range <= _maxRange) {
                if (!_showRange) {
                    // Normal melee mode: always offer Target.
                    _targetCmd = 'T';
                } else if (_target != _attacker) {
                    const Goldbox::Data::ADnDCharacter *adnd =
                        dynamic_cast<const Goldbox::Data::ADnDCharacter *>(_attacker);
                    const bool hasRanged = adnd && adnd->hasRangedWeapon();

                    if (!hasRanged) {
                        _targetCmd = 'T';
                    } else {
                        // Ranged-weapon path: validate item and equipped state.
                        Data::Items::CharacterItem *rangedItem = nullptr;
                        Data::ADnDCharacter *adndMut =
                            const_cast<Data::ADnDCharacter *>(adnd);
                        if (adndMut && adndMut->getRangedAttackItem(&rangedItem) && rangedItem) {
                            // Build target list to check equipped state.
                            Combat::TargetList tmp;
                            _session->buildFacingTargetList(_attacker, tmp);
                            const bool hasTargets = !tmp.targetOrder.empty();
                            if (!hasTargets || adndMut->isEquippedRangedWeapon())
                                _targetCmd = 'A';  // Aim (ranged)
                        }
                    }
                }
            }
        }
    }

    // Build menu: Next / Previous / Manual / [Target|Aim] / Center / Exit
    Common::Array<Common::String> labels;
    Common::Array<char>           keys;

    labels.push_back("Next");    keys.push_back('N');
    labels.push_back("Prev");    keys.push_back('P');
    labels.push_back("Manual");  keys.push_back('M');
    if (_targetCmd == 'T') {
        labels.push_back("Target"); keys.push_back('T');
    } else if (_targetCmd == 'A') {
        labels.push_back("Aim");    keys.push_back('A');
    }
    labels.push_back("Center"); keys.push_back('C');
    labels.push_back("Exit");   keys.push_back('E');

    _menuModel.items.clear();
    _menuModel.currentSelection = 0;
    _menuModel.generateMenuItems(labels, true);

    // Store key mapping alongside the model for msgKeypress lookup.
    // We reuse the shortcut array embedded in the menu items' text.
    // The HorizontalMenu posts its result via the parent's handleMenuResult,
    // so we just need the keys array accessible in msgKeypress.
    // Store it in a member for use in msgKeypress.
    // (Re-built each activate() so always in sync.)

    delete _horizontalMenu;
    _horizontalMenu = nullptr;

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

    _horizontalMenu = new HorizontalMenu("AimMenu", cfg);
    _horizontalMenu->activate();
}

void CombatTargetCommandDialog::draw() {
    if (!_isVisible || !_horizontalMenu)
        return;
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 8);
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool CombatTargetCommandDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    const bool wasActive = _horizontalMenu->isActive();
    const bool handled   = _horizontalMenu->msgKeypress(msg);

    if (!handled)
        return false;

    if (wasActive && !_horizontalMenu->isActive()) {
        char ascii = msg.ascii;
        if (ascii >= 'a' && ascii <= 'z')
            ascii = (char)(ascii - 32);

        // Map 'A' (ranged Aim) to 'T' so the parent sees a unified confirm.
        char result = ascii;
        if (result == 'A')
            result = 'T';

        if (g_events)
            g_events->postMenuResult("CombatAttack", true,
                                     Common::KEYCODE_RETURN,
                                     (int)result,
                                     Common::String(), true, false);
        deactivate();
    }

    return true;
}

void CombatTargetCommandDialog::handleMenuResult(const MenuResultMessage &) {
    // Not used — results are posted directly in msgKeypress.
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
