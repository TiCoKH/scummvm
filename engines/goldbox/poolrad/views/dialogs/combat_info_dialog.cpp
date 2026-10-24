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

#include "goldbox/poolrad/views/dialogs/combat_info_dialog.h"
#include "goldbox/gfx/surface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

CombatInfoDialog::CombatInfoDialog(CombatInfoHost *host)
        : Dialog("CombatInfo"), _host(host) {
}

void CombatInfoDialog::showContent(Dialog *content) {
    if (_content == content) {
        if (_content && !_content->isActive())
            _content->activate();
        return;
    }

    closeContent();
    if (!content)
        return;

    if (_host)
        _host->attachPromptContent(content);
    _content = content;
    _content->activate();
}

void CombatInfoDialog::closeContent() {
    if (!_content)
        return;

    Dialog *old = _content;
    _content = nullptr;
    old->deactivate();
    if (_host)
        _host->detachPromptContent(old);
}

void CombatInfoDialog::closeContent(Dialog *content) {
    if (isShowing(content))
        closeContent();
}

void CombatInfoDialog::showInfo(const Common::String &text) {
    _infoText = text;
    _hasInfo = true;
}

void CombatInfoDialog::clearPrompt() {
    Surface s = getSurface();
    s.clearBox(kLeft, kRow, kRight, kRow, kBackgroundColor);
}

void CombatInfoDialog::draw() {
    if (_hasInfo) {
        clearPrompt();
        Surface s = getSurface();
        s.writeStringC(kLeft, kRow, kTextColor, _infoText);
        _hasInfo = false;
    }

    if (_content)
        _content->draw();
}

bool CombatInfoDialog::msgKeypress(const KeypressMessage &msg) {
    return _content ? _content->send(msg) : false;
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
