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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_INFO_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_INFO_DIALOG_H

#include "goldbox/poolrad/views/dialogs/dialog.h"
#include "common/str.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

/**
 * Host side of the combat info line. The prompt content dialogs post their
 * results to their UI parent, so they must stay attached to the combat view;
 * the host performs the actual attach/detach.
 */
class CombatInfoHost {
public:
    virtual ~CombatInfoHost() {}
    virtual void attachPromptContent(Dialog *content) = 0;
    virtual void detachPromptContent(Dialog *content) = 0;
};

/**
 * Bottom information/prompt line of the combat screen (row 24).
 *
 * Owns whatever currently occupies the line: one interactive content dialog
 * (combat menu, move/attack/spell prompts, yes/no questions) and/or a
 * one-shot information text such as "Move/Attack, Move Left = n".
 * Mirrors the original SCREEN_ClearPrompt + horizontal menu usage.
 *
 * Not attached to the UI tree itself: CombatView draws it in a fixed order
 * after the viewport and the message panel.
 */
class CombatInfoDialog : public Dialog {
public:
    static const int kRow = 24;
    static const int kLeft = 0;
    static const int kRight = 39;
    static const int kTextColor = 10;
    static const uint8 kBackgroundColor = 8;

    explicit CombatInfoDialog(CombatInfoHost *host);
    ~CombatInfoDialog() override {}

    /**
     * Make content the single owner of the prompt line. Any previous
     * content is deactivated and detached first.
     */
    void showContent(Dialog *content);

    /** Deactivate and detach the current content, if any. */
    void closeContent();

    /** Close the current content only if it is the given dialog. */
    void closeContent(Dialog *content);

    Dialog *getContent() const { return _content; }
    bool hasContent() const { return _content != nullptr; }
    bool isShowing(const Dialog *content) const {
        return content && _content == content;
    }

    /** Queue a one-shot info text, drawn by the next draw(). */
    void showInfo(const Common::String &text);

    /** Clear the prompt line immediately (SCREEN_ClearPrompt). */
    void clearPrompt();

    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;

private:
    CombatInfoHost *_host;
    Dialog *_content = nullptr;
    Common::String _infoText;
    bool _hasInfo = false;
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_COMBAT_INFO_DIALOG_H
