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

#include "goldbox/poolrad/views/dialogs/shop_base_dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * Temple/healer service dialog (DIALOG_Temple equivalent).
 * Primary action: Heal (H) — cures conditions and restores HP for gold.
 * Exit text references "a priest".
 */
class TempleDialog : public ShopBaseDialog {
public:
    TempleDialog(const Common::String &name = "Temple");

protected:
    void actionPrimary() override;
    void getShopFlags(bool &hasItems, bool &hasMoney) override;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_TEMPLE_DIALOG_H
