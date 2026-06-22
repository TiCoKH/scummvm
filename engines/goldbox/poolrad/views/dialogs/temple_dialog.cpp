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

#include "goldbox/poolrad/views/dialogs/temple_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/views/dialogs/prompt_message.h"
#include "goldbox/poolrad/views/dialogs/vertical_menu.h"
#include "goldbox/poolrad/data/poolrad_character.h"
#include "goldbox/data/effects/effect.h"
#include "goldbox/vm_interface.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using PoolradCharacter = Goldbox::Poolrad::Data::PoolradCharacter;
using Effects = Goldbox::Data::Effects::Effects;

// Costs match the original DOS binary values.
const TempleDialog::HealService TempleDialog::kHealServices[] = {
    {"Cure Blindness",       1000},
    {"Cure Disease",         1000},
    {"Cure Light Wounds",     100},
    {"Cure Serious Wounds",   350},
    {"Cure Critical Wounds",  600},
    {"Neutralize Poison",    1000},
    {"Raise Dead",           5500},
    {"Remove Curse",         3500},
    {"Stone to Flesh",       2000}
};

// Disease effect IDs (original diseaseEffectTable[1..6]).
static const uint8 kDiseaseEffects[] = {
    (uint8)Effects::E_CAUSE_DISEASE_1,
    (uint8)Effects::E_DISEASE_CONFUSED
};
static const int kDiseaseEffectCount = ARRAYSIZE(kDiseaseEffects);

// "not affected" messages per service index.
static const char *kNotAffectedMsg[] = {
    "is not blind.",
    "is not diseased.",
    nullptr,  // wound cures always proceed
    nullptr,
    nullptr,
    "is not poisoned.",
    "is not dead.",
    "is not cursed.",
    "is not stoned."
};

static ShopBaseConfig makeTempleConfig() {
    ShopBaseConfig cfg;
    cfg.type = SHOP_TEMPLE;
    cfg.menuPrompt = "";
    cfg.exitConfirmLine1 = "As you leave a priest says, 'Excuse me"
        " but you left some things here.'";
    cfg.exitConfirmLine2 = "Do you want to go back and retrieve them?";
    return cfg;
}

TempleDialog::TempleDialog(const Common::String &name)
    : ShopBaseDialog(name, makeTempleConfig()),
      _healStage(HEAL_NONE), _healSelectedIndex(-1),
      _healSelector(nullptr), _yesNoDialog(nullptr),
      _healPrompt(nullptr) {
}

TempleDialog::~TempleDialog() {
    delete _healSelector;
    delete _yesNoDialog;
    delete _healPrompt;
}

void TempleDialog::getShopFlags(bool &hasItems, bool &hasMoney) {
    const TreasurePool &pool = VmInterface::getTreasurePool();
    hasItems = pool.hasItems();
    hasMoney = pool.hasAnyCoin();
}

// --- Primary action ---

void TempleDialog::actionPrimary() {
    openHealSelector();
}

void TempleDialog::buildHealMenuItems() {
    _healMenuItems.items.clear();
    _healMenuItems.currentSelection = 0;

    // Load service names from global_strings.yml "temple.services.0" .. "temple.services.9"
    for (int i = 0; i < 10; ++i) {
        Common::String key = Common::String::format("temple.services.%d", i);
        MenuItem mi;
        mi.text = VmInterface::getString(key);
        mi.shortcut = 0;
        mi.active = true;
        mi.shortcutFirst = false;
        _healMenuItems.items.push_back(mi);
    }
}

void TempleDialog::openHealSelector() {
    _healStage = HEAL_SELECTOR;
    buildHealMenuItems();

    if (_healSelector) {
        delete _healSelector;
        _healSelector = nullptr;
    }

    _healPromptOpts.clear();

    // Load prompt options from yml as space-separated string
    Common::Array<Common::String> promptTokens =
        VmInterface::getStringTokens("temple.prompt");
    for (uint i = 0; i < promptTokens.size(); ++i)
        _healPromptOpts.push_back(promptTokens[i]);

    VerticalMenuConfig cfg;
    cfg.promptTxt = "";
    cfg.promptOptions = &_healPromptOpts;
    cfg.menuItemList = &_healMenuItems;
    cfg.headColor = 15;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.xStart = 2;
    cfg.yStart = 4;
    cfg.xEnd = 38;
    cfg.yEnd = 15;
    cfg.title = "";
    cfg.addExit = false;

    _healSelector = new VerticalMenu("TempleHealMenu", cfg);
    setDialogParent(_healSelector, this);
    _healSelector->activate();
    redraw();
}

void TempleDialog::closeHealSelector() {
    if (_healSelector) {
        _healSelector->deactivate();
        delete _healSelector;
        _healSelector = nullptr;
    }
    _healStage = HEAL_NONE;
    _healSelectedIndex = -1;

    Surface s = getSurface();
    s.clearBox(1, 1, 38, 22, 0);
    redraw();
}

void TempleDialog::handleHealSelectorResult(const MenuResultMessage &result) {
    Common::KeyCode key = result._keyCode;

    if (key == Common::KEYCODE_h || key == Common::KEYCODE_RETURN) {
        int sel = _healMenuItems.currentSelection;
        if (sel >= 0 && sel < kHealServiceCount)
            beginHealService(sel);
        else
            closeHealSelector();  // index 9 (Exit) or out of range
    } else {
        closeHealSelector();
    }
}

// --- Heal service flow ---

void TempleDialog::beginHealService(int index) {
    _healSelectedIndex = index;

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    PoolradCharacter *ch = dynamic_cast<PoolradCharacter *>(base);
    if (!ch) {
        closeHealSelector();
        return;
    }

    // Wound cures (indices 2-4) skip the "not affected" check
    if (index >= 2 && index <= 4) {
        showPayConfirmPrompt();
        return;
    }

    // Check if character has the condition
    if (!hasCondition(ch, index)) {
        showNotAffectedPrompt();
    } else {
        showPayConfirmPrompt();
    }
}

void TempleDialog::showNotAffectedPrompt() {
    _healStage = HEAL_NOT_AFFECTED;
    destroyYesNo();

    // Display "<Name> is not <condition>." in the body area
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    if (base && kNotAffectedMsg[_healSelectedIndex]) {
        Surface s = getSurface();
        Common::String msg = Common::String::format("%s %s",
            base->name.c_str(), kNotAffectedMsg[_healSelectedIndex]);
        s.drawWindow(1, 17, 38, 22, 0);
        s.writeStringC(2, 18, 28, msg);
    }

    HorizontalYesNoConfig cfg;
    cfg.promptTxt = "Cast cure anyway?";
    cfg.promptColor = 28;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.backgroundColor = 0;

    _yesNoDialog = new HorizontalYesNo("TempleNotAffected", cfg);
    setDialogParent(_yesNoDialog, this);
    _yesNoDialog->activate();
    redraw();
}

void TempleDialog::showPayConfirmPrompt() {
    _healStage = HEAL_CONFIRM_PAY;
    destroyYesNo();

    // Display: "<Service> will only cost <N> gold pieces."
    uint16 cost = kHealServices[_healSelectedIndex].cost;
    Common::String costMsg = Common::String::format(
        "%s will only cost %u gold pieces.",
        kHealServices[_healSelectedIndex].name, cost);

    Surface s = getSurface();
    s.drawWindow(1, 17, 38, 22, 0);
    s.writeStringC(2, 18, 28, costMsg);

    HorizontalYesNoConfig cfg;
    cfg.promptTxt = "Pay for cure?";
    cfg.promptColor = 16;
    cfg.textColor = 10;
    cfg.selectColor = 15;
    cfg.backgroundColor = 0;

    _yesNoDialog = new HorizontalYesNo("TemplePayConfirm", cfg);
    setDialogParent(_yesNoDialog, this);
    _yesNoDialog->activate();
    redraw();
}

void TempleDialog::handleNotAffectedResult(const MenuResultMessage &result) {
    destroyYesNo();

    if (result._keyCode == Common::KEYCODE_y) {
        // Player wants to cast anyway — proceed to payment
        showPayConfirmPrompt();
    } else {
        // Declined — return to heal selector
        _healStage = HEAL_SELECTOR;
        if (_healSelector)
            _healSelector->activate();
        redraw();
    }
}

void TempleDialog::handlePayConfirmResult(const MenuResultMessage &result) {
    destroyYesNo();

    if (result._keyCode != Common::KEYCODE_y) {
        // Declined payment — return to heal selector
        _healStage = HEAL_SELECTOR;
        if (_healSelector)
            _healSelector->activate();
        redraw();
        return;
    }

    // Attempt payment
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    PoolradCharacter *ch = dynamic_cast<PoolradCharacter *>(base);
    if (!ch) {
        closeHealSelector();
        return;
    }

    uint16 cost = kHealServices[_healSelectedIndex].cost;
    uint32 charGold = ch->valuableItems.getGoldValue();

    if (charGold >= cost) {
        deductGoldFromCharacter(ch, cost);
    } else {
        TreasurePool &pool = VmInterface::getTreasurePool();
        uint32 poolGold = pool.coins().getGoldValue();
        if (poolGold < cost) {
            showHealMessage("Not enough money");
            _healStage = HEAL_SELECTOR;
            if (_healSelector)
                _healSelector->activate();
            return;
        }
        deductGoldFromPool(cost);
    }

    // Payment succeeded — apply the cure
    completeHeal();
}

void TempleDialog::completeHeal() {
    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    PoolradCharacter *ch = dynamic_cast<PoolradCharacter *>(base);
    if (ch)
        applyHealEffect(ch, _healSelectedIndex);

    // Show "is cured" message
    Common::String msg;
    if (ch)
        msg = Common::String::format("%s is cured.", ch->name.c_str());
    else
        msg = "Is cured.";
    showHealMessage(msg);

    // Return to heal selector
    _healStage = HEAL_SELECTOR;
    if (_healSelector)
        _healSelector->activate();
}

// --- Condition check ---

bool TempleDialog::hasCondition(PoolradCharacter *ch, int index) {
    switch (index) {
    case 0: // Cure Blindness
        return ch->effects.hasEffectType((uint8)Effects::E_BLINDED);

    case 1: // Cure Disease
        for (int i = 0; i < kDiseaseEffectCount; ++i) {
            if (ch->effects.hasEffectType(kDiseaseEffects[i]))
                return true;
        }
        return false;

    case 2: // Cure Light Wounds
    case 3: // Cure Serious Wounds
    case 4: // Cure Critical Wounds
        return ch->hitPoints.current < ch->hitPoints.max;

    case 5: // Neutralize Poison
        return ch->effects.hasEffectType((uint8)Effects::E_POISONED);

    case 6: // Raise Dead
        return (ch->healthStatus == Goldbox::Data::S_DEAD) ||
               (ch->healthStatus == Goldbox::Data::S_ANIMATED);

    case 7: // Remove Curse
        for (int i = 0; i < ch->inventory.count(); ++i) {
            if (ch->inventory[i].cursed)
                return true;
        }
        return ch->effects.hasEffectType((uint8)Effects::E_BESTOW_CURSE);

    case 8: // Stone to Flesh
        return ch->healthStatus == Goldbox::Data::S_STONED;

    default:
        return false;
    }
}

// --- Effect application ---

void TempleDialog::applyHealEffect(PoolradCharacter *ch, int index) {
    switch (index) {
    case 0: // Cure Blindness
        removeAllEffectsOfType(ch, (uint8)Effects::E_BLINDED);
        break;

    case 1: // Cure Disease
        for (int i = 0; i < kDiseaseEffectCount; ++i)
            removeAllEffectsOfType(ch, kDiseaseEffects[i]);
        break;

    case 2: { // Cure Light Wounds: 1d8
        uint8 rolled = (uint8)VmInterface::rollDice(1, 8);
        ch->heal(rolled);
        break;
    }

    case 3: { // Cure Serious Wounds: 2d8 + 1
        uint8 rolled = (uint8)(VmInterface::rollDice(2, 8) + 1);
        ch->heal(rolled);
        break;
    }

    case 4: { // Cure Critical Wounds: 3d8 + 3
        uint8 rolled = (uint8)(VmInterface::rollDice(3, 8) + 3);
        ch->heal(rolled);
        break;
    }

    case 5: // Neutralize Poison — 3 related effects
        removeAllEffectsOfType(ch, (uint8)Effects::E_POISONED);
        removeAllEffectsOfType(ch, (uint8)Effects::E_SLOW_POISON);
        removeAllEffectsOfType(ch, (uint8)Effects::E_POISON_DAMAGE);
        break;

    case 6: // Raise Dead
        applyRaiseDead(ch);
        break;

    case 7: // Remove Curse
        applyRemoveCurse(ch);
        break;

    case 8: // Stone to Flesh
        ch->healthStatus = Goldbox::Data::S_OKAY;
        ch->enabled = true;
        ch->hitPoints.current = 1;
        break;
    }
}

void TempleDialog::applyRaiseDead(PoolradCharacter *ch) {
    removeAllEffectsOfType(ch, (uint8)Effects::E_ANIMATE_DEAD);
    removeAllEffectsOfType(ch, (uint8)Effects::E_POISONED);

    ch->hitPoints.current = 1;
    ch->healthStatus = Goldbox::Data::S_OKAY;
    ch->enabled = true;

    // Permanent constitution loss
    if (ch->abilities.constitution.current > 0)
        ch->abilities.constitution.current -= 1;
    if (ch->abilities.constitution.base > 0)
        ch->abilities.constitution.base -= 1;

    // HP reduction from lowered constitution (only if CON > 13)
    uint8 con = ch->abilities.constitution.current;
    if (con > 13) {
        uint8 hpDiff = (ch->hitPoints.max > ch->hitPointsRolled)
                       ? (ch->hitPoints.max - ch->hitPointsRolled) : 0;
        uint divisor = 0;

        for (int classIdx = 0; classIdx < BASE_CLASS_NUM; ++classIdx) {
            uint8 lvl = ch->levels.levels[classIdx];
            if (lvl == 0)
                continue;
            if (classIdx == 2) {
                divisor += (con - 14) * lvl;
            } else if (con < 16) {
                divisor += lvl;
            } else {
                divisor += lvl * 2;
            }
        }

        if (divisor > 0) {
            uint8 hpReduction = hpDiff / divisor;
            if (con < 17 || ch->levels.levels[2] > 0) {
                if (hpReduction >= ch->hitPoints.max)
                    ch->hitPoints.max = 1;
                else
                    ch->hitPoints.max -= hpReduction;
            }
        }
    }
}

void TempleDialog::applyRemoveCurse(PoolradCharacter *ch) {
    for (int i = 0; i < ch->inventory.count(); ++i)
        ch->inventory[i].cursed = 0;
    removeAllEffectsOfType(ch, (uint8)Effects::E_BESTOW_CURSE);
}

void TempleDialog::removeAllEffectsOfType(PoolradCharacter *ch, uint8 type) {
    int idx;
    while ((idx = ch->effects.findEffectIndexByType(type)) >= 0)
        ch->effects.removeEffectAt(idx);
}

// --- UI helpers ---

void TempleDialog::showHealMessage(const Common::String &msg) {
    if (_healPrompt) {
        delete _healPrompt;
        _healPrompt = nullptr;
    }

    PromptMessageConfig cfg;
    cfg.message = msg;
    cfg.textColor = 10;
    _healPrompt = new PromptMessage("TempleHealMsg", cfg);
    setDialogParent(_healPrompt, this);
    _healPrompt->activate();
}

void TempleDialog::destroyYesNo() {
    if (_yesNoDialog) {
        _yesNoDialog->deactivate();
        delete _yesNoDialog;
        _yesNoDialog = nullptr;
    }
}

// --- Gold helpers ---

void TempleDialog::deductGoldFromCharacter(
        Goldbox::Poolrad::Data::PoolradCharacter *ch, uint32 cost) {
    uint32 charGold = ch->valuableItems.getGoldValue();
    ch->valuableItems.setFromGoldValue(charGold - cost);
}

void TempleDialog::deductGoldFromPool(uint32 cost) {
    TreasurePool &pool = VmInterface::getTreasurePool();
    uint32 poolGold = pool.coins().getGoldValue();
    pool.coins().setFromGoldValue(poolGold - cost);
}

// --- Event routing ---

bool TempleDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    if (_healPrompt && _healPrompt->isActive())
        return _healPrompt->handleKeypress(msg);

    switch (_healStage) {
    case HEAL_NOT_AFFECTED:
    case HEAL_CONFIRM_PAY:
        if (_yesNoDialog && _yesNoDialog->isActive())
            return _yesNoDialog->msgKeypress(msg);
        return true;

    case HEAL_SELECTOR:
        if (_healSelector && _healSelector->isActive())
            return _healSelector->dispatchKeypress(msg);
        return true;

    default:
        break;
    }

    return ShopBaseDialog::msgKeypress(msg);
}

void TempleDialog::handleMenuResult(const MenuResultMessage &result) {
    switch (_healStage) {
    case HEAL_SELECTOR:
        handleHealSelectorResult(result);
        return;
    case HEAL_NOT_AFFECTED:
        handleNotAffectedResult(result);
        return;
    case HEAL_CONFIRM_PAY:
        handlePayConfirmResult(result);
        return;
    default:
        break;
    }
    ShopBaseDialog::handleMenuResult(result);
}

void TempleDialog::draw() {
    if (!_isVisible)
        return;

    if (_healStage != HEAL_NONE) {
        Surface s = getSurface();

        if (_healSelector) {
            // Outer frame and title bar borders
            s.drawWindow(1, 1, 38, 22, 0);
            s.drawWindow(1, 1, 38, 1, 0);

            // Greeting: "<Name>, how can we help you?"
            Goldbox::Data::PlayerCharacter *base =
                VmInterface::getSelectedCharacter();
            if (base) {
                Common::String greeting = Common::String::format(
                    "%s, how can we help you?", base->name.c_str());
                if (greeting.size() > 38)
                    greeting = greeting.substr(0, 38);
                s.writeStringC(1, 1, 28, greeting);
            }

            if (_healSelector->isActive())
                _healSelector->draw();

            // Body window border drawn AFTER vertical menu to
            // prevent the menu's embedded HorizontalMenu from
            // overwriting it.
            s.drawWindow(1, 17, 38, 22, 0);
        }

        if (_yesNoDialog && _yesNoDialog->isActive())
            _yesNoDialog->draw();

        if (_healPrompt && _healPrompt->isActive())
            _healPrompt->draw();
        return;
    }

    ShopBaseDialog::draw();

    if (_healPrompt && _healPrompt->isActive())
        _healPrompt->draw();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
