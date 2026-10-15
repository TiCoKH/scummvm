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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_GAME_SPEED_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_GAME_SPEED_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "goldbox/core/menu_item.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalMenu;

/**
 * DIALOG_SetGameSpeed — nested game-speed configuration interaction.
 *
 * Mirrors DIALOG_SetGameSpeed:
 *   - Displays current speed in the prompt.
 *   - Offers Slower (S) when speed < max, Faster (F) when speed > min.
 *   - Always offers Exit (E).
 *   - S increments _textDelay (slower), F decrements it (faster).
 *   - Posts MenuResultMessage to parent on exit.
 *
 * CFG_GAME_SPEED maps to g_engine->_textDelay (range 1–5).
 * Speed boundaries: Slower available below 5, Faster available above 1.
 */
class GameSpeedDialog : public Dialog {
public:
    GameSpeedDialog();
    ~GameSpeedDialog() override;

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

private:
    MenuItemList   _menuModel;
    HorizontalMenu *_horizontalMenu;

    void rebuild();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_GAME_SPEED_DIALOG_H
