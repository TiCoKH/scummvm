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

#ifndef GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_PANEL_DIALOG_H
#define GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_PANEL_DIALOG_H

#include "common/array.h"
#include "common/ptr.h"
#include "goldbox/gfx/dax_anim_decoder.h"
#include "goldbox/poolrad/views/dialogs/dialog.h"

namespace Goldbox {
namespace Gfx {
class Pic;
}
}

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

class InGamePanelDialog : public Dialog {
public:
    enum RuntimeMode {
        kRuntimeNone = 0,
        kRuntimeShop,
        kRuntimeCamping,
        kRuntimeAfterCombat
    };

private:
    Common::String _title;
    bool _usePortraitBlocks = false;
    bool _useAnimatedResource = false;
    RuntimeMode _runtimeMode = kRuntimeNone;

    uint8 _pictureId = 0xFF;
    uint8 _loadedPictureId = 0xFF;
    Common::ScopedPtr<Goldbox::Gfx::Pic> _picture;

    Common::String _resourceName;
    uint8 _resourceBlockId = 0xFF;
    bool _resourceMasked = false;
    Common::String _loadedResourceName;
    uint8 _loadedResourceBlockId = 0xFF;
    bool _loadedResourceMasked = false;
    uint8 _resourceFrameCount = 0;
    uint8 _animFrameIndex = 0;
    uint8 _animTick = 0;
    uint8 _animFrameDelayTicks = 2;
    Common::Array<Common::SharedPtr<Goldbox::Gfx::Pic> > _animFrames;
    Goldbox::Gfx::DaxAnimDecoder _animDecoder;

    uint8 _headBlockId = 0xFF;
    uint8 _bodyBlockId = 0xFF;
    uint8 _loadedHeadBlockId = 0xFF;
    uint8 _loadedBodyBlockId = 0xFF;
    Common::ScopedPtr<Goldbox::Gfx::Pic> _headPicture;
    Common::ScopedPtr<Goldbox::Gfx::Pic> _bodyPicture;

public:
    InGamePanelDialog(const Common::String &name,
        const Common::String &title);
    ~InGamePanelDialog() override;

    void setRuntimeMode(RuntimeMode mode) {
        _runtimeMode = mode;
        redraw();
    }

    void setTitle(const Common::String &title) {
        _title = title;
        redraw();
    }

    void setPictureId(uint8 pictureId);
    void setPortraitBlockIds(uint8 headBlockId, uint8 bodyBlockId);
    void setAnimatedResource(const Common::String &resourceName,
        uint8 blockId, bool masked);

    void draw() override;

private:
    void syncRuntimeResources();
    void clearAnimatedFrames();
    void rebuildAnimatedFrames(Goldbox::Data::DaxBlockPic *picBlock,
        const Goldbox::Gfx::DaxAnimResourceKey &key);
};

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox

#endif // GOLDBOX_POOLRAD_VIEWS_DIALOGS_IN_GAME_PANEL_DIALOG_H
