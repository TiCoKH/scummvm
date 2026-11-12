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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_REST_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_REST_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class HorizontalYesNo;

/**
 * GameTime.CampRest: the rest-duration menu (seeded by Camp.CampRestMenu
 * from the party's spell preparation time), then resting in five-minute
 * ticks with the remaining time shown; a key offers to stop. Posts a
 * MenuResult to its parent when done, _intValue = 1 when the party was
 * interrupted. The host must call tick() while it is active.
 */
class RestDialog : public Dialog {
public:
    RestDialog(const Common::String &name = "Rest");
    ~RestDialog() override;

    /** Seed the rest time from the party and show the rest menu. */
    void begin();

    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void handleMenuResult(const MenuResultMessage &result) override;
    bool tick() override;

    bool wasInterrupted() const { return _interrupted; }

private:
    // C_Time components of dsRestingTimeGbvm.
    enum {
        kMinutes = 1,
        kTurns = 2,     // tens of minutes
        kHours = 3,
        kDays = 4,
        kMonths = 5,
        kTimeSize = 7
    };

    enum State {
        STATE_MENU = 0,
        STATE_RESTING,
        STATE_STOP_CONFIRM
    };

    State _state = STATE_MENU;
    uint16 _time[kTimeSize];
    int _sel = kTurns;
    bool _interrupted = false;
    HorizontalYesNo *_stopConfirm = nullptr;

    void validateTime();
    void decrementTime(uint16 count, int unit);
    uint32 totalMinutes() const;
    void setFromMinutes(uint32 minutes);
    void startResting();
    void finish();
    void drawRestTime(int highlight);
    void drawMenu();
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_REST_DIALOG_H
