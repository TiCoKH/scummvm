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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_SET_ICON_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_SET_ICON_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/poolrad/data/poolrad_character.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class SetIcon : public Dialog {
public:
    SetIcon(const Common::String &name,
            Goldbox::Poolrad::Data::PoolradCharacter *pc);
    ~SetIcon() override;

    bool msgKeypress(const KeypressMessage &msg) override;
    void draw() override;
    void handleMenuResult(bool success, Common::KeyCode key, short value) override;

private:
    Goldbox::Poolrad::Data::PoolradCharacter *_pc;

    static const uint8 kBackgroundColor = 8;
    static const uint8 kWindowLeft = 1;
    static const uint8 kWindowTop = 1;
    static const uint8 kWindowRight = 38;
    static const uint8 kWindowBottom = 22;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_SET_ICON_H
