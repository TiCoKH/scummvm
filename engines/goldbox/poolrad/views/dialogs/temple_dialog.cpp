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

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

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
    : ShopBaseDialog(name, makeTempleConfig()) {
}

void TempleDialog::actionPrimary() {
    // TODO: TEMPLE_Heal — present healing cost menu for selected character.
}

void TempleDialog::getShopFlags(bool &hasItems, bool &hasMoney) {
    // TODO: Query VM/engine state for unclaimed items deposited at temple.
    hasItems = false;
    hasMoney = false;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
