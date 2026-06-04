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

#include "goldbox/poolrad/views/dialogs/in_game_panel_dialog.h"
#include "goldbox/data/daxblock.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/poolrad/poolrad.h"
#include "goldbox/runtime/runtime_exchange.h"
#include "goldbox/vm_interface.h"

namespace Goldbox {
namespace Poolrad {
namespace Views {
namespace Dialogs {

static const uint8 kPicNone = 0xFF;
static const uint8 kCampingDefaultPicId = 0x1D;
static const uint8 kAfterCombatDefaultPicId = 0x01;

void InGamePanelDialog::syncRuntimeResources() {
    if (_runtimeMode == kRuntimeNone)
        return;

    uint8 runtimePictureId = kPicNone;
    uint8 runtimeBodyId = kPicNone;

    if (::Goldbox::Poolrad::g_engine) {
        const RuntimeExchange *exchange =
            ::Goldbox::Poolrad::g_engine->getRuntimeExchange();
        ::Goldbox::RuntimeMapSnapshot snapshot;
        if (exchange && exchange->captureMapSnapshot(snapshot) && snapshot.valid) {
            runtimePictureId = snapshot.pictureHeadId;
            runtimeBodyId = snapshot.pictureBodyId;
        }
    }

    switch (_runtimeMode) {
    case kRuntimeShop:
        setPortraitBlockIds(runtimePictureId, runtimeBodyId);
        break;
    case kRuntimeCamping: {
        const uint8 picId = (runtimePictureId != kPicNone)
            ? runtimePictureId
            : kCampingDefaultPicId;
        setPortraitBlockIds(kPicNone, kPicNone);
        setAnimatedResource("PIC", picId, false);
        break;
    }
    case kRuntimeAfterCombat: {
        const uint8 picId = (runtimePictureId != kPicNone)
            ? runtimePictureId
            : kAfterCombatDefaultPicId;
        setPortraitBlockIds(kPicNone, kPicNone);
        setAnimatedResource("PIC", picId, false);
        break;
    }
    default:
        break;
    }
}

void InGamePanelDialog::clearAnimatedFrames() {
    _animFrames.clear();
    _animFrameIndex = 0;
    _animTick = 0;
}

void InGamePanelDialog::rebuildAnimatedFrames(Goldbox::Data::DaxBlockPic *picBlock,
        const Goldbox::Gfx::DaxAnimResourceKey &key) {
    clearAnimatedFrames();

    if (!picBlock)
        return;

    if (!_animDecoder.decodeFrames(*picBlock, key, _animFrames,
            _resourceFrameCount)) {
        clearAnimatedFrames();
    }
}

InGamePanelDialog::InGamePanelDialog(const Common::String &name,
        const Common::String &title)
    : Dialog(name),
      _title(title) {
    // Inner frame content area used by SHOP/CAMPING/AFTER_COMBAT picture panel.
    setBounds(Window(3, 3, 13, 13));
}

InGamePanelDialog::~InGamePanelDialog() {
}

void InGamePanelDialog::setPictureId(uint8 pictureId) {
    _usePortraitBlocks = false;
    _useAnimatedResource = false;

    if (_pictureId == pictureId)
        return;

    _pictureId = pictureId;
    _loadedPictureId = 0xFF;
    _picture.reset();
    clearAnimatedFrames();
    redraw();
}

void InGamePanelDialog::setPortraitBlockIds(uint8 headBlockId,
        uint8 bodyBlockId) {
    _useAnimatedResource = false;
    _usePortraitBlocks = true;

    if (_headBlockId == headBlockId && _bodyBlockId == bodyBlockId)
        return;

    _headBlockId = headBlockId;
    _bodyBlockId = bodyBlockId;
    clearAnimatedFrames();
    redraw();
}

void InGamePanelDialog::setAnimatedResource(const Common::String &resourceName,
        uint8 blockId, bool masked) {
    _usePortraitBlocks = false;
    _useAnimatedResource = true;

    if (_resourceName == resourceName && _resourceBlockId == blockId
            && _resourceMasked == masked)
        return;

    _resourceName = resourceName;
    _resourceBlockId = blockId;
    _resourceMasked = masked;
    clearAnimatedFrames();
    redraw();
}

void InGamePanelDialog::draw() {
    if (!_isVisible)
        return;

    syncRuntimeResources();

    Surface s = getSurface();
    s.clearBox(0, 0, 10, 10, 0);

    if (_usePortraitBlocks) {
        // Decompile parity: only reload head/body blocks if ID changed and
        // requested ID is not 0xFF.
        if (_headBlockId != 0xFF && _loadedHeadBlockId != _headBlockId) {
            Goldbox::Data::DaxBlock *rawHead =
                VmInterface::getDaxHead().getBlockById(_headBlockId);
            Goldbox::Data::DaxBlockPic *headBlock =
                dynamic_cast<Goldbox::Data::DaxBlockPic *>(rawHead);
            if (headBlock)
                _headPicture.reset(Goldbox::Gfx::Pic::read(headBlock));
            else
                _headPicture.reset();
            _loadedHeadBlockId = _headBlockId;
        }

        if (_bodyBlockId != 0xFF && _loadedBodyBlockId != _bodyBlockId) {
            Goldbox::Data::DaxBlock *rawBody =
                VmInterface::getDaxBody().getBlockById(_bodyBlockId);
            Goldbox::Data::DaxBlockPic *bodyBlock =
                dynamic_cast<Goldbox::Data::DaxBlockPic *>(rawBody);
            if (bodyBlock)
                _bodyPicture.reset(Goldbox::Gfx::Pic::read(bodyBlock));
            else
                _bodyPicture.reset();
            _loadedBodyBlockId = _bodyBlockId;
        }

        // Keep the same vertical composition used by existing portrait display:
        // head at y=0, body at y=5 chars inside this panel.
        if (_headPicture)
            _headPicture->drawAtCharPos(&s, 0, 0);
        if (_bodyPicture)
            _bodyPicture->drawAtCharPos(&s, 0, 5);

        if (_headPicture || _bodyPicture)
            return;
    } else if (_useAnimatedResource) {
        if (_loadedResourceName != _resourceName
                || _loadedResourceBlockId != _resourceBlockId
                || _loadedResourceMasked != _resourceMasked) {
            _picture.reset();
            _resourceFrameCount = 0;
            clearAnimatedFrames();

            Goldbox::Data::DaxBlockContainer *container = nullptr;
            if (_resourceName.equalsIgnoreCase("PIC")
                    || _resourceName.equalsIgnoreCase("FINAL")) {
                container = &VmInterface::getDaxPic();
            } else if (_resourceName.equalsIgnoreCase("CPIC")) {
                container = &VmInterface::getDaxCPic();
            } else if (_resourceName.equalsIgnoreCase("HEAD")) {
                container = &VmInterface::getDaxHead();
            } else if (_resourceName.equalsIgnoreCase("BODY")) {
                container = &VmInterface::getDaxBody();
            } else if (_resourceName.equalsIgnoreCase("CHEAD")) {
                container = &VmInterface::getDaxCHead();
            } else if (_resourceName.equalsIgnoreCase("CBODY")) {
                container = &VmInterface::getDaxCBody();
            }

            if (container && _resourceBlockId != 0xFF) {
                Goldbox::Data::DaxBlock *rawBlock =
                    container->getBlockById(_resourceBlockId);
                Goldbox::Data::DaxBlockPic *picBlock =
                    dynamic_cast<Goldbox::Data::DaxBlockPic *>(rawBlock);

                if (picBlock) {
                    Goldbox::Gfx::DaxAnimResourceKey key;
                    key._resourceName = _resourceName;
                    key._blockId = _resourceBlockId;
                    key._masked = _resourceMasked;
                    rebuildAnimatedFrames(picBlock, key);

                    // Fallback: if DaxAnimDecoder failed, use Pic::readFrame
                    // to decode frames directly from packed nibble data.
                    if (_animFrames.empty()) {
                        const int frames = (picBlock->frameCount > 1)
                            ? picBlock->frameCount : 1;
                        for (int i = 0; i < frames; ++i) {
                            Goldbox::Gfx::Pic *frame =
                                Goldbox::Gfx::Pic::readFrame(picBlock, i);
                            if (frame)
                                _animFrames.push_back(
                                    Common::SharedPtr<Goldbox::Gfx::Pic>(frame));
                            else
                                break;
                        }
                        _resourceFrameCount = (uint8)_animFrames.size();
                    }
                }
            }

            _loadedResourceName = _resourceName;
            _loadedResourceBlockId = _resourceBlockId;
            _loadedResourceMasked = _resourceMasked;
        }

        if (!_animFrames.empty()) {
            _animFrames[_animFrameIndex]->draw(&s, 0, 0);

            if (_animFrames.size() > 1) {
                _animTick++;
                if (_animTick >= _animFrameDelayTicks) {
                    _animTick = 0;
                    _animFrameIndex++;
                    if (_animFrameIndex >= _animFrames.size())
                        _animFrameIndex = 0;
                }

                // Keep animating while this panel is active.
                redraw();
            }

            return;
        }
    } else {
        if (_pictureId != 0xFF && _loadedPictureId != _pictureId) {
            Goldbox::Data::DaxBlock *rawBlock =
                VmInterface::getDaxManager().getPic().getBlockById(_pictureId);
            Goldbox::Data::DaxBlockPic *picBlock =
                dynamic_cast<Goldbox::Data::DaxBlockPic *>(rawBlock);

            if (picBlock)
                _picture.reset(Goldbox::Gfx::Pic::read(picBlock));
            else
                _picture.reset();

            _loadedPictureId = _pictureId;
        }

        if (_picture) {
            _picture->draw(&s, 0, 0);
            return;
        }
    }

    if (_title.empty())
        return;

    int x = 5 - static_cast<int>(_title.size() / 2);
    if (x < 0)
        x = 0;
    s.writeStringC(x, 5, 11, _title);
}

} // namespace Dialogs
} // namespace Views
} // namespace Poolrad
} // namespace Goldbox
