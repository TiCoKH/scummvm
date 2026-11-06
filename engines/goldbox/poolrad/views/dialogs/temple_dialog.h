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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_TEMPLE_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_TEMPLE_DIALOG_H

#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Data {
class PoolradCharacter;
}
namespace Views {
namespace Dialogs {

class HorizontalYesNo;
class PromptMessage;
class VerticalMenu;

/**
 * Temple/healer service dialog (DIALOG_Temple equivalent).
 * Primary action: Heal (H) — presents a vertical menu of healing
 * services with gold costs.  Cures conditions and restores HP.
 *
 * Flow per original:
 *  1. Select service from vertical menu
 *  2. If character lacks condition → "Cast cure anyway?" Y/N
 *  3. Show cost message → "Pay for cure?" Y/N
 *  4. Deduct gold (character first, pool fallback)
 *  5. Apply effect, show "is cured" message
 */
class TempleDialog : public ShopBaseDialog {
public:
    TempleDialog(const Common::String &name = "Temple");
    ~TempleDialog() override;

    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;
    void draw() override;

protected:
    void actionPrimary() override;

    /**
     * GB13-025: tear down every temple-specific child (heal selector, Y/N
     * prompts, heal message) in addition to the base transaction stages on
     * exit/cancel/quit, then reset the heal stage.
     */
    void resetServiceChildren() override;

private:
    enum HealStage {
        HEAL_NONE = 0,
        HEAL_SELECTOR,
        HEAL_NOT_AFFECTED,
        HEAL_CONFIRM_PAY
    };
    HealStage _healStage;
    int _healSelectedIndex;

    // GB13-029 (Req 36.1): explicit PoR service metadata. Each temple service
    // carries its original DOS price PLUS a presence predicate (the set of RAW
    // PoR effect ids whose presence means the character "has the condition")
    // and a rule operation (the RAW effect-id removal list run in order, and
    // the cure-wounds dice). Using RAW ids at the game layer is deliberate:
    // the generic Effects enum ordinals do NOT equal the PoR raw save ids, so
    // a metadata table of raw ids (not enum names) is what matches the binary.
    struct HealService {
        const char *name;
        uint16 cost;
        const uint8 *conditionIds;  // presence predicate: any-of these raw ids
        uint8 conditionCount;
        const uint8 *removeIds;     // rule op: remove these raw ids, in order
        uint8 removeCount;
        uint8 healDiceNum;          // cure-wounds dice (0 == no HP heal)
        uint8 healDiceSides;
        uint8 healBonus;
    };
    static const HealService kHealServices[];
    static const int kHealServiceCount = 9;

    // GB13-029 (Req 36.2): staged payment contract shared with GB13-030/031.
    // Captures the service index, its cost, the cast-anyway decision and the
    // resolved funding source so Raise Dead (task 34) and Remove Curse (task
    // 35) can reuse the exact ConfirmCureAnyway -> ConfirmCureCost -> funding
    // -> operation flow without duplicating the char-else-pool rule.
    struct PaymentPlan {
        enum FundingSource {
            FUND_NONE = 0,     // insufficient everywhere: do NOT act
            FUND_CHARACTER,    // current character funds are sufficient
            FUND_POOL          // pooled funds are sufficient (char was not)
        };
        int serviceIndex;
        uint16 cost;
        bool castAnyway;       // player chose to pay despite "not affected"
        FundingSource funding;

        PaymentPlan()
            : serviceIndex(-1), cost(0), castAnyway(false),
              funding(FUND_NONE) {}
    };
    PaymentPlan _payment;

    // Resolve the funding source WITHOUT mutating balances. Current-character
    // funds if sufficient, ELSE pooled funds if sufficient, ELSE FUND_NONE.
    // Two insufficient balances are NEVER combined. Pure/headless: testable.
    static PaymentPlan::FundingSource resolveFunding(uint32 charGold,
        uint32 poolGold, uint16 cost);

    MenuItemList _healMenuItems;
    Common::Array<Common::String> _healPromptOpts;
    VerticalMenu *_healSelector;
    HorizontalYesNo *_yesNoDialog;
    PromptMessage *_healPrompt;

    void openHealSelector();
    void closeHealSelector();
    void buildHealMenuItems();
    void handleHealSelectorResult(const MenuResultMessage &result);

    void beginHealService(int index);
    void showNotAffectedPrompt();
    void showPayConfirmPrompt();
    void completeHeal();

    void handleNotAffectedResult(const MenuResultMessage &result);
    void handlePayConfirmResult(const MenuResultMessage &result);

    bool hasCondition(Goldbox::Poolrad::Data::PoolradCharacter *ch, int index);
    void applyHealEffect(Goldbox::Poolrad::Data::PoolradCharacter *ch,
        int index);
    void applyRaiseDead(Goldbox::Poolrad::Data::PoolradCharacter *ch);
    void applyRemoveCurse(Goldbox::Poolrad::Data::PoolradCharacter *ch);

    // GB13-029 (Req 36.3): remove ONE instance of a raw effect id through the
    // EffectSystem so the original on-remove behavior runs (EFF_REMOVE handler
    // for immediate effects, recalc via onEffectsChanged, bridge notify),
    // replacing the former direct vector erase. Returns true if an instance
    // was removed. See the .cpp for the scoped-removal / effect-lock notes.
    bool removeRawEffectInstance(
        Goldbox::Poolrad::Data::PoolradCharacter *ch, uint8 rawId);

    void showHealMessage(const Common::String &msg);
    void destroyYesNo();

    void deductGoldFromCharacter(
        Goldbox::Poolrad::Data::PoolradCharacter *ch, uint32 cost);
    void deductGoldFromPool(uint32 cost);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_TEMPLE_DIALOG_H
