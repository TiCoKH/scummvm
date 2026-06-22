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
    void getShopFlags(bool &hasItems, bool &hasMoney) override;

private:
    enum HealStage {
        HEAL_NONE = 0,
        HEAL_SELECTOR,
        HEAL_NOT_AFFECTED,
        HEAL_CONFIRM_PAY
    };
    HealStage _healStage;
    int _healSelectedIndex;

    struct HealService {
        const char *name;
        uint16 cost;
    };
    static const HealService kHealServices[];
    static const int kHealServiceCount = 9;

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
    void removeAllEffectsOfType(
        Goldbox::Poolrad::Data::PoolradCharacter *ch, uint8 type);

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
