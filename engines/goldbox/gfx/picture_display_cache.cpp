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

#include "goldbox/gfx/picture_display_cache.h"
#include "goldbox/gfx/pic.h"
#include "goldbox/vm_interface.h"
#include "goldbox/data/daxblock.h"
#include "common/debug.h"
#include "common/system.h"

namespace Goldbox {
namespace Gfx {

PictureDisplayCache::PictureDisplayCache()
    : _active(false), _headPicId(0xFF), _bodyPicId(0),
      _currentFrame(0), _animDirection(1),
      _lastFrameTime(0), _animInterval(500) {
}

PictureDisplayCache::~PictureDisplayCache() {
}

void PictureDisplayCache::clear() {
    _active = false;
    _headPicId = 0xFF;
    _bodyPicId = 0;
    _headPic.reset();
    _bodyPic.reset();
    _headFrames.clear();
    _currentFrame = 0;
    _animDirection = 1;
    _lastFrameTime = 0;
}

void PictureDisplayCache::load(uint8 headPicId, uint8 bodyPicId) {
    _headPicId = headPicId;
    _bodyPicId = bodyPicId;
    _headPic.reset();
    _bodyPic.reset();
    _headFrames.clear();
    _currentFrame = 0;
    _animDirection = 1;
    _lastFrameTime = 0;

    if (headPicId == 0xFF) {
        // Scene picture from PIC DAX (EGAPIC/DaxBlockSprit format).
        Data::DaxBlock *rawBlock = VmInterface::getDaxPic().getBlockById(bodyPicId);
        Data::DaxBlockSprit *spritBlock = rawBlock
            ? dynamic_cast<Data::DaxBlockSprit *>(rawBlock) : nullptr;
        if (spritBlock && spritBlock->frameCount() >= 1) {
            for (int i = 0; i < spritBlock->frameCount(); ++i) {
                Pic *frame = Pic::readEgaPicFrame(spritBlock, i);
                if (frame)
                    _headFrames.push_back(Common::SharedPtr<Pic>(frame));
                else
                    break;
            }
        }
    } else {
        // Portrait from HEAD DAX (DaxBlockPic format).
        Data::DaxBlock *rawBlock = VmInterface::getDaxHead().getBlockById(headPicId);
        Data::DaxBlockPic *block = rawBlock
            ? dynamic_cast<Data::DaxBlockPic *>(rawBlock) : nullptr;
        if (block) {
            const int frames = (block->frameCount > 1) ? block->frameCount : 1;
            for (int i = 0; i < frames; ++i) {
                Pic *frame = Pic::readFrame(block, i);
                if (frame)
                    _headFrames.push_back(Common::SharedPtr<Pic>(frame));
                else
                    break;
            }
        }

        // Body from BODY DAX (DaxBlockPic format).
        Data::DaxBlock *bodyRaw = VmInterface::getDaxBody().getBlockById(bodyPicId);
        Data::DaxBlockPic *bodyBlock = bodyRaw
            ? dynamic_cast<Data::DaxBlockPic *>(bodyRaw) : nullptr;
        if (bodyBlock)
            _bodyPic.reset(Pic::read(bodyBlock));
    }

    if (!_headFrames.empty()) {
        _headPic = _headFrames[0];
        _lastFrameTime = g_system->getMillis();
    }

    _active = (_headPic.get() != nullptr);
    if (!_active) {
        debug(3, "PictureDisplayCache::load: failed to load head=%u body=%u",
            (unsigned)headPicId, (unsigned)bodyPicId);
    }
}

const Pic *PictureDisplayCache::currentHeadFrame() const {
    if (_headFrames.empty())
        return _headPic.get();
    return _headFrames[_currentFrame].get();
}

bool PictureDisplayCache::tickAnimation(uint32 intervalMs) {
    if (_headFrames.size() <= 1)
        return false;

    uint32 now = g_system->getMillis();
    if (now - _lastFrameTime < intervalMs)
        return false;

    _lastFrameTime = now;

    int nextFrame = (int)_currentFrame + _animDirection;
    if (nextFrame >= (int)_headFrames.size()) {
        _animDirection = -1;
        nextFrame = (int)_currentFrame + _animDirection;
    } else if (nextFrame < 0) {
        _animDirection = 1;
        nextFrame = (int)_currentFrame + _animDirection;
    }

    _currentFrame = (uint8)nextFrame;
    _headPic = _headFrames[_currentFrame];
    return true;
}

bool PictureDisplayCache::tickAnimation() {
    return tickAnimation(_animInterval);
}

} // namespace Gfx
} // namespace Goldbox
