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
#include "goldbox/data/effects/effect_system.h"
#include "goldbox/data/effects/effect_handler_base.h"
#include "goldbox/poolrad/data/poolrad_effects.h"
#include "goldbox/spells/spell_context.h"
#include "goldbox/spells/spell_definition.h"
#include "goldbox/spells/spell_handlers.h"
#include "goldbox/poolrad/ecl/poolrad_engine_host_impl.h"
#include "goldbox/vm_interface.h"
#include "goldbox/runtime/treasure_pool.h"
#include "goldbox/events.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

using PoolradCharacter = Goldbox::Poolrad::Data::PoolradCharacter;

// GB13-029 (Req 36.1): RAW PoR effect ids used at the game layer. These are
// the original single-byte .SPC effect ids (see poolrad_effects.h), NOT the
// generic Effects enum ordinals in effect.h -- the same raw value means a
// different thing per game, so the temple MUST key off the raw ids directly.
//
// Decimal / hex cross-reference against PoolradEffects:
//   blindness = 33 (0x21)  == E_POOLRAD_BLIND
//   poison check = 55 (0x37); poison remove = 55,22,15 IN ORDER
//                            (0x37, 0x16 E_POOLRAD_SLOW_POISON_END, 0x0F poison dmg)
//   disease group = 31,34,43,44,50,57
//                            (0x1F,0x22 E_POOLRAD_DISEASED,0x2B,0x2C,0x32,0x39)
// The disease grouping is the temple's own conditional set and is kept DISTINCT
// from Magic.RemoveDiseaseEffects (which groups differently); both reuse the
// same low-level EffectSystem removal, but the temple owns this id list.

// Blindness (Cure Blindness): check + remove raw 33.
static const uint8 kTempleBlindIds[] = { 33 };

// Disease (Cure Disease): present iff ANY raw id below is set; remove each
// PRESENT id. Order preserved as listed.
static const uint8 kTempleDiseaseIds[] = { 31, 34, 43, 44, 50, 57 };

// Poison (Neutralize Poison): presence CHECKS raw 55 only; removal runs raw
// 55 then 22 then 15 IN ORDER (per the original removal sequence).
static const uint8 kTemplePoisonCheckIds[] = { 55 };
static const uint8 kTemplePoisonRemoveIds[] = { 55, 22, 15 };

// Raise Dead / Remove Curse / Stone to Flesh keep their existing presence
// predicates (status / inventory curse flags), which are not raw-effect-id
// sets; their rule operations are owned by GB13-030 (task 34) and GB13-031
// (task 35). Their metadata rows below carry only price + the no-raw-id
// predicate marker; apply logic still routes through applyHealEffect.

// Costs match the original DOS binary values. The presence/removal raw-id
// columns drive hasCondition() and applyHealEffect() for the in-scope
// services (blindness, disease, wounds, poison).
const TempleDialog::HealService TempleDialog::kHealServices[] = {
    // name                 cost  condIds              condN  remIds                 remN   dN dS bonus
    {"Cure Blindness",       1000, kTempleBlindIds,       1,    kTempleBlindIds,        1,    0, 0, 0},
    {"Cure Disease",         1000, kTempleDiseaseIds,     6,    kTempleDiseaseIds,      6,    0, 0, 0},
    {"Cure Light Wounds",     100, nullptr,               0,    nullptr,                0,    1, 8, 0},
    {"Cure Serious Wounds",   350, nullptr,               0,    nullptr,                0,    2, 8, 1},
    {"Cure Critical Wounds",  600, nullptr,               0,    nullptr,                0,    3, 8, 3},
    {"Neutralize Poison",    1000, kTemplePoisonCheckIds, 1,    kTemplePoisonRemoveIds, 3,    0, 0, 0},
    {"Raise Dead",           5500, nullptr,               0,    nullptr,                0,    0, 0, 0},
    {"Remove Curse",         3500, nullptr,               0,    nullptr,                0,    0, 0, 0},
    {"Stone to Flesh",       2000, nullptr,               0,    nullptr,                0,    0, 0, 0}
};

// "not affected" messages per service index.
static const char *kNotAffectedMsg[] = {
    "is not blind.",
    "is not Diseased.",
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
    cfg.exitConfirmLine1 = "As you leave a priest says, \"Excuse me"
        " but you have left some money here\" ";
    cfg.exitConfirmLine2 = "Do you want to go back and retrieve your money?";
    cfg.exitConfirmColor2 = 10;
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

// --- Primary action ---

void TempleDialog::actionPrimary() {
    openHealSelector();
}

void TempleDialog::resetServiceChildren() {
    // Temple-specific children: heal selector, Y/N prompts, heal message.
    if (_healSelector) {
        _healSelector->deactivate();
        delete _healSelector;
        _healSelector = nullptr;
    }
    destroyYesNo();
    if (_healPrompt) {
        _healPrompt->deactivate();
        delete _healPrompt;
        _healPrompt = nullptr;
    }
    _healStage = HEAL_NONE;
    _healSelectedIndex = -1;

    // Then the shared base transaction stages (Take/Appraise/Exit-confirm).
    ShopBaseDialog::resetServiceChildren();
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

    // GB13-029 (Req 36.2): open a fresh staged payment plan for this service.
    _payment = PaymentPlan();
    _payment.serviceIndex = index;
    _payment.cost = kHealServices[index].cost;

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    PoolradCharacter *ch = dynamic_cast<PoolradCharacter *>(base);
    if (!ch) {
        closeHealSelector();
        return;
    }

    // GB13-030 (Req 40.1): capture eligibility NOW, before any prompt, and
    // retain it on the plan. For Raise Dead (6) and Stone to Flesh (8) this
    // pre-prompt eligibility is what applyHealEffect gates the mutation on, so
    // a cast-anyway on a LIVING / NON-STONED target pays and reports cured but
    // mutates nothing. (Other services are unaffected; they key off their own
    // raw-id / HP predicates at apply-time.)
    _payment.eligibleAtRequest = hasCondition(ch, index);

    // Wound cures (indices 2-4) skip the "not affected" check
    if (index >= 2 && index <= 4) {
        showPayConfirmPrompt();
        return;
    }

    // Stage: ConfirmCureAnyway only when the character lacks the condition.
    if (!_payment.eligibleAtRequest) {
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
    cfg.promptTxt = "cast cure anyway: ";
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
    cfg.promptTxt = "pay for cure ";
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
        // ConfirmCureAnyway accepted — record the decision, go to payment.
        _payment.castAnyway = true;
        showPayConfirmPrompt();
    } else {
        // Declined — abandon the plan, return to heal selector.
        _payment = PaymentPlan();
        _healStage = HEAL_SELECTOR;
        if (_healSelector)
            _healSelector->activate();
        redraw();
    }
}

// GB13-029 (Req 36.2): funding-choice / denomination-normalization stage.
// Pure decision: current-character funds if sufficient, ELSE pooled funds if
// sufficient, ELSE FUND_NONE. Two insufficient balances are NEVER combined.
TempleDialog::PaymentPlan::FundingSource TempleDialog::resolveFunding(
        uint32 charGold, uint32 poolGold, uint16 cost) {
    if (charGold >= cost)
        return PaymentPlan::FUND_CHARACTER;
    if (poolGold >= cost)
        return PaymentPlan::FUND_POOL;
    return PaymentPlan::FUND_NONE;
}

void TempleDialog::handlePayConfirmResult(const MenuResultMessage &result) {
    destroyYesNo();

    if (result._keyCode != Common::KEYCODE_y) {
        // ConfirmCureCost declined — abandon the plan, return to selector.
        _payment = PaymentPlan();
        _healStage = HEAL_SELECTOR;
        if (_healSelector)
            _healSelector->activate();
        redraw();
        return;
    }

    Goldbox::Data::PlayerCharacter *base = VmInterface::getSelectedCharacter();
    PoolradCharacter *ch = dynamic_cast<PoolradCharacter *>(base);
    if (!ch) {
        closeHealSelector();
        return;
    }

    // Stage: funding choice. Resolve WITHOUT mutating, then normalize the
    // denomination through the existing deduct helpers (which round-trip the
    // gold value via the coin slots). Deliberately does NOT widen Word/LongInt
    // boundaries and NEVER combines two insufficient balances.
    _payment.serviceIndex = _healSelectedIndex;
    _payment.cost = kHealServices[_healSelectedIndex].cost;

    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    _payment.funding = resolveFunding(ch->valuableItems.getGoldValue(),
        pool.poolGoldValue(), _payment.cost);

    switch (_payment.funding) {
    case PaymentPlan::FUND_CHARACTER:
        deductGoldFromCharacter(ch, _payment.cost);
        break;
    case PaymentPlan::FUND_POOL:
        deductGoldFromPool(_payment.cost);
        break;
    case PaymentPlan::FUND_NONE:
    default:
        // Neither source alone is sufficient: no mutation, no cure.
        showHealMessage("Not enough money.");
        _payment = PaymentPlan();
        _healStage = HEAL_SELECTOR;
        if (_healSelector)
            _healSelector->activate();
        return;
    }

    // Payment succeeded — apply the single cure operation.
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
    case 0: // Cure Blindness  -- raw-id presence (33)
    case 1: // Cure Disease    -- raw-id presence (any of 31,34,43,44,50,57)
    case 5: { // Neutralize Poison -- raw-id presence (checks 55 only)
        // GB13-029 (Req 36.1): metadata-driven presence via RAW ids. Present
        // iff ANY of the service's condition ids is on the character.
        const HealService &svc = kHealServices[index];
        for (uint8 i = 0; i < svc.conditionCount; ++i) {
            if (ch->effects.hasEffect(svc.conditionIds[i]))
                return true;
        }
        return false;
    }

    case 2: // Cure Light Wounds
    case 3: // Cure Serious Wounds
    case 4: // Cure Critical Wounds
        return ch->hitPoints.current < ch->hitPoints.max;

    case 6: // Raise Dead (presence owned here; apply logic is GB13-030/task 34)
        return (ch->healthStatus == Goldbox::Data::S_DEAD) ||
               (ch->healthStatus == Goldbox::Data::S_ANIMATED);

    case 7: // Remove Curse (apply logic is GB13-031/task 35)
        // GB13-031 (Req 41.1): present iff a raw curse effect 36
        // (E_POOLRAD_BESTOW_CURSE == 0x24) OR any cursed inventory item is set.
        // The former predicate keyed off E_BESTOW_CURSE (37), the wrong id;
        // the shared RemoveCurseHandler clears raw 36, so presence must match.
        for (int i = 0; i < ch->inventory.count(); ++i) {
            if (ch->inventory[i].cursed)
                return true;
        }
        return ch->effects.hasEffect(
            (uint8)Goldbox::Data::Effects::E_POOLRAD_BESTOW_CURSE);

    case 8: // Stone to Flesh (apply logic is GB13-030/task 34)
        return ch->healthStatus == Goldbox::Data::S_STONED;

    default:
        return false;
    }
}

// --- Effect application ---

void TempleDialog::applyHealEffect(PoolradCharacter *ch, int index) {
    const HealService &svc = kHealServices[index];

    switch (index) {
    case 0: // Cure Blindness      -- remove raw 33
    case 1: // Cure Disease        -- remove each PRESENT of 31,34,43,44,50,57
    case 5: // Neutralize Poison   -- remove 55,22,15 IN ORDER
        // GB13-029 (Req 36.3): metadata-driven removal of RAW ids, in list
        // order, through the EffectSystem so the original on-remove behavior
        // runs. One instance per present id (removeRawEffectInstance removes
        // the first match). For the disease group a non-present id is simply
        // skipped; for poison the fixed 55,22,15 order is the list order.
        // Cure Disease and Neutralize Poison hold the effect lock so the
        // periodic disease/poison handlers do not re-arm while removed.
        if (index != 0 && g_engine)
            g_engine->setInSpellProcess(true);
        for (uint8 i = 0; i < svc.removeCount; ++i) {
            if (ch->effects.hasEffect(svc.removeIds[i]))
                removeRawEffectInstance(ch, svc.removeIds[i]);
        }
        if (index != 0 && g_engine)
            g_engine->setInSpellProcess(false);
        break;

    case 2: // Cure Light Wounds:   1d8
    case 3: // Cure Serious Wounds:  2d8 + 1
    case 4: { // Cure Critical Wounds: 3d8 + 3
        // FxEngine.RestoreHitPoints(..., False): only the living, blocked by
        // mummy rot, wakes the unconscious; the result is ignored.
        uint8 rolled = (uint8)(VmInterface::rollDice(svc.healDiceNum,
            svc.healDiceSides) + svc.healBonus);
        ch->healHp(rolled, false);
        break;
    }

    case 6: // Raise Dead (GB13-030 / task 34 owns this body)
        applyRaiseDead(ch);
        break;

    case 7: // Remove Curse (GB13-031 / task 35 owns this body)
        applyRemoveCurse(ch);
        break;

    case 8:
        // Stone to Flesh (GB13-030 / task 34). Req 40.3: change status / HP
        // ONLY if the target was Stoned at request time (captured pre-prompt).
        // A cast-anyway on a non-stoned target pays + reports cured but leaves
        // status, enabled and HP untouched.
        if (_payment.eligibleAtRequest) {
            ch->healthStatus = Goldbox::Data::S_OKAY;
            ch->enabled = true;
            ch->hitPoints.current = 1;
        }
        break;
    }
}

// GB13-030 (Req 40.2): pure Constitution-loss HP arithmetic, mirroring the
// DOS byte math. `newCon` is the ALREADY-lowered current Constitution; the
// caller has done the -1. The gate con > 13 means only CON 14..18 reduces HP.
//
// Divisor (unsigned accumulator, documented as `uint` to mirror the DOS 16-bit
// intermediate so the (con-14)*lvl / lvl*2 products do not wrap in a byte):
//   per the eight base-class level entries, the FIGHTER slot (index
//   C_FIGHTER == 2) contributes (con - 14) * lvl; every other populated slot
//   contributes lvl   when con < 16, else lvl * 2.
// hpLoss = max(hpMax - hpRolled, 0), a byte (hpMax, hpRolled are bytes).
//
// DIVISOR-ZERO: the original does nothing in this case, so the hpLoss is
// RETAINED (HP max is left as-is). We make that explicit here rather than
// relying on fall-through. There is NO invented minimum-HP clamp.
//
// OPEN (trace-gated): the original `>= hpMax -> hpMax = 1` branch below. It is
// kept ONLY because it is the confirmable port of the DOS clamp that prevents
// the unsigned `hpMax -= hpReduction` from wrapping past zero (hpReduction can
// exceed hpMax once the divisor is small). If a trace shows the DOS binary
// does NOT clamp and instead wraps, this branch is the residual to revisit.
// We do NOT add any clamp beyond this confirmable one.
TempleDialog::RaiseDeadResult TempleDialog::computeRaiseDeadResult(
        uint8 newCon, uint8 hpMax, uint8 hpRolled, const uint8 *levels,
        int levelCount) {
    RaiseDeadResult res;
    res.newHpMax = hpMax;

    if (newCon <= 13)
        return res;

    uint8 hpLoss = (hpMax > hpRolled) ? (uint8)(hpMax - hpRolled) : (uint8)0;

    uint divisor = 0;
    for (int classIdx = 0; classIdx < levelCount; ++classIdx) {
        uint8 lvl = levels[classIdx];
        if (lvl == 0)
            continue;
        if (classIdx == (int)Goldbox::Data::C_FIGHTER)
            divisor += (uint)(newCon - 14) * lvl;
        else if (newCon < 16)
            divisor += lvl;
        else
            divisor += (uint)lvl * 2;
    }

    // Divisor-zero: hpLoss retained, HP max unchanged. Explicit, no clamp.
    if (divisor == 0)
        return res;

    uint8 hpReduction = (uint8)(hpLoss / divisor);
    if (newCon < 17 || levels[(int)Goldbox::Data::C_FIGHTER] > 0) {
        if (hpReduction >= hpMax)
            res.newHpMax = 1;       // confirmable DOS clamp; see OPEN note above
        else
            res.newHpMax = (uint8)(hpMax - hpReduction);
    }
    return res;
}

void TempleDialog::applyRaiseDead(PoolradCharacter *ch) {
    // GB13-030 (Req 40.1): a LIVING target reaching here via cast-anyway must
    // NOT change any revival stats (no Con/HP/status mutation). Gate on the
    // eligibility captured BEFORE the prompt. Payment + "is cured" still ran.
    if (!_payment.eligibleAtRequest)
        return;

    // Req 40.1/40.2: remove animation (RAW 32 == E_POOLRAD_ANIMATE_DEAD)
    // and poison (RAW 55) through the shared EffectSystem removal path so the
    // on-remove behavior runs. These are the ORIGINAL PoR raw .SPC ids, not
    // the generic Effects enum ordinals (E_ANIMATE_DEAD==71 was the wrong id
    // the prior body used; the correct animation id is 0x20 == 32).
    const uint8 kRawAnimateDead = 32;   // E_POOLRAD_ANIMATE_DEAD (0x20)
    const uint8 kRawPoison = 55;        // poison (0x37, == E_POISONED)
    // Held under the effect lock, so the animation does not "collapse".
    if (g_engine)
        g_engine->setInSpellProcess(true);
    // Temple.RaiseDead removes one node of each, as here.
    removeRawEffectInstance(ch, kRawAnimateDead);
    removeRawEffectInstance(ch, kRawPoison);
    if (g_engine)
        g_engine->setInSpellProcess(false);

    ch->hitPoints.current = 1;
    ch->healthStatus = Goldbox::Data::S_OKAY;
    ch->enabled = true;

    // Req 40.1: reduce CURRENT Constitution ONLY by 1. The base score is NOT
    // touched (the former base -= 1 was the bug).
    if (ch->abilities.constitution.current > 0)
        ch->abilities.constitution.current -= 1;

    // Req 40.2: HP reduction from the lowered current Constitution. Delegated
    // to the pure helper so the odd DOS byte arithmetic stays unit-testable.
    RaiseDeadResult r = computeRaiseDeadResult(
        ch->abilities.constitution.current, ch->hitPoints.max,
        ch->hitPointsRolled, ch->levels.levels.begin(), BASE_CLASS_NUM);
    ch->hitPoints.max = r.newHpMax;
}

void TempleDialog::applyRemoveCurse(PoolradCharacter *ch) {
    // GB13-031 (Req 41.1/41.2/41.4): run the SHARED one-target Remove Curse
    // RULE on the current target, reusing the existing RemoveCurseHandler
    // sequence rather than "remove all curses". The handler removes raw curse
    // effect 36 (E_POOLRAD_BESTOW_CURSE) FIRST and ONLY IF ABSENT clears the FIRST
    // cursed inventory item -- one thing per invocation. Because the temple
    // calls it once per service, successive services peel off
    // effect -> first item -> second item in that order.
    //
    // This is a SERVICE-ORIGIN invocation: we build a SpellContext/
    // TargetSelection and call the rule handler directly. We do NOT route
    // through g_spellCasting->castSpell or any SpellsMenu, so NO memorized
    // spell slot is spent (Req 41.4 -- temple Remove Curse consumes no
    // memorized spell; only the player cast path spends a resource, and that
    // path is untouched). RemoveCurseHandler::execute itself never touches
    // memorized spells.
    //
    // The EffectSystem is constructed the SAME way removeRawEffectInstance
    // builds it (g_effectHandler + the Poolrad host bridge), so the removal of
    // raw 36 runs the original on-remove behavior through the shared path.
    //
    // OPEN (trace-gated): a service-origin cast ideally reuses the engine's
    // registered Remove Curse SpellDefinition. RemoveCurseHandler::execute
    // ignores `definition` ((void)definition), so a default SpellDefinition is
    // correct for THIS handler; if a future trace shows the service must carry
    // the looked-up definition (e.g. for messaging), swap in the registry
    // lookup the player cast path uses. The rule semantics above are unchanged.
    Goldbox::Data::Effects::EffectSystem effectSystem(
        Goldbox::Data::Effects::g_effectHandler,
        Goldbox::Poolrad::getEffectHostBridge());

    Goldbox::Spells::SpellContext ctx;
    ctx.caster = ch;
    ctx.effectSystem = &effectSystem;
    ctx.inCombat = false;
    ctx.fromMagicItem = false;   // service origin, not a memorized-slot cast

    Goldbox::Spells::TargetSelection targets;
    targets.targetCharacters.push_back(ch);   // the current (selected) target only

    Goldbox::Spells::SpellDefinition definition;   // handler ignores definition

    // One service == one invocation: removes raw 36 if present, else the first
    // cursed item. Do NOT loop here (that would be "remove all curses").
    Goldbox::Spells::RemoveCurseHandler().execute(ctx, definition, targets);
}

// GB13-029 (Req 36.3): remove ONE instance of a raw effect id through the
// EffectSystem, replacing the former direct vector erase in
// removeAllEffectsOfType. removeEffectById fires EFF_REMOVE for immediate
// effects, calls onEffectsChanged() (recalc of derived stats) and notifies
// the bridge -- i.e. it runs the original on-remove behavior -- then erases
// the first matching node. Removing exactly one instance per call matches the
// finding's stated "remove one instance when duplicates exist" semantics.
//
// Callers that cure disease, poison or animation hold the effect lock
// (Engine::setInSpellProcess), as Temple.pas does with GDVars.effectLock.
// It only stops handlers from re-adding themselves (AddEffectIfNoLock) and
// the Animate Dead "collapses" message.
bool TempleDialog::removeRawEffectInstance(PoolradCharacter *ch, uint8 rawId) {
    Goldbox::Data::Effects::EffectSystem effectSystem(
        Goldbox::Data::Effects::g_effectHandler,
        Goldbox::Poolrad::getEffectHostBridge());
    return effectSystem.removeEffectById(*ch, ch->effects, rawId);
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
    Runtime::TreasurePool &pool = VmInterface::getTreasurePool();
    uint32 poolGold = pool.poolGoldValue();
    pool.setPoolFromGoldValue(poolGold - cost);
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
