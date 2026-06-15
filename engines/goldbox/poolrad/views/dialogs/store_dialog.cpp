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

#include "goldbox/poolrad/views/dialogs/store_dialog.h"
#include "goldbox/vm_interface.h"
#include "goldbox/runtime/treasure_pool.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

static ShopBaseConfig makeStoreConfig() {
    ShopBaseConfig cfg;
    cfg.type = SHOP_STORE;
    cfg.menuPrompt = "";
    cfg.exitConfirmLine1 = "As you Leave the Shopkeeper says,"
        " 'Excuse me but you left some things here.'";
    cfg.exitConfirmLine2 = "Do you want to go back and get your stuff?";
    return cfg;
}

StoreDialog::StoreDialog(const Common::String &name)
    : ShopBaseDialog(name, makeStoreConfig()) {
}

void StoreDialog::onShopActivate() {
    // TODO: Pre-build item display text list (ITEM_buildListDisplayText loop).
}

void StoreDialog::actionPrimary() {
    // TODO: DIALOG_Buy — open item purchase vertical menu.
}

void StoreDialog::getShopFlags(bool &hasItems, bool &hasMoney) {
    const TreasurePool &pool = VmInterface::getTreasurePool();
    hasItems = pool.hasItems();
    hasMoney = pool.hasAnyCoin();
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
