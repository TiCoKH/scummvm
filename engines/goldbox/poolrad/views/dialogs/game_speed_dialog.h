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
 *   - S increments the speed (slower), F decrements it (faster).
 *   - Posts MenuResultMessage to parent on exit.
 *
 * The original CFG_GAME_SPEED is a 0..9 value (0 = fastest, 9 = slowest). The
 * engine's single timing control is the 1..5 text-delay, so the dialog keeps
 * its own 0..9 value and maps it onto that control on confirmation; it never
 * consumes an actor action.
 */
class GameSpeedDialog : public Dialog {
public:
    // Original game-speed domain (DoneMenu / SpeedMenu accept 0..9).
    static const int kMinSpeed = 0;
    static const int kMaxSpeed = 9;

    GameSpeedDialog();
    ~GameSpeedDialog() override;

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;

    /** Clamp an arbitrary value into the original 0..9 speed range. */
    static int clampSpeed(int speed);

    /** Map a 0..9 game speed onto the engine's 1..5 text-delay control. */
    static uint speedToTextDelay(int speed);

    /** Recover the 0..9 game speed from the engine's 1..5 text-delay. */
    static int textDelayToSpeed(uint textDelay);

private:
    MenuItemList   _menuModel;
    HorizontalMenu *_horizontalMenu;

    // Working 0..9 speed, seeded from the engine timing on activate().
    int _speed;
    // Prompt-line background: gray in combat, black elsewhere (camp).
    uint8 _backgroundColor = 0;

    void rebuild();
    void confirmAndReturn();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_GAME_SPEED_DIALOG_H
