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

#include "goldbox/poolrad/views/dialogs/rest_dialog.h"
#include "goldbox/poolrad/views/dialogs/horizontal_yesno.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/vm_interface.h"
#include "goldbox/events.h"
#include "common/system.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

namespace {

// GDConsts.unitScale: segment, minute, turn, hour, day, month, year.
const uint16 kUnitScale[7] = { 10, 10, 6, 24, 30, 12, 256 };

// The menu as GameTime.CampRestMenu shows it; capitals are the keys.
const char *const kRestMenu = "Rest   daYs Hours Mins   Inc Dec   Exit";

// GameTime.CampRest redraws the remaining time every fifth tick.
const int kTicksPerDisplay = 5;

Common::String twoDigits(uint v) {
    return Common::String::format("%02u", v % 100);
}

} // namespace

RestDialog::RestDialog(const Common::String &name) : Dialog(name) {
    setBounds(Window(0, 0, 39, 24));
    for (int i = 0; i < kTimeSize; ++i)
        _time[i] = 0;
}

RestDialog::~RestDialog() {
    delete _stopConfirm;
}

// Camp.CampRestMenu seeds hours, tens of minutes and minutes from the
// party's longest preparation time, then GameTime.CampRest shows the menu.
void RestDialog::begin() {
    const uint32 minutes =
        Poolrad::g_engine ? Poolrad::g_engine->campRestMinutes() : 0;
    for (int i = 0; i < kTimeSize; ++i)
        _time[i] = 0;
    _time[kHours] = (uint16)(minutes / 60);
    _time[kTurns] = (uint16)((minutes - 60 * _time[kHours]) / 10);
    _time[kMinutes] = (uint16)(minutes % 10);

    _sel = kTurns;
    _interrupted = false;
    _state = STATE_MENU;
    activate();
    redraw();
}

void RestDialog::deactivate() {
    if (_stopConfirm) {
        _stopConfirm->deactivate();
        delete _stopConfirm;
        _stopConfirm = nullptr;
    }
    Dialog::deactivate();
}

// GameTime.ValidateRestTime: carry each component, fold months into days
// and cap the days at 99.
void RestDialog::validateTime() {
    for (int i = 0; i < kTimeSize - 1; ++i) {
        while (_time[i] >= kUnitScale[i]) {
            ++_time[i + 1];
            _time[i] -= kUnitScale[i];
        }
    }
    if (_time[kMonths] > 0) {
        _time[kDays] += kUnitScale[kDays] * _time[kMonths];
        _time[kMonths] = 0;
        if (_time[kDays] > 99)
            _time[kDays] = 99;
    }
}

// GameTime.DecrementRestTime: subtract, borrowing from higher components;
// with nothing left to borrow from, the timer is cleared.
void RestDialog::decrementTime(uint16 count, int unit) {
    if (totalMinutes() == 0)
        return;

    while (count > _time[unit]) {
        int j = unit + 1;
        while (j < kMonths && _time[j] == 0)
            ++j;
        if (j == kMonths) {
            for (int i = 0; i < kTimeSize; ++i)
                _time[i] = 0;
            count = 0;
            break;
        }
        for (int k = j; k > unit; --k) {
            --_time[k];
            _time[k - 1] += kUnitScale[k - 1];
        }
    }
    _time[unit] -= count;
    validateTime();
}

uint32 RestDialog::totalMinutes() const {
    return (uint32)_time[kDays] * 1440 + (uint32)_time[kHours] * 60 +
        (uint32)_time[kTurns] * 10 + _time[kMinutes];
}

void RestDialog::setFromMinutes(uint32 minutes) {
    for (int i = 0; i < kTimeSize; ++i)
        _time[i] = 0;
    _time[kDays] = (uint16)(minutes / 1440);
    minutes %= 1440;
    _time[kHours] = (uint16)(minutes / 60);
    minutes %= 60;
    _time[kTurns] = (uint16)(minutes / 10);
    _time[kMinutes] = (uint16)(minutes % 10);
}

void RestDialog::startResting() {
    const uint32 minutes = totalMinutes();
    if (minutes == 0 || !Poolrad::g_engine ||
            !Poolrad::g_engine->beginRest(minutes)) {
        finish();
        return;
    }
    _state = STATE_RESTING;
    redraw();
}

void RestDialog::finish() {
    _state = STATE_MENU;
    deactivate();
    if (_parent && g_events)
        g_events->postMenuResult(_parent->getName(), true,
            Common::KEYCODE_e, _interrupted ? 1 : 0, Common::String(),
            true, false);
}

bool RestDialog::tick() {
    if (!_isActive || _state != STATE_RESTING || !Poolrad::g_engine)
        return false;

    const bool resting = Poolrad::g_engine->stepRest(kTicksPerDisplay);
    setFromMinutes(Poolrad::g_engine->restRemainingMinutes());
    redraw();
    if (resting)
        return false;

    if (Poolrad::g_engine->restInterrupted()) {
        _interrupted = true;
        // "The Party is rudely interrupted!" and TextRend.PauseOnly.
        Surface s = getSurface();
        s.clearBox(1, 17, 38, 22, 0);
        drawRestTime(0);
        s.writeStringC(1, 19, 15, "The Party is rudely interrupted!");
        if (g_events) {
            const uint32 end = g_system->getMillis() +
                VmInterface::getTextDelay() * 225;
            while (g_system->getMillis() < end)
                if (!g_events->pumpModalInputFrame())
                    break;
        }
    }
    finish();
    return false;
}

bool RestDialog::msgKeypress(const KeypressMessage &msg) {
    if (!_isActive)
        return false;

    switch (_state) {
    case STATE_RESTING: {
        // A key offers to stop.
        _state = STATE_STOP_CONFIRM;
        HorizontalYesNoConfig cfg;
        cfg.promptTxt = "Stop Resting? ";
        cfg.promptColor = 13;
        cfg.textColor = 10;
        cfg.selectColor = 15;
        cfg.backgroundColor = 0;
        delete _stopConfirm;
        _stopConfirm = new HorizontalYesNo("RestStopConfirm", cfg);
        setDialogParent(_stopConfirm, this);
        _stopConfirm->activate();
        redraw();
        return true;
    }
    case STATE_STOP_CONFIRM:
        if (_stopConfirm)
            _stopConfirm->msgKeypress(msg);
        return true;
    default:
        break;
    }

    // GameTime.CampRestMenu: arrows are Inc/Dec and cycle the column.
    char key = msg.ascii;
    if (key >= 'a' && key <= 'z')
        key -= 32;
    switch (msg.keycode) {
    case Common::KEYCODE_UP:
        key = 'I';
        break;
    case Common::KEYCODE_DOWN:
        key = 'D';
        break;
    case Common::KEYCODE_LEFT:
        _sel = (_sel >= kDays) ? kTurns : _sel + 1;
        key = 0;
        break;
    case Common::KEYCODE_RIGHT:
        _sel = (_sel <= kTurns) ? kDays : _sel - 1;
        key = 0;
        break;
    case Common::KEYCODE_RETURN:
        key = 'R';
        break;
    case Common::KEYCODE_ESCAPE:
        key = 'E';
        break;
    default:
        break;
    }

    switch (key) {
    case 'R':
        startResting();
        return true;
    case 'E':
        finish();
        return true;
    case 'Y':
        _sel = kDays;
        break;
    case 'H':
        _sel = kHours;
        break;
    case 'M':
        _sel = kTurns;
        break;
    case 'I':
        if (_sel == kTurns)
            _time[kMinutes] += 5;
        else
            ++_time[_sel];
        validateTime();
        break;
    case 'D':
        if (_sel == kTurns)
            decrementTime(5, kMinutes);
        else
            decrementTime(1, _sel);
        validateTime();
        break;
    default:
        break;
    }
    redraw();
    return true;
}

void RestDialog::handleMenuResult(const MenuResultMessage &result) {
    if (_state != STATE_STOP_CONFIRM)
        return;

    const bool stop = result._success &&
        result._keyCode == Common::KEYCODE_y;
    if (_stopConfirm) {
        _stopConfirm->deactivate();
        delete _stopConfirm;
        _stopConfirm = nullptr;
    }
    if (stop) {
        if (Poolrad::g_engine)
            Poolrad::g_engine->stopRest();
        finish();
        return;
    }
    _state = STATE_RESTING;
    redraw();
}

// GameTime.DisplayRestTime: "Rest Time: DD:HH:MM" on row 17, the selected
// column in white.
void RestDialog::drawRestTime(int highlight) {
    Surface s = getSurface();
    s.writeStringC(1, 17, 10, "Rest Time:");
    s.writeStringC(12, 17, highlight == kDays ? 15 : 10,
        twoDigits(_time[kDays]));
    s.writeStringC(14, 17, 10, ":");
    s.writeStringC(15, 17, highlight == kHours ? 15 : 10,
        twoDigits(_time[kHours]));
    s.writeStringC(17, 17, 10, ":");
    s.writeStringC(18, 17, highlight == kTurns ? 15 : 10,
        twoDigits(10 * _time[kTurns] + _time[kMinutes]));
}

void RestDialog::drawMenu() {
    Surface s = getSurface();
    s.clearBox(0, 24, 39, 24, 0);
    // The current item (Rest) is highlighted; other items show their key.
    int col = 0;
    for (const char *p = kRestMenu; *p; ++p, ++col) {
        const bool inRest = col < 4;
        const bool isKey = (*p >= 'A' && *p <= 'Z');
        s.writeStringC(col, 24, (inRest || isKey) ? 15 : 10,
            Common::String(*p));
    }
}

void RestDialog::draw() {
    if (!_isVisible)
        return;

    Surface s = getSurface();
    s.clearBox(1, 17, 38, 22, 0);
    drawRestTime(_state == STATE_MENU ? _sel : 0);

    if (_state == STATE_MENU) {
        drawMenu();
    } else if (_state == STATE_STOP_CONFIRM && _stopConfirm) {
        _stopConfirm->setRedraw();
        _stopConfirm->draw();
    } else {
        s.clearBox(0, 24, 39, 24, 0);
    }
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
