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

#include "goldbox/poolrad/views/dialogs/combat_menu_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_menu.h"
#include "goldbox/data/player_character.h"
#include "goldbox/data/adnd_character.h"
#include "goldbox/data/effects/effect_runtime.h"
#include "goldbox/data/effects/character_effects.h"
#include "goldbox/data/rules/rules_types.h"
#include "goldbox/combat/combat_globals.h"
#include "goldbox/events.h"
#include "goldbox/gfx/surface.h"
#include "goldbox/engine.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatMenuDialog::CombatMenuDialog()
    : Dialog("CombatMenu"), _horizontalMenu(nullptr),
      _hasMove(false), _hasUse(false), _hasCast(false), _hasTurn(false) {
    setBounds(Window(0, 24, 39, 24));
}

CombatMenuDialog::~CombatMenuDialog() {
    delete _horizontalMenu;
}

void CombatMenuDialog::beginMenu(Goldbox::Data::PlayerCharacter *actor,
                                 Combat::CombatSession *session) {
    _hasMove = false;
    _hasUse  = false;
    _hasCast = false;
    _hasTurn = false;

    if (!actor || !session)
        return;

    // Effect Set 15 (ES_POISON_CYCLE) runs before menu construction,
    // mirroring UTIL_checkEffectSet(0x0f, char_ptr).
    const Combat::CombatParams &params = session->getParams();
    if (params.effectRuntime && actor->getEffects()) {
        params.effectRuntime->checkEffectSet(
            Goldbox::Data::Effects::ES_POISON_CYCLE,
            *actor->getEffects(), *actor,
            const_cast<Combat::CombatGlobals *>(&session->getGlobals()));
    }

    const Goldbox::Data::CombatAction *cs = actor->combatState;

    // Move: available when movement points remain.
    _hasMove = cs && cs->movePoints != 0;

    // Use: available when character carries at least one item.
    const Goldbox::Data::ADnDCharacter *adnd =
        dynamic_cast<const Goldbox::Data::ADnDCharacter *>(actor);
    _hasUse = adnd && adnd->getInventory().count() > 0;

    // Cast: character has memorized spells AND combat state permits casting
    //       AND world magic is not suppressed.
    bool hasSpells = false;
    const Goldbox::Poolrad::Data::PoolradCharacter *pc =
        dynamic_cast<const Goldbox::Poolrad::Data::PoolradCharacter *>(actor);
    if (pc) {
        for (int i = 0; i < 21; ++i) {
            if (pc->spells.memorizedSpells[i] != 0) {
                hasSpells = true;
                break;
            }
        }
    }
    _hasCast = hasSpells &&
               cs && cs->canCast &&
               session->getGlobals().magicEnabled;

    // Turn Undead: cleric level > 0 and hasn't already turned this round.
    if (adnd) {
        const uint8 clericLevel = adnd->levels[Goldbox::Data::C_CLERIC];
        _hasTurn = clericLevel > 0 && cs && !cs->turnedUndead;
    }
}

void CombatMenuDialog::buildMenuModel() {
    _menuModel.items.clear();
    _menuModel.currentSelection = 0;
    _actions.clear();
    _shortcuts.clear();

    Common::Array<Common::String> labels;

    // Move — conditional on movement points.
    if (_hasMove) {
        labels.push_back("Move");
        _actions.push_back(Combat::CombatSession::PA_MOVE);
        _shortcuts.push_back('M');
    }

    // View — always present.
    labels.push_back("View");
    _actions.push_back(Combat::CombatSession::PA_VIEW);
    _shortcuts.push_back('V');

    // Aim — always present (select target and attack).
    labels.push_back("Aim");
    _actions.push_back(Combat::CombatSession::PA_ATTACK);
    _shortcuts.push_back('A');

    // Use — conditional on inventory.
    if (_hasUse) {
        labels.push_back("Use");
        _actions.push_back(Combat::CombatSession::PA_USE);
        _shortcuts.push_back('U');
    }

    // Cast — conditional on spells + can_cast + magic allowed.
    if (_hasCast) {
        labels.push_back("Cast");
        _actions.push_back(Combat::CombatSession::PA_CAST);
        _shortcuts.push_back('C');
    }

    // Turn — conditional on cleric level and not already turned.
    if (_hasTurn) {
        labels.push_back("Turn");
        _actions.push_back(Combat::CombatSession::PA_TURN);
        _shortcuts.push_back('T');
    }

    // Quick — always present (auto-attack).
    labels.push_back("Quick");
    _actions.push_back(Combat::CombatSession::PA_QUICK);
    _shortcuts.push_back('Q');

    // Done — always present (end turn).
    labels.push_back("Done");
    _actions.push_back(Combat::CombatSession::PA_DONE);
    _shortcuts.push_back('D');

    _menuModel.generateMenuItems(labels, true);
}

void CombatMenuDialog::activate() {
    if (_isActive && _horizontalMenu)
        return;

    Dialog::activate();

    delete _horizontalMenu;
    _horizontalMenu = nullptr;

    buildMenuModel();

    HorizontalMenuConfig cfg;
    cfg.promptTxt             = "";
    cfg.menuItemList          = &_menuModel;
    cfg.textColor             = 10;
    cfg.selectColor           = 15;
    cfg.promptColor           = 15;
    cfg.allowNumPad           = true;
    cfg.suppressUnhandledKeys = false;
    cfg.backgroundColor       = 8;
    cfg.singleItemMode        = false;

    _horizontalMenu = new HorizontalMenu("CombatHMenu", cfg);
    _horizontalMenu->activate();
}

void CombatMenuDialog::deactivate() {
    if (_horizontalMenu) {
        _horizontalMenu->deactivate();
        delete _horizontalMenu;
        _horizontalMenu = nullptr;
    }
    Dialog::deactivate();
}

void CombatMenuDialog::draw() {
    if (!_isVisible || !_horizontalMenu)
        return;
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 8);
    _horizontalMenu->setRedraw();
    _horizontalMenu->draw();
}

bool CombatMenuDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive || !_horizontalMenu)
        return false;

    const bool wasActive = _horizontalMenu->isActive();
    const bool handled   = _horizontalMenu->msgKeypress(msg);

    if (!handled)
        return false;

    if (wasActive && !_horizontalMenu->isActive()) {
        char ascii = msg.ascii;
        if (ascii >= 'a' && ascii <= 'z')
            ascii = static_cast<char>(ascii - 32);

        // Match shortcut against the dynamically built table.
        for (uint i = 0; i < _shortcuts.size(); ++i) {
            if (ascii == _shortcuts[i]) {
                postActionResult(_actions[i]);
                return true;
            }
        }

        // RETURN confirms current selection.
        if (msg.keycode == Common::KEYCODE_RETURN) {
            const int sel = _menuModel.currentSelection;
            if (sel >= 0 && sel < (int)_actions.size())
                postActionResult(_actions[sel]);
            return true;
        }

        // No valid action matched — keep menu active.
        _horizontalMenu->activate();
        _horizontalMenu->setRedraw();
    }

    return true;
}

void CombatMenuDialog::postActionResult(Combat::CombatSession::PlayerAction action) {
    if (g_events)
        g_events->postMenuResult("Combat", true, Common::KEYCODE_RETURN,
                                 static_cast<int>(action), Common::String(),
                                 true, false);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
