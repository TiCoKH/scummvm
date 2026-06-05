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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_CAMP_MENU_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_CAMP_MENU_DIALOG_H

#include "common/array.h"
#include "common/ptr.h"
#include "goldbox/core/menu_item.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Gfx {
class Pic;
}

namespace Poolrad {
namespace Views {

class InGameView;

namespace Dialogs {

class HorizontalMenu;
class SpellBookDialog;

/**
 * Self-contained camping dialog (VIEW_PartyCamp equivalent).
 *
 * Owns:
 *  - Campfire picture display (PIC 29, animated multi-frame)
 *  - Horizontal menu: Save / View / Magic / Rest / Alter / Exit
 *  - Key dispatch to sub-actions
 *  - "The party makes camp..." text output
 *
 * Posts MenuResult to parent InGameView on exit:
 *  - _success=true, _intValue=1 if rest was interrupted (encounter)
 *  - _success=true, _intValue=0 on normal exit ('E')
 *
 * Blocks movement/navigation keys — only camp menu keys are accepted.
 */
class CampMenuDialog : public Dialog {
public:
    static const uint8 kCampfirePicId = 29;

private:
    InGameView *_parentView;
    MenuItemList _menuModel;
    HorizontalMenu *_horizontalMenu;
    SpellBookDialog *_spellBookDialog;

    // Animated campfire frames.
    Common::Array<Common::SharedPtr<Goldbox::Gfx::Pic>> _campfireFrames;
    uint8 _currentFrame;
    uint32 _lastFrameTime;
    uint16 _frameIntervalMs;

    bool _isInterrupted;

    void buildMenuModel();
    void loadCampfireFrames();
    void exitCamp();
    void handleMenuKey(char key);

public:
    CampMenuDialog(const Common::String &name, InGameView *parent);
    ~CampMenuDialog() override;

    void activate() override;
    void deactivate() override;
    void draw() override;
    bool msgKeypress(const KeypressMessage &msg) override;
    void timeout() override;

    bool isInterrupted() const { return _isInterrupted; }
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_CAMP_MENU_DIALOG_H
